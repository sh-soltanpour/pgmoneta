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


#ifndef PGMONETA_RM_GIN_H
#define PGMONETA_RM_GIN_H


#include <rm.h>
#include "stdint.h"
#include "wal_reader.h"

#define XLOG_GIN_CREATE_PTREE  0x10


/*
 * Index tuple header structure
 *
 * All index tuples start with IndexTupleData.  If the HasNulls bit is set,
 * this is followed by an IndexAttributeBitMapData.  The index attribute
 * values follow, beginning at a MAXALIGN boundary.
 *
 * Note that the space allocated for the bitmap does not vary with the number
 * of attributes; that is because we don't have room to store the number of
 * attributes in the header.  Given the MAXALIGN constraint there's no space
 * savings to be had anyway, for usual values of INDEX_MAX_KEYS.
 */

typedef struct IndexTupleData
{
    ItemPointerData t_tid;		/* reference TID to heap tuple */

    /* ---------------
     * t_info is laid out in the following fashion:
     *
     * 15th (high) bit: has nulls
     * 14th bit: has var-width attributes
     * 13th bit: AM-defined meaning
     * 12-0 bit: size of tuple
     * ---------------
     */

    unsigned short t_info;		/* various info about tuple */

} IndexTupleData;				/* MORE DATA FOLLOWS AT END OF STRUCT */



/*
 * Posting item in a non-leaf posting-tree page
 */
typedef struct
{
    /* We use BlockIdData not BlockNumber to avoid padding space wastage */
    BlockIdData child_blkno;
    ItemPointerData key;
} PostingItem;


/*
 * Flags used in ginxlogInsert and ginxlogSplit records
 */
#define GIN_INSERT_ISDATA	0x01	/* for both insert and split records */
#define GIN_INSERT_ISLEAF	0x02	/* ditto */
#define GIN_SPLIT_ROOT		0x04	/* only for split records */



typedef struct ginxlogCreatePostingTree
{
    uint32_t		size;
    /* A compressed posting list follows */
} ginxlogCreatePostingTree;

/*
 * The format of the insertion record varies depending on the page type.
 * ginxlogInsert is the common part between all variants.
 *
 * Backup Blk 0: target page
 * Backup Blk 1: left child, if this insertion finishes an incomplete split
 */

#define XLOG_GIN_INSERT  0x20

typedef struct
{
    uint16_t		flags;			/* GIN_INSERT_ISLEAF and/or GIN_INSERT_ISDATA */

    /*
     * FOLLOWS:
     *
     * 1. if not leaf page, block numbers of the left and right child pages
     * whose split this insertion finishes, as BlockIdData[2] (beware of
     * adding fields in this struct that would make them not 16-bit aligned)
     *
     * 2. a ginxlogInsertEntry or ginxlogRecompressDataLeaf struct, depending
     * on tree type.
     *
     * NB: the below structs are only 16-bit aligned when appended to a
     * ginxlogInsert struct! Beware of adding fields to them that require
     * stricter alignment.
     */
} ginxlogInsert;

typedef struct
{
    OffsetNumber offset;
    bool		isDelete;
    IndexTupleData tuple;		/* variable length */
} ginxlogInsertEntry;


typedef struct
{
    uint16_t		nactions;

    /* Variable number of 'actions' follow */
} ginxlogRecompressDataLeaf;

typedef struct
{
    OffsetNumber offset;
    PostingItem newitem;
} ginxlogInsertDataInternal;

#define XLOG_GIN_SPLIT	0x30



typedef struct ginxlogSplit
{
    RelFileNode node;
    BlockNumber rrlink;			/* right link, or root's blocknumber if root
								 * split */
    BlockNumber leftChildBlkno; /* valid on a non-leaf split */
    BlockNumber rightChildBlkno;
    uint16_t		flags;			/* see below */
} ginxlogSplit;

/*
 * Vacuum simply WAL-logs the whole page, when anything is modified. This
 * is functionally identical to XLOG_FPI records, but is kept separate for
 * debugging purposes. (When inspecting the WAL stream, it's easier to see
 * what's going on when GIN vacuum records are marked as such, not as heap
 * records.) This is currently only used for entry tree leaf pages.
 */
#define XLOG_GIN_VACUUM_PAGE	0x40

/*
 * Vacuuming posting tree leaf page is WAL-logged like recompression caused
 * by insertion.
 */
#define XLOG_GIN_VACUUM_DATA_LEAF_PAGE	0x90

typedef struct ginxlogVacuumDataLeafPage
{
    ginxlogRecompressDataLeaf data;
} ginxlogVacuumDataLeafPage;


#define XLOG_GIN_DELETE_PAGE	0x50

typedef struct ginxlogDeletePage
{
    OffsetNumber parentOffset;
    BlockNumber rightLink;
    TransactionId deleteXid;	/* last Xid which could see this page in scan */
} ginxlogDeletePage;

#define XLOG_GIN_UPDATE_META_PAGE 0x60


typedef struct GinMetaPageData
{
    /*
     * Pointers to head and tail of pending list, which consists of GIN_LIST
     * pages.  These store fast-inserted entries that haven't yet been moved
     * into the regular GIN structure.
     */
    BlockNumber head;
    BlockNumber tail;

    /*
     * Free space in bytes in the pending list's tail page.
     */
    uint32_t		tailFreeSize;

    /*
     * We store both number of pages and number of heap tuples that are in the
     * pending list.
     */
    BlockNumber nPendingPages;
    int64_t		nPendingHeapTuples;

    /*
     * Statistics for planner use (accurate as of last VACUUM)
     */
    BlockNumber nTotalPages;
    BlockNumber nEntryPages;
    BlockNumber nDataPages;
    int64_t		nEntries;

    /*
     * GIN version number (ideally this should have been at the front, but too
     * late now.  Don't move it!)
     *
     * Currently 2 (for indexes initialized in 9.4 or later)
     *
     * Version 1 (indexes initialized in version 9.1, 9.2 or 9.3), is
     * compatible, but may contain uncompressed posting tree (leaf) pages and
     * posting lists. They will be converted to compressed format when
     * modified.
     *
     * Version 0 (indexes initialized in 9.0 or before) is compatible but may
     * be missing null entries, including both null keys and placeholders.
     * Reject full-index-scan attempts on such indexes.
     */
    int32_t		ginVersion;
} GinMetaPageData;

#define GIN_CURRENT_VERSION		2


/*
 * Backup Blk 0: metapage
 * Backup Blk 1: tail page
 */
typedef struct ginxlogUpdateMeta
{
    RelFileNode node;
    GinMetaPageData metadata;
    BlockNumber prevTail;
    BlockNumber newRightlink;
    int32_t		ntuples;		/* if ntuples > 0 then metadata.tail was
								 * updated with that many tuples; else new sub
								 * list was inserted */
    /* array of inserted tuples follows */
} ginxlogUpdateMeta;

#define XLOG_GIN_INSERT_LISTPAGE  0x70

typedef struct ginxlogInsertListPage
{
    BlockNumber rightlink;
    int32_t		ntuples;
    /* array of inserted tuples follows */
} ginxlogInsertListPage;

/*
 * Backup Blk 0: metapage
 * Backup Blk 1 to (ndeleted + 1): deleted pages
 */

#define XLOG_GIN_DELETE_LISTPAGE  0x80

/*
 * The WAL record for deleting list pages must contain a block reference to
 * all the deleted pages, so the number of pages that can be deleted in one
 * record is limited by XLR_MAX_BLOCK_ID. (block_id 0 is used for the
 * metapage.)
 */
#define GIN_NDELETE_AT_ONCE Min(16, XLR_MAX_BLOCK_ID - 1)
typedef struct ginxlogDeleteListPages
{
    GinMetaPageData metadata;
    int32_t		ndeleted;
} ginxlogDeleteListPages;


typedef struct
{
    ItemPointerData first;		/* first item in this posting list (unpacked) */
    uint16_t		nbytes;			/* number of bytes that follow */
    unsigned char bytes[FLEXIBLE_ARRAY_MEMBER]; /* varbyte encoded items */
} GinPostingList;

/* Action types */
#define GIN_SEGMENT_UNMODIFIED	0	/* no action (not used in WAL records) */
#define GIN_SEGMENT_DELETE		1	/* a whole segment is removed */
#define GIN_SEGMENT_INSERT		2	/* a whole segment is added */
#define GIN_SEGMENT_REPLACE		3	/* a segment is replaced */
#define GIN_SEGMENT_ADDITEMS	4	/* items are added to existing segment */


#define SizeOfGinPostingList(plist) (offsetof(GinPostingList, bytes) + SHORTALIGN((plist)->nbytes) )


#endif //PGMONETA_RM_GIN_H
