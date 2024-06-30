//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#include "rm_brin.h"
#include "utils.h"


char*
brin_desc(char* buf, DecodedXLogRecord *record)
{
    char	   *rec = XLogRecGetData(record);
    uint8_t		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    info &= XLOG_BRIN_OPMASK;
    if (info == XLOG_BRIN_CREATE_INDEX)
    {
        xl_brin_createidx *xlrec = (xl_brin_createidx *) rec;

        buf = pgmoneta_format_and_append(buf, "v%d pagesPerRange %u",
                         xlrec->version, xlrec->pagesPerRange);
    }
    else if (info == XLOG_BRIN_INSERT)
    {
        xl_brin_insert *xlrec = (xl_brin_insert *) rec;

        buf = pgmoneta_format_and_append(buf, "heapBlk %u pagesPerRange %u offnum %u",
                         xlrec->heapBlk,
                         xlrec->pagesPerRange,
                         xlrec->offnum);
    }
    else if (info == XLOG_BRIN_UPDATE)
    {
        xl_brin_update *xlrec = (xl_brin_update *) rec;

        buf = pgmoneta_format_and_append(buf, "heapBlk %u pagesPerRange %u old offnum %u, new offnum %u",
                         xlrec->insert.heapBlk,
                         xlrec->insert.pagesPerRange,
                         xlrec->oldOffnum,
                         xlrec->insert.offnum);
    }
    else if (info == XLOG_BRIN_SAMEPAGE_UPDATE)
    {
        xl_brin_samepage_update *xlrec = (xl_brin_samepage_update *) rec;

        buf = pgmoneta_format_and_append(buf, "offnum %u", xlrec->offnum);
    }
    else if (info == XLOG_BRIN_REVMAP_EXTEND)
    {
        xl_brin_revmap_extend *xlrec = (xl_brin_revmap_extend *) rec;

        buf = pgmoneta_format_and_append(buf, "targetBlk %u", xlrec->targetBlk);
    }
    else if (info == XLOG_BRIN_DESUMMARIZE)
    {
        xl_brin_desummarize *xlrec = (xl_brin_desummarize *) rec;

        buf = pgmoneta_format_and_append(buf, "pagesPerRange %u, heapBlk %u, page offset %u",
                         xlrec->pagesPerRange, xlrec->heapBlk, xlrec->regOffset);
    }
    return buf;
}