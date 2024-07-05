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

#ifndef PGMONETA_WAL_READER_H
#define PGMONETA_WAL_READER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include "stdbool.h"
#include "transaction.h"


#define MAXIMUM_ALIGNOF 8 // TODO: double check this value
#define ALIGNOF_SHORT 2 // TODO: double check this value
#define MAXALIGN(x) (((x) + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1))

#define TYPEALIGN(ALIGNVAL, LEN)  \
        (((uintptr_t) (LEN) + ((ALIGNVAL) -1)) & ~((uintptr_t) ((ALIGNVAL) -1)))
#define MAXALIGNTYPE(LEN) TYPEALIGN(MAXIMUM_ALIGNOF, (LEN))
#define SHORTALIGN(LEN)			TYPEALIGN(ALIGNOF_SHORT, (LEN))

#define XLOG_PAGE_MAGIC 0xD10D  // WAL version indicator

#define InvalidXLogRecPtr   0
#define InvalidBuffer   0

#define XLP_FIRST_IS_CONTRECORD   0x0001
#define XLP_LONG_HEADER   0x0002

typedef uint32_t TimeLineID;
typedef uint64_t XLogRecPtr;
typedef uint32_t pg_crc32c;
typedef uint8_t RmgrId;
typedef uint64_t XLogSegNo;
typedef uint16_t RepOriginId;

typedef int Buffer;
typedef uint32_t BlockNumber;
typedef unsigned int Oid;
typedef Oid RelFileNumber;
#define InvalidOid		((Oid) 0)

#define FLEXIBLE_ARRAY_MEMBER   /* empty */


typedef enum ForkNumber {
   InvalidForkNumber = -1,
   MAIN_FORKNUM = 0,
   FSM_FORKNUM,
   VISIBILITYMAP_FORKNUM,
   INIT_FORKNUM,

   /*
    * NOTE: if you add a new fork, change MAX_FORKNUM and possibly
    * FORKNAMECHARS below, and update the forkNames array in
    * src/common/relpath.c
    */
} ForkNumber;


typedef enum WalLevel
{
    WAL_LEVEL_MINIMAL = 0,
    WAL_LEVEL_REPLICA,
    WAL_LEVEL_LOGICAL
} WalLevel;


#define InvalidRepOriginId   0

typedef struct XLogPageHeaderData
{
   uint16_t xlp_magic;         /* magic value for correctness checks */
   uint16_t xlp_info;          /* flag bits, see below */
   TimeLineID xlp_tli;       /* TimeLineID of first record on page */
   XLogRecPtr xlp_pageaddr;      /* XLOG address of this page */
   uint32_t xlp_rem_len;       /* total len of remaining data for record */
} XLogPageHeaderData;

typedef XLogPageHeaderData* XLogPageHeader;

typedef struct XLogLongPageHeaderData
{
   XLogPageHeaderData std;        /* standard header fields */
   uint64_t xlp_sysid;         /* system identifier from pg_control */
   uint32_t xlp_seg_size;      /* just as a cross-check */
   uint32_t xlp_xlog_blcksz;       /* just as a cross-check */
} XLogLongPageHeaderData;

typedef struct XLogRecord
{
   uint32_t xl_tot_len;      /* total len of entire record */
   TransactionId xl_xid;        /* xact id */
   XLogRecPtr xl_prev;         /* ptr to previous record in log */
   uint8_t xl_info;         /* flag bits, see below */
   RmgrId xl_rmid;         /* resource manager for this record */
   /* 2 bytes of padding here, initialize to zero */
   pg_crc32c xl_crc;          /* CRC for this record */

   /* XLogRecordBlockHeaders and XLogRecordDataHeader follow, no padding */

} XLogRecord;

typedef struct XLogRecordBlockHeader
{
   uint8_t id;              /* block reference ID */
   uint8_t fork_flags;      /* fork within the relation, and flags */
   uint16_t data_length;     /* number of payload bytes (not including page
                              * image) */

   /* If BKPBLOCK_HAS_IMAGE, an XLogRecordBlockImageHeader struct follows */
   /* If BKPBLOCK_SAME_REL is not set, a RelFileLocator follows */
   /* BlockNumber follows */
} XLogRecordBlockHeader;

#define SizeOfXLogRecordBlockHeader (offsetof(XLogRecordBlockHeader, data_length) + sizeof(uint16_t))

typedef struct XLogRecordDataHeaderShort
{
   uint8_t id;              /* XLR_BLOCK_ID_DATA_SHORT */
   uint8_t data_length;     /* number of payload bytes */
} XLogRecordDataHeaderShort;

#define SizeOfXLogRecordDataHeaderShort (sizeof(uint8_t) * 2)

typedef struct XLogRecordDataHeaderLong
{
   uint8_t id;              /* XLR_BLOCK_ID_DATA_LONG */
   /* followed by uint32 data_length, unaligned */
} XLogRecordDataHeaderLong;

#define SizeOfXLogRecordDataHeaderLong (sizeof(uint8_t) + sizeof(uint32))

typedef struct RelFileLocator
{
   Oid spcOid;          /* tablespace */
   Oid dbOid;           /* database */
   RelFileNumber relNumber;     /* relation */
} RelFileLocator;

typedef struct
{
   /* Is this block ref in use? */
   bool in_use;

   /* Identify the block this refers to */
   RelFileLocator rlocator;
   ForkNumber forknum;
   BlockNumber blkno;

   /* Prefetching workspace. */
   Buffer prefetch_buffer;

   /* copy of the fork_flags field from the XLogRecordBlockHeader */
   uint8_t flags;

   /* Information on full-page image, if any */
   bool has_image;       /* has image, even for consistency checking */
   bool apply_image;     /* has image that should be restored */
   char* bkp_image;
   uint16_t hole_offset;
   uint16_t hole_length;
   uint16_t bimg_len;
   uint8_t bimg_info;

   /* Buffer holding the rmgr-specific data associated with this block */
   bool has_data;
   char* data;
   uint16_t data_len;
   uint16_t data_bufsz;
} DecodedBkpBlock;

typedef struct DecodedXLogRecord
{
   /* Private member used for resource management. */
   size_t size;            /* total size of decoded record */
   bool oversized;       /* outside the regular decode buffer? */
   struct DecodedXLogRecord* next;  /* decoded record queue link */

   /* Public members. */
   XLogRecPtr lsn;             /* location */
   XLogRecPtr next_lsn;        /* location of next record */
   XLogRecord header;          /* header */
   RepOriginId record_origin;
   TransactionId toplevel_xid;  /* XID of top-level transaction */
   char* main_data;       /* record's main data portion */
   uint32_t main_data_len;   /* main data portion's length */
   int max_block_id;    /* highest block_id in use (-1 if none) */
   DecodedBkpBlock blocks[FLEXIBLE_ARRAY_MEMBER];
} DecodedXLogRecord;


typedef struct RelFileNode
{
    Oid spcNode;         /* tablespace */
    Oid dbNode;             /* database */
    Oid relNode;         /* relation */
} RelFileNode;

#define XLogRecHasBlockRef(record, block_id)           \
        ((record->max_block_id >= (block_id)) && \
         (record->blocks[block_id].in_use))

#define XLogRecHasBlockImage(record, block_id)     \
        (record->blocks[block_id].has_image)

#define XLogRecHasBlockData(record, block_id)      \
        (record->blocks[block_id].has_data)

#define XLR_MAX_BLOCK_ID            32

#define XLR_BLOCK_ID_DATA_SHORT     255
#define XLR_BLOCK_ID_DATA_LONG      254
#define XLR_BLOCK_ID_ORIGIN         253
#define XLR_BLOCK_ID_TOPLEVEL_XID   252

#define BKPBLOCK_FORK_MASK  0x0F
#define BKPBLOCK_FLAG_MASK  0xF0
#define BKPBLOCK_HAS_IMAGE  0x10    /* block data is an XLogRecordBlockImage */
#define BKPBLOCK_HAS_DATA   0x20
#define BKPBLOCK_WILL_INIT  0x40    /* redo will re-init the page */
#define BKPBLOCK_SAME_REL   0x80    /* RelFileLocator omitted, same as
                                     * previous */

/* Information stored in bimg_info */
#define BKPIMAGE_HAS_HOLE       0x01    /* page image has "hole" */
#define BKPIMAGE_IS_COMPRESSED      0x02  /* page image is compressed */
#define BKPIMAGE_APPLY          0x04    /* page image should be restored
                                         * during replay */

/* compression methods supported */
#define BKPIMAGE_COMPRESS_PGLZ  0x04
#define BKPIMAGE_COMPRESS_LZ4   0x08
#define BKPIMAGE_COMPRESS_ZSTD  0x10

#define BKPIMAGE_COMPRESSED(info) \
        ((info & BKPIMAGE_IS_COMPRESSED) != 0)

void parse_wal_segment_headers(char* path);

bool decode_xlog_record(char* buffer, DecodedXLogRecord* decoded, XLogRecord* record, uint32_t block_size);

void get_record_length(DecodedXLogRecord* record, uint32_t* rec_len, uint32_t* fpi_len);

void display_decoded_record(DecodedXLogRecord* record);

void XLogRecGetBlockRefInfo(DecodedXLogRecord* record, bool pretty, bool detailed_format, uint32_t* fpi_len);

char*XLogRecGetBlockData(DecodedXLogRecord* record, uint8_t block_id, size_t* len);

void find_next_record(FILE* file, XLogRecord* record, XLogRecPtr RecPtr, XLogLongPageHeaderData* pData);

void parse_page(char* page);

#define SizeOfXLogLongPHD   MAXALIGN(sizeof(XLogLongPageHeaderData))
#define SizeOfXLogShortPHD   MAXALIGN(sizeof(XLogPageHeaderData))
#define SizeOfXLogRecord    (offsetof(XLogRecord, xl_crc) + sizeof(pg_crc32c))
#define XLogPageHeaderSize(hdr)    (((hdr)->xlp_info & XLP_LONG_HEADER) ? SizeOfXLogLongPHD : SizeOfXLogShortPHD)
#define XLogSegNoOffsetToRecPtr(segno, offset, wal_segsz_bytes, dest)  (dest) = (segno) * (wal_segsz_bytes) + (offset)
#define XLogSegmentsPerXLogId(wal_segsz_bytes)  (UINT64_C(0x100000000) / (wal_segsz_bytes))
#define XLogRecPtrIsInvalid(r)  ((r) == InvalidXLogRecPtr)
#define XLByteToSeg(xlrp, logSegNo, wal_segsz_bytes) \
        logSegNo = (xlrp) / (wal_segsz_bytes)

#define XLByteToPrevSeg(xlrp, logSegNo, wal_segsz_bytes) \
        logSegNo = ((xlrp) - 1) / (wal_segsz_bytes)

#define MAX(a, b) \
        ({ __typeof__ (a) _a = (a); \
           __typeof__ (b) _b = (b); \
           _a > _b ? _a : _b; })

#define MIN(a, b) \
        ({ __typeof__ (a) _a = (a); \
           __typeof__ (b) _b = (b); \
           _a < _b ? _a : _b; })

#define XLogSegmentOffset(xlogptr, wal_segsz_bytes) \
        ((xlogptr) & ((wal_segsz_bytes) - 1))

#define LSN_FORMAT_ARGS(lsn) ((uint32_t) ((lsn) >> 32)), ((uint32_t) (lsn))
#define XLogRecGetData(record) ((record)->main_data)
#define XLogRecGetInfo(record) ((record)->header.xl_info)
#define XLogRecGetBlock(record, i) (&record->blocks[(i)])
#define XLogRecBlockImageApply(record, block_id) (record->blocks[block_id].apply_image)
#define XLogRecGetOrigin(record) (record->record_origin)
#define XLogRecGetDataLen(record) (record->main_data_len)

static inline void
XLogFromFileName(const char* fname, TimeLineID* tli, XLogSegNo* logSegNo, int wal_segsz_bytes)
{
   uint32_t log;
   uint32_t seg;

   sscanf(fname, "%08X%08X%08X", tli, &log, &seg);
   *logSegNo = (uint64_t) log * XLogSegmentsPerXLogId(wal_segsz_bytes) + seg;
}

#endif //PGMONETA_WAL_READER_H
