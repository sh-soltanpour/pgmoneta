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

#ifndef PGMONETA_RM_HEAP_H
#define PGMONETA_RM_HEAP_H

#include "rm.h"
#include "stdint.h"
#include "wal_reader.h"

#define XLOG_HEAP_INSERT        0x00
#define XLOG_HEAP_DELETE        0x10
#define XLOG_HEAP_UPDATE        0x20
#define XLOG_HEAP_TRUNCATE        0x30
#define XLOG_HEAP_HOT_UPDATE    0x40
#define XLOG_HEAP_CONFIRM        0x50
#define XLOG_HEAP_LOCK            0x60
#define XLOG_HEAP_INPLACE        0x70

#define XLOG_HEAP_OPMASK        0x70
/*
 * When we insert 1st item on new page in INSERT, UPDATE, HOT_UPDATE,
 * or MULTI_INSERT, we can (and we do) restore entire page in redo
 */
#define XLOG_HEAP_INIT_PAGE        0x80

/*These opcodes are associated with RM_HEAP2_ID, but are not logically different from
 * the ones above associated with RM_HEAP_ID.  XLOG_HEAP_OPMASK applies to
 * these, too.
 */
#define XLOG_HEAP2_REWRITE        0x00
#define XLOG_HEAP2_PRUNE        0x10
#define XLOG_HEAP2_VACUUM        0x20
#define XLOG_HEAP2_FREEZE_PAGE    0x30
#define XLOG_HEAP2_VISIBLE        0x40
#define XLOG_HEAP2_MULTI_INSERT 0x50
#define XLOG_HEAP2_LOCK_UPDATED 0x60
#define XLOG_HEAP2_NEW_CID        0x70

/* flags for infobits_set */
#define XLHL_XMAX_IS_MULTI        0x01
#define XLHL_XMAX_LOCK_ONLY        0x02
#define XLHL_XMAX_EXCL_LOCK        0x04
#define XLHL_XMAX_KEYSHR_LOCK    0x08
#define XLHL_KEYS_UPDATED        0x10

struct xl_heap_insert
{
   offset_number offnum;         /* inserted tuple's offset */
   uint8_t flags;

   /* xl_heap_header & TUPLE DATA in backup block 0 */
};

#define SIZE_OF_HEAP_INSERT    (offsetof(xl_heap_insert, flags) + sizeof(uint8))

/* This is what we need to know about delete */
struct xl_heap_delete
{
   transaction_id xmax;             /* xmax of the deleted tuple */
   offset_number offnum;         /* deleted tuple's offset */
   uint8_t infobits_set;     /* infomask bits */
   uint8_t flags;
};

#define SIZE_OF_HEAP_DELETE    (offsetof(xl_heap_delete, flags) + sizeof(uint8))

struct xl_heap_update
{
   transaction_id old_xmax;         /* xmax of the old tuple */
   offset_number old_offnum;     /* old tuple's offset */
   uint8_t old_infobits_set;     /* infomask bits to set on old tuple */
   uint8_t flags;
   transaction_id new_xmax;         /* xmax of the new tuple */
   offset_number new_offnum;     /* new tuple's offset */

   /*
    * If XLH_UPDATE_CONTAINS_OLD_TUPLE or XLH_UPDATE_CONTAINS_OLD_KEY flags
    * are set, xl_heap_header and tuple data for the old tuple follow.
    */
};

#define SIZE_OF_HEAP_UPDATE    (offsetof(xl_heap_update, new_offnum) + sizeof(OffsetNumber))

struct xl_heap_truncate
{
   oid dbId;
   uint32_t nrelids;
   uint8_t flags;
   oid relids[FLEXIBLE_ARRAY_MEMBER];
};

#define SIZE_OF_HEAP_TRUNCATE    (offsetof(xl_heap_truncate, relids))
/*
 * xl_heap_truncate flag values, 8 bits are available.
 */
#define XLH_TRUNCATE_CASCADE                    (1 << 0)
#define XLH_TRUNCATE_RESTART_SEQS                (1 << 1)

/* This is what we need to know about confirmation of speculative insertion */
struct xl_heap_confirm
{
   offset_number offnum;         /* confirmed tuple's offset on page */
};

#define SIZE_OF_HEAP_CONFIRM    (offsetof(xl_heap_confirm, offnum) + sizeof(OffsetNumber))

/* flag bits for xl_heap_lock / xl_heap_lock_updated's flag field */
#define XLH_LOCK_ALL_FROZEN_CLEARED        0x01

/* This is what we need to know about lock */
struct xl_heap_lock
{
   transaction_id locking_xid;     /* might be a multi_xact_id not xid */
   offset_number offnum;         /* locked tuple's offset on page */
   int8_t infobits_set;     /* infomask and infomask2 bits to set */
   uint8_t flags;             /* XLH_LOCK_* flag bits */
};

#define SIZE_OF_HEAP_LOCK    (offsetof(xl_heap_lock, flags) + sizeof(int8))

/* This is what we need to know about in-place update */
struct xl_heap_inplace
{
   offset_number offnum;         /* updated tuple's offset on page */
   /* TUPLE DATA FOLLOWS AT END OF STRUCT */
};

#define SIZE_OF_HEAP_INPLACE    (offsetof(xl_heap_inplace, offnum) + sizeof(OffsetNumber))

/*
 * This is what we need to know about page pruning (both during VACUUM and
 * during opportunistic pruning)
 *
 * The array of OffsetNumbers following the fixed part of the record contains:
 *	* for each redirected item: the item offset, then the offset redirected to
 *	* for each now-dead item: the item offset
 *	* for each now-unused item: the item offset
 * The total number of OffsetNumbers is therefore 2*nredirected+ndead+nunused.
 * Note that nunused is not explicitly stored, but may be found by reference
 * to the total record length.
 *
 * Requires a super-exclusive lock.
 */
struct xl_heap_prune
{
   transaction_id latestRemovedXid;
   uint16_t nredirected;
   uint16_t ndead;
   /* OFFSET NUMBERS are in the block reference 0 */
};

#define SIZE_OF_HEAP_PRUNE (offsetof(xl_heap_prune, ndead) + sizeof(uint16))

/*
 * The vacuum page record is similar to the prune record, but can only mark
 * already dead items as unused
 *
 * Used by heap vacuuming only.  Does not require a super-exclusive lock.
 */
struct xl_heap_vacuum
{
   uint16_t nunused;
   /* OFFSET NUMBERS are in the block reference 0 */
};

#define SIZE_OF_HEAP_VACUUM (offsetof(xl_heap_vacuum, nunused) + sizeof(uint16))

/*
 * This is what we need to know about a block being frozen during vacuum
 *
 * Backup block 0's data contains an array of xl_heap_freeze_tuple structs,
 * one for each tuple.
 */
struct xl_heap_freeze_page
{
   transaction_id cutoff_xid;
   uint16_t ntuples;
};

#define SIZE_OF_HEAP_FREEZE_PAGE (offsetof(xl_heap_freeze_page, ntuples) + sizeof(uint16))

/*
 * This is what we need to know about setting a visibility map bit
 *
 * Backup blk 0: visibility map buffer
 * Backup blk 1: heap buffer
 */
struct xl_heap_visible
{
   transaction_id cutoff_xid;
   uint8_t flags;
};

#define SIZE_OF_HEAP_VISIBLE (offsetof(xl_heap_visible, flags) + sizeof(uint8))

typedef uint32_t command_id;

struct xl_heap_new_cid
{
   /*
    * store toplevel xid so we don't have to merge cids from different
    * transactions
    */
   transaction_id top_xid;
   command_id cmin;
   command_id cmax;
   command_id combocid;         /* just for debugging */

   /*
    * Store the relfilenode/ctid pair to facilitate lookups.
    */
   struct rel_file_node target_node;
   struct item_pointer_data target_tid;
};

/*
 * This is what we need to know about a multi-insert.
 *
 * The main data of the record consists of this xl_heap_multi_insert header.
 * 'offsets' array is omitted if the whole page is reinitialized
 * (XLOG_HEAP_INIT_PAGE).
 *
 * In block 0's data portion, there is an xl_multi_insert_tuple struct,
 * followed by the tuple data for each tuple. There is padding to align
 * each xl_multi_insert_tuple struct.
 */
struct xl_heap_multi_insert
{
   uint8_t flags;
   uint16_t ntuples;
   offset_number offsets[FLEXIBLE_ARRAY_MEMBER];
};

#define SIZE_OF_HEAP_MULTI_INSERT    offsetof(xl_heap_multi_insert, offsets)

/* This is what we need to know about locking an updated version of a row */
struct xl_heap_lock_updated
{
   transaction_id xmax;
   offset_number offnum;
   uint8_t infobits_set;
   uint8_t flags;
};

char*heap_desc(char* buf, struct decoded_xlog_record* record);

char*heap2_desc(char* buf, struct decoded_xlog_record* record);

#endif //PGMONETA_RM_HEAP_H
