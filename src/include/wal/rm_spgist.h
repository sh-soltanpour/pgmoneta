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

#ifndef PGMONETA_RM_SPGIST_H
#define PGMONETA_RM_SPGIST_H

#include "wal_reader.h"
#include "rm.h"

/* XLOG record types for SPGiST */
/* #define XLOG_SPGIST_CREATE_INDEX       0x00 */	/* not used anymore */
#define XLOG_SPGIST_ADD_LEAF		0x10
#define XLOG_SPGIST_MOVE_LEAFS		0x20
#define XLOG_SPGIST_ADD_NODE		0x30
#define XLOG_SPGIST_SPLIT_TUPLE		0x40
#define XLOG_SPGIST_PICKSPLIT		0x50
#define XLOG_SPGIST_VACUUM_LEAF		0x60
#define XLOG_SPGIST_VACUUM_ROOT		0x70
#define XLOG_SPGIST_VACUUM_REDIRECT 0x80

/*
 * Some redo functions need an SpGistState, although only a few of its fields
 * need to be valid.  spgxlogState carries the required info in xlog records.
 * (See fillFakeState in spgxlog.c for more comments.)
 */
typedef struct spgxlogState
{
    TransactionId myXid;
    bool		isBuild;
} spgxlogState;

/*
 * Backup Blk 0: destination page for leaf tuple
 * Backup Blk 1: parent page (if any)
 */
typedef struct spgxlogAddLeaf
{
    bool		newPage;		/* init dest page? */
    bool		storesNulls;	/* page is in the nulls tree? */
    offset_number offnumLeaf;	/* offset where leaf tuple gets placed */
    offset_number offnumHeadLeaf;	/* offset of head tuple in chain, if any */

    offset_number offnumParent;	/* where the parent downlink is, if any */
    uint16_t		nodeI;

    /* new leaf tuple follows (unaligned!) */
} spgxlogAddLeaf;

/*
 * Backup Blk 0: source leaf page
 * Backup Blk 1: destination leaf page
 * Backup Blk 2: parent page
 */
typedef struct spgxlogMoveLeafs
{
    uint16_t		nMoves;			/* number of tuples moved from source page */
    bool		newPage;		/* init dest page? */
    bool		replaceDead;	/* are we replacing a DEAD source tuple? */
    bool		storesNulls;	/* pages are in the nulls tree? */

    /* where the parent downlink is */
    offset_number offnumParent;
    uint16_t		nodeI;

    spgxlogState stateSrc;

    /*----------
     * data follows:
     *		array of deleted tuple numbers, length nMoves
     *		array of inserted tuple numbers, length nMoves + 1 or 1
     *		list of leaf tuples, length nMoves + 1 or 1 (unaligned!)
     *
     * Note: if replaceDead is true then there is only one inserted tuple
     * number and only one leaf tuple in the data, because we are not copying
     * the dead tuple from the source
     *----------
     */
    offset_number offsets[FLEXIBLE_ARRAY_MEMBER];
} spgxlogMoveLeafs;

#define SizeOfSpgxlogMoveLeafs	offsetof(spgxlogMoveLeafs, offsets)

/*
 * Backup Blk 0: original page
 * Backup Blk 1: where new tuple goes, if not same place
 * Backup Blk 2: where parent downlink is, if updated and different from
 *				 the old and new
 */
typedef struct spgxlogAddNode
{
    /*
     * Offset of the original inner tuple, in the original page (on backup
     * block 0).
     */
    offset_number offnum;

    /*
     * Offset of the new tuple, on the new page (on backup block 1). Invalid,
     * if we overwrote the old tuple in the original page).
     */
    offset_number offnumNew;
    bool		newPage;		/* init new page? */

    /*----
     * Where is the parent downlink? parentBlk indicates which page it's on,
     * and offnumParent is the offset within the page. The possible values for
     * parentBlk are:
     *
     * 0: parent == original page
     * 1: parent == new page
     * 2: parent == different page (blk ref 2)
     * -1: parent not updated
     *----
     */
    int8_t		parentBlk;
    offset_number offnumParent;	/* offset within the parent page */

    uint16_t		nodeI;

    spgxlogState stateSrc;

    /*
     * updated inner tuple follows (unaligned!)
     */
} spgxlogAddNode;

/*
 * Backup Blk 0: where the prefix tuple goes
 * Backup Blk 1: where the postfix tuple goes (if different page)
 */
typedef struct spgxlogSplitTuple
{
    /* where the prefix tuple goes */
    offset_number offnumPrefix;

    /* where the postfix tuple goes */
    offset_number offnumPostfix;
    bool		newPage;		/* need to init that page? */
    bool		postfixBlkSame; /* was postfix tuple put on same page as
								 * prefix? */

    /*
     * new prefix inner tuple follows, then new postfix inner tuple (both are
     * unaligned!)
     */
} spgxlogSplitTuple;

/*
 * buffer references in the rdata array are:
 * Backup Blk 0: Src page (only if not root)
 * Backup Blk 1: Dest page (if used)
 * Backup Blk 2: Inner page
 * Backup Blk 3: Parent page (if any, and different from Inner)
 */
typedef struct spgxlogPickSplit
{
    bool		isRootSplit;

    uint16_t		nDelete;		/* n to delete from Src */
    uint16_t		nInsert;		/* n to insert on Src and/or Dest */
    bool		initSrc;		/* re-init the Src page? */
    bool		initDest;		/* re-init the Dest page? */

    /* where to put new inner tuple */
    offset_number offnumInner;
    bool		initInner;		/* re-init the Inner page? */

    bool		storesNulls;	/* pages are in the nulls tree? */

    /* where the parent downlink is, if any */
    bool		innerIsParent;	/* is parent the same as inner page? */
    offset_number offnumParent;
    uint16_t		nodeI;

    spgxlogState stateSrc;

    /*----------
     * data follows:
     *		array of deleted tuple numbers, length nDelete
     *		array of inserted tuple numbers, length nInsert
     *		array of page selector bytes for inserted tuples, length nInsert
     *		new inner tuple (unaligned!)
     *		list of leaf tuples, length nInsert (unaligned!)
     *----------
     */
    offset_number offsets[FLEXIBLE_ARRAY_MEMBER];
} spgxlogPickSplit;

#define SizeOfSpgxlogPickSplit offsetof(spgxlogPickSplit, offsets)

typedef struct spgxlogVacuumLeaf
{
    uint16_t		nDead;			/* number of tuples to become DEAD */
    uint16_t		nPlaceholder;	/* number of tuples to become PLACEHOLDER */
    uint16_t		nMove;			/* number of tuples to move */
    uint16_t		nChain;			/* number of tuples to re-chain */

    spgxlogState stateSrc;

    /*----------
     * data follows:
     *		tuple numbers to become DEAD
     *		tuple numbers to become PLACEHOLDER
     *		tuple numbers to move from (and replace with PLACEHOLDER)
     *		tuple numbers to move to (replacing what is there)
     *		tuple numbers to update nextOffset links of
     *		tuple numbers to insert in nextOffset links
     *----------
     */
    offset_number offsets[FLEXIBLE_ARRAY_MEMBER];
} spgxlogVacuumLeaf;

#define SizeOfSpgxlogVacuumLeaf offsetof(spgxlogVacuumLeaf, offsets)

typedef struct spgxlogVacuumRoot
{
    /* vacuum a root page when it is also a leaf */
    uint16_t		nDelete;		/* number of tuples to delete */

    spgxlogState stateSrc;

    /* offsets of tuples to delete follow */
    offset_number offsets[FLEXIBLE_ARRAY_MEMBER];
} spgxlogVacuumRoot;

#define SizeOfSpgxlogVacuumRoot offsetof(spgxlogVacuumRoot, offsets)

typedef struct spgxlogVacuumRedirect
{
    uint16_t		nToPlaceholder; /* number of redirects to make placeholders */
    offset_number firstPlaceholder;	/* first placeholder tuple to remove */
    TransactionId newestRedirectXid;	/* newest XID of removed redirects */

    /* offsets of redirect tuples to make placeholders follow */
    offset_number offsets[FLEXIBLE_ARRAY_MEMBER];
} spgxlogVacuumRedirect;

#define SizeOfSpgxlogVacuumRedirect offsetof(spgxlogVacuumRedirect, offsets)

char* spg_desc(char* buf, struct decoded_xlog_record *record);

#endif //PGMONETA_RM_SPGIST_H
