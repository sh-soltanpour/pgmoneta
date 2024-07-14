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

#ifndef PGMONETA_RELPATH_H
#define PGMONETA_RELPATH_H

#include "wal/wal_reader.h"

typedef int BackendId;        /* unique currently active backend identifier */

#define InvalidBackendId      (-1)

#define DEFAULTTABLESPACE_OID 1663
#define GLOBALTABLESPACE_OID 1664

#define PG_MAJORVERSION "14"
#define CATALOG_VERSION_NO "202107181"
#define TABLESPACE_VERSION_DIRECTORY   "PG_" PG_MAJORVERSION "_" CATALOG_VERSION_NO

#define RELPATHBACKEND(rnode, backend, forknum) \
        get_relation_path((rnode).dbNode, (rnode).spcNode, (rnode).relNode, \
                        backend, forknum)

/* First argument is a rel_file_node */
#define RELPATHPERM(rnode, forknum) \
        RELPATHBACKEND(rnode, InvalidBackendId, forknum)

char*
get_relation_path(oid dbNode, oid spcNode, oid relNode, int backendId, enum fork_number forkNumber);

#endif //PGMONETA_RELPATH_H
