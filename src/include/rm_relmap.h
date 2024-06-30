//
// Created by Shahryar Soltanpour on 2024-06-25.
//

#ifndef PGMONETA_RM_RELMAP_H
#define PGMONETA_RM_RELMAP_H

#include "wal_reader.h"
#define XLOG_RELMAP_UPDATE		0x00

typedef struct xl_relmap_update
{
    Oid			dbid;			/* database ID, or 0 for shared map */
    Oid			tsid;			/* database's tablespace, or pg_global */
    int32_t 		nbytes;			/* size of relmap data */
    char		data[FLEXIBLE_ARRAY_MEMBER];
} xl_relmap_update;

#define MinSizeOfRelmapUpdate offsetof(xl_relmap_update, data)

char* relmap_desc(char* buf, DecodedXLogRecord *record);

#endif //PGMONETA_RM_RELMAP_H
