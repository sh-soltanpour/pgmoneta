//
// Created by Shahryar Soltanpour on 2024-06-25.
//

#ifndef PGMONETA_RM_CLOG_H
#define PGMONETA_RM_CLOG_H

#include "wal_reader.h"

typedef int XidStatus;

#define TRANSACTION_STATUS_IN_PROGRESS		0x00
#define TRANSACTION_STATUS_COMMITTED		0x01
#define TRANSACTION_STATUS_ABORTED			0x02
#define TRANSACTION_STATUS_SUB_COMMITTED	0x03

typedef struct xl_clog_truncate
{
    int			pageno;
    TransactionId oldestXact;
    Oid			oldestXactDb;
} xl_clog_truncate;

#define CLOG_ZEROPAGE		0x00
#define CLOG_TRUNCATE		0x10

char* clog_desc(char* buf, DecodedXLogRecord *record);

#endif //PGMONETA_RM_CLOG_H
