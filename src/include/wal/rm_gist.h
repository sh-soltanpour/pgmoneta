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

#ifndef PGMONETA_RM_GIST_H
#define PGMONETA_RM_GIST_H

#include "wal_reader.h"
#include "rm.h"

typedef xlog_rec_ptr gist_nsn;

#define XLOG_GIST_PAGE_UPDATE    0x00
#define XLOG_GIST_DELETE         0x10  /* delete leaf index tuples for a
                                        * page */
#define XLOG_GIST_PAGE_REUSE     0x20  /* old page is about to be reused
                                        * from FSM */
#define XLOG_GIST_PAGE_SPLIT     0x30
/* #define XLOG_GIST_INSERT_COMPLETE	 0x40 */	/* not used anymore */
/* #define XLOG_GIST_CREATE_INDEX		 0x50 */	/* not used anymore */
#define XLOG_GIST_PAGE_DELETE    0x60
#define XLOG_GIST_ASSIGN_LSN     0x70  /* nop, assign new LSN */

/*
 * Backup Blk 0: updated page.
 * Backup Blk 1: If this operation completes a page split, by inserting a
 *				 downlink for the split page, the left half of the split
 */
struct gist_xlog_page_update
{
   /* number of deleted offsets */
   uint16_t ntodelete;
   uint16_t ntoinsert;

   /*
    * In payload of blk 0 : 1. todelete OffsetNumbers 2. tuples to insert
    */
};

/*
 * Backup Blk 0: Leaf page, whose index tuples are deleted.
 */
struct gist_xlog_delete
{
   transaction_id latestRemovedXid;
   uint16_t ntodelete;           /* number of deleted offsets */

   /*
    * In payload of blk 0 : todelete OffsetNumbers
    */
};

#define SizeOfGistxlogDelete  (offsetof(gist_xlog_delete, ntodelete) + sizeof(uint16_t))

/*
 * Backup Blk 0: If this operation completes a page split, by inserting a
 *				 downlink for the split page, the left half of the split
 * Backup Blk 1 - npage: split pages (1 is the original page)
 */
struct gist_xlog_page_split
{
   block_number origrlink;       /* rightlink of the page before split */
   gist_nsn orignsn;          /* NSN of the page before split */
   bool origleaf;          /* was splitted page a leaf page? */

   uint16_t npage;               /* # of pages in the split */
   bool markfollowright;      /* set F_FOLLOW_RIGHT flags */

   /*
    * follow: 1. gistxlogPage and array of IndexTupleData per page
    */
};

/*
 * Backup Blk 0: page that was deleted.
 * Backup Blk 1: parent page, containing the downlink to the deleted page.
 */
struct gist_xlog_page_delete
{
   struct full_transaction_id deleteXid;  /* last Xid which could see page in scan */
   offset_number downlinkOffset;    /* Offset of downlink referencing this
                                     * page */
};

#define SizeOfGistxlogPageDelete (offsetof(gistxlogPageDelete, downlinkOffset) + sizeof(OffsetNumber))

/*
 * This is what we need to know about page reuse, for hot standby.
 */
struct gist_xlog_page_reuse
{
   struct rel_file_node node;
   block_number block;
   struct full_transaction_id latestRemovedFullXid;
};

#define SizeOfGistxlogPageReuse  (offsetof(gistxlogPageReuse, latestRemovedFullXid) + sizeof(FullTransactionId))

char* gist_desc(char* buf, struct decoded_xlog_record* record);

#endif //PGMONETA_RM_GIST_H
