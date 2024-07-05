//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#include "wal_reader.h"
#include "wal/rm_logicalmsg.h"
#include "wal/rm.h"
#include "utils.h"
#include "assert.h"


char*
logicalmsg_desc(char* buf, DecodedXLogRecord *record)
{
    char	   *rec = XLogRecGetData(record);
    uint8_t		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    if (info == XLOG_LOGICAL_MESSAGE)
    {
        xl_logical_message *xlrec = (xl_logical_message *) rec;
        char	   *prefix = xlrec->message;
        char	   *message = xlrec->message + xlrec->prefix_size;
        char	   *sep = "";

        assert(prefix[xlrec->prefix_size - 1] == '\0');

        buf = pgmoneta_format_and_append(buf, "%s, prefix \"%s\"; payload (%zu bytes): ",
                         xlrec->transactional ? "transactional" : "non-transactional",
                         prefix, xlrec->message_size);
        /* Write message payload as a series of hex bytes */
        for (int cnt = 0; cnt < xlrec->message_size; cnt++)
        {
            buf = pgmoneta_format_and_append(buf, "%s%02X", sep, (unsigned char) message[cnt]);
            sep = " ";
        }
    }
    return buf;
}