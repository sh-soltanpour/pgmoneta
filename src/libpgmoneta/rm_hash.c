//
// Created by Shahryar Soltanpour on 2024-06-25.
//

#include "rm_hash.h"
#include "rm.h"
#include "utils.h"

char*
hash_desc(char* buf, DecodedXLogRecord *record)
{
    char	   *rec = XLogRecGetData(record);
    uint8_t		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    switch (info)
    {
        case XLOG_HASH_INIT_META_PAGE:
        {
            xl_hash_init_meta_page *xlrec = (xl_hash_init_meta_page *) rec;

            buf = pgmoneta_format_and_append(buf, "num_tuples %g, fillfactor %d",
                             xlrec->num_tuples, xlrec->ffactor);
            break;
        }
        case XLOG_HASH_INIT_BITMAP_PAGE:
        {
            xl_hash_init_bitmap_page *xlrec = (xl_hash_init_bitmap_page *) rec;

            buf = pgmoneta_format_and_append(buf, "bmsize %d", xlrec->bmsize);
            break;
        }
        case XLOG_HASH_INSERT:
        {
            xl_hash_insert *xlrec = (xl_hash_insert *) rec;

            buf = pgmoneta_format_and_append(buf, "off %u", xlrec->offnum);
            break;
        }
        case XLOG_HASH_ADD_OVFL_PAGE:
        {
            xl_hash_add_ovfl_page *xlrec = (xl_hash_add_ovfl_page *) rec;

            buf = pgmoneta_format_and_append(buf, "bmsize %d, bmpage_found %c",
                             xlrec->bmsize, (xlrec->bmpage_found) ? 'T' : 'F');
            break;
        }
        case XLOG_HASH_SPLIT_ALLOCATE_PAGE:
        {
            xl_hash_split_allocate_page *xlrec = (xl_hash_split_allocate_page *) rec;

            buf = pgmoneta_format_and_append(buf, "new_bucket %u, meta_page_masks_updated %c, issplitpoint_changed %c",
                             xlrec->new_bucket,
                             (xlrec->flags & XLH_SPLIT_META_UPDATE_MASKS) ? 'T' : 'F',
                             (xlrec->flags & XLH_SPLIT_META_UPDATE_SPLITPOINT) ? 'T' : 'F');
            break;
        }
        case XLOG_HASH_SPLIT_COMPLETE:
        {
            xl_hash_split_complete *xlrec = (xl_hash_split_complete *) rec;

            buf = pgmoneta_format_and_append(buf, "old_bucket_flag %u, new_bucket_flag %u",
                             xlrec->old_bucket_flag, xlrec->new_bucket_flag);
            break;
        }
        case XLOG_HASH_MOVE_PAGE_CONTENTS:
        {
            xl_hash_move_page_contents *xlrec = (xl_hash_move_page_contents *) rec;

            buf = pgmoneta_format_and_append(buf, "ntups %d, is_primary %c",
                             xlrec->ntups,
                             xlrec->is_prim_bucket_same_wrt ? 'T' : 'F');
            break;
        }
        case XLOG_HASH_SQUEEZE_PAGE:
        {
            xl_hash_squeeze_page *xlrec = (xl_hash_squeeze_page *) rec;

            buf = pgmoneta_format_and_append(buf, "prevblkno %u, nextblkno %u, ntups %d, is_primary %c",
                             xlrec->prevblkno,
                             xlrec->nextblkno,
                             xlrec->ntups,
                             xlrec->is_prim_bucket_same_wrt ? 'T' : 'F');
            break;
        }
        case XLOG_HASH_DELETE:
        {
            xl_hash_delete *xlrec = (xl_hash_delete *) rec;

            buf = pgmoneta_format_and_append(buf, "clear_dead_marking %c, is_primary %c",
                             xlrec->clear_dead_marking ? 'T' : 'F',
                             xlrec->is_primary_bucket_page ? 'T' : 'F');
            break;
        }
        case XLOG_HASH_UPDATE_META_PAGE:
        {
            xl_hash_update_meta_page *xlrec = (xl_hash_update_meta_page *) rec;

            buf = pgmoneta_format_and_append(buf, "ntuples %g",
                             xlrec->ntuples);
            break;
        }
        case XLOG_HASH_VACUUM_ONE_PAGE:
        {
            xl_hash_vacuum_one_page *xlrec = (xl_hash_vacuum_one_page *) rec;

            buf = pgmoneta_format_and_append(buf, "ntuples %d, latestRemovedXid %u",
                             xlrec->ntuples,
                             xlrec->latestRemovedXid);
            break;
        }
    }
    return buf;
}