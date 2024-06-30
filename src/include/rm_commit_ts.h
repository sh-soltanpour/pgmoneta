//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#ifndef PGMONETA_RM_COMMIT_TS_H
#define PGMONETA_RM_COMMIT_TS_H

#include "wal_reader.h"
#include "rm.h"
#include "rm_xlog.h" //TODO: remove this and move timestamp somewhere else


/* XLOG stuff */
#define COMMIT_TS_ZEROPAGE		0x00
#define COMMIT_TS_TRUNCATE		0x10

typedef struct xl_commit_ts_set
{
    TimestampTz timestamp;
    RepOriginId nodeid;
    TransactionId mainxid;
    /* subxact Xids follow */
}			xl_commit_ts_set;

#define SizeOfCommitTsSet	(offsetof(xl_commit_ts_set, mainxid) + \
							 sizeof(TransactionId))

typedef struct xl_commit_ts_truncate
{
    int			pageno;
    TransactionId oldestXid;
} xl_commit_ts_truncate;

#define SizeOfCommitTsTruncate	(offsetof(xl_commit_ts_truncate, oldestXid) + \
								 sizeof(TransactionId))

char* commit_ts_desc(char* buf, DecodedXLogRecord *record);


#endif //PGMONETA_RM_COMMIT_TS_H
