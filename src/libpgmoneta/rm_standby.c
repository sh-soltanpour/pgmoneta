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

#include "rm_standby.h"

static void
standby_desc_running_xacts(xl_running_xacts* xlrec)
{
   int i;

   printf("nextXid %u latestCompletedXid %u oldestRunningXid %u",
          xlrec->nextXid,
          xlrec->latestCompletedXid,
          xlrec->oldestRunningXid);
   if (xlrec->xcnt > 0)
   {
      printf("; %d xacts:", xlrec->xcnt);
      for (i = 0; i < xlrec->xcnt; i++)
      {
         printf(" %u", xlrec->xids[i]);
      }
   }

   if (xlrec->subxid_overflow)
   {
      printf("; subxid overflowed");
   }

   if (xlrec->subxcnt > 0)
   {
      printf("; %d subxacts:", xlrec->subxcnt);
      for (i = 0; i < xlrec->subxcnt; i++)
      {
         printf(" %u", xlrec->xids[xlrec->xcnt + i]);
      }
   }
}

void
standby_desc_invalidations(int nmsgs, SharedInvalidationMessage* msgs, Oid dbId, Oid tsId, bool relcacheInitFileInval
                           )
{
   int i;

   /* Do nothing if there are no invalidation messages */
   if (nmsgs <= 0)
   {
      return;
   }

   if (relcacheInitFileInval)
   {
      printf("; relcache init file inval dbid %u tsid %u", dbId, tsId);
   }

   printf("; inval msgs:");
   for (i = 0; i < nmsgs; i++)
   {
      SharedInvalidationMessage* msg = &msgs[i];

      if (msg->id >= 0)
      {
         printf(" catcache %d", msg->id);
      }
      else if (msg->id == SHAREDINVALCATALOG_ID)
      {
         printf(" catalog %u", msg->cat.catId);
      }
      else if (msg->id == SHAREDINVALRELCACHE_ID)
      {
         printf(" relcache %u", msg->rc.relId);
      }
      /* not expected, but print something anyway */
      else if (msg->id == SHAREDINVALSMGR_ID)
      {
         printf(" smgr");
      }
      /* not expected, but print something anyway */
      else if (msg->id == SHAREDINVALRELMAP_ID)
      {
         printf(" relmap db %u", msg->rm.dbId);
      }
      else if (msg->id == SHAREDINVALSNAPSHOT_ID)
      {
         printf(" snapshot %u", msg->sn.relId);
      }
      else
      {
         printf(" unrecognized id %d", msg->id);
      }
   }
}

void
standby_desc(DecodedXLogRecord* record)
{
   char* rec = record->main_data;
   uint8_t info = record->header.xl_info & ~XLR_INFO_MASK;

   if (info == XLOG_STANDBY_LOCK)
   {
      xl_standby_locks* xlrec = (xl_standby_locks*) rec;
      int i;

      for (i = 0; i < xlrec->nlocks; i++)
      {
         printf("xid %u db %u rel %u ",
                xlrec->locks[i].xid, xlrec->locks[i].dbOid,
                xlrec->locks[i].relOid);
      }
   }
   else if (info == XLOG_RUNNING_XACTS)
   {
      xl_running_xacts* xlrec = (xl_running_xacts*) rec;

      standby_desc_running_xacts(xlrec);
   }
   else if (info == XLOG_INVALIDATIONS)
   {
      xl_invalidations* xlrec = (xl_invalidations*) rec;
      standby_desc_invalidations(xlrec->nmsgs, xlrec->msgs,
                                 xlrec->dbId, xlrec->tsId,
                                 xlrec->relcacheInitFileInval);

   }
}
