//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#include "wal/rm_generic.h"
#include "wal/rm.h"
#include "utils.h"
#include "string.h"

char *
generic_desc(char *buf, struct decoded_xlog_record *record) {
    pointer ptr = XLogRecGetData(record),
            end = ptr + XLogRecGetDataLen(record);

    while (ptr < end) {
        offset_number offset,
                length;

        memcpy(&offset, ptr, sizeof(offset));
        ptr += sizeof(offset);
        memcpy(&length, ptr, sizeof(length));
        ptr += sizeof(length);
        ptr += length;

        if (ptr < end)
            buf = pgmoneta_format_and_append(buf, "offset %u, length %u; ", offset, length);
        else
            buf = pgmoneta_format_and_append(buf, "offset %u, length %u", offset, length);
    }
    return buf;
}
