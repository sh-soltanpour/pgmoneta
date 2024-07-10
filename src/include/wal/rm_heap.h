//
// Created by Shahryar Soltanpour on 2024-06-18.
//

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

typedef struct xl_heap_insert
{
   offset_number offnum;         /* inserted tuple's offset */
   uint8_t flags;

   /* xl_heap_header & TUPLE DATA in backup block 0 */
} xl_heap_insert;

#define SizeOfHeapInsert    (offsetof(xl_heap_insert, flags) + sizeof(uint8))

/* This is what we need to know about delete */
typedef struct xl_heap_delete
{
   TransactionId xmax;             /* xmax of the deleted tuple */
   offset_number offnum;         /* deleted tuple's offset */
   uint8_t infobits_set;     /* infomask bits */
   uint8_t flags;
} xl_heap_delete;

#define SizeOfHeapDelete    (offsetof(xl_heap_delete, flags) + sizeof(uint8))

typedef struct xl_heap_update
{
   TransactionId old_xmax;         /* xmax of the old tuple */
   offset_number old_offnum;     /* old tuple's offset */
   uint8_t old_infobits_set;     /* infomask bits to set on old tuple */
   uint8_t flags;
   TransactionId new_xmax;         /* xmax of the new tuple */
   offset_number new_offnum;     /* new tuple's offset */

   /*
    * If XLH_UPDATE_CONTAINS_OLD_TUPLE or XLH_UPDATE_CONTAINS_OLD_KEY flags
    * are set, xl_heap_header and tuple data for the old tuple follow.
    */
} xl_heap_update;

#define SizeOfHeapUpdate    (offsetof(xl_heap_update, new_offnum) + sizeof(OffsetNumber))

typedef struct xl_heap_truncate
{
   oid dbId;
   uint32_t nrelids;
   uint8_t flags;
   oid relids[FLEXIBLE_ARRAY_MEMBER];
} xl_heap_truncate;

#define SizeOfHeapTruncate    (offsetof(xl_heap_truncate, relids))
/*
 * xl_heap_truncate flag values, 8 bits are available.
 */
#define XLH_TRUNCATE_CASCADE                    (1 << 0)
#define XLH_TRUNCATE_RESTART_SEQS                (1 << 1)

/* This is what we need to know about confirmation of speculative insertion */
typedef struct xl_heap_confirm
{
   offset_number offnum;         /* confirmed tuple's offset on page */
} xl_heap_confirm;

#define SizeOfHeapConfirm    (offsetof(xl_heap_confirm, offnum) + sizeof(OffsetNumber))

/* flag bits for xl_heap_lock / xl_heap_lock_updated's flag field */
#define XLH_LOCK_ALL_FROZEN_CLEARED        0x01

/* This is what we need to know about lock */
typedef struct xl_heap_lock
{
   TransactionId locking_xid;     /* might be a MultiXactId not xid */
   offset_number offnum;         /* locked tuple's offset on page */
   int8_t infobits_set;     /* infomask and infomask2 bits to set */
   uint8_t flags;             /* XLH_LOCK_* flag bits */
} xl_heap_lock;

#define SizeOfHeapLock    (offsetof(xl_heap_lock, flags) + sizeof(int8))

/* This is what we need to know about in-place update */
typedef struct xl_heap_inplace
{
   offset_number offnum;         /* updated tuple's offset on page */
   /* TUPLE DATA FOLLOWS AT END OF STRUCT */
} xl_heap_inplace;

#define SizeOfHeapInplace    (offsetof(xl_heap_inplace, offnum) + sizeof(OffsetNumber))

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
typedef struct xl_heap_prune
{
   TransactionId latestRemovedXid;
   uint16_t nredirected;
   uint16_t ndead;
   /* OFFSET NUMBERS are in the block reference 0 */
} xl_heap_prune;

#define SizeOfHeapPrune (offsetof(xl_heap_prune, ndead) + sizeof(uint16))

/*
 * The vacuum page record is similar to the prune record, but can only mark
 * already dead items as unused
 *
 * Used by heap vacuuming only.  Does not require a super-exclusive lock.
 */
typedef struct xl_heap_vacuum
{
   uint16_t nunused;
   /* OFFSET NUMBERS are in the block reference 0 */
} xl_heap_vacuum;

#define SizeOfHeapVacuum (offsetof(xl_heap_vacuum, nunused) + sizeof(uint16))

/*
 * This is what we need to know about a block being frozen during vacuum
 *
 * Backup block 0's data contains an array of xl_heap_freeze_tuple structs,
 * one for each tuple.
 */
typedef struct xl_heap_freeze_page
{
   TransactionId cutoff_xid;
   uint16_t ntuples;
} xl_heap_freeze_page;

#define SizeOfHeapFreezePage (offsetof(xl_heap_freeze_page, ntuples) + sizeof(uint16))

/*
 * This is what we need to know about setting a visibility map bit
 *
 * Backup blk 0: visibility map buffer
 * Backup blk 1: heap buffer
 */
typedef struct xl_heap_visible
{
   TransactionId cutoff_xid;
   uint8_t flags;
} xl_heap_visible;

#define SizeOfHeapVisible (offsetof(xl_heap_visible, flags) + sizeof(uint8))

typedef uint32_t CommandId;


typedef struct xl_heap_new_cid
{
   /*
    * store toplevel xid so we don't have to merge cids from different
    * transactions
    */
   TransactionId top_xid;
   CommandId cmin;
   CommandId cmax;
   CommandId combocid;         /* just for debugging */

   /*
    * Store the relfilenode/ctid pair to facilitate lookups.
    */
   RelFileNode target_node;
   struct item_pointer_data target_tid;
} xl_heap_new_cid;

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
typedef struct xl_heap_multi_insert
{
   uint8_t flags;
   uint16_t ntuples;
   offset_number offsets[FLEXIBLE_ARRAY_MEMBER];
} xl_heap_multi_insert;

#define SizeOfHeapMultiInsert    offsetof(xl_heap_multi_insert, offsets)

/* This is what we need to know about locking an updated version of a row */
typedef struct xl_heap_lock_updated
{
   TransactionId xmax;
   offset_number offnum;
   uint8_t infobits_set;
   uint8_t flags;
} xl_heap_lock_updated;

char*heap_desc(char* buf, struct decoded_xlog_record* record);

char*heap2_desc(char* buf, struct decoded_xlog_record* record);

#endif //PGMONETA_RM_HEAP_H
