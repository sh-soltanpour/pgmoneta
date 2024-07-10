/*
 * Copyright (C) 2024 The pgmoneta community
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may
 * be used to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "wal_reader.h"
#include "wal/rm_heap.h"
#include "utils.h"

static char*
out_infobits(char* buf, uint8_t infobits)
{
   if (infobits & XLHL_XMAX_IS_MULTI)
   {
      buf = pgmoneta_format_and_append(buf, "IS_MULTI ");
   }
   if (infobits & XLHL_XMAX_LOCK_ONLY)
   {
       buf = pgmoneta_format_and_append(buf, "LOCK_ONLY ");
   }
   if (infobits & XLHL_XMAX_EXCL_LOCK)
   {
       buf = pgmoneta_format_and_append(buf, "EXCL_LOCK ");
   }
   if (infobits & XLHL_XMAX_KEYSHR_LOCK)
   {
       buf = pgmoneta_format_and_append(buf, "KEYSHR_LOCK ");
   }
   if (infobits & XLHL_KEYS_UPDATED)
   {
       buf = pgmoneta_format_and_append(buf, "KEYS_UPDATED ");
   }
   return buf;
}

char*
heap_desc(char* buf, struct decoded_xlog_record* record)
{
   char* rec = record->main_data;
   uint8_t info = record->header.xl_info & ~XLR_INFO_MASK;

   info &= XLOG_HEAP_OPMASK;
   if (info == XLOG_HEAP_INSERT)
   {
      xl_heap_insert* xlrec = (xl_heap_insert*) rec;

      buf = pgmoneta_format_and_append(buf, "off %u flags 0x%02X", xlrec->offnum, xlrec->flags);
   }
   else if (info == XLOG_HEAP_DELETE)
   {
      xl_heap_delete* xlrec = (xl_heap_delete*) rec;

      buf = pgmoneta_format_and_append(buf, "off %u flags 0x%02X ",
                                       xlrec->offnum,
                                       xlrec->flags);
      buf = out_infobits(buf, xlrec->infobits_set);
   }
   else if (info == XLOG_HEAP_UPDATE)
   {
      xl_heap_update* xlrec = (xl_heap_update*) rec;

       buf = pgmoneta_format_and_append(buf, "off %u xmax %u flags 0x%02X ",
                                 xlrec->old_offnum,
                                 xlrec->old_xmax,
                                 xlrec->flags);
       buf = out_infobits(buf, xlrec->old_infobits_set);
       buf = pgmoneta_format_and_append(buf, "; new off %u xmax %u",
                                 xlrec->new_offnum,
                                 xlrec->new_xmax);
   }
   else if (info == XLOG_HEAP_HOT_UPDATE)
   {
      xl_heap_update* xlrec = (xl_heap_update*) rec;

       buf = pgmoneta_format_and_append(buf, "off %u xmax %u flags 0x%02X ",
                                 xlrec->old_offnum,
                                 xlrec->old_xmax,
                                 xlrec->flags);
       buf = out_infobits(buf, xlrec->old_infobits_set);
       buf = pgmoneta_format_and_append(buf, "; new off %u xmax %u",
                                 xlrec->new_offnum,
                                 xlrec->new_xmax);
   }
   else if (info == XLOG_HEAP_TRUNCATE)
   {
      xl_heap_truncate* xlrec = (xl_heap_truncate*) rec;
      int i;

      if (xlrec->flags & XLH_TRUNCATE_CASCADE)
      {
          buf = pgmoneta_format_and_append(buf, "cascade ");
      }
      if (xlrec->flags & XLH_TRUNCATE_RESTART_SEQS)
      {
          buf = pgmoneta_format_and_append(buf, "restart_seqs ");
      }
       buf = pgmoneta_format_and_append(buf, "nrelids %u relids", xlrec->nrelids);
      for (i = 0; i < xlrec->nrelids; i++)
      {
          buf = pgmoneta_format_and_append(buf, " %u", xlrec->relids[i]);
      }
   }
   else if (info == XLOG_HEAP_CONFIRM)
   {
      xl_heap_confirm* xlrec = (xl_heap_confirm*) rec;

       buf = pgmoneta_format_and_append(buf, "off %u", xlrec->offnum);
   }
   else if (info == XLOG_HEAP_LOCK)
   {
      xl_heap_lock* xlrec = (xl_heap_lock*) rec;

       buf = pgmoneta_format_and_append(buf, "off %u: xid %u: flags 0x%02X ",
                                 xlrec->offnum, xlrec->locking_xid, xlrec->flags);
       buf = out_infobits(buf, xlrec->infobits_set);
   }
   else if (info == XLOG_HEAP_INPLACE)
   {
      xl_heap_inplace* xlrec = (xl_heap_inplace*) rec;

       buf = pgmoneta_format_and_append(buf, "off %u", xlrec->offnum);
   }
   return buf;
}

char*
heap2_desc(char* buf, struct decoded_xlog_record* record)
{
   char* rec = record->main_data;
   uint8_t info = record->header.xl_info & ~XLR_INFO_MASK;

   info &= XLOG_HEAP_OPMASK;
   if (info == XLOG_HEAP2_PRUNE)
   {
      xl_heap_prune* xlrec = (xl_heap_prune*) rec;

       buf = pgmoneta_format_and_append(buf, "latestRemovedXid %u nredirected %u ndead %u",
                                 xlrec->latestRemovedXid,
                                 xlrec->nredirected,
                                 xlrec->ndead);
   }
   else if (info == XLOG_HEAP2_VACUUM)
   {
      xl_heap_vacuum* xlrec = (xl_heap_vacuum*) rec;

       buf = pgmoneta_format_and_append(buf, "nunused %u", xlrec->nunused);
   }
   else if (info == XLOG_HEAP2_FREEZE_PAGE)
   {
      xl_heap_freeze_page* xlrec = (xl_heap_freeze_page*) rec;

       buf = pgmoneta_format_and_append(buf, "cutoff xid %u ntuples %u",
                                 xlrec->cutoff_xid, xlrec->ntuples);
   }
   else if (info == XLOG_HEAP2_VISIBLE)
   {
      xl_heap_visible* xlrec = (xl_heap_visible*) rec;

       buf = pgmoneta_format_and_append(buf, "cutoff xid %u flags 0x%02X",
                                 xlrec->cutoff_xid, xlrec->flags);
   }
   else if (info == XLOG_HEAP2_MULTI_INSERT)
   {
      xl_heap_multi_insert* xlrec = (xl_heap_multi_insert*) rec;

       buf = pgmoneta_format_and_append(buf, "%d tuples flags 0x%02X", xlrec->ntuples,
                                 xlrec->flags);
   }
   else if (info == XLOG_HEAP2_LOCK_UPDATED)
   {
      xl_heap_lock_updated* xlrec = (xl_heap_lock_updated*) rec;

       buf = pgmoneta_format_and_append(buf, "off %u: xmax %u: flags 0x%02X ",
                                 xlrec->offnum, xlrec->xmax, xlrec->flags);
      buf = out_infobits(buf, xlrec->infobits_set);
   }
   else if (info == XLOG_HEAP2_NEW_CID)
   {
      xl_heap_new_cid* xlrec = (xl_heap_new_cid*) rec;

      buf = pgmoneta_format_and_append(buf, "rel %u/%u/%u; tid %u/%u",
                                 xlrec->target_node.spcNode,
                                 xlrec->target_node.dbNode,
                                 xlrec->target_node.relNode,
                                 ItemPointerGetBlockNumber(&(xlrec->target_tid)),
                                 ItemPointerGetOffsetNumber(&(xlrec->target_tid)));
      buf = pgmoneta_format_and_append(buf, "; cmin: %u, cmax: %u, combo: %u",
                                 xlrec->cmin, xlrec->cmax, xlrec->combocid);
   }
   return buf;
}