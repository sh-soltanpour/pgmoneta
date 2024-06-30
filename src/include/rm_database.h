//
// Created by Shahryar Soltanpour on 2024-06-25.
//

#ifndef PGMONETA_RM_DATABASE_H
#define PGMONETA_RM_DATABASE_H

#include "wal_reader.h"

/* record types */
#define XLOG_DBASE_CREATE		0x00
#define XLOG_DBASE_DROP			0x10

typedef struct xl_dbase_create_rec
{
    /* Records copying of a single subdirectory incl. contents */
    Oid			db_id;
    Oid			tablespace_id;
    Oid			src_db_id;
    Oid			src_tablespace_id;
} xl_dbase_create_rec;

typedef struct xl_dbase_drop_rec
{
    Oid			db_id;
    int			ntablespaces;	/* number of tablespace IDs */
    Oid			tablespace_ids[FLEXIBLE_ARRAY_MEMBER];
} xl_dbase_drop_rec;
#define MinSizeOfDbaseDropRec offsetof(xl_dbase_drop_rec, tablespace_ids)


char* database_desc(char* buf, DecodedXLogRecord *record);

#endif //PGMONETA_RM_DATABASE_H
