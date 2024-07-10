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


#ifndef PGMONETA_PG_CONTROL_H
#define PGMONETA_PG_CONTROL_H
#include "wal_reader.h"
#include "transaction.h"

typedef int64_t pg_time_t;

/*
 * Body of CheckPoint XLOG records.  This is declared here because we keep
 * a copy of the latest one in pg_control for possible disaster recovery.
 * Changing this struct requires a PG_CONTROL_VERSION bump.
 */
typedef struct CheckPoint
{
    xlog_rec_ptr	redo;			/* next RecPtr available when we began to
								 * create CheckPoint (i.e. REDO start point) */
    timeline_id	ThisTimeLineID; /* current TLI */
    timeline_id	PrevTimeLineID; /* previous TLI, if this record begins a new
								 * timeline (equals ThisTimeLineID otherwise) */
    bool		fullPageWrites; /* current full_page_writes */
    FullTransactionId nextXid;	/* next free transaction ID */
    oid			nextOid;		/* next free OID */
    MultiXactId nextMulti;		/* next free MultiXactId */
    MultiXactOffset nextMultiOffset;	/* next free MultiXact offset */
    TransactionId oldestXid;	/* cluster-wide minimum datfrozenxid */
    oid			oldestXidDB;	/* database with minimum datfrozenxid */
    MultiXactId oldestMulti;	/* cluster-wide minimum datminmxid */
    oid			oldestMultiDB;	/* database with minimum datminmxid */
    pg_time_t	time;			/* time stamp of checkpoint */
    TransactionId oldestCommitTsXid;	/* oldest Xid with valid commit
										 * timestamp */
    TransactionId newestCommitTsXid;	/* newest Xid with valid commit
										 * timestamp */

    /*
     * Oldest XID still running. This is only needed to initialize hot standby
     * mode from an online checkpoint, so we only bother calculating this for
     * online checkpoints and only when wal_level is replica. Otherwise it's
     * set to InvalidTransactionId.
     */
    TransactionId oldestActiveXid;
} CheckPoint;

/* XLOG info values for XLOG rmgr */
#define XLOG_CHECKPOINT_SHUTDOWN		0x00
#define XLOG_CHECKPOINT_ONLINE			0x10
#define XLOG_NOOP						0x20
#define XLOG_NEXTOID					0x30
#define XLOG_SWITCH						0x40
#define XLOG_BACKUP_END					0x50
#define XLOG_PARAMETER_CHANGE			0x60
#define XLOG_RESTORE_POINT				0x70
#define XLOG_FPW_CHANGE					0x80
#define XLOG_END_OF_RECOVERY			0x90
#define XLOG_FPI_FOR_HINT				0xA0
#define XLOG_FPI						0xB0
/* 0xC0 is used in Postgres 9.5-11 */
#define XLOG_OVERWRITE_CONTRECORD		0xD0

#endif //PGMONETA_PG_CONTROL_H
