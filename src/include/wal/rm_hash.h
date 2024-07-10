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

#ifndef PGMONETA_RM_HASH_H
#define PGMONETA_RM_HASH_H

#include "wal_reader.h"
#include "rm.h"
typedef oid regproc;
typedef regproc RegProcedure;
/* Number of buffers required for XLOG_HASH_SQUEEZE_PAGE operation */
#define HASH_XLOG_FREE_OVFL_BUFS	6

/*
 * XLOG records for hash operations
 */
#define XLOG_HASH_INIT_META_PAGE	0x00	/* initialize the meta page */
#define XLOG_HASH_INIT_BITMAP_PAGE	0x10	/* initialize the bitmap page */
#define XLOG_HASH_INSERT		0x20	/* add index tuple without split */
#define XLOG_HASH_ADD_OVFL_PAGE 0x30	/* add overflow page */
#define XLOG_HASH_SPLIT_ALLOCATE_PAGE	0x40	/* allocate new page for split */
#define XLOG_HASH_SPLIT_PAGE	0x50	/* split page */
#define XLOG_HASH_SPLIT_COMPLETE	0x60	/* completion of split operation */
#define XLOG_HASH_MOVE_PAGE_CONTENTS	0x70	/* remove tuples from one page
												 * and add to another page */
#define XLOG_HASH_SQUEEZE_PAGE	0x80	/* add tuples to one of the previous
										 * pages in chain and free the ovfl
										 * page */
#define XLOG_HASH_DELETE		0x90	/* delete index tuples from a page */
#define XLOG_HASH_SPLIT_CLEANUP 0xA0	/* clear split-cleanup flag in primary
										 * bucket page after deleting tuples
										 * that are moved due to split	*/
#define XLOG_HASH_UPDATE_META_PAGE	0xB0	/* update meta page after vacuum */

#define XLOG_HASH_VACUUM_ONE_PAGE	0xC0	/* remove dead tuples from index
											 * page */

/*
 * xl_hash_split_allocate_page flag values, 8 bits are available.
 */
#define XLH_SPLIT_META_UPDATE_MASKS		(1<<0)
#define XLH_SPLIT_META_UPDATE_SPLITPOINT		(1<<1)

/*
 * This is what we need to know about simple (without split) insert.
 *
 * This data record is used for XLOG_HASH_INSERT
 *
 * Backup Blk 0: original page (data contains the inserted tuple)
 * Backup Blk 1: metapage (HashMetaPageData)
 */
typedef struct xl_hash_insert
{
    OffsetNumber offnum;
} xl_hash_insert;

#define SizeOfHashInsert	(offsetof(xl_hash_insert, offnum) + sizeof(OffsetNumber))

/*
 * This is what we need to know about addition of overflow page.
 *
 * This data record is used for XLOG_HASH_ADD_OVFL_PAGE
 *
 * Backup Blk 0: newly allocated overflow page
 * Backup Blk 1: page before new overflow page in the bucket chain
 * Backup Blk 2: bitmap page
 * Backup Blk 3: new bitmap page
 * Backup Blk 4: metapage
 */
typedef struct xl_hash_add_ovfl_page
{
    uint16_t		bmsize;
    bool		bmpage_found;
} xl_hash_add_ovfl_page;

#define SizeOfHashAddOvflPage	\
	(offsetof(xl_hash_add_ovfl_page, bmpage_found) + sizeof(bool))

/*
 * This is what we need to know about allocating a page for split.
 *
 * This data record is used for XLOG_HASH_SPLIT_ALLOCATE_PAGE
 *
 * Backup Blk 0: page for old bucket
 * Backup Blk 1: page for new bucket
 * Backup Blk 2: metapage
 */
typedef struct xl_hash_split_allocate_page
{
    uint32_t		new_bucket;
    uint16_t		old_bucket_flag;
    uint16_t		new_bucket_flag;
    uint8_t		flags;
} xl_hash_split_allocate_page;

#define SizeOfHashSplitAllocPage	\
	(offsetof(xl_hash_split_allocate_page, flags) + sizeof(uint8_t))

/*
 * This is what we need to know about completing the split operation.
 *
 * This data record is used for XLOG_HASH_SPLIT_COMPLETE
 *
 * Backup Blk 0: page for old bucket
 * Backup Blk 1: page for new bucket
 */
typedef struct xl_hash_split_complete
{
    uint16_t		old_bucket_flag;
    uint16_t		new_bucket_flag;
} xl_hash_split_complete;

#define SizeOfHashSplitComplete \
	(offsetof(xl_hash_split_complete, new_bucket_flag) + sizeof(uint16_t))

/*
 * This is what we need to know about move page contents required during
 * squeeze operation.
 *
 * This data record is used for XLOG_HASH_MOVE_PAGE_CONTENTS
 *
 * Backup Blk 0: bucket page
 * Backup Blk 1: page containing moved tuples
 * Backup Blk 2: page from which tuples will be removed
 */
typedef struct xl_hash_move_page_contents
{
    uint16_t		ntups;
    bool		is_prim_bucket_same_wrt;	/* true if the page to which
											 * tuples are moved is same as
											 * primary bucket page */
} xl_hash_move_page_contents;

#define SizeOfHashMovePageContents	\
	(offsetof(xl_hash_move_page_contents, is_prim_bucket_same_wrt) + sizeof(bool))

/*
 * This is what we need to know about the squeeze page operation.
 *
 * This data record is used for XLOG_HASH_SQUEEZE_PAGE
 *
 * Backup Blk 0: page containing tuples moved from freed overflow page
 * Backup Blk 1: freed overflow page
 * Backup Blk 2: page previous to the freed overflow page
 * Backup Blk 3: page next to the freed overflow page
 * Backup Blk 4: bitmap page containing info of freed overflow page
 * Backup Blk 5: meta page
 */
typedef struct xl_hash_squeeze_page
{
    block_number prevblkno;
    block_number nextblkno;
    uint16_t		ntups;
    bool		is_prim_bucket_same_wrt;	/* true if the page to which
											 * tuples are moved is same as
											 * primary bucket page */
    bool		is_prev_bucket_same_wrt;	/* true if the page to which
											 * tuples are moved is the page
											 * previous to the freed overflow
											 * page */
} xl_hash_squeeze_page;

#define SizeOfHashSqueezePage	\
	(offsetof(xl_hash_squeeze_page, is_prev_bucket_same_wrt) + sizeof(bool))

/*
 * This is what we need to know about the deletion of index tuples from a page.
 *
 * This data record is used for XLOG_HASH_DELETE
 *
 * Backup Blk 0: primary bucket page
 * Backup Blk 1: page from which tuples are deleted
 */
typedef struct xl_hash_delete
{
    bool		clear_dead_marking; /* true if this operation clears
									 * LH_PAGE_HAS_DEAD_TUPLES flag */
    bool		is_primary_bucket_page; /* true if the operation is for
										 * primary bucket page */
} xl_hash_delete;

#define SizeOfHashDelete	(offsetof(xl_hash_delete, is_primary_bucket_page) + sizeof(bool))

/*
 * This is what we need for metapage update operation.
 *
 * This data record is used for XLOG_HASH_UPDATE_META_PAGE
 *
 * Backup Blk 0: meta page
 */
typedef struct xl_hash_update_meta_page
{
    double		ntuples;
} xl_hash_update_meta_page;

#define SizeOfHashUpdateMetaPage	\
	(offsetof(xl_hash_update_meta_page, ntuples) + sizeof(double))

/*
 * This is what we need to initialize metapage.
 *
 * This data record is used for XLOG_HASH_INIT_META_PAGE
 *
 * Backup Blk 0: meta page
 */
typedef struct xl_hash_init_meta_page
{
    double		num_tuples;
    RegProcedure procid;
    uint16_t		ffactor;
} xl_hash_init_meta_page;

#define SizeOfHashInitMetaPage		\
	(offsetof(xl_hash_init_meta_page, ffactor) + sizeof(uint16_t))

/*
 * This is what we need to initialize bitmap page.
 *
 * This data record is used for XLOG_HASH_INIT_BITMAP_PAGE
 *
 * Backup Blk 0: bitmap page
 * Backup Blk 1: meta page
 */
typedef struct xl_hash_init_bitmap_page
{
    uint16_t		bmsize;
} xl_hash_init_bitmap_page;

#define SizeOfHashInitBitmapPage	\
	(offsetof(xl_hash_init_bitmap_page, bmsize) + sizeof(uint16_t))

/*
 * This is what we need for index tuple deletion and to
 * update the meta page.
 *
 * This data record is used for XLOG_HASH_VACUUM_ONE_PAGE
 *
 * Backup Blk 0: bucket page
 * Backup Blk 1: meta page
 */
typedef struct xl_hash_vacuum_one_page
{
    TransactionId latestRemovedXid;
    int			ntuples;

    /* TARGET OFFSET NUMBERS FOLLOW AT THE END */
} xl_hash_vacuum_one_page;

#define SizeOfHashVacuumOnePage \
	(offsetof(xl_hash_vacuum_one_page, ntuples) + sizeof(int))


char* hash_desc(char* buf, struct decoded_xlog_record *record);

#endif //PGMONETA_RM_HASH_H
