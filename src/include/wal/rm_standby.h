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

#ifndef PGMONETA_RM_STANDBY_H
#define PGMONETA_RM_STANDBY_H

#include "stdbool.h"
#include "wal_reader.h"
#include "sinval.h"
#include "rm.h"

#define XLOG_STANDBY_LOCK           0x00
#define XLOG_RUNNING_XACTS          0x10
#define XLOG_INVALIDATIONS          0x20

struct xl_standby_lock {
    transaction_id xid;           /* xid of holder of AccessExclusiveLock */
    oid dbOid;           /* DB containing table */
    oid relOid;          /* OID of table */
};

struct xl_standby_locks {
    int nlocks;          /* number of entries in locks array */
    struct xl_standby_lock locks[FLEXIBLE_ARRAY_MEMBER];
};

/*
 * When we write running xact data to WAL, we use this structure.
 */
struct xl_running_xacts {
    int xcnt;            /* # of xact ids in xids[] */
    int subxcnt;         /* # of subxact ids in xids[] */
    bool subxid_overflow;     /* snapshot overflowed, subxids missing */
    transaction_id nextXid;       /* xid from TransamVariables->nextXid */
    transaction_id oldestRunningXid;  /* *not* oldestXmin */
    transaction_id latestCompletedXid;    /* so we can set xmax */

    transaction_id xids[FLEXIBLE_ARRAY_MEMBER];
};

struct xl_invalidations {
    oid dbId;            /* MyDatabaseId */
    oid tsId;            /* MyDatabaseTableSpace */
    bool relcacheInitFileInval;   /* invalidate relcache init files */
    int nmsgs;           /* number of shared inval msgs */
    union shared_invalidation_message msgs[FLEXIBLE_ARRAY_MEMBER];
};

#define MinSizeOfInvalidations offsetof(xl_invalidations, msgs)

char *
standby_desc(char *buf, struct decoded_xlog_record *record);

char *
standby_desc_invalidations(char *buf, int nmsgs, union shared_invalidation_message *msgs, oid dbId, oid tsId,
                           bool relcacheInitFileInval);


#endif //PGMONETA_RM_STANDBY_H
