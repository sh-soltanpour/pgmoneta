//
// Created by Shahryar Soltanpour on 2024-06-25.
//

#ifndef PGMONETA_RM_SEQ_H
#define PGMONETA_RM_SEQ_H

#include "wal_reader.h"

/* XLOG stuff */
#define XLOG_SEQ_LOG			0x00

typedef struct xl_seq_rec
{
    RelFileNode node;
    /* SEQUENCE TUPLE DATA FOLLOWS AT THE END */
} xl_seq_rec;

char* seq_desc(char* buf, DecodedXLogRecord *record);

#endif //PGMONETA_RM_SEQ_H
