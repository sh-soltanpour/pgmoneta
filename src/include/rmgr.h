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

#define PG_RMGR(symname, name) {name},
#define RM_MAX_ID           UINT8_MAX

typedef struct
{
   char* name;
   uint8_t id;
} RmgrData;

RmgrData RmgrTable[RM_MAX_ID + 1] = {
   PG_RMGR(RM_XLOG_ID, "XLOG")
   PG_RMGR(RM_XACT_ID, "Transaction")
   PG_RMGR(RM_SMGR_ID, "Storage")
   PG_RMGR(RM_CLOG_ID, "CLOG")
   PG_RMGR(RM_DBASE_ID, "Database")
   PG_RMGR(RM_TBLSPC_ID, "Tablespace")
   PG_RMGR(RM_MULTIXACT_ID, "MultiXact")
   PG_RMGR(RM_RELMAP_ID, "RelMap")
   PG_RMGR(RM_STANDBY_ID, "Standby")
   PG_RMGR(RM_HEAP2_ID, "Heap2")
   PG_RMGR(RM_HEAP_ID, "Heap")
   PG_RMGR(RM_BTREE_ID, "Btree")
   PG_RMGR(RM_HASH_ID, "Hash")
   PG_RMGR(RM_GIN_ID, "Gin")
   PG_RMGR(RM_GIST_ID, "Gist")
   PG_RMGR(RM_SEQ_ID, "Sequence")
   PG_RMGR(RM_SPGIST_ID, "SPGist")
   PG_RMGR(RM_BRIN_ID, "BRIN")
   PG_RMGR(RM_COMMIT_TS_ID, "CommitTs")
   PG_RMGR(RM_REPLORIGIN_ID, "ReplicationOrigin")
   PG_RMGR(RM_GENERIC_ID, "Generic")
   PG_RMGR(RM_LOGICALMSG_ID, "LogicalMessage")
};

#endif // PGMONETA_RMGR_H
