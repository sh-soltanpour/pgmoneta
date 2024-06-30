//
// Created by Shahryar Soltanpour on 2024-06-25.
//

#include "rm_seq.h"
#include "rm.h"
#include "utils.h"

char*
seq_desc(char* buf, DecodedXLogRecord *record)
{
    char	   *rec = XLogRecGetData(record);
    uint8_t		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;
    xl_seq_rec *xlrec = (xl_seq_rec *) rec;

    if (info == XLOG_SEQ_LOG)
        buf = pgmoneta_format_and_append(buf, "rel %u/%u/%u",
                         xlrec->node.spcNode, xlrec->node.dbNode,
                         xlrec->node.relNode);
    return buf;
}