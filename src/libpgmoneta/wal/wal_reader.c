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
#include "wal/wal_reader.h"
#include "wal/rmgr.h"
#include "string.h"
#include "assert.h"
#include "time.h"
#include "utils.h"

void
print_short_page_header(struct xlog_page_header_data* header)
{
   printf("Page Header:\n");
   printf("  xlp_magic: %u\n", header->xlp_magic);
   printf("  xlp_info: %u\n", header->xlp_info);
   printf("  xlp_tli: %u\n", header->xlp_tli);
   printf("  xlp_pageaddr: %llu\n", (unsigned long long) header->xlp_pageaddr);
   printf("  xlp_rem_len: %u\n", header->xlp_rem_len);
   printf("-----------------\n");
}

void
print_long_page_header(struct xlog_long_page_header_data* header)
{
   printf("Long Page Header:\n");
   print_short_page_header(&header->std);
   printf("  xlp_sysid: %llu\n", (unsigned long long) header->xlp_sysid);
   printf("  xlp_seg_size: %u\n", header->xlp_seg_size);
   printf("  xlp_xlog_blcksz: %u\n", header->xlp_xlog_blcksz);
   printf("-----------------\n");
}

void
print_record(struct xlog_record* record)
{
   printf("%s\t", RmgrTable[record->xl_rmid].name);
   printf("len: %u\t", record->xl_tot_len);
   printf("xl_xid: %u\n", record->xl_xid);
   printf("xl_info: %u\n", record->xl_info);
   printf("xl_prev: %llu\n", (unsigned long long) record->xl_prev);
   printf("xl_rmid: %u\n", record->xl_rmid);
}

void
parse_wal_segment_headers(char* path)
{
   time_t t;
   time(&t);
   struct tm* ltime = localtime(&t);
   printf("test2 %d\n", ltime->tm_mday);

   struct xlog_record* record = NULL;
   struct xlog_long_page_header_data* long_header = NULL;
   char* buffer = NULL;
   struct decoded_xlog_record* decoded = NULL;

   FILE* file = fopen(path, "rb");
   if (file == NULL)
   {
      pgmoneta_log_fatal("Error: Could not open file %s\n", path);
   }

   long_header = malloc(SIZE_OF_XLOG_LONG_PHD);
   fread(long_header, SIZE_OF_XLOG_LONG_PHD, 1, file);

   record = malloc(SIZE_OF_XLOG_RECORD);

   if (record == NULL)
   {
      pgmoneta_log_fatal("Error: Could not allocate memory for record\n");
      free(long_header);
      fclose(file);
      exit(EXIT_FAILURE);
   }

   struct xlog_page_header_data* page_header = NULL;
   page_header = malloc(SIZE_OF_XLOG_SHORT_PHD);
   if (page_header == NULL)
   {
      pgmoneta_log_fatal("Error: Could not allocate memory for page header\n");
      free(record);
      free(long_header);
      fclose(file);
      exit(EXIT_FAILURE);
   }

   uint32_t next_record = ftell(file);
   int page_number = 0;

   while (true)
   {

      if (next_record >= (long_header->xlp_xlog_blcksz * (page_number + 1)))
      {
         page_number++;
         fseek(file, page_number * long_header->xlp_xlog_blcksz, SEEK_SET);
         fread(page_header, SIZE_OF_XLOG_SHORT_PHD, 1, file);
         next_record = MAXALIGN(ftell(file) + page_header->xlp_rem_len);
         continue;
      }
      fseek(file, next_record, SEEK_SET);
      if (ftell(file) + SIZE_OF_XLOG_RECORD > long_header->xlp_xlog_blcksz * (page_number + 1))
      {
         uint32_t end_of_page = (page_number + 1) * long_header->xlp_xlog_blcksz;
         size_t bytes_read = fread(record, 1, end_of_page - ftell(file), file);

         fseek(file, SIZE_OF_XLOG_SHORT_PHD, SEEK_CUR);
         fread(record + bytes_read, 1, SIZE_OF_XLOG_RECORD - bytes_read, file);
         print_record(record);
         page_number++;
      }
      else if (fread(record, SIZE_OF_XLOG_RECORD, 1, file) != 1)
      {
         pgmoneta_log_fatal("Error: Could not read second record\n");
         free(record);
         free(long_header);
         fclose(file);
         exit(EXIT_FAILURE);
      }

      if (record->xl_tot_len == 0)
      {
         return;
      }
      uint32_t data_length = record->xl_tot_len - SIZE_OF_XLOG_RECORD;
      next_record = ftell(file) + MAXALIGN(record->xl_tot_len - SIZE_OF_XLOG_RECORD);
      uint32_t end_of_page = (page_number + 1) * long_header->xlp_xlog_blcksz;

      buffer = malloc(data_length);
      if (buffer == NULL)
      {
         pgmoneta_log_fatal("Error: Could not allocate memory for buffer\n");
         break;
      }

      if (data_length + ftell(file) >= end_of_page)
      {
         size_t bytes_read = 0;
         size_t total_bytes_read = 0;
         uint32_t remaining_data_length = data_length;
         bytes_read = fread(buffer, 1, end_of_page - ftell(file), file);
         total_bytes_read += bytes_read;
         remaining_data_length -= bytes_read;
         while (remaining_data_length != 0)
         {
            fseek(file, SIZE_OF_XLOG_SHORT_PHD, SEEK_CUR);
            bytes_read = fread(buffer + total_bytes_read, 1,
                               MIN(remaining_data_length, long_header->xlp_xlog_blcksz - SIZE_OF_XLOG_SHORT_PHD), file);
            remaining_data_length -= bytes_read;
            total_bytes_read += bytes_read;
         }
         assert(total_bytes_read == data_length);
      }
      else
      {
         size_t bytes_read = fread(buffer, 1, data_length, file);

         assert(bytes_read == data_length);
      }

      decoded = malloc(sizeof(struct decoded_xlog_record));

      if (!decode_xlog_record(buffer, decoded, record, long_header->xlp_xlog_blcksz))
      {
         pgmoneta_log_fatal("error in decoding\n");
         continue;
      }
      else
      {
         display_decoded_record(decoded);
      }
   }
   fclose(file);
}

void
display_decoded_record(struct decoded_xlog_record* record)
{

   uint32_t rec_len;
   uint32_t fpi_len;

   print_record(&record->header);
   get_record_length(record, &rec_len, &fpi_len);
   printf("rec/tot_len: %u/%u\t", rec_len, record->header.xl_tot_len);

   //if record is rmgr is standby
   char* buf = NULL;
   printf("before desc\n");
   buf = RmgrTable[record->header.xl_rmid].rm_desc(buf, record);
   printf("before block ref info\n");
   buf = get_record_block_ref_info(buf, record, false, true, &fpi_len);
   printf("after block ref info\n");
   printf("%s\n", buf);
   free(buf);
   printf("\n------------------------------\n");
}

bool
decode_xlog_record(char* buffer, struct decoded_xlog_record* decoded, struct xlog_record* record, uint32_t block_size)
{
#define COPY_HEADER_FIELD(_dst, _size)          \
        do {                                        \
           if (remaining < _size)                  \
           goto shortdata_err;                 \
           memcpy(_dst, ptr, _size);               \
           ptr += _size;                           \
           remaining -= _size;                     \
        } while (0)

   decoded->header = *record;
//    decoded->lsn = lsn;
   decoded->next = NULL;
   decoded->record_origin = INVALID_REP_ORIGIN_ID;
   decoded->toplevel_xid = INVALID_TRANSACTION_ID;
   decoded->main_data = NULL;
   decoded->main_data_len = 0;
   decoded->max_block_id = -1;

   //read id
   int remaining = 0;
   uint32_t datatotal = 0;
   char* ptr = NULL;
   char* out = NULL;
   struct rel_file_locator* rlocator = NULL;
   uint8_t block_id;

   remaining = record->xl_tot_len - SIZE_OF_XLOG_RECORD;
   ptr = (char*) buffer;

   while (remaining > datatotal)
   {
      COPY_HEADER_FIELD(&block_id, sizeof(uint8_t));

      if (block_id == XLR_BLOCK_ID_DATA_SHORT)
      {
         /* xlog_record_data_header_short */
         uint8_t main_data_len;
         COPY_HEADER_FIELD(&main_data_len, sizeof(uint8_t));
         decoded->main_data_len = main_data_len;
         datatotal += main_data_len;
      }
      else if (block_id == XLR_BLOCK_ID_DATA_LONG)
      {
         /* xlog_record_data_header_long */
         uint32_t main_data_len;
         COPY_HEADER_FIELD(&main_data_len, sizeof(uint32_t));
         decoded->main_data_len = main_data_len;
         datatotal += main_data_len;
      }
      else if (block_id == XLR_BLOCK_ID_ORIGIN)
      {
         COPY_HEADER_FIELD(&decoded->record_origin, sizeof(rep_origin_id));
      }
      else if (block_id == XLR_BLOCK_ID_TOPLEVEL_XID)
      {
         COPY_HEADER_FIELD(&decoded->toplevel_xid, sizeof(transaction_id));
      }
      else if (block_id <= XLR_MAX_BLOCK_ID)
      {
         /* xlog_record_bloch_header */
         struct decoded_bkp_block* blk;
         uint8_t fork_flags;

         /* mark any intervening block IDs as not in use */
         for (int i = decoded->max_block_id + 1; i < block_id; ++i)
         {
            decoded->blocks[i].in_use = false;
         }

         if (block_id <= decoded->max_block_id)
         {
            goto err;
         }
         decoded->max_block_id = block_id;

         blk = &decoded->blocks[block_id];
         blk->in_use = true;
         blk->apply_image = false;

         COPY_HEADER_FIELD(&fork_flags, sizeof(uint8_t));
         blk->forknum = fork_flags & BKPBLOCK_FORK_MASK;
         blk->flags = fork_flags;
         blk->has_image = ((fork_flags & BKPBLOCK_HAS_IMAGE) != 0);
         blk->has_data = ((fork_flags & BKPBLOCK_HAS_DATA) != 0);

         blk->prefetch_buffer = InvalidBuffer;
         COPY_HEADER_FIELD(&blk->data_len, sizeof(uint16_t));
         /* cross-check that the HAS_DATA flag is set iff data_length > 0 */
         if (blk->has_data && blk->data_len == 0)
         {
            pgmoneta_log_fatal("BKPBLOCK_HAS_DATA set, but no data included");
            goto err;
         }
         if (!blk->has_data && blk->data_len != 0)
         {
            pgmoneta_log_fatal("BKPBLOCK_HAS_DATA not set, but data length is not zero");
            goto err;
         }
         datatotal += blk->data_len;

         if (blk->has_image)
         {
            COPY_HEADER_FIELD(&blk->bimg_len, sizeof(uint16_t));
            COPY_HEADER_FIELD(&blk->hole_offset, sizeof(uint16_t));
            COPY_HEADER_FIELD(&blk->bimg_info, sizeof(uint8_t));

            blk->apply_image = ((blk->bimg_info & BKPIMAGE_APPLY) != 0);

            if (BKPIMAGE_COMPRESSED(blk->bimg_info))
            {
               if (blk->bimg_info & BKPIMAGE_HAS_HOLE)
               {
                  COPY_HEADER_FIELD(&blk->hole_length, sizeof(uint16_t));
               }
               else
               {
                  blk->hole_length = 0;
               }
            }
            else
            {
               blk->hole_length = block_size - blk->bimg_len;
            }
            datatotal += blk->bimg_len;

            /*
             * cross-check that hole_offset > 0, hole_length > 0 and
             * bimg_len < BLCKSZ if the HAS_HOLE flag is set.
             */
            if ((blk->bimg_info & BKPIMAGE_HAS_HOLE) &&
                (blk->hole_offset == 0 ||
                 blk->hole_length == 0 ||
                 blk->bimg_len == block_size))
            {
               pgmoneta_log_fatal(
                  "BKPIMAGE_HAS_HOLE set, but hole offset %u length %u block image length %u at %X/%X");
               goto err;
            }

            /*
             * cross-check that hole_offset == 0 and hole_length == 0 if
             * the HAS_HOLE flag is not set.
             */
            if (!(blk->bimg_info & BKPIMAGE_HAS_HOLE) &&
                (blk->hole_offset != 0 || blk->hole_length != 0))
            {
               pgmoneta_log_fatal("BKPIMAGE_HAS_HOLE not set, but hole offset %u length %u at %X/%X");
               goto err;
            }

            /*
             * Cross-check that bimg_len < BLCKSZ if it is compressed.
             */
            if (BKPIMAGE_COMPRESSED(blk->bimg_info) &&
                blk->bimg_len == block_size)
            {
               pgmoneta_log_fatal("BKPIMAGE_COMPRESSED set, but block image length %u at %X/%X");
               goto err;
            }

            /*
             * cross-check that bimg_len = BLCKSZ if neither HAS_HOLE is
             * set nor COMPRESSED().
             */
            if (!(blk->bimg_info & BKPIMAGE_HAS_HOLE) &&
                !BKPIMAGE_COMPRESSED(blk->bimg_info) &&
                blk->bimg_len != block_size)
            {
               pgmoneta_log_fatal(
                  "neither BKPIMAGE_HAS_HOLE nor BKPIMAGE_COMPRESSED set, but block image length is %u at %X/%X");
               goto err;
            }
         }
         if (!(fork_flags & BKPBLOCK_SAME_REL))
         {
            COPY_HEADER_FIELD(&blk->rlocator, sizeof(struct rel_file_locator));
            rlocator = &blk->rlocator;
         }
         else
         {
            if (rlocator == NULL)
            {
               pgmoneta_log_fatal("BKPBLOCK_SAME_REL set but no previous rel at %X/%X");
               goto err;
            }

            blk->rlocator = *rlocator;
         }
         COPY_HEADER_FIELD(&blk->blkno, sizeof(block_number));
      }
      else
      {
         pgmoneta_log_fatal("invalid block_id %u at %X/%X");
         goto err;
      }
   }
   assert(remaining == datatotal);

   out = ((char*) decoded) +
         offsetof(struct decoded_xlog_record, blocks) +
         sizeof(decoded->blocks[0]) * (decoded->max_block_id + 1);
   /* block data first */
   for (block_id = 0; block_id <= decoded->max_block_id; block_id++)
   {
      struct decoded_bkp_block* blk = &decoded->blocks[block_id];

      if (!blk->in_use)
      {
         continue;
      }

      assert(blk->has_image || !blk->apply_image);

      if (blk->has_image)
      {
         /* no need to align image */
         blk->bkp_image = out;
         memcpy(out, ptr, blk->bimg_len);
         ptr += blk->bimg_len;
         out += blk->bimg_len;
      }
      if (blk->has_data)
      {
         out = (char*) MAXALIGNTYPE(out);
         blk->data = out;
         memcpy(blk->data, ptr, blk->data_len);
         ptr += blk->data_len;
         out += blk->data_len;
      }
   }

   if (decoded->main_data_len > 0)
   {
      decoded->main_data = malloc(decoded->main_data_len);
      if (decoded->main_data == NULL)
      {
         goto
         shortdata_err;
      }
      memcpy(decoded->main_data, ptr, decoded->main_data_len);
   }

   return true;

shortdata_err:
   printf("shortdata_err\n");
   return false;

err:
   printf("err\n");
   return false;
}

char*
get_record_block_data(struct decoded_xlog_record* record, uint8_t block_id, size_t* len)
{
   {
      struct decoded_bkp_block* bkpb;

      if (block_id > record->max_block_id ||
          !record->blocks[block_id].in_use)
      {
         return NULL;
      }

      bkpb = &record->blocks[block_id];

      if (!bkpb->has_data)
      {
         if (len)
         {
            *len = 0;
         }
         return NULL;
      }
      else
      {
         if (len)
         {
            *len = bkpb->data_len;
         }
         return bkpb->data;
      }
   }
}

void
get_record_length(struct decoded_xlog_record* record, uint32_t* rec_len, uint32_t* fpi_len)
{
   int block_id;

   *fpi_len = 0;
   for (block_id = 0; block_id <= record->max_block_id; block_id++)
   {
      if (!XLogRecHasBlockRef(record, block_id))
      {
         continue;
      }

      if (XLogRecHasBlockImage(record, block_id))
      {
         *fpi_len += record->blocks[block_id].bimg_len;
      }
   }

/*
 * Calculate the length of the record as the total length - the length of
 * all the block images.
 */
   *rec_len = record->header.xl_tot_len - *fpi_len;
}

char*
get_record_block_ref_info(char* buf, struct decoded_xlog_record* record, bool pretty, bool detailed_format, uint32_t* fpi_len
                          )
{
   int block_id;

   assert(record != NULL);

   if (detailed_format && pretty)
   {
      buf = pgmoneta_format_and_append(buf, "\n");
   }

   for (block_id = 0; block_id <= record->max_block_id; block_id++)
   {
      struct rel_file_locator rlocator;
      enum fork_number forknum;
      block_number blk;

      if (!get_record_block_tag_extended(record, block_id, &rlocator, &forknum, &blk, NULL))
      {
         continue;
      }

      if (detailed_format)
      {
         /* Get block references in detailed format. */

         if (pretty)
         {
            buf = pgmoneta_format_and_append(buf, "\t");
         }
         else if (block_id > 0)
         {
            buf = pgmoneta_format_and_append(buf, " ");
         }

         buf = pgmoneta_format_and_append(buf, "blkref #%d: rel %u/%u/%u forknum %d blk %u",
                                          block_id,
                                          rlocator.spcOid, rlocator.dbOid, rlocator.relNumber,
                                          forknum,
                                          blk);

         if (XLogRecHasBlockImage(record, block_id))
         {
            uint8_t bimg_info = record->blocks[block_id].bimg_info;

            /* Calculate the amount of FPI data in the record. */
            if (fpi_len)
            {
               *fpi_len += record->blocks[block_id].bimg_len;
            }

            if (BKPIMAGE_COMPRESSED(bimg_info))
            {
               const char* method;

               if ((bimg_info & BKPIMAGE_COMPRESS_PGLZ) != 0)
               {
                  method = "pglz";
               }
               else if ((bimg_info & BKPIMAGE_COMPRESS_LZ4) != 0)
               {
                  method = "lz4";
               }
               else if ((bimg_info & BKPIMAGE_COMPRESS_ZSTD) != 0)
               {
                  method = "zstd";
               }
               else
               {
                  method = "unknown";
               }

               buf = pgmoneta_format_and_append(buf,
                                                " (FPW%s); hole: offset: %u, length: %u, "
                                                "compression saved: %u, method: %s",
                                                record->blocks[block_id].apply_image ?
                                                "" : " for WAL verification",
                                                record->blocks[block_id].hole_offset,
                                                record->blocks[block_id].hole_length,
                                                8192 -
                                                record->blocks[block_id].hole_length -
                                                record->blocks[block_id].bimg_len,
                                                method);
            }
            else
            {
               buf = pgmoneta_format_and_append(buf,
                                                " (FPW%s); hole: offset: %u, length: %u",
                                                XLOG_REC_BLOCK_IMAGE_APPLY(record, block_id) ?
                                                "" : " for WAL verification",
                                                XLOG_REC_GET_BLOCK(record, block_id)->hole_offset,
                                                XLOG_REC_GET_BLOCK(record, block_id)->hole_length);
            }
         }

         if (pretty)
         {
            buf = pgmoneta_format_and_append(buf, "\n");
         }
      }
      else
      {
         /* Get block references in short format. */

         if (forknum != MAIN_FORKNUM)
         {
            buf = pgmoneta_format_and_append(buf,
                                             ", blkref #%d: rel %u/%u/%u fork %d blk %u",
                                             block_id,
                                             rlocator.spcOid, rlocator.dbOid, rlocator.relNumber,
                                             forknum,
                                             blk);
         }
         else
         {
            buf = pgmoneta_format_and_append(buf,
                                             ", blkref #%d: rel %u/%u/%u blk %u",
                                             block_id,
                                             rlocator.spcOid, rlocator.dbOid, rlocator.relNumber,
                                             blk);
         }

         if (XLogRecHasBlockImage(record, block_id))
         {
            /* Calculate the amount of FPI data in the record. */
            if (fpi_len)
            {
               *fpi_len += XLOG_REC_GET_BLOCK(record, block_id)->bimg_len;
            }

            if (XLOG_REC_BLOCK_IMAGE_APPLY(record, block_id))
            {
               buf = pgmoneta_format_and_append(buf, " FPW");
            }
            else
            {
               buf = pgmoneta_format_and_append(buf, " FPW for WAL verification");
            }
         }
      }
   }

   if (!detailed_format && pretty)
   {
      buf = pgmoneta_format_and_append(buf, "\n");
   }
   return buf;

}

bool
get_record_block_tag_extended(struct decoded_xlog_record* pRecord, int id, struct rel_file_locator* pLocator, enum fork_number* pNumber,
                              block_number* pInt, buffer* pVoid)
{
   struct decoded_bkp_block* bkpb;

   if (!XLogRecHasBlockRef(pRecord, id))
   {
      return false;
   }

   bkpb = XLOG_REC_GET_BLOCK(pRecord, id);
   if (pLocator)
   {
      *pLocator = bkpb->rlocator;
   }
   if (pNumber)
   {
      *pNumber = bkpb->forknum;
   }
   if (pInt)
   {
      *pInt = bkpb->blkno;
   }
   if (pVoid)
   {
      *pVoid = bkpb->prefetch_buffer;
   }
   return true;
}
