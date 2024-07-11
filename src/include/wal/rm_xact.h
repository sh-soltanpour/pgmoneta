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

#ifndef PGMONETA_RM_XACT_H
#define PGMONETA_RM_XACT_H

#include "wal_reader.h"
#include "rm.h"
#include "rm_xlog.h"
#include "sinval.h"

/*
 * Maximum size of Global Transaction ID (including '\0').
 *
 * Note that the max value of GIDSIZE must fit in the uint16 gidlen,
 * specified in TwoPhaseFileHeader.
 */
#define GIDSIZE 200

/* mask for filtering opcodes out of xl_info */
#define XLOG_XACT_OPMASK            0x70

/* does this record have a 'xinfo' field or not */
#define XLOG_XACT_HAS_INFO            0x80
/* ----------------
 *		transaction-related XLOG entries
 * ----------------
 */

/*
 * XLOG allows to store some information in high 4 bits of log record xl_info
 * field. We use 3 for the opcode, and one about an optional flag variable.
 */
#define XLOG_XACT_COMMIT            0x00
#define XLOG_XACT_PREPARE            0x10
#define XLOG_XACT_ABORT                0x20
#define XLOG_XACT_COMMIT_PREPARED    0x30
#define XLOG_XACT_ABORT_PREPARED    0x40
#define XLOG_XACT_ASSIGNMENT        0x50
#define XLOG_XACT_INVALIDATIONS        0x60
/* free opcode 0x70 */

/* mask for filtering opcodes out of xl_info */
#define XLOG_XACT_OPMASK            0x70

/* does this record have a 'xinfo' field or not */
#define XLOG_XACT_HAS_INFO            0x80

/*
 * The following flags, stored in xinfo, determine which information is
 * contained in commit/abort records.
 */
#define XACT_XINFO_HAS_DBINFO            (1U << 0)
#define XACT_XINFO_HAS_SUBXACTS            (1U << 1)
#define XACT_XINFO_HAS_RELFILENODES        (1U << 2)
#define XACT_XINFO_HAS_INVALS            (1U << 3)
#define XACT_XINFO_HAS_TWOPHASE            (1U << 4)
#define XACT_XINFO_HAS_ORIGIN            (1U << 5)
#define XACT_XINFO_HAS_AE_LOCKS            (1U << 6)
#define XACT_XINFO_HAS_GID                (1U << 7)

/*
 * Also stored in xinfo, these indicating a variety of additional actions that
 * need to occur when emulating transaction effects during recovery.
 *
 * They are named XactCompletion... to differentiate them from
 * EOXact... routines which run at the end of the original transaction
 * completion.
 */
#define XACT_COMPLETION_APPLY_FEEDBACK            (1U << 29)
#define XACT_COMPLETION_UPDATE_RELCACHE_FILE    (1U << 30)
#define XACT_COMPLETION_FORCE_SYNC_COMMIT        (1U << 31)

/* Access macros for above flags */
#define XactCompletionApplyFeedback(xinfo) \
        ((xinfo & XACT_COMPLETION_APPLY_FEEDBACK) != 0)
#define XactCompletionRelcacheInitFileInval(xinfo) \
        ((xinfo & XACT_COMPLETION_UPDATE_RELCACHE_FILE) != 0)
#define XactCompletionForceSyncCommit(xinfo) \
        ((xinfo & XACT_COMPLETION_FORCE_SYNC_COMMIT) != 0)

struct xl_xact_assignment
{
   transaction_id xtop;             /* assigned XID's top-level XID */
   int nsubxacts;         /* number of subtransaction XIDs */
   transaction_id xsub[FLEXIBLE_ARRAY_MEMBER];     /* assigned subxids */
};

#define MinSizeOfXactAssignment offsetof(xl_xact_assignment, xsub)

/*
 * Commit and abort records can contain a lot of information. But a large
 * portion of the records won't need all possible pieces of information. So we
 * only include what's needed.
 *
 * A minimal commit/abort record only consists of a xl_xact_commit/abort
 * struct. The presence of additional information is indicated by bits set in
 * 'xl_xact_xinfo->xinfo'. The presence of the xinfo field itself is signaled
 * by a set XLOG_XACT_HAS_INFO bit in the xl_info field.
 *
 * NB: All the individual data chunks should be sized to multiples of
 * sizeof(int) and only require int32 alignment. If they require bigger
 * alignment, they need to be copied upon reading.
 */

/* sub-records for commit/abort */

struct xl_xact_xinfo
{
   /*
    * Even though we right now only require 1 byte of space in xinfo we use
    * four so following records don't have to care about alignment. Commit
    * records can be large, so copying large portions isn't attractive.
    */
   uint32_t xinfo;
};

struct xl_xact_dbinfo
{
   oid dbId;             /* MyDatabaseId */
   oid tsId;             /* MyDatabaseTableSpace */
};

struct xl_xact_subxacts
{
   int nsubxacts;         /* number of subtransaction XIDs */
   transaction_id subxacts[FLEXIBLE_ARRAY_MEMBER];
};
#define MinSizeOfXactSubxacts offsetof(struct xl_xact_subxacts, subxacts)

struct xl_xact_relfilenodes
{
   int nrels;             /* number of relations */
   struct rel_file_node xnodes[FLEXIBLE_ARRAY_MEMBER];
};
#define MinSizeOfXactRelfilenodes offsetof(struct xl_xact_relfilenodes, xnodes)

struct xl_xact_invals
{
   int nmsgs;             /* number of shared inval msgs */
   union shared_invalidation_message msgs[FLEXIBLE_ARRAY_MEMBER];
};
#define MinSizeOfXactInvals offsetof(struct xl_xact_invals, msgs)

struct xl_xact_twophase
{
   transaction_id xid;
};

struct xl_xact_origin
{
   xlog_rec_ptr origin_lsn;
   TimestampTz origin_timestamp;
};

struct xl_xact_commit
{
   TimestampTz xact_time;         /* time of commit */

   /* xl_xact_xinfo follows if XLOG_XACT_HAS_INFO */
   /* xl_xact_dbinfo follows if XINFO_HAS_DBINFO */
   /* xl_xact_subxacts follows if XINFO_HAS_SUBXACT */
   /* xl_xact_relfilenodes follows if XINFO_HAS_RELFILENODES */
   /* xl_xact_invals follows if XINFO_HAS_INVALS */
   /* xl_xact_twophase follows if XINFO_HAS_TWOPHASE */
   /* twophase_gid follows if XINFO_HAS_GID. As a null-terminated string. */
   /* xl_xact_origin follows if XINFO_HAS_ORIGIN, stored unaligned! */
};
#define MinSizeOfXactCommit (offsetof(struct xl_xact_commit, xact_time) + sizeof(TimestampTz))

struct xl_xact_abort
{
   TimestampTz xact_time;         /* time of abort */

   /* xl_xact_xinfo follows if XLOG_XACT_HAS_INFO */
   /* xl_xact_dbinfo follows if XINFO_HAS_DBINFO */
   /* xl_xact_subxacts follows if XINFO_HAS_SUBXACT */
   /* xl_xact_relfilenodes follows if XINFO_HAS_RELFILENODES */
   /* No invalidation messages needed. */
   /* xl_xact_twophase follows if XINFO_HAS_TWOPHASE */
   /* twophase_gid follows if XINFO_HAS_GID. As a null-terminated string. */
   /* xl_xact_origin follows if XINFO_HAS_ORIGIN, stored unaligned! */
};
#define MinSizeOfXactAbort sizeof(struct xl_xact_abort)

struct xl_xact_prepare
{
   uint32_t magic;             /* format identifier */
   uint32_t total_len;         /* actual file length */
   transaction_id xid;             /* original transaction XID */
   oid database;         /* OID of database it was in */
   TimestampTz prepared_at;     /* time of preparation */
   oid owner;             /* user running the transaction */
   int32_t nsubxacts;         /* number of following subxact XIDs */
   int32_t ncommitrels;     /* number of delete-on-commit rels */
   int32_t nabortrels;         /* number of delete-on-abort rels */
   int32_t ninvalmsgs;         /* number of cache invalidation messages */
   bool initfileinval;     /* does relcache init file need invalidation? */
   uint16_t gidlen;             /* length of the GID - GID follows the header */
   xlog_rec_ptr origin_lsn;         /* lsn of this record at origin node */
   TimestampTz origin_timestamp;     /* time of prepare at origin node */
};

/*
 * Commit/Abort records in the above form are a bit verbose to parse, so
 * there's a deconstructed versions generated by ParseCommit/AbortRecord() for
 * easier consumption.
 */
struct xl_xact_parsed_commit
{
   TimestampTz xact_time;
   uint32_t xinfo;

   oid dbId;             /* MyDatabaseId */
   oid tsId;             /* MyDatabaseTableSpace */

   int nsubxacts;
   transaction_id* subxacts;

   int nrels;
   struct rel_file_node* xnodes;

   int nmsgs;
   union shared_invalidation_message* msgs;

   transaction_id twophase_xid;  /* only for 2PC */
   char twophase_gid[GIDSIZE];     /* only for 2PC */
   int nabortrels;         /* only for 2PC */
   struct rel_file_node* abortnodes;     /* only for 2PC */

   xlog_rec_ptr origin_lsn;
   TimestampTz origin_timestamp;
};

typedef struct xl_xact_parsed_commit xl_xact_parsed_prepare;

struct xl_xact_parsed_abort
{
   TimestampTz xact_time;
   uint32_t xinfo;

   oid dbId;             /* MyDatabaseId */
   oid tsId;             /* MyDatabaseTableSpace */

   int nsubxacts;
   transaction_id* subxacts;

   int nrels;
   struct rel_file_node* xnodes;

   transaction_id twophase_xid;  /* only for 2PC */
   char twophase_gid[GIDSIZE];     /* only for 2PC */

   xlog_rec_ptr origin_lsn;
   TimestampTz origin_timestamp;
} xl_xact_parsed_abort;

char*xact_desc(char* buf, struct decoded_xlog_record* record);

#endif //PGMONETA_RM_XACT_H
