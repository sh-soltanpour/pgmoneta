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


#ifndef PGMONETA_RMGR_H
#define PGMONETA_RMGR_H

#include <stdint.h>
#include "rm_xlog.h"
#include "rm_xact.h"
#include "rm_storage.h"
#include "rm_clog.h"
#include "rm_database.h"
#include "rm_tablespace.h"
#include "rm_mxact.h"
#include "rm_relmap.h"
#include "rm_standby.h"
#include "rm_heap.h"
#include "rm_btree.h"
#include "rm_hash.h"
#include "rm_gin.h"
#include "rm_gist.h"
#include "rm_seq.h"
#include "rm_spgist.h"
#include "rm_brin.h"
#include "rm_generic.h"
#include "rm_commit_ts.h"
#include "rm_replorigin.h"
#include "rm_logicalmsg.h"

#define PG_RMGR(symname, name, desc) {name, desc},
#define RM_MAX_ID           UINT8_MAX

struct rmgr_data
{
    char* name;
    char* (*rm_desc)(char* buf, struct decoded_xlog_record *record);
};

struct rmgr_data RmgrTable[RM_MAX_ID + 1] = {
        PG_RMGR(RM_XLOG_ID, "XLOG", xlog_desc)
        PG_RMGR(RM_XACT_ID, "Transaction", xact_desc)
        PG_RMGR(RM_SMGR_ID, "Storage", storage_desc)
        PG_RMGR(RM_CLOG_ID, "CLOG", clog_desc)
        PG_RMGR(RM_DBASE_ID, "Database", database_desc)
        PG_RMGR(RM_TBLSPC_ID, "Tablespace", tablespace_desc)
        PG_RMGR(RM_MULTIXACT_ID, "MultiXact", multixact_desc)
        PG_RMGR(RM_RELMAP_ID, "RelMap", relmap_desc)
        PG_RMGR(RM_STANDBY_ID, "Standby", standby_desc)
        PG_RMGR(RM_HEAP2_ID, "Heap2", heap2_desc)
        PG_RMGR(RM_HEAP_ID, "Heap", heap_desc)
        PG_RMGR(RM_BTREE_ID, "Btree", btree_desc)
        PG_RMGR(RM_HASH_ID, "Hash", hash_desc)
        PG_RMGR(RM_GIN_ID, "Gin", gin_desc)
        PG_RMGR(RM_GIST_ID, "Gist", gist_desc)
        PG_RMGR(RM_SEQ_ID, "Sequence", seq_desc)
        PG_RMGR(RM_SPGIST_ID, "SPGist", spg_desc)
        PG_RMGR(RM_BRIN_ID, "BRIN", brin_desc)
        PG_RMGR(RM_COMMIT_TS_ID, "CommitTs", commit_ts_desc)
        PG_RMGR(RM_REPLORIGIN_ID, "ReplicationOrigin", replorigin_desc)
        PG_RMGR(RM_GENERIC_ID, "Generic", generic_desc)
        PG_RMGR(RM_LOGICALMSG_ID, "LogicalMessage", logicalmsg_desc)
};


#endif // PGMONETA_RMGR_H
