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
#include "wal_reader.h"
#include "rm_btree.h"
#include "assert.h"

void
array_desc(void* array,
           size_t elem_size,
           int count
           )
{
   if (count == 0)
   {
      printf(" []");
      return;
   }
   printf(" [");
   for (int i = 0; i < count; i++)
   {
      printf("%u", *(OffsetNumber*) ((char*) array + elem_size * i));
      if (i < count - 1)
      {
         printf(", ");
      }
   }
   printf("]");
}

static void
delvacuum_desc(char* block_data, uint16_t ndeleted, uint16_t nupdated)
{
   OffsetNumber* deletedoffsets;
   OffsetNumber* updatedoffsets;
   xl_btree_update* updates;

   /* Output deleted page offset number array */
   printf(", deleted:");
   deletedoffsets = (OffsetNumber*) block_data;
   array_desc(deletedoffsets, sizeof(OffsetNumber), ndeleted);

   /*
    * Output updates as an array of "update objects", where each element
    * contains a page offset number from updated array.  (This is not the
    * most literal representation of the underlying physical data structure
    * that we could use.  Readability seems more important here.)
    */
   printf(", updated: [");
   updatedoffsets = (OffsetNumber*) (block_data + ndeleted *
                                     sizeof(OffsetNumber));
   updates = (xl_btree_update*) ((char*) updatedoffsets +
                                 nupdated *
                                 sizeof(OffsetNumber));
   for (int i = 0; i < nupdated; i++)
   {
      OffsetNumber off = updatedoffsets[i];

      assert(OffsetNumberIsValid(off));
      assert(updates->ndeletedtids > 0);

      /*
       * "ptid" is the symbol name used when building each xl_btree_update's
       * array of offsets into a posting list tuple's ItemPointerData array.
       * xl_btree_update describes a subset of the existing TIDs to delete.
       */
      printf("{ off: %u, nptids: %u, ptids: [",
             off, updates->ndeletedtids);
      for (int p = 0; p < updates->ndeletedtids; p++)
      {
         uint16_t* ptid;

         ptid = (uint16_t*) ((char*) updates + SizeOfBtreeUpdate) + p;
         printf("%u", *ptid);

         if (p < updates->ndeletedtids - 1)
         {
            printf(", ");
         }
      }
      printf("] }");
      if (i < nupdated - 1)
      {
         printf(", ");
      }

      updates = (xl_btree_update*)
                ((char*) updates + SizeOfBtreeUpdate +
                 updates->ndeletedtids * sizeof(uint16_t));
   }
   printf("]");
}

void
btree_desc(DecodedXLogRecord* record)
{
   char* rec = record->main_data;
   uint8_t info = record->header.xl_info & ~XLR_INFO_MASK;

   switch (info)
   {
      case XLOG_BTREE_INSERT_LEAF:
      case XLOG_BTREE_INSERT_UPPER:
      case XLOG_BTREE_INSERT_META:
      case XLOG_BTREE_INSERT_POST: {
         xl_btree_insert* xlrec = (xl_btree_insert*) rec;
         printf(" off: %u", xlrec->offnum);
         break;
      }
      case XLOG_BTREE_SPLIT_L:
      case XLOG_BTREE_SPLIT_R: {
         xl_btree_split* xlrec = (xl_btree_split*) rec;

         printf("level: %u, firstrightoff: %d, newitemoff: %d, postingoff: %d",
                xlrec->level, xlrec->firstrightoff,
                xlrec->newitemoff, xlrec->postingoff);
         break;
      }
      case XLOG_BTREE_DEDUP: {
         xl_btree_dedup* xlrec = (xl_btree_dedup*) rec;

         printf("nintervals: %u", xlrec->nintervals);
         break;
      }
      case XLOG_BTREE_VACUUM: {
         xl_btree_vacuum* xlrec = (xl_btree_vacuum*) rec;

         printf("ndeleted: %u, nupdated: %u",
                xlrec->ndeleted, xlrec->nupdated);

         if (XLogRecHasBlockData(record, 0))
         {
            delvacuum_desc(XLogRecGetBlockData(record, 0, NULL),
                           xlrec->ndeleted, xlrec->nupdated);
         }
         break;
      }
      case XLOG_BTREE_DELETE: {
         xl_btree_delete* xlrec = (xl_btree_delete*) rec;

         printf("snapshotConflictHorizon: %u, ndeleted: %u, nupdated: %u, isCatalogRel: %c",
                xlrec->snapshotConflictHorizon,
                xlrec->ndeleted, xlrec->nupdated,
                xlrec->isCatalogRel ? 'T' : 'F');

         if (XLogRecHasBlockData(record, 0))
         {
            delvacuum_desc(XLogRecGetBlockData(record, 0, NULL),
                           xlrec->ndeleted, xlrec->nupdated);
         }
         break;
      }
      case XLOG_BTREE_MARK_PAGE_HALFDEAD: {
         xl_btree_mark_page_halfdead* xlrec = (xl_btree_mark_page_halfdead*) rec;

         printf("topparent: %u, leaf: %u, left: %u, right: %u",
                xlrec->topparent, xlrec->leafblk, xlrec->leftblk, xlrec->rightblk);
         break;
      }
      case XLOG_BTREE_UNLINK_PAGE_META:
      case XLOG_BTREE_UNLINK_PAGE: {
         xl_btree_unlink_page* xlrec = (xl_btree_unlink_page*) rec;

         printf("left: %u, right: %u, level: %u, safexid: %u:%u, ",
                xlrec->leftsib, xlrec->rightsib, xlrec->level,
                EpochFromFullTransactionId(xlrec->safexid),
                XidFromFullTransactionId(xlrec->safexid));
         printf("leafleft: %u, leafright: %u, leaftopparent: %u",
                xlrec->leafleftsib, xlrec->leafrightsib,
                xlrec->leaftopparent);
         break;
      }
      case XLOG_BTREE_NEWROOT: {
         xl_btree_newroot* xlrec = (xl_btree_newroot*) rec;

         printf("level: %u", xlrec->level);
         break;
      }
      case XLOG_BTREE_REUSE_PAGE: {
         xl_btree_reuse_page* xlrec = (xl_btree_reuse_page*) rec;

         printf("rel: %u/%u/%u, snapshotConflictHorizon: %u:%u, isCatalogRel: %c",
                xlrec->locator.spcOid, xlrec->locator.dbOid,
                xlrec->locator.relNumber,
                EpochFromFullTransactionId(xlrec->snapshotConflictHorizon),
                XidFromFullTransactionId(xlrec->snapshotConflictHorizon),
                xlrec->isCatalogRel ? 'T' : 'F');
         break;
      }
      case XLOG_BTREE_META_CLEANUP: {
         xl_btree_metadata* xlrec;

         xlrec = (xl_btree_metadata*) XLogRecGetBlockData(record, 0, NULL);
         printf("last_cleanup_num_delpages: %u",
                xlrec->last_cleanup_num_delpages);
         break;
      }
   }
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