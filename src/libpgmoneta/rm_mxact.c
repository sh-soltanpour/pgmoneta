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


#include "wal/rm_mxact.h"
#include "wal/rm.h"
#include "utils.h"


static char*
out_member(char* buf, MultiXactMember *member)
{
    buf = pgmoneta_format_and_append(buf, "%u ", member->xid);
    switch (member->status)
    {
        case MultiXactStatusForKeyShare:
            buf = pgmoneta_format_and_append(buf, "(keysh) ");
            break;
        case MultiXactStatusForShare:
            buf = pgmoneta_format_and_append(buf, "(sh) ");
            break;
        case MultiXactStatusForNoKeyUpdate:
            buf = pgmoneta_format_and_append(buf, "(fornokeyupd) ");
            break;
        case MultiXactStatusForUpdate:
            buf = pgmoneta_format_and_append(buf, "(forupd) ");
            break;
        case MultiXactStatusNoKeyUpdate:
            buf = pgmoneta_format_and_append(buf, "(nokeyupd) ");
            break;
        case MultiXactStatusUpdate:
            buf = pgmoneta_format_and_append(buf, "(upd) ");
            break;
        default:
            buf = pgmoneta_format_and_append(buf, "(unk) ");
            break;
    }
    return buf;
}


char*
multixact_desc(char* buf, DecodedXLogRecord *record)
{
    char	   *rec = XLogRecGetData(record);
    uint8_t		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    if (info == XLOG_MULTIXACT_ZERO_OFF_PAGE ||
        info == XLOG_MULTIXACT_ZERO_MEM_PAGE)
    {
        int			pageno;

        memcpy(&pageno, rec, sizeof(int));
        buf = pgmoneta_format_and_append(buf, "%d", pageno);
    }
    else if (info == XLOG_MULTIXACT_CREATE_ID)
    {
        xl_multixact_create *xlrec = (xl_multixact_create *) rec;
        int			i;

        buf = pgmoneta_format_and_append(buf, "%u offset %u nmembers %d: ", xlrec->mid,
                         xlrec->moff, xlrec->nmembers);
        for (i = 0; i < xlrec->nmembers; i++)
            out_member(buf, &xlrec->members[i]);
    }
    else if (info == XLOG_MULTIXACT_TRUNCATE_ID)
    {
        xl_multixact_truncate *xlrec = (xl_multixact_truncate *) rec;

        buf = pgmoneta_format_and_append(buf, "offsets [%u, %u), members [%u, %u)",
                         xlrec->startTruncOff, xlrec->endTruncOff,
                         xlrec->startTruncMemb, xlrec->endTruncMemb);
    }
    return buf;
}