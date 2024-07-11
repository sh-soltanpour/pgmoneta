//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#include "wal/rm_brin.h"
#include "utils.h"


char*
brin_desc(char* buf, struct decoded_xlog_record *record)
{
    char	   *rec = XLogRecGetData(record);
    uint8_t		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    info &= XLOG_BRIN_OPMASK;
    if (info == XLOG_BRIN_CREATE_INDEX)
    {
        struct xl_brin_createidx *xlrec = (struct xl_brin_createidx *) rec;

        buf = pgmoneta_format_and_append(buf, "v%d pagesPerRange %u",
                         xlrec->version, xlrec->pagesPerRange);
    }
    else if (info == XLOG_BRIN_INSERT)
    {
        struct xl_brin_insert *xlrec = (struct xl_brin_insert *) rec;

        buf = pgmoneta_format_and_append(buf, "heapBlk %u pagesPerRange %u offnum %u",
                         xlrec->heapBlk,
                         xlrec->pagesPerRange,
                         xlrec->offnum);
    }
    else if (info == XLOG_BRIN_UPDATE)
    {
        struct xl_brin_update *xlrec = (struct xl_brin_update *) rec;

        buf = pgmoneta_format_and_append(buf, "heapBlk %u pagesPerRange %u old offnum %u, new offnum %u",
                         xlrec->insert.heapBlk,
                         xlrec->insert.pagesPerRange,
                         xlrec->oldOffnum,
                         xlrec->insert.offnum);
    }
    else if (info == XLOG_BRIN_SAMEPAGE_UPDATE)
    {
        struct xl_brin_samepage_update *xlrec = (struct xl_brin_samepage_update *) rec;

        buf = pgmoneta_format_and_append(buf, "offnum %u", xlrec->offnum);
    }
    else if (info == XLOG_BRIN_REVMAP_EXTEND)
    {
        struct xl_brin_revmap_extend *xlrec = (struct xl_brin_revmap_extend *) rec;

        buf = pgmoneta_format_and_append(buf, "targetBlk %u", xlrec->targetBlk);
    }
    else if (info == XLOG_BRIN_DESUMMARIZE)
    {
        struct xl_brin_desummarize *xlrec = (struct xl_brin_desummarize *) rec;

        buf = pgmoneta_format_and_append(buf, "pagesPerRange %u, heapBlk %u, page offset %u",
                         xlrec->pagesPerRange, xlrec->heapBlk, xlrec->regOffset);
    }
    return buf;
}