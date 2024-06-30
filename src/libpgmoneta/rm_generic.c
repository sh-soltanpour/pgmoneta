//
// Created by Shahryar Soltanpour on 2024-06-26.
//

#include "rm_generic.h"
#include "rm.h"
#include "utils.h"
#include "string.h"

char*
generic_desc(char* buf, DecodedXLogRecord *record)
{
    Pointer		ptr = XLogRecGetData(record),
            end = ptr + XLogRecGetDataLen(record);

    while (ptr < end)
    {
        OffsetNumber offset,
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
