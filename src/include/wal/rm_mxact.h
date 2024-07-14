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

#ifndef PGMONETA_RM_MXACT_H
#define PGMONETA_RM_MXACT_H

#include "wal_reader.h"

/*
 * The first two multi_xact_id values are reserved to store the truncation Xid
 * and epoch of the first segment, so we start assigning multixact values from
 * 2.
 */
#define INVALID_MULTI_XACT_ID ((MultiXactId) 0)
#define FIRST_MULTI_XACT_ID   ((MultiXactId) 1)
#define MAX_MULTI_XACT_ID     ((MultiXactId) 0xFFFFFFFF)

#define MULTI_XACT_ID_IS_VALID(multi) ((multi) != INVALID_MULTI_XACT_ID)

#define MAX_MULTI_XACT_OFFSET ((MultiXactOffset) 0xFFFFFFFF)

/* Number of SLRU buffers to use for multixact */
#define NUM_MULTIXACTOFFSET_BUFFERS    8
#define NUM_MULTIXACTMEMBER_BUFFERS    16

/*
 * Possible multixact lock modes ("status").  The first four modes are for
 * tuple locks (FOR KEY SHARE, FOR SHARE, FOR NO KEY UPDATE, FOR UPDATE); the
 * next two are used for update and delete modes.
 */
enum MULTI_XACT_STATUS
{
   MultiXactStatusForKeyShare = 0x00,
   MultiXactStatusForShare = 0x01,
   MultiXactStatusForNoKeyUpdate = 0x02,
   MultiXactStatusForUpdate = 0x03,
   /* an update that doesn't touch "key" columns */
   MultiXactStatusNoKeyUpdate = 0x04,
   /* other updates, and delete */
   MultiXactStatusUpdate = 0x05
};

#define MaxMultiXactStatus MultiXactStatusUpdate

/* does a status value correspond to a tuple update? */
#define ISUPDATE_from_mxstatus(status) \
        ((status) > MultiXactStatusForUpdate)

struct multi_xact_member
{
   transaction_id xid;
   enum MULTI_XACT_STATUS status;
};

/* ----------------
 *		multixact-related XLOG entries
 * ----------------
 */

#define XLOG_MULTIXACT_ZERO_OFF_PAGE   0x00
#define XLOG_MULTIXACT_ZERO_MEM_PAGE   0x10
#define XLOG_MULTIXACT_CREATE_ID    0x20
#define XLOG_MULTIXACT_TRUNCATE_ID     0x30

struct xl_multixact_create
{
   multi_xact_id mid;         /* new MultiXact's ID */
   multi_xact_offset moff;       /* its starting offset in members file */
   int32_t nmembers;          /* number of member XIDs */
   struct multi_xact_member members[FLEXIBLE_ARRAY_MEMBER];
};

#define SIZE_OF_MULTI_XACT_CREATE (offsetof(xl_multixact_create, members))

struct xl_multixact_truncate
{
   oid oldestMultiDB;

   /* to-be-truncated range of multixact offsets */
   multi_xact_id startTruncOff;  /* just for completeness' sake */
   multi_xact_id endTruncOff;

   /* to-be-truncated range of multixact members */
   multi_xact_offset startTruncMemb;
   multi_xact_offset endTruncMemb;
};

#define SIZE_OF_MULTI_XACT_TRUNCATE (sizeof(xl_multixact_truncate))

char* multixact_desc(char* buf, struct decoded_xlog_record* record);

#endif //PGMONETA_RM_MXACT_H
