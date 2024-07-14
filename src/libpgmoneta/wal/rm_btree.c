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

#include "stdbool.h"
#include "wal/wal_reader.h"
#include "wal/rm_btree.h"
#include "assert.h"
#include "utils.h"

char*
array_desc(char* buf, void* array,
           size_t elem_size,
           int count
           )
{
   if (count == 0)
   {
      buf = pgmoneta_format_and_append(buf, " []");
      return buf;
   }
   buf = pgmoneta_format_and_append(buf, " [");
   for (int i = 0; i < count; i++)
   {
      buf = pgmoneta_format_and_append(buf, "%u", *(offset_number*) ((char*) array + elem_size * i));
      if (i < count - 1)
      {
         buf = pgmoneta_format_and_append(buf, ", ");
      }
   }
   buf = pgmoneta_format_and_append(buf, "]");
   return buf;
}

static char*
delvacuum_desc(char* buf, char* block_data, uint16_t ndeleted, uint16_t nupdated)
{
   offset_number* deletedoffsets;
   offset_number* updatedoffsets;
   struct xl_btree_update* updates;

   /* Output deleted page offset number array */
   buf = pgmoneta_format_and_append(buf, ", deleted:");
   deletedoffsets = (offset_number*) block_data;
   buf = array_desc(buf, deletedoffsets, sizeof(offset_number), ndeleted);

   /*
    * Output updates as an array of "update objects", where each element
    * contains a page offset number from updated array.  (This is not the
    * most literal representation of the underlying physical data structure
    * that we could use.  Readability seems more important here.)
    */
   buf = pgmoneta_format_and_append(buf, ", updated: [");
   updatedoffsets = (offset_number*) (block_data + ndeleted *
                                      sizeof(offset_number));
   updates = (struct xl_btree_update*) ((char*) updatedoffsets +
                                        nupdated *
                                        sizeof(offset_number));
   for (int i = 0; i < nupdated; i++)
   {
      offset_number off = updatedoffsets[i];

      assert(OFFSET_NUMBER_IS_VALID(off));
      assert(updates->ndeletedtids > 0);

      /*
       * "ptid" is the symbol name used when building each xl_btree_update's
       * array of offsets into a posting list tuple's item_pointer_data array.
       * xl_btree_update describes a subset of the existing TIDs to delete.
       */
      buf = pgmoneta_format_and_append(buf, "{ off: %u, nptids: %u, ptids: [",
                                       off, updates->ndeletedtids);
      for (int p = 0; p < updates->ndeletedtids; p++)
      {
         uint16_t* ptid;

         ptid = (uint16_t*) ((char*) updates + SIZE_OF_BTREE_UPDATE) + p;
         buf = pgmoneta_format_and_append(buf, "%u", *ptid);

         if (p < updates->ndeletedtids - 1)
         {
            buf = pgmoneta_format_and_append(buf, ", ");
         }
      }
      buf = pgmoneta_format_and_append(buf, "] }");
      if (i < nupdated - 1)
      {
         buf = pgmoneta_format_and_append(buf, ", ");
      }

      updates = (struct xl_btree_update*)
                ((char*) updates + SIZE_OF_BTREE_UPDATE +
                 updates->ndeletedtids * sizeof(uint16_t));
   }
   buf = pgmoneta_format_and_append(buf, "]");

   return buf;
}

char*
btree_desc(char* buf, struct decoded_xlog_record* record)
{
   char* rec = record->main_data;
   uint8_t info = record->header.xl_info & ~XLR_INFO_MASK;

   switch (info)
   {
      case XLOG_BTREE_INSERT_LEAF:
      case XLOG_BTREE_INSERT_UPPER:
      case XLOG_BTREE_INSERT_META:
      case XLOG_BTREE_INSERT_POST: {
         struct xl_btree_insert* xlrec = (struct xl_btree_insert*) rec;
         buf = pgmoneta_format_and_append(buf, " off: %u", xlrec->offnum);
         break;
      }
      case XLOG_BTREE_SPLIT_L:
      case XLOG_BTREE_SPLIT_R: {
         struct xl_btree_split* xlrec = (struct xl_btree_split*) rec;

         buf = pgmoneta_format_and_append(buf, "level: %u, firstrightoff: %d, newitemoff: %d, postingoff: %d",
                                          xlrec->level, xlrec->firstrightoff,
                                          xlrec->newitemoff, xlrec->postingoff);
         break;
      }
      case XLOG_BTREE_DEDUP: {
         struct xl_btree_dedup* xlrec = (struct xl_btree_dedup*) rec;

         buf = pgmoneta_format_and_append(buf, "nintervals: %u", xlrec->nintervals);
         break;
      }
      case XLOG_BTREE_VACUUM: {
         struct xl_btree_vacuum* xlrec = (struct xl_btree_vacuum*) rec;

         buf = pgmoneta_format_and_append(buf, "ndeleted: %u, nupdated: %u",
                                          xlrec->ndeleted, xlrec->nupdated);

         if (XLogRecHasBlockData(record, 0))
         {
            buf = delvacuum_desc(buf, get_record_block_data(record, 0, NULL),
                                 xlrec->ndeleted, xlrec->nupdated);
         }
         break;
      }
      case XLOG_BTREE_DELETE: {
         struct xl_btree_delete* xlrec = (struct xl_btree_delete*) rec;

         buf = pgmoneta_format_and_append(buf, "snapshotConflictHorizon: %u, ndeleted: %u, nupdated: %u, isCatalogRel: %c",
                                          xlrec->snapshotConflictHorizon,
                                          xlrec->ndeleted, xlrec->nupdated,
                                          xlrec->isCatalogRel ? 'T' : 'F');

         if (XLogRecHasBlockData(record, 0))
         {
            buf = delvacuum_desc(buf, get_record_block_data(record, 0, NULL),
                                 xlrec->ndeleted, xlrec->nupdated);
         }
         break;
      }
      case XLOG_BTREE_MARK_PAGE_HALFDEAD: {
         struct xl_btree_mark_page_halfdead* xlrec = (struct xl_btree_mark_page_halfdead*) rec;

         buf = pgmoneta_format_and_append(buf, "topparent: %u, leaf: %u, left: %u, right: %u",
                                          xlrec->topparent, xlrec->leafblk, xlrec->leftblk, xlrec->rightblk);
         break;
      }
      case XLOG_BTREE_UNLINK_PAGE_META:
      case XLOG_BTREE_UNLINK_PAGE: {
         struct xl_btree_unlink_page* xlrec = (struct xl_btree_unlink_page*) rec;

         buf = pgmoneta_format_and_append(buf, "left: %u, right: %u, level: %u, safexid: %u:%u, ",
                                          xlrec->leftsib, xlrec->rightsib, xlrec->level,
                                          EPOCH_FROM_FULL_TRANSACTION_ID(xlrec->safexid),
                                          XID_FROM_FULL_TRANSACTION_ID(xlrec->safexid));
         buf = pgmoneta_format_and_append(buf, "leafleft: %u, leafright: %u, leaftopparent: %u",
                                          xlrec->leafleftsib, xlrec->leafrightsib,
                                          xlrec->leaftopparent);
         break;
      }
      case XLOG_BTREE_NEWROOT: {
         struct xl_btree_newroot* xlrec = (struct xl_btree_newroot*) rec;

         buf = pgmoneta_format_and_append(buf, "level: %u", xlrec->level);
         break;
      }
      case XLOG_BTREE_REUSE_PAGE: {
         struct xl_btree_reuse_page* xlrec = (struct xl_btree_reuse_page*) rec;

         buf = pgmoneta_format_and_append(buf, "rel: %u/%u/%u, snapshotConflictHorizon: %u:%u, isCatalogRel: %c",
                                          xlrec->locator.spcOid, xlrec->locator.dbOid,
                                          xlrec->locator.relNumber,
                                          EPOCH_FROM_FULL_TRANSACTION_ID(xlrec->snapshotConflictHorizon),
                                          XID_FROM_FULL_TRANSACTION_ID(xlrec->snapshotConflictHorizon),
                                          xlrec->isCatalogRel ? 'T' : 'F');
         break;
      }
      case XLOG_BTREE_META_CLEANUP: {
         struct xl_btree_metadata* xlrec;

         xlrec = (struct xl_btree_metadata*) get_record_block_data(record, 0, NULL);
         buf = pgmoneta_format_and_append(buf, "last_cleanup_num_delpages: %u",
                                          xlrec->last_cleanup_num_delpages);
         break;
      }
   }
   return buf;
}

const char*
btree_identify (uint8_t info)
{
   {
      const char* id = NULL;

      switch (info & ~XLR_INFO_MASK)
      {
         case XLOG_BTREE_INSERT_LEAF:
            id = "INSERT_LEAF";
            break;
         case XLOG_BTREE_INSERT_UPPER:
            id = "INSERT_UPPER";
            break;
         case XLOG_BTREE_INSERT_META:
            id = "INSERT_META";
            break;
         case XLOG_BTREE_SPLIT_L:
            id = "SPLIT_L";
            break;
         case XLOG_BTREE_SPLIT_R:
            id = "SPLIT_R";
            break;
         case XLOG_BTREE_INSERT_POST:
            id = "INSERT_POST";
            break;
         case XLOG_BTREE_DEDUP:
            id = "DEDUP";
            break;
         case XLOG_BTREE_VACUUM:
            id = "VACUUM";
            break;
         case XLOG_BTREE_DELETE:
            id = "DELETE";
            break;
         case XLOG_BTREE_MARK_PAGE_HALFDEAD:
            id = "MARK_PAGE_HALFDEAD";
            break;
         case XLOG_BTREE_UNLINK_PAGE:
            id = "UNLINK_PAGE";
            break;
         case XLOG_BTREE_UNLINK_PAGE_META:
            id = "UNLINK_PAGE_META";
            break;
         case XLOG_BTREE_NEWROOT:
            id = "NEWROOT";
            break;
         case XLOG_BTREE_REUSE_PAGE:
            id = "REUSE_PAGE";
            break;
         case XLOG_BTREE_META_CLEANUP:
            id = "META_CLEANUP";
            break;
      }

      return id;
   }
}