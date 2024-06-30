//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#include "rm_replorigin.h"
#include "rm.h"
#include "utils.h"


char*
replorigin_desc(char* buf, DecodedXLogRecord *record)
{
    char	   *rec = XLogRecGetData(record);
    uint8_t		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    switch (info)
    {
        case XLOG_REPLORIGIN_SET:
        {
            xl_replorigin_set *xlrec;

            xlrec = (xl_replorigin_set *) rec;

            buf = pgmoneta_format_and_append(buf, "set %u; lsn %X/%X; force: %d",
                             xlrec->node_id,
                             LSN_FORMAT_ARGS(xlrec->remote_lsn),
                             xlrec->force);
            break;
        }
        case XLOG_REPLORIGIN_DROP:
        {
            xl_replorigin_drop *xlrec;

            xlrec = (xl_replorigin_drop *) rec;

            buf = pgmoneta_format_and_append(buf, "drop %u", xlrec->node_id);
            break;
        }
    }
    return buf;
}
