//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#ifndef PGMONETA_RM_LOGICALMSG_H
#define PGMONETA_RM_LOGICALMSG_H

#include "wal_reader.h"

#define XLOG_LOGICAL_MESSAGE	0x00

typedef struct xl_logical_message
{
    oid			dbId;			/* database oid emitted from */
    bool		transactional;	/* is message transactional? */
    size_t 		prefix_size;	/* length of prefix */
    size_t 		message_size;	/* size of the message */
    /* payload, including null-terminated prefix of length prefix_size */
    char		message[FLEXIBLE_ARRAY_MEMBER];
} xl_logical_message;

#define SizeOfLogicalMessage	(offsetof(xl_logical_message, message))

char* logicalmsg_desc(char* buf, struct decoded_xlog_record *record);

#endif //PGMONETA_RM_LOGICALMSG_H
