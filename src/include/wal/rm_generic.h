//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#ifndef PGMONETA_RM_GENERIC_H
#define PGMONETA_RM_GENERIC_H

#include "wal_reader.h"

typedef char *pointer;

char* generic_desc(char* buf, struct decoded_xlog_record *record);

#endif //PGMONETA_RM_GENERIC_H
