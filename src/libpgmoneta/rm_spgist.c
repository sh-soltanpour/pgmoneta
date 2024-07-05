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

#include "wal/rm_spgist.h"
#include "wal/rm.h"
#include "utils.h"

char*
spg_desc(char* buf, DecodedXLogRecord *record)
{
    char	   *rec = XLogRecGetData(record);
    uint8_t		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

    switch (info)
    {
        case XLOG_SPGIST_ADD_LEAF:
        {
            spgxlogAddLeaf *xlrec = (spgxlogAddLeaf *) rec;

            buf = pgmoneta_format_and_append(buf, "off: %u, headoff: %u, parentoff: %u, nodeI: %u",
                             xlrec->offnumLeaf, xlrec->offnumHeadLeaf,
                             xlrec->offnumParent, xlrec->nodeI);
            if (xlrec->newPage)
                buf = pgmoneta_format_and_append(buf, " (newpage)");
            if (xlrec->storesNulls)
                buf = pgmoneta_format_and_append(buf, " (nulls)");
        }
            break;
        case XLOG_SPGIST_MOVE_LEAFS:
        {
            spgxlogMoveLeafs *xlrec = (spgxlogMoveLeafs *) rec;

            buf = pgmoneta_format_and_append(buf, "nmoves: %u, parentoff: %u, nodeI: %u",
                             xlrec->nMoves,
                             xlrec->offnumParent, xlrec->nodeI);
            if (xlrec->newPage)
                buf = pgmoneta_format_and_append(buf, " (newpage)");
            if (xlrec->replaceDead)
                buf = pgmoneta_format_and_append(buf, " (replacedead)");
            if (xlrec->storesNulls)
                buf = pgmoneta_format_and_append(buf, " (nulls)");
        }
            break;
        case XLOG_SPGIST_ADD_NODE:
        {
            spgxlogAddNode *xlrec = (spgxlogAddNode *) rec;

            buf = pgmoneta_format_and_append(buf, "off: %u, newoff: %u, parentBlk: %d, "
                                  "parentoff: %u, nodeI: %u",
                             xlrec->offnum,
                             xlrec->offnumNew,
                             xlrec->parentBlk,
                             xlrec->offnumParent,
                             xlrec->nodeI);
            if (xlrec->newPage)
                buf = pgmoneta_format_and_append(buf, " (newpage)");
        }
            break;
        case XLOG_SPGIST_SPLIT_TUPLE:
        {
            spgxlogSplitTuple *xlrec = (spgxlogSplitTuple *) rec;

            buf = pgmoneta_format_and_append(buf, "prefixoff: %u, postfixoff: %u",
                             xlrec->offnumPrefix,
                             xlrec->offnumPostfix);
            if (xlrec->newPage)
                buf = pgmoneta_format_and_append(buf, " (newpage)");
            if (xlrec->postfixBlkSame)
                buf = pgmoneta_format_and_append(buf, " (same)");
        }
            break;
        case XLOG_SPGIST_PICKSPLIT:
        {
            spgxlogPickSplit *xlrec = (spgxlogPickSplit *) rec;

            buf = pgmoneta_format_and_append(buf, "ndelete: %u, ninsert: %u, inneroff: %u, "
                                  "parentoff: %u, nodeI: %u",
                             xlrec->nDelete, xlrec->nInsert,
                             xlrec->offnumInner,
                             xlrec->offnumParent, xlrec->nodeI);
            if (xlrec->innerIsParent)
                buf = pgmoneta_format_and_append(buf, " (innerIsParent)");
            if (xlrec->storesNulls)
                buf = pgmoneta_format_and_append(buf, " (nulls)");
            if (xlrec->isRootSplit)
                buf = pgmoneta_format_and_append(buf, " (isRootSplit)");
        }
            break;
        case XLOG_SPGIST_VACUUM_LEAF:
        {
            spgxlogVacuumLeaf *xlrec = (spgxlogVacuumLeaf *) rec;

            buf = pgmoneta_format_and_append(buf, "ndead: %u, nplaceholder: %u, nmove: %u, nchain: %u",
                             xlrec->nDead, xlrec->nPlaceholder,
                             xlrec->nMove, xlrec->nChain);
        }
            break;
        case XLOG_SPGIST_VACUUM_ROOT:
        {
            spgxlogVacuumRoot *xlrec = (spgxlogVacuumRoot *) rec;

            buf = pgmoneta_format_and_append(buf, "ndelete: %u",
                             xlrec->nDelete);
        }
            break;
        case XLOG_SPGIST_VACUUM_REDIRECT:
        {
            spgxlogVacuumRedirect *xlrec = (spgxlogVacuumRedirect *) rec;

            buf = pgmoneta_format_and_append(buf, "ntoplaceholder: %u, firstplaceholder: %u, newestredirectxid: %u",
                             xlrec->nToPlaceholder,
                             xlrec->firstPlaceholder,
                             xlrec->newestRedirectXid);
        }
            break;
    }
    return buf;
}