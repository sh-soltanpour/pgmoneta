//
// Created by Shahryar Soltanpour on 2024-05-26.
//

#ifndef PGMONETA_RMGR_H
#define PGMONETA_RMGR_H

#include <stdint.h>

#define PG_RMGR(symname, name) {name},
#define RM_MAX_ID           UINT8_MAX

typedef struct {
    char *name;
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

