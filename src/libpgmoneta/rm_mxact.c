//
// Created by Shahryar Soltanpour on 2024-06-25.
//


#include "rm_mxact.h"
#include "rm.h"
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