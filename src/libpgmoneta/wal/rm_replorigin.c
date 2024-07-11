//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#include "wal/rm_replorigin.h"
#include "wal/rm.h"
#include "utils.h"


char*
replorigin_desc(char* buf, struct decoded_xlog_record *record)
{
    char	   *rec = XLogRecGetData(record);
    uint8_t		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    switch (info)
    {
        case XLOG_REPLORIGIN_SET:
        {
            struct xl_replorigin_set *xlrec;

            xlrec = (struct xl_replorigin_set *) rec;

            buf = pgmoneta_format_and_append(buf, "set %u; lsn %X/%X; force: %d",
                             xlrec->node_id,
                             LSN_FORMAT_ARGS(xlrec->remote_lsn),
                             xlrec->force);
            break;
        }
        case XLOG_REPLORIGIN_DROP:
        {
            struct xl_replorigin_drop *xlrec;

            xlrec = (struct xl_replorigin_drop *) rec;

            buf = pgmoneta_format_and_append(buf, "drop %u", xlrec->node_id);
            break;
        }
    }
    return buf;
}
