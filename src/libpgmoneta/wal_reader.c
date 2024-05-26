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

void
log_short_page_header(XLogPageHeaderData* header)
{
   pgmoneta_log_debug("Page Header:\n");
   pgmoneta_log_debug("  xlp_magic: %u\n", header->xlp_magic);
   pgmoneta_log_debug("  xlp_info: %u\n", header->xlp_info);
   pgmoneta_log_debug("  xlp_tli: %u\n", header->xlp_tli);
   pgmoneta_log_debug("  xlp_pageaddr: %llu\n", (unsigned long long) header->xlp_pageaddr);
   pgmoneta_log_debug("  xlp_rem_len: %u\n", header->xlp_rem_len);
}

void
log_long_page_header(XLogLongPageHeaderData* header)
{
   pgmoneta_log_debug("Long Page Header:\n");
   log_short_page_header(&header->std);
   pgmoneta_log_debug("  xlp_sysid: %llu\n", (unsigned long long) header->xlp_sysid);
   pgmoneta_log_debug("  xlp_seg_size: %u\n", header->xlp_seg_size);
   pgmoneta_log_debug("  xlp_xlog_blcksz: %u\n", header->xlp_xlog_blcksz);
}

void
parse_wal_segment_headers(char* path)
{
   FILE* file = fopen(path, "rb");
   if (file == NULL)
   {
      pgmoneta_log_fatal("Error: Could not open file %s\n", path);
   }
   char long_header_buffer[SizeOfXLogLongPHD];
   uint32_t page_size;
   char* page = NULL;

   fread(long_header_buffer, 1, SizeOfXLogLongPHD, file);
   XLogLongPageHeaderData* long_header = (XLogLongPageHeaderData*) long_header_buffer;
   log_long_page_header(long_header);

   page_size = long_header->xlp_xlog_blcksz;
   page = (char*) malloc(page_size);

   fseek(file, page_size, SEEK_SET);

   while ((fread(page, 1, page_size, file)) == page_size)
   {
      parse_page(page);

   }
   fclose(file);
}

void
parse_page(char* page)
{
   XLogPageHeaderData* header = (XLogPageHeaderData*) page;
   if (header->xlp_magic != XLOG_PAGE_MAGIC)
   {
      pgmoneta_log_fatal("Error: Invalid magic number, expected %x, actual %x\n", XLOG_PAGE_MAGIC,
                         header->xlp_magic);
   }
   log_short_page_header(header);
}
