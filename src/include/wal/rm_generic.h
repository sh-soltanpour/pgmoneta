//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#ifndef PGMONETA_RM_GENERIC_H
#define PGMONETA_RM_GENERIC_H

#include "wal_reader.h"

typedef char *Pointer;

char* generic_desc(char* buf, DecodedXLogRecord *record);

#endif //PGMONETA_RM_GENERIC_H
