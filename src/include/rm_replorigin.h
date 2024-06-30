//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#ifndef PGMONETA_RM_REPLORIGIN_H
#define PGMONETA_RM_REPLORIGIN_H

#include "wal_reader.h"

typedef struct xl_replorigin_set
{
    XLogRecPtr	remote_lsn;
    RepOriginId node_id;
    bool		force;
} xl_replorigin_set;

typedef struct xl_replorigin_drop
{
    RepOriginId node_id;
} xl_replorigin_drop;

#define XLOG_REPLORIGIN_SET		0x00
#define XLOG_REPLORIGIN_DROP		0x10

#define InvalidRepOriginId 0
#define DoNotReplicateId PG_UINT16_MAX

char* replorigin_desc(char* buf, DecodedXLogRecord *record);


#endif //PGMONETA_RM_REPLORIGIN_H
