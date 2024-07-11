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

#ifndef PGMONETA_SINVAL_H
#define PGMONETA_SINVAL_H

#include "wal_reader.h"

typedef signed char int8;       /* == 8 bits */

struct shared_inval_catcache_msg
{
   int8 id;                     /* cache ID --- must be first */
   oid dbId;                    /* database ID, or 0 if a shared relation */
   uint32_t hashValue;            /* hash value of key for this catcache */
};

#define SHAREDINVALCATALOG_ID   (-1)

struct shared_inval_catalog_msg
{
   int8 id;                     /* type field --- must be first */
   oid dbId;                    /* database ID, or 0 if a shared catalog */
   oid catId;                   /* ID of catalog whose contents are invalid */
};

#define SHAREDINVALRELCACHE_ID  (-2)

struct shared_inval_relcache_msg
{
   int8 id;                     /* type field --- must be first */
   oid dbId;                    /* database ID, or 0 if a shared relation */
   oid relId;                   /* relation ID, or 0 if whole relcache */
};

#define SHAREDINVALSMGR_ID      (-3)

struct shared_inval_smgr_msg
{
   /* note: field layout chosen to pack into 16 bytes */
   int8 id;                     /* type field --- must be first */
   int8 backend_hi;             /* high bits of backend procno, if temprel */
   uint16_t backend_lo;           /* low bits of backend procno, if temprel */
   struct rel_file_locator rlocator;     /* spcOid, dbOid, relNumber */
};

#define SHAREDINVALRELMAP_ID    (-4)

struct shared_inval_relmap_msg
{
   int8 id;                     /* type field --- must be first */
   oid dbId;                    /* database ID, or 0 for shared catalogs */
};

#define SHAREDINVALSNAPSHOT_ID  (-5)

struct shared_inval_snapshot_msg
{
   int8 id;                     /* type field --- must be first */
   oid dbId;                    /* database ID, or 0 if a shared relation */
   oid relId;                   /* relation ID */
};

union shared_invalidation_message
{
   int8 id;                     /* type field --- must be first */
   struct shared_inval_catcache_msg cc;
   struct shared_inval_catalog_msg cat;
   struct shared_inval_relcache_msg rc;
   struct shared_inval_smgr_msg sm;
   struct shared_inval_relmap_msg rm;
   struct shared_inval_snapshot_msg sn;
};

#endif //PGMONETA_SINVAL_H
