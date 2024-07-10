//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#include "wal/rm_commit_ts.h"
#include "utils.h"


char*
commit_ts_desc(char* buf, struct decoded_xlog_record *record)
{
    char	   *rec = XLogRecGetData(record);
    uint8_t		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    if (info == COMMIT_TS_ZEROPAGE)
    {
        int			pageno;

        memcpy(&pageno, rec, sizeof(int));
        buf = pgmoneta_format_and_append(buf, "%d", pageno);
    }
    else if (info == COMMIT_TS_TRUNCATE)
    {
        struct xl_commit_ts_truncate *trunc = (struct xl_commit_ts_truncate *) rec;

        buf = pgmoneta_format_and_append(buf, "pageno %d, oldestXid %u",
                         trunc->pageno, trunc->oldestXid);
    }
    return buf;
}