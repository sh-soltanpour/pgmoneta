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
#ifndef PGMONETA_RM_BTREE_H
#define PGMONETA_RM_BTREE_H

#include "stdint.h"
#include "wal_reader.h"
#include "rm.h"
#include "transaction.h"

char* btree_desc(char* buf, struct decoded_xlog_record* record);
const char* btree_identify (uint8_t info);

typedef struct ItemIdData
{
   unsigned lp_off: 15,       /* offset to tuple (from start of page) */
            lp_flags: 2, /* state of line pointer, see below */
            lp_len: 15; /* byte length of tuple */
} ItemIdData;

#define InvalidOffsetNumber     ((offset_number) 0)
#define FirstOffsetNumber       ((offset_number) 1)
#define MaxOffsetNumber         ((offset_number) (8192 / sizeof(ItemIdData))) // TODO: Replace 8192 with block size from pg_control

/* ----------------
 *      support macros
 * ----------------
 */

/*
 * OffsetNumberIsValid
 *      True iff the offset number is valid.
 */
#define OffsetNumberIsValid(offsetNumber) \
        ((bool) ((offsetNumber != InvalidOffsetNumber) && \
                 (offsetNumber <= MaxOffsetNumber)))

#define XLOG_BTREE_INSERT_LEAF  0x00    /* add index tuple without split */
#define XLOG_BTREE_INSERT_UPPER 0x10    /* same, on a non-leaf page */
#define XLOG_BTREE_INSERT_META  0x20    /* same, plus update metapage */
#define XLOG_BTREE_SPLIT_L      0x30    /* add index tuple with split */
#define XLOG_BTREE_SPLIT_R      0x40    /* as above, new item on right */
#define XLOG_BTREE_INSERT_POST  0x50    /* add index tuple with posting split */
#define XLOG_BTREE_DEDUP        0x60    /* deduplicate tuples for a page */
#define XLOG_BTREE_DELETE       0x70    /* delete leaf index tuples for a page */
#define XLOG_BTREE_UNLINK_PAGE  0x80    /* delete a half-dead page */
#define XLOG_BTREE_UNLINK_PAGE_META 0x90    /* same, and update metapage */
#define XLOG_BTREE_NEWROOT      0xA0    /* new root page */
#define XLOG_BTREE_MARK_PAGE_HALFDEAD 0xB0  /* mark a leaf as half-dead */
#define XLOG_BTREE_VACUUM       0xC0    /* delete entries on a page during
                                         * vacuum */
#define XLOG_BTREE_REUSE_PAGE   0xD0    /* old page is about to be reused from
                                         * FSM */
#define XLOG_BTREE_META_CLEANUP 0xE0    /* update cleanup-related data in the
                                         * metapage */


/*
 * All that we need to regenerate the meta-data page
 */
struct xl_btree_metadata
{
   uint32_t version;
   block_number root;
   uint32_t level;
   block_number fastroot;
   uint32_t fastlevel;
   uint32_t last_cleanup_num_delpages;
   bool allequalimage;
};

/*
 * This is what we need to know about simple (without split) insert.
 *
 * This data record is used for INSERT_LEAF, INSERT_UPPER, INSERT_META, and
 * INSERT_POST.  Note that INSERT_META and INSERT_UPPER implies it's not a
 * leaf page, while INSERT_POST and INSERT_LEAF imply that it must be a leaf
 * page.
 *
 * Backup Blk 0: original page
 * Backup Blk 1: child's left sibling, if INSERT_UPPER or INSERT_META
 * Backup Blk 2: xl_btree_metadata, if INSERT_META
 *
 * Note: The new tuple is actually the "original" new item in the posting
 * list split insert case (i.e. the INSERT_POST case).  A split offset for
 * the posting list is logged before the original new item.  Recovery needs
 * both, since it must do an in-place update of the existing posting list
 * that was split as an extra step.  Also, recovery generates a "final"
 * newitem.  See _bt_swap_posting() for details on posting list splits.
 */
struct xl_btree_insert
{
   offset_number offnum;

   /* POSTING SPLIT OFFSET FOLLOWS (INSERT_POST case) */
   /* NEW TUPLE ALWAYS FOLLOWS AT THE END */
};

#define SizeOfBtreeInsert   (offsetof(xl_btree_insert, offnum) + sizeof(OffsetNumber))

/*
 * On insert with split, we save all the items going into the right sibling
 * so that we can restore it completely from the log record.  This way takes
 * less xlog space than the normal approach, because if we did it standardly,
 * XLogInsert would almost always think the right page is new and store its
 * whole page image.  The left page, however, is handled in the normal
 * incremental-update fashion.
 *
 * Note: XLOG_BTREE_SPLIT_L and XLOG_BTREE_SPLIT_R share this data record.
 * There are two variants to indicate whether the inserted tuple went into the
 * left or right split page (and thus, whether the new item is stored or not).
 * We always log the left page high key because suffix truncation can generate
 * a new leaf high key using user-defined code.  This is also necessary on
 * internal pages, since the firstright item that the left page's high key was
 * based on will have been truncated to zero attributes in the right page (the
 * separator key is unavailable from the right page).
 *
 * Backup Blk 0: original page / new left page
 *
 * The left page's data portion contains the new item, if it's the _L variant.
 * _R variant split records generally do not have a newitem (_R variant leaf
 * page split records that must deal with a posting list split will include an
 * explicit newitem, though it is never used on the right page -- it is
 * actually an orignewitem needed to update existing posting list).  The new
 * high key of the left/original page appears last of all (and must always be
 * present).
 *
 * Page split records that need the REDO routine to deal with a posting list
 * split directly will have an explicit newitem, which is actually an
 * orignewitem (the newitem as it was before the posting list split, not
 * after).  A posting list split always has a newitem that comes immediately
 * after the posting list being split (which would have overlapped with
 * orignewitem prior to split).  Usually REDO must deal with posting list
 * splits with an _L variant page split record, and usually both the new
 * posting list and the final newitem go on the left page (the existing
 * posting list will be inserted instead of the old, and the final newitem
 * will be inserted next to that).  However, _R variant split records will
 * include an orignewitem when the split point for the page happens to have a
 * lastleft tuple that is also the posting list being split (leaving newitem
 * as the page split's firstright tuple).  The existence of this corner case
 * does not change the basic fact about newitem/orignewitem for the REDO
 * routine: it is always state used for the left page alone.  (This is why the
 * record's postingoff field isn't a reliable indicator of whether or not a
 * posting list split occurred during the page split; a non-zero value merely
 * indicates that the REDO routine must reconstruct a new posting list tuple
 * that is needed for the left page.)
 *
 * This posting list split handling is equivalent to the xl_btree_insert REDO
 * routine's INSERT_POST handling.  While the details are more complicated
 * here, the concept and goals are exactly the same.  See _bt_swap_posting()
 * for details on posting list splits.
 *
 * Backup Blk 1: new right page
 *
 * The right page's data portion contains the right page's tuples in the form
 * used by _bt_restore_page.  This includes the new item, if it's the _R
 * variant.  The right page's tuples also include the right page's high key
 * with either variant (moved from the left/original page during the split),
 * unless the split happened to be of the rightmost page on its level, where
 * there is no high key for new right page.
 *
 * Backup Blk 2: next block (orig page's rightlink), if any
 * Backup Blk 3: child's left sibling, if non-leaf split
 */
struct xl_btree_split
{
   uint32_t level;           /* tree level of page being split */
   offset_number firstrightoff;  /* first origpage item on rightpage */
   offset_number newitemoff;     /* new item's offset */
   uint16_t postingoff;      /* offset inside orig posting tuple */
};

#define SizeOfBtreeSplit    (offsetof(xl_btree_split, postingoff) + sizeof(uint16_t))

/*
 * When page is deduplicated, consecutive groups of tuples with equal keys are
 * merged together into posting list tuples.
 *
 * The WAL record represents a deduplication pass for a leaf page.  An array
 * of BTDedupInterval structs follows.
 */
struct xl_btree_dedup
{
   uint16_t nintervals;

   /* DEDUPLICATION INTERVALS FOLLOW */
};

#define SizeOfBtreeDedup    (offsetof(xl_btree_dedup, nintervals) + sizeof(uint16_t))

/*
 * This is what we need to know about page reuse within btree.  This record
 * only exists to generate a conflict point for Hot Standby.
 *
 * Note that we must include a rel_file_locator in the record because we don't
 * actually register the buffer with the record.
 */
struct xl_btree_reuse_page
{
   struct rel_file_locator locator;
   block_number block;
   FullTransactionId snapshotConflictHorizon;
   bool isCatalogRel;    /* to handle recovery conflict during logical
                          * decoding on standby */
};

#define SizeOfBtreeReusePage    (offsetof(xl_btree_reuse_page, isCatalogRel) + sizeof(bool))

/*
 * xl_btree_vacuum and xl_btree_delete records describe deletion of index
 * tuples on a leaf page.  The former variant is used by VACUUM, while the
 * latter variant is used by the ad-hoc deletions that sometimes take place
 * when btinsert() is called.
 *
 * The records are very similar.  The only difference is that xl_btree_delete
 * have snapshotConflictHorizon/isCatalogRel fields for recovery conflicts.
 * (VACUUM operations can just rely on earlier conflicts generated during
 * pruning of the table whose TIDs the to-be-deleted index tuples point to.
 * There are also small differences between each REDO routine that we don't go
 * into here.)
 *
 * xl_btree_vacuum and xl_btree_delete both represent deletion of any number
 * of index tuples on a single leaf page using page offset numbers.  Both also
 * support "updates" of index tuples, which is how deletes of a subset of TIDs
 * contained in an existing posting list tuple are implemented.
 *
 * Updated posting list tuples are represented using xl_btree_update metadata.
 * The REDO routines each use the xl_btree_update entries (plus each
 * corresponding original index tuple from the target leaf page) to generate
 * the final updated tuple.
 *
 * Updates are only used when there will be some remaining TIDs left by the
 * REDO routine.  Otherwise the posting list tuple just gets deleted outright.
 */
struct xl_btree_vacuum
{
   uint16_t ndeleted;
   uint16_t nupdated;

   /*----
    * In payload of blk 0 :
    * - DELETED TARGET OFFSET NUMBERS
    * - UPDATED TARGET OFFSET NUMBERS
    * - UPDATED TUPLES METADATA (xl_btree_update) ITEMS
    *----
    */
};

#define SizeOfBtreeVacuum   (offsetof(xl_btree_vacuum, nupdated) + sizeof(uint16_t))

struct xl_btree_delete
{
   TransactionId snapshotConflictHorizon;
   uint16_t ndeleted;
   uint16_t nupdated;
   bool isCatalogRel;    /* to handle recovery conflict during logical
                          * decoding on standby */

   /*----
    * In payload of blk 0 :
    * - DELETED TARGET OFFSET NUMBERS
    * - UPDATED TARGET OFFSET NUMBERS
    * - UPDATED TUPLES METADATA (xl_btree_update) ITEMS
    *----
    */
};

#define SizeOfBtreeDelete   (offsetof(xl_btree_delete, isCatalogRel) + sizeof(bool))

/*
 * The offsets that appear in xl_btree_update metadata are offsets into the
 * original posting list from tuple, not page offset numbers.  These are
 * 0-based.  The page offset number for the original posting list tuple comes
 * from the main xl_btree_vacuum/xl_btree_delete record.
 */
struct xl_btree_update
{
   uint16_t ndeletedtids;

   /* POSTING LIST uint16_t OFFSETS TO A DELETED TID FOLLOW */
};

#define SizeOfBtreeUpdate   (offsetof(struct xl_btree_update, ndeletedtids) + sizeof(uint16_t))

/*
 * This is what we need to know about marking an empty subtree for deletion.
 * The target identifies the tuple removed from the parent page (note that we
 * remove this tuple's downlink and the *following* tuple's key).  Note that
 * the leaf page is empty, so we don't need to store its content --- it is
 * just reinitialized during recovery using the rest of the fields.
 *
 * Backup Blk 0: leaf block
 * Backup Blk 1: top parent
 */
struct xl_btree_mark_page_halfdead
{
   offset_number poffset;        /* deleted tuple id in parent page */

   /* information needed to recreate the leaf page: */
   block_number leafblk;         /* leaf block ultimately being deleted */
   block_number leftblk;         /* leaf block's left sibling, if any */
   block_number rightblk;        /* leaf block's right sibling */
   block_number topparent;       /* topmost internal page in the subtree */
};

#define SizeOfBtreeMarkPageHalfDead (offsetof(xl_btree_mark_page_halfdead, topparent) + sizeof(BlockNumber))

/*
 * This is what we need to know about deletion of a btree page.  Note that we
 * only leave behind a small amount of bookkeeping information in deleted
 * pages (deleted pages must be kept around as tombstones for a while).  It is
 * convenient for the REDO routine to regenerate its target page from scratch.
 * This is why WAL record describes certain details that are actually directly
 * available from the target page.
 *
 * Backup Blk 0: target block being deleted
 * Backup Blk 1: target block's left sibling, if any
 * Backup Blk 2: target block's right sibling
 * Backup Blk 3: leaf block (if different from target)
 * Backup Blk 4: metapage (if rightsib becomes new fast root)
 */
struct xl_btree_unlink_page
{
   block_number leftsib;         /* target block's left sibling, if any */
   block_number rightsib;        /* target block's right sibling */
   uint32_t level;           /* target block's level */
   FullTransactionId safexid;   /* target block's BTPageSetDeleted() XID */

   /*
    * Information needed to recreate a half-dead leaf page with correct
    * topparent link.  The fields are only used when deletion operation's
    * target page is an internal page.  REDO routine creates half-dead page
    * from scratch to keep things simple (this is the same convenient
    * approach used for the target page itself).
    */
   block_number leafleftsib;
   block_number leafrightsib;
   block_number leaftopparent;   /* next child down in the subtree */

   /* xl_btree_metadata FOLLOWS IF XLOG_BTREE_UNLINK_PAGE_META */
};

#define SizeOfBtreeUnlinkPage   (offsetof(xl_btree_unlink_page, leaftopparent) + sizeof(BlockNumber))

/*
 * New root log record.  There are zero tuples if this is to establish an
 * empty root, or two if it is the result of splitting an old root.
 *
 * Note that although this implies rewriting the metadata page, we don't need
 * an xl_btree_metadata record --- the rootblk and level are sufficient.
 *
 * Backup Blk 0: new root page (2 tuples as payload, if splitting old root)
 * Backup Blk 1: left child (if splitting an old root)
 * Backup Blk 2: metapage
 */
struct xl_btree_newroot
{
   block_number rootblk;         /* location of new root (redundant with blk 0) */
   uint32_t level;           /* its tree level */
};

#define SizeOfBtreeNewroot  (offsetof(xl_btree_newroot, level) + sizeof(uint32_t))

#endif //PGMONETA_RM_BTREE_H