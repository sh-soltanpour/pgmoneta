//
// Created by Shahryar Soltanpour on 2024-06-25.
//


#include "rm_tablespace.h"
#include "rm.h"
#include "utils.h"


char *
tablespace_desc(char *buf, DecodedXLogRecord *record) {
    char *rec = XLogRecGetData(record);
    uint8_t info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    if (info == XLOG_TBLSPC_CREATE) {
        xl_tblspc_create_rec *xlrec = (xl_tblspc_create_rec *) rec;

        buf = pgmoneta_format_and_append(buf, "%u \"%s\"", xlrec->ts_id, xlrec->ts_path);
    } else if (info == XLOG_TBLSPC_DROP) {
        xl_tblspc_drop_rec *xlrec = (xl_tblspc_drop_rec *) rec;

        buf = pgmoneta_format_and_append(buf, "%u", xlrec->ts_id);
    }
    return buf;
}