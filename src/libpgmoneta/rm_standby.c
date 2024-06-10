//
// Created by Shahryar Soltanpour on 2024-06-09.
//

#include "rm_standby.h"


static void
standby_desc_running_xacts(xl_running_xacts *xlrec)
{
    int         i;

    printf("nextXid %u latestCompletedXid %u oldestRunningXid %u",
                     xlrec->nextXid,
                     xlrec->latestCompletedXid,
                     xlrec->oldestRunningXid);
    if (xlrec->xcnt > 0)
    {
        printf("; %d xacts:", xlrec->xcnt);
        for (i = 0; i < xlrec->xcnt; i++)
            printf(" %u", xlrec->xids[i]);
    }

    if (xlrec->subxid_overflow)
        printf("; subxid overflowed");

    if (xlrec->subxcnt > 0)
    {
        printf("; %d subxacts:", xlrec->subxcnt);
        for (i = 0; i < xlrec->subxcnt; i++)
            printf(" %u", xlrec->xids[xlrec->xcnt + i]);
    }
}




void
standby_desc(DecodedXLogRecord *record)
{
    char       *rec = record->main_data;
    uint8_t       info = record->header.xl_info & ~XLR_INFO_MASK;

    if (info == XLOG_STANDBY_LOCK)
    {
        xl_standby_locks *xlrec = (xl_standby_locks *) rec;
        int         i;

        for (i = 0; i < xlrec->nlocks; i++)
            printf("xid %u db %u rel %u ",
                             xlrec->locks[i].xid, xlrec->locks[i].dbOid,
                             xlrec->locks[i].relOid);
    }
    else if (info == XLOG_RUNNING_XACTS)
    {
        xl_running_xacts *xlrec = (xl_running_xacts *) rec;

        standby_desc_running_xacts(xlrec);
    }
    else if (info == XLOG_INVALIDATIONS)
    {
//        xl_invalidations *xlrec = (xl_invalidations *) rec;
        printf("NOT DEFINED");

    }
}

