//
// Created by Shahryar Soltanpour on 2024-06-25.
//

#ifndef PGMONETA_RM_MXACT_H
#define PGMONETA_RM_MXACT_H


#include "wal_reader.h"



/*
 * The first two MultiXactId values are reserved to store the truncation Xid
 * and epoch of the first segment, so we start assigning multixact values from
 * 2.
 */
#define InvalidMultiXactId	((MultiXactId) 0)
#define FirstMultiXactId	((MultiXactId) 1)
#define MaxMultiXactId		((MultiXactId) 0xFFFFFFFF)

#define MultiXactIdIsValid(multi) ((multi) != InvalidMultiXactId)

#define MaxMultiXactOffset	((MultiXactOffset) 0xFFFFFFFF)

/* Number of SLRU buffers to use for multixact */
#define NUM_MULTIXACTOFFSET_BUFFERS		8
#define NUM_MULTIXACTMEMBER_BUFFERS		16

/*
 * Possible multixact lock modes ("status").  The first four modes are for
 * tuple locks (FOR KEY SHARE, FOR SHARE, FOR NO KEY UPDATE, FOR UPDATE); the
 * next two are used for update and delete modes.
 */
typedef enum
{
    MultiXactStatusForKeyShare = 0x00,
    MultiXactStatusForShare = 0x01,
    MultiXactStatusForNoKeyUpdate = 0x02,
    MultiXactStatusForUpdate = 0x03,
    /* an update that doesn't touch "key" columns */
    MultiXactStatusNoKeyUpdate = 0x04,
    /* other updates, and delete */
    MultiXactStatusUpdate = 0x05
} MultiXactStatus;

#define MaxMultiXactStatus MultiXactStatusUpdate

/* does a status value correspond to a tuple update? */
#define ISUPDATE_from_mxstatus(status) \
			((status) > MultiXactStatusForUpdate)


typedef struct MultiXactMember
{
    TransactionId xid;
    MultiXactStatus status;
} MultiXactMember;


/* ----------------
 *		multixact-related XLOG entries
 * ----------------
 */

#define XLOG_MULTIXACT_ZERO_OFF_PAGE	0x00
#define XLOG_MULTIXACT_ZERO_MEM_PAGE	0x10
#define XLOG_MULTIXACT_CREATE_ID		0x20
#define XLOG_MULTIXACT_TRUNCATE_ID		0x30

typedef struct xl_multixact_create
{
    MultiXactId mid;			/* new MultiXact's ID */
    MultiXactOffset moff;		/* its starting offset in members file */
    int32_t		nmembers;		/* number of member XIDs */
    MultiXactMember members[FLEXIBLE_ARRAY_MEMBER];
} xl_multixact_create;

#define SizeOfMultiXactCreate (offsetof(xl_multixact_create, members))

typedef struct xl_multixact_truncate
{
    Oid			oldestMultiDB;

    /* to-be-truncated range of multixact offsets */
    MultiXactId startTruncOff;	/* just for completeness' sake */
    MultiXactId endTruncOff;

    /* to-be-truncated range of multixact members */
    MultiXactOffset startTruncMemb;
    MultiXactOffset endTruncMemb;
} xl_multixact_truncate;

#define SizeOfMultiXactTruncate (sizeof(xl_multixact_truncate))


char* multixact_desc(char* buf, DecodedXLogRecord *record);


#endif //PGMONETA_RM_MXACT_H
