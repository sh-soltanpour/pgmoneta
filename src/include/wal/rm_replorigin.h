//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#ifndef PGMONETA_RM_REPLORIGIN_H
#define PGMONETA_RM_REPLORIGIN_H

#include "wal_reader.h"

struct xl_replorigin_set
{
    xlog_rec_ptr	remote_lsn;
    rep_origin_id node_id;
    bool		force;
};

struct xl_replorigin_drop
{
    rep_origin_id node_id;
};

#define XLOG_REPLORIGIN_SET		0x00
#define XLOG_REPLORIGIN_DROP		0x10

#define InvalidRepOriginId 0
#define DoNotReplicateId PG_UINT16_MAX

char* replorigin_desc(char* buf, struct decoded_xlog_record *record);


#endif //PGMONETA_RM_REPLORIGIN_H
