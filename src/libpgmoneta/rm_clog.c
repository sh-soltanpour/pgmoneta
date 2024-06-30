//
// Created by Shahryar Soltanpour on 2024-06-25.
//

#include "rm_clog.h"
#include "string.h"
#include "utils.h"
#include "rm.h"

char*
clog_desc(char* buf,  DecodedXLogRecord *record)
{
    char	   *rec = XLogRecGetData(record);
    uint8_t		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    if (info == CLOG_ZEROPAGE)
    {
        int			pageno;

        memcpy(&pageno, rec, sizeof(int));
        buf = pgmoneta_format_and_append(buf, "page %d", pageno);
    }
    else if (info == CLOG_TRUNCATE)
    {
        xl_clog_truncate xlrec;

        memcpy(&xlrec, rec, sizeof(xl_clog_truncate));
        buf = pgmoneta_format_and_append(buf, "page %d; oldestXact %u",
                         xlrec.pageno, xlrec.oldestXact);
    }
    return buf;
}