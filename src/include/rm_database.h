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

#ifndef PGMONETA_RM_DATABASE_H
#define PGMONETA_RM_DATABASE_H

#include "wal_reader.h"

/* record types */
#define XLOG_DBASE_CREATE		0x00
#define XLOG_DBASE_DROP			0x10

typedef struct xl_dbase_create_rec
{
    /* Records copying of a single subdirectory incl. contents */
    Oid			db_id;
    Oid			tablespace_id;
    Oid			src_db_id;
    Oid			src_tablespace_id;
} xl_dbase_create_rec;

typedef struct xl_dbase_drop_rec
{
    Oid			db_id;
    int			ntablespaces;	/* number of tablespace IDs */
    Oid			tablespace_ids[FLEXIBLE_ARRAY_MEMBER];
} xl_dbase_drop_rec;
#define MinSizeOfDbaseDropRec offsetof(xl_dbase_drop_rec, tablespace_ids)


char* database_desc(char* buf, DecodedXLogRecord *record);

#endif //PGMONETA_RM_DATABASE_H
