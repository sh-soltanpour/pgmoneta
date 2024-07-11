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

#ifndef PGMONETA_RM_H
#define PGMONETA_RM_H

#define XLR_INFO_MASK           0x0F
#define XLR_RMGR_INFO_MASK      0xF0

typedef uint32_t block_number;

// offset
typedef uint16_t offset_number;


struct block_id_data
{
    uint16_t bi_hi;
    uint16_t bi_lo;
};

struct item_pointer_data
{
    struct block_id_data ip_blkid;
    offset_number ip_posid;
};
typedef struct block_id_data *block_id;	/* block identifier */



#define ItemPointerGetOffsetNumberNoCheck(pointer) \
        ( \
           (pointer)->ip_posid \
        )

/*
 * ItemPointerGetOffsetNumber
 *		As above, but verifies that the item pointer looks valid.
 */
#define ItemPointerGetOffsetNumber(pointer) \
        ( \
           ItemPointerGetOffsetNumberNoCheck(pointer) \
        )

/*
 * ItemPointerGetBlockNumberNoCheck
 *		Returns the block number of a disk item pointer.
 */
#define ItemPointerGetBlockNumberNoCheck(pointer) \
        ( \
           BlockIdGetBlockNumber(&(pointer)->ip_blkid) \
        )

/*
 * ItemPointerGetBlockNumber
 *		As above, but verifies that the item pointer looks valid.
 */
#define ItemPointerGetBlockNumber(pointer) \
        ( \
           ItemPointerGetBlockNumberNoCheck(pointer) \
        )

#define BlockIdGetBlockNumber(blockId) \
        ( \
           ((((block_number) (blockId)->bi_hi) << 16) | ((block_number) (blockId)->bi_lo)) \
        )

#define PostingItemGetBlockNumber(pointer) \
	BlockIdGetBlockNumber(&(pointer)->child_blkno)

#define PostingItemSetBlockNumber(pointer, blockNumber) \
	BlockIdSet(&((pointer)->child_blkno), (blockNumber))

#endif //PGMONETA_RM_H
