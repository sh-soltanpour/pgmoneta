//
// Created by Shahryar Soltanpour on 2024-06-09.
//

#ifndef PGMONETA_RM_XLOG_H
#define PGMONETA_RM_XLOG_H

#include "stdbool.h"
#include "wal_reader.h"



#define XLR_INFO_MASK           0x0F
#define XLR_RMGR_INFO_MASK      0xF0


#define XLOG_STANDBY_LOCK           0x00
#define XLOG_RUNNING_XACTS          0x10
#define XLOG_INVALIDATIONS          0x20

typedef struct xl_standby_lock
{
    TransactionId xid;          /* xid of holder of AccessExclusiveLock */
    Oid         dbOid;          /* DB containing table */
    Oid         relOid;         /* OID of table */
} xl_standby_lock;

typedef struct xl_standby_locks {
    int nlocks;         /* number of entries in locks array */
    xl_standby_lock locks[FLEXIBLE_ARRAY_MEMBER];
} xl_standby_locks;

/*
 * When we write running xact data to WAL, we use this structure.
 */
typedef struct xl_running_xacts {
    int xcnt;           /* # of xact ids in xids[] */
    int subxcnt;        /* # of subxact ids in xids[] */
    bool subxid_overflow;    /* snapshot overflowed, subxids missing */
    TransactionId nextXid;      /* xid from TransamVariables->nextXid */
    TransactionId oldestRunningXid; /* *not* oldestXmin */
    TransactionId latestCompletedXid;   /* so we can set xmax */

    TransactionId xids[FLEXIBLE_ARRAY_MEMBER];
} xl_running_xacts;


//typedef struct xl_invalidations {
//    Oid dbId;           /* MyDatabaseId */
//    Oid tsId;           /* MyDatabaseTableSpace */
//    bool relcacheInitFileInval;  /* invalidate relcache init files */
//    int nmsgs;          /* number of shared inval msgs */
//    SharedInvalidationMessage msgs[FLEXIBLE_ARRAY_MEMBER];
//} xl_invalidations;

#define MinSizeOfInvalidations offsetof(xl_invalidations, msgs)


void standby_desc(DecodedXLogRecord *record);

#endif //PGMONETA_RM_XLOG_H
