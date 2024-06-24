//
// Created by Shahryar Soltanpour on 2024-06-23.
//

#ifndef PGMONETA_RELPATH_H
#define PGMONETA_RELPATH_H

#include "wal_reader.h"

typedef int BackendId;			/* unique currently active backend identifier */

#define InvalidBackendId		(-1)


#define DEFAULTTABLESPACE_OID 1663
#define GLOBALTABLESPACE_OID 1664


#define PG_MAJORVERSION "14"
#define CATALOG_VERSION_NO	"202107181"
#define TABLESPACE_VERSION_DIRECTORY	"PG_" PG_MAJORVERSION "_" CATALOG_VERSION_NO



#define relpathbackend(rnode, backend, forknum) \
	GetRelationPath((rnode).dbNode, (rnode).spcNode, (rnode).relNode, \
					backend, forknum)

/* First argument is a RelFileNode */
#define relpathperm(rnode, forknum) \
	relpathbackend(rnode, InvalidBackendId, forknum)

char *
GetRelationPath(Oid dbNode, Oid spcNode, Oid relNode, int backendId, ForkNumber forkNumber);

#endif //PGMONETA_RELPATH_H
