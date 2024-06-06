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

#include "logging.h"
#include "wal_reader.h"
#include "rmgr.h"

void
log_short_page_header(XLogPageHeaderData *header) {
    printf("Page Header:\n");
    printf("  xlp_magic: %u\n", header->xlp_magic);
    printf("  xlp_info: %u\n", header->xlp_info);
    printf("  xlp_tli: %u\n", header->xlp_tli);
    printf("  xlp_pageaddr: %llu\n", (unsigned long long) header->xlp_pageaddr);
    printf("  xlp_rem_len: %u\n", header->xlp_rem_len);
    printf("-----------------\n");
}

void
log_long_page_header(XLogLongPageHeaderData *header) {
    printf("Long Page Header:\n");
    log_short_page_header(&header->std);
    printf("  xlp_sysid: %llu\n", (unsigned long long) header->xlp_sysid);
    printf("  xlp_seg_size: %u\n", header->xlp_seg_size);
    printf("  xlp_xlog_blcksz: %u\n", header->xlp_xlog_blcksz);
    printf("-----------------\n");
}

void print_record(XLogRecord *record) {
    printf("xl_tot_len: %u\n", record->xl_tot_len);
    printf("xl_xid: %u\n", record->xl_xid);
    printf("xl_info: %u\n", record->xl_info);
    printf("xl_prev: %llu\n", (unsigned long long) record->xl_prev);
    printf("xl_rmid: %u\n", record->xl_rmid);
    printf("rmgrname: %s\n", RmgrTable[record->xl_rmid].name);
    printf("-----------------\n");
}

void
parse_wal_segment_headers(char *path) {
    XLogRecord *record = NULL;
    XLogLongPageHeaderData *long_header = NULL;

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        pgmoneta_log_fatal("Error: Could not open file %s\n", path);
    }


    long_header = malloc(SizeOfXLogLongPHD);
    printf("size of long header: %lu\n", SizeOfXLogLongPHD);


    fread(long_header, SizeOfXLogLongPHD, 1, file);
    log_long_page_header(long_header);

    if (long_header->std.xlp_info & XLP_FIRST_IS_CONTRECORD) {
        printf("IS CONT RECORD");
    }

    record = malloc(SizeOfXLogRecord);
    if (record == NULL) {
        pgmoneta_log_fatal("Error: Could not allocate memory for record\n");
        free(long_header);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    XLogPageHeaderData *page_header = NULL;
    page_header = malloc(SizeOfXLogShortPHD);
    if (page_header == NULL) {
        pgmoneta_log_fatal("Error: Could not allocate memory for page header\n");
        free(record);
        free(long_header);
        fclose(file);
        exit(EXIT_FAILURE);
    };


    int next_record = ftell(file);
    int count = 0;
    int page_number = 0;
    while (count++ < 500) {
        printf("next_record: %d\n", next_record);
        if (next_record > (long_header->xlp_xlog_blcksz * (page_number + 1))) {
            page_number++;
            printf("Next page\n");
            fseek(file, page_number * long_header->xlp_xlog_blcksz, SEEK_SET);
            fread(page_header, SizeOfXLogShortPHD, 1, file);
            log_short_page_header(page_header);
            next_record = MAXALIGN(ftell(file) + page_header->xlp_rem_len);
            continue;
        }
        fseek(file, next_record, SEEK_SET);
        if (fread(record, SizeOfXLogRecord, 1, file) != 1) {
            pgmoneta_log_fatal("Error: Could not read second record\n");
            free(record);
            free(long_header);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        if (record->xl_tot_len == 0) {
            break;
        }
        print_record(record);
        next_record = ftell(file) + MAXALIGN(record->xl_tot_len - SizeOfXLogRecord);
    }
}