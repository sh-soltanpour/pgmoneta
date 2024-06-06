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


#define MAXALIGN(x) (((x) + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1))
#define XLOG_PAGE_MAGIC 0xD10D  // WAL version indicator

#define InvalidXLogRecPtr   0


#define XLP_FIRST_IS_CONTRECORD   0x0001
#define XLP_LONG_HEADER   0x0002


#define XLOG_BLCKSZ 8192


typedef uint32_t TimeLineID;
typedef uint64_t XLogRecPtr;
typedef uint32_t pg_crc32c;
typedef uint32_t TransactionId;
typedef uint8_t RmgrId;
typedef uint64_t XLogSegNo;


typedef struct XLogPageHeaderData {
    uint16_t xlp_magic;        /* magic value for correctness checks */
    uint16_t xlp_info;         /* flag bits, see below */
    TimeLineID xlp_tli;      /* TimeLineID of first record on page */
    XLogRecPtr xlp_pageaddr;     /* XLOG address of this page */
    uint32_t xlp_rem_len;      /* total len of remaining data for record */
} XLogPageHeaderData;

typedef XLogPageHeaderData *XLogPageHeader;

typedef struct XLogLongPageHeaderData {
    XLogPageHeaderData std;       /* standard header fields */
    uint64_t xlp_sysid;        /* system identifier from pg_control */
    uint32_t xlp_seg_size;     /* just as a cross-check */
    uint32_t xlp_xlog_blcksz;      /* just as a cross-check */
} XLogLongPageHeaderData;

typedef struct XLogRecord {
    uint32_t xl_tot_len;     /* total len of entire record */
    TransactionId xl_xid;       /* xact id */
    XLogRecPtr xl_prev;        /* ptr to previous record in log */
    uint8_t xl_info;        /* flag bits, see below */
    RmgrId xl_rmid;        /* resource manager for this record */
    /* 2 bytes of padding here, initialize to zero */
    pg_crc32c xl_crc;         /* CRC for this record */

    /* XLogRecordBlockHeaders and XLogRecordDataHeader follow, no padding */

} XLogRecord;


void parse_wal_segment_headers(char *path);
void find_next_record(FILE *file, XLogRecord *record, XLogRecPtr RecPtr, XLogLongPageHeaderData *pData);
void parse_page(char *page);

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

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })


#define XLogSegmentOffset(xlogptr, wal_segsz_bytes) \
     ((xlogptr) & ((wal_segsz_bytes) - 1))


static inline void
XLogFromFileName(const char *fname, TimeLineID *tli, XLogSegNo *logSegNo, int wal_segsz_bytes) {
    uint32_t log;
    uint32_t seg;

    sscanf(fname, "%08X%08X%08X", tli, &log, &seg);
    *logSegNo = (uint64_t) log * XLogSegmentsPerXLogId(wal_segsz_bytes) + seg;
}


#endif //PGMONETA_WAL_READER_H
