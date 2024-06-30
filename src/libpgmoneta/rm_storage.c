//
// Created by Shahryar Soltanpour on 2024-06-25.
//


#include "utils.h"
#include "rm_storage.h"
#include "rm.h"
#include "relpath.h"

char*
storage_desc(char* buf, DecodedXLogRecord *record)
{
    char	   *rec = XLogRecGetData(record);
    uint8_t		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    if (info == XLOG_SMGR_CREATE)
    {
        xl_smgr_create *xlrec = (xl_smgr_create *) rec;
        char	   *path = relpathperm(xlrec->rnode, xlrec->forkNum);

        buf = pgmoneta_format_and_append(buf, path);
        free(path);
    }
    else if (info == XLOG_SMGR_TRUNCATE)
    {
        xl_smgr_truncate *xlrec = (xl_smgr_truncate *) rec;
        char	   *path = relpathperm(xlrec->rnode, MAIN_FORKNUM);

        buf = pgmoneta_format_and_append(buf, "%s to %u blocks flags %d", path,
                         xlrec->blkno, xlrec->flags);
        free(path);
    }
    return buf;
}
