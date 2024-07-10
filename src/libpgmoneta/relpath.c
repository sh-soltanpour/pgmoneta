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


/*
 * GetRelationPath - construct path to a relation's file
 *
 * Result is a palloc'd string.
 *
 * Note: ideally, backendId would be declared as type BackendId, but relpath.h
 * would have to include a backend-only header to do that; doesn't seem worth
 * the trouble considering BackendId is just int anyway.
 */

#include "relpath.h"
#include "assert.h"
#include "utils.h"


char *
GetRelationPath(oid dbNode, oid spcNode, oid relNode,
                int backendId, enum fork_number forkNumber)
{
    const char *const forkNames[] = {
            "main",						/* MAIN_FORKNUM */
            "fsm",						/* FSM_FORKNUM */
            "vm",						/* VISIBILITYMAP_FORKNUM */
            "init"						/* INIT_FORKNUM */
    };
    char	   *path;
    path = NULL;

    if (spcNode == GLOBALTABLESPACE_OID)
    {
        /* Shared system relations live in {datadir}/global */
        assert(dbNode == 0);
        assert(backendId == InvalidBackendId);
        if (forkNumber != MAIN_FORKNUM)
            path = pgmoneta_format_and_append(path,"global/%u_%s",
                            relNode, forkNames[forkNumber]);
        else
            path = pgmoneta_format_and_append(path,"global/%u", relNode);
    }
    else if (spcNode == DEFAULTTABLESPACE_OID)
    {
        /* The default tablespace is {datadir}/base */
        if (backendId == InvalidBackendId)
        {
            if (forkNumber != MAIN_FORKNUM)
                path = pgmoneta_format_and_append(path,"base/%u/%u_%s",
                                dbNode, relNode,
                                forkNames[forkNumber]);
            else
                path = pgmoneta_format_and_append(path,"base/%u/%u",
                                dbNode, relNode);
        }
        else
        {
            if (forkNumber != MAIN_FORKNUM)
                path = pgmoneta_format_and_append(path,"base/%u/t%d_%u_%s",
                                dbNode, backendId, relNode,
                                forkNames[forkNumber]);
            else
                path = pgmoneta_format_and_append(path,"base/%u/t%d_%u",
                                dbNode, backendId, relNode);
        }
    }
    else
    {
        /* All other tablespaces are accessed via symlinks */
        if (backendId == InvalidBackendId)
        {
            if (forkNumber != MAIN_FORKNUM)
                path = pgmoneta_format_and_append(path,"pg_tblspc/%u/%s/%u/%u_%s",
                                spcNode, TABLESPACE_VERSION_DIRECTORY,
                                dbNode, relNode,
                                forkNames[forkNumber]);
            else
                path = pgmoneta_format_and_append(path,"pg_tblspc/%u/%s/%u/%u",
                                spcNode, TABLESPACE_VERSION_DIRECTORY,
                                dbNode, relNode);
        }
        else
        {
            if (forkNumber != MAIN_FORKNUM)
                path = pgmoneta_format_and_append(path,"pg_tblspc/%u/%s/%u/t%d_%u_%s",
                                spcNode, TABLESPACE_VERSION_DIRECTORY,
                                dbNode, backendId, relNode,
                                forkNames[forkNumber]);
            else
                path = pgmoneta_format_and_append(path,"pg_tblspc/%u/%s/%u/t%d_%u",
                                spcNode, TABLESPACE_VERSION_DIRECTORY,
                                dbNode, backendId, relNode);
        }
    }
    return path;
}
