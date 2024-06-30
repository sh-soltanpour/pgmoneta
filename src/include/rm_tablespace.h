//
// Created by Shahryar Soltanpour on 2024-06-25.
//

#ifndef PGMONETA_RM_TABLESPACE_H
#define PGMONETA_RM_TABLESPACE_H

#include "wal_reader.h"

/* XLOG stuff */
#define XLOG_TBLSPC_CREATE		0x00
#define XLOG_TBLSPC_DROP		0x10

typedef struct xl_tblspc_create_rec
{
    Oid			ts_id;
    char		ts_path[FLEXIBLE_ARRAY_MEMBER]; /* null-terminated string */
} xl_tblspc_create_rec;

typedef struct xl_tblspc_drop_rec
{
    Oid			ts_id;
} xl_tblspc_drop_rec;

char* tablespace_desc(char* buf, DecodedXLogRecord *record);

#endif //PGMONETA_RM_TABLESPACE_H
