//
// Created by Shahryar Soltanpour on 2024-06-25.
//


#include "rm_relmap.h"
#include "rm.h"
#include "utils.h"

char *
relmap_desc(char *buf, DecodedXLogRecord *record) {
    char *rec = XLogRecGetData(record);
    uint8_t info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    if (info == XLOG_RELMAP_UPDATE) {
        xl_relmap_update *xlrec = (xl_relmap_update *) rec;

        buf = pgmoneta_format_and_append(buf, "database %u tablespace %u size %u",
                                         xlrec->dbid, xlrec->tsid, xlrec->nbytes);
    }
    return buf;
}