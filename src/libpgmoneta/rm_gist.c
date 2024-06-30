/*
 * Copyright (C) 2024 The pgmoneta community
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may
 * be used to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
