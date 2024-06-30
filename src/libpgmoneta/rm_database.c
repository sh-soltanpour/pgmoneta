//
// Created by Shahryar Soltanpour on 2024-06-25.
//


#include "rm_database.h"
#include "rm.h"
#include "utils.h"


char*
database_desc(char* buf, DecodedXLogRecord *record)
{
    char	   *rec = XLogRecGetData(record);
    uint8_t		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    if (info == XLOG_DBASE_CREATE)
    {
        xl_dbase_create_rec *xlrec = (xl_dbase_create_rec *) rec;

        buf = pgmoneta_format_and_append(buf, "copy dir %u/%u to %u/%u",
                         xlrec->src_tablespace_id, xlrec->src_db_id,
                         xlrec->tablespace_id, xlrec->db_id);
    }
    else if (info == XLOG_DBASE_DROP)
    {
        xl_dbase_drop_rec *xlrec = (xl_dbase_drop_rec *) rec;
        int			i;

        buf = pgmoneta_format_and_append(buf, "dir");
        for (i = 0; i < xlrec->ntablespaces; i++)
            buf = pgmoneta_format_and_append(buf, " %u/%u",
                             xlrec->tablespace_ids[i], xlrec->db_id);
    }
    return buf;
}