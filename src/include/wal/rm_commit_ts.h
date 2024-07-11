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

#ifndef PGMONETA_RM_COMMIT_TS_H
#define PGMONETA_RM_COMMIT_TS_H

#include "wal_reader.h"
#include "rm.h"
#include "rm_xlog.h" //TODO: remove this and move timestamp somewhere else

/* XLOG stuff */
#define COMMIT_TS_ZEROPAGE    0x00
#define COMMIT_TS_TRUNCATE    0x10

struct xl_commit_ts_set
{
   TimestampTz timestamp;
   rep_origin_id nodeid;
   transaction_id mainxid;
   /* subxact Xids follow */
};

#define SizeOfCommitTsSet  (offsetof(xl_commit_ts_set, mainxid) + \
                            sizeof(TransactionId))

struct xl_commit_ts_truncate
{
   int pageno;
   transaction_id oldestXid;
};

#define SizeOfCommitTsTruncate   (offsetof(xl_commit_ts_truncate, oldestXid) + \
                                  sizeof(TransactionId))

char* commit_ts_desc(char* buf, struct decoded_xlog_record* record);

#endif //PGMONETA_RM_COMMIT_TS_H
