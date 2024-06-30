//
// Created by Shahryar Soltanpour on 2024-06-25.
//

#ifndef PGMONETA_RM_STORAGE_H
#define PGMONETA_RM_STORAGE_H

#include "wal_reader.h"

/* XLOG gives us high 4 bits */
#define XLOG_SMGR_CREATE	0x10
#define XLOG_SMGR_TRUNCATE	0x20

/* XLOG gives us high 4 bits */
#define XLOG_SMGR_CREATE	0x10
#define XLOG_SMGR_TRUNCATE	0x20

typedef struct xl_smgr_create
{
    RelFileNode rnode;
    ForkNumber	forkNum;
} xl_smgr_create;

/* flags for xl_smgr_truncate */
#define SMGR_TRUNCATE_HEAP		0x0001
#define SMGR_TRUNCATE_VM		0x0002
#define SMGR_TRUNCATE_FSM		0x0004
#define SMGR_TRUNCATE_ALL		\
	(SMGR_TRUNCATE_HEAP|SMGR_TRUNCATE_VM|SMGR_TRUNCATE_FSM)

typedef struct xl_smgr_truncate
{
    BlockNumber blkno;
    RelFileNode rnode;
    int			flags;
} xl_smgr_truncate;

char* storage_desc(char* buf, DecodedXLogRecord *record);


#endif //PGMONETA_RM_STORAGE_H
