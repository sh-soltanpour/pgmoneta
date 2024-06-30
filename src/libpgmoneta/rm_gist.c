//
// Created by Shahryar Soltanpour on 2024-06-25.
//

#include "rm_gist.h"
#include "rm.h"
#include "utils.h"


static char *
out_gistxlogPageUpdate(char *buf, gistxlogPageUpdate *xlrec) {
    return buf;
}

static char *
out_gistxlogPageReuse(char *buf, gistxlogPageReuse *xlrec) {
    buf = pgmoneta_format_and_append(buf, "rel %u/%u/%u; blk %u; latestRemovedXid %u:%u",
                                     xlrec->node.spcNode, xlrec->node.dbNode,
                                     xlrec->node.relNode, xlrec->block,
                                     EpochFromFullTransactionId(xlrec->latestRemovedFullXid),
                                     XidFromFullTransactionId(xlrec->latestRemovedFullXid));
    return buf;
}

static char *
out_gistxlogDelete(char *buf, gistxlogDelete *xlrec) {
    buf = pgmoneta_format_and_append(buf, "delete: latestRemovedXid %u, nitems: %u",
                                     xlrec->latestRemovedXid, xlrec->ntodelete);
    return buf;

}

static char*
out_gistxlogPageSplit(char *buf, gistxlogPageSplit *xlrec) {
    buf = pgmoneta_format_and_append(buf, "page_split: splits to %d pages",
            xlrec->npage);
    return buf;
}

static char*
out_gistxlogPageDelete(char *buf, gistxlogPageDelete *xlrec) {
    buf = pgmoneta_format_and_append(buf, "deleteXid %u:%u; downlink %u",
            EpochFromFullTransactionId(xlrec->deleteXid),
            XidFromFullTransactionId(xlrec->deleteXid),
            xlrec->downlinkOffset);
    return buf;
}

char *
gist_desc(char *buf, DecodedXLogRecord *record) {
    char *rec = XLogRecGetData(record);
    uint8_t info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    switch (info) {
        case XLOG_GIST_PAGE_UPDATE:
            buf = out_gistxlogPageUpdate(buf, (gistxlogPageUpdate *) rec);
            break;
        case XLOG_GIST_PAGE_REUSE:
            buf = out_gistxlogPageReuse(buf, (gistxlogPageReuse *) rec);
            break;
        case XLOG_GIST_DELETE:
            buf = out_gistxlogDelete(buf, (gistxlogDelete *) rec);
            break;
        case XLOG_GIST_PAGE_SPLIT:
            buf = out_gistxlogPageSplit(buf, (gistxlogPageSplit *) rec);
            break;
        case XLOG_GIST_PAGE_DELETE:
            buf = out_gistxlogPageDelete(buf, (gistxlogPageDelete *) rec);
            break;
        case XLOG_GIST_ASSIGN_LSN:
            /* No details to write out */
            break;
    }
    return buf;
}
