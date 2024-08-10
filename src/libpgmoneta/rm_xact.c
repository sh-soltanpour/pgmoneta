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


#include "wal/rm_xact.h"
#include "utils.h"
#include "wal/rm_standby.h"
#include "relpath.h"


static char *xact_desc_relations(char *buf, char *label, int nrels, struct rel_file_node *xnodes);
static char *xact_desc_commit(char *buf, uint8_t info, struct xl_xact_commit *xlrec, rep_origin_id origin_id);
static char *xact_desc_abort(char *buf, uint8_t info, struct xl_xact_abort *xlrec);
static char *xact_desc_prepare(char *buf, uint8_t info, struct xl_xact_prepare *xlrec);
static char *xact_desc_assignment(char *buf, struct xl_xact_assignment *xlrec);
static char *xact_desc_subxacts(char *buf, int nsubxacts, transaction_id *subxacts);
void ParseAbortRecord(uint8_t info, struct xl_xact_abort *xlrec, struct xl_xact_parsed_abort *parsed);
void ParsePrepareRecord(uint8_t info, struct xl_xact_prepare *xlrec, xl_xact_parsed_prepare *parsed);
void ParseCommitRecord(uint8_t info, struct xl_xact_commit *xlrec, struct xl_xact_parsed_commit *parsed);


static char *
xact_desc_relations(char *buf, char *label, int nrels,
                    struct rel_file_node *xnodes) {
    int i;

    if (nrels > 0) {
        buf = pgmoneta_format_and_append(buf, "; %s:", label);
        for (i = 0; i < nrels; i++) {
            char *path = relpathperm(xnodes[i], MAIN_FORKNUM);

            buf = pgmoneta_format_and_append(buf, " %s", path);
            free(path);
        }
    }
    return buf;
}

static char *
xact_desc_commit(char *buf, uint8_t info, struct xl_xact_commit *xlrec, rep_origin_id origin_id) {
    struct xl_xact_parsed_commit parsed;
    ParseCommitRecord(info, xlrec, &parsed);

    /* If this is a prepared xact, show the xid of the original xact */
    if (TransactionIdIsValid(parsed.twophase_xid))
        buf = pgmoneta_format_and_append(buf, "%u: ", parsed.twophase_xid);

    buf = pgmoneta_format_and_append(buf, timestamptz_to_str(xlrec->xact_time));

    buf = xact_desc_relations(buf, "rels", parsed.nrels, parsed.xnodes);
    buf = xact_desc_subxacts(buf, parsed.nsubxacts, parsed.subxacts);

    buf = standby_desc_invalidations(buf, parsed.nmsgs, parsed.msgs, parsed.dbId,
                               parsed.tsId,
                               XactCompletionRelcacheInitFileInval(parsed.xinfo));

    if (XactCompletionForceSyncCommit(parsed.xinfo))
        buf = pgmoneta_format_and_append(buf, "; sync");

    if (parsed.xinfo & XACT_XINFO_HAS_ORIGIN) {
        buf = pgmoneta_format_and_append(buf, "; origin: node %u, lsn %X/%X, at %s",
                                   origin_id,
                                   LSN_FORMAT_ARGS(parsed.origin_lsn),
                                   timestamptz_to_str(parsed.origin_timestamp));
    }
    return buf;
}

static char *
xact_desc_abort(char *buf, uint8_t info, struct xl_xact_abort *xlrec) {
    struct xl_xact_parsed_abort parsed;

    ParseAbortRecord(info, xlrec, &parsed);

    /* If this is a prepared xact, show the xid of the original xact */
    if (TransactionIdIsValid(parsed.twophase_xid))
        buf = pgmoneta_format_and_append(buf, "%u: ", parsed.twophase_xid);

    buf = pgmoneta_format_and_append(buf, timestamptz_to_str(xlrec->xact_time));

    buf = xact_desc_relations(buf, "rels", parsed.nrels, parsed.xnodes);
    buf = xact_desc_subxacts(buf, parsed.nsubxacts, parsed.subxacts);

    return buf;
}

static char *
xact_desc_prepare(char *buf, uint8_t info, struct xl_xact_prepare *xlrec) {
    xl_xact_parsed_prepare parsed;

    ParsePrepareRecord(info, xlrec, &parsed);

    buf = pgmoneta_format_and_append(buf, "gid %s: ", parsed.twophase_gid);
    buf = pgmoneta_format_and_append(buf, timestamptz_to_str(parsed.xact_time));

    buf = xact_desc_relations(buf, "rels(commit)", parsed.nrels, parsed.xnodes);
    buf = xact_desc_relations(buf, "rels(abort)", parsed.nabortrels,
                        parsed.abortnodes);
    buf = xact_desc_subxacts(buf, parsed.nsubxacts, parsed.subxacts);

    buf = standby_desc_invalidations(buf, parsed.nmsgs, parsed.msgs, parsed.dbId,
                               parsed.tsId, xlrec->initfileinval);
    return buf;
}

static char *
xact_desc_assignment(char *buf, struct xl_xact_assignment *xlrec) {
    int i;

    buf = pgmoneta_format_and_append(buf, "subxacts:");

    for (i = 0; i < xlrec->nsubxacts; i++)
        buf = pgmoneta_format_and_append(buf, " %u", xlrec->xsub[i]);
    return buf;
}

static char*
xact_desc_subxacts(char* buf, int nsubxacts, transaction_id *subxacts)
{
    int			i;

    if (nsubxacts > 0)
    {
        buf = pgmoneta_format_and_append(buf, "; subxacts:");
        for (i = 0; i < nsubxacts; i++)
            buf = pgmoneta_format_and_append(buf, " %u", subxacts[i]);
    }
    return buf;
}

char *
xact_desc(char *buf, struct decoded_xlog_record *record) {
    char *rec = XLogRecGetData(record);
    uint8_t info = XLogRecGetInfo(record) & XLOG_XACT_OPMASK;


    if (info == XLOG_XACT_COMMIT || info == XLOG_XACT_COMMIT_PREPARED) {
        struct xl_xact_commit *xlrec = (struct xl_xact_commit *) rec;

        buf = xact_desc_commit(buf, XLogRecGetInfo(record), xlrec,
                         XLogRecGetOrigin(record));
    } else if (info == XLOG_XACT_ABORT || info == XLOG_XACT_ABORT_PREPARED) {
        struct xl_xact_abort *xlrec = (struct xl_xact_abort *) rec;

        buf = xact_desc_abort(buf, XLogRecGetInfo(record), xlrec);
    } else if (info == XLOG_XACT_PREPARE) {
        struct xl_xact_prepare *xlrec = (struct xl_xact_prepare *) rec;

        buf = xact_desc_prepare(buf, XLogRecGetInfo(record), xlrec);
    } else if (info == XLOG_XACT_ASSIGNMENT) {
        struct xl_xact_assignment *xlrec = (struct xl_xact_assignment *) rec;

        /*
         * Note that we ignore the WAL record's xid, since we're more
         * interested in the top-level xid that issued the record and which
         * xids are being reported here.
         */
        buf = pgmoneta_format_and_append(buf, "xtop %u: ", xlrec->xtop);
        buf = xact_desc_assignment(buf, xlrec);
    } else if (info == XLOG_XACT_INVALIDATIONS) {
        struct xl_xact_invals *xlrec = (struct xl_xact_invals *) rec;

        buf = standby_desc_invalidations(buf, xlrec->nmsgs, xlrec->msgs, InvalidOid,
                                   InvalidOid, false);
    }
    return buf;
}

void
ParseAbortRecord(uint8_t info, struct xl_xact_abort *xlrec, struct xl_xact_parsed_abort *parsed)
{
    char	   *data = ((char *) xlrec) + MinSizeOfXactAbort;

    memset(parsed, 0, sizeof(*parsed));

    parsed->xinfo = 0;			/* default, if no XLOG_XACT_HAS_INFO is
								 * present */

    parsed->xact_time = xlrec->xact_time;

    if (info & XLOG_XACT_HAS_INFO)
    {
        struct xl_xact_xinfo *xl_xinfo = (struct xl_xact_xinfo *) data;

        parsed->xinfo = xl_xinfo->xinfo;

        data += sizeof(struct xl_xact_xinfo);
    }

    if (parsed->xinfo & XACT_XINFO_HAS_DBINFO)
    {
        struct xl_xact_dbinfo *xl_dbinfo = (struct xl_xact_dbinfo *) data;

        parsed->dbId = xl_dbinfo->dbId;
        parsed->tsId = xl_dbinfo->tsId;

        data += sizeof(struct xl_xact_dbinfo);
    }

    if (parsed->xinfo & XACT_XINFO_HAS_SUBXACTS)
    {
        struct xl_xact_subxacts *xl_subxacts = (struct xl_xact_subxacts *) data;

        parsed->nsubxacts = xl_subxacts->nsubxacts;
        parsed->subxacts = xl_subxacts->subxacts;

        data += MinSizeOfXactSubxacts;
        data += parsed->nsubxacts * sizeof(transaction_id);
    }

    if (parsed->xinfo & XACT_XINFO_HAS_RELFILENODES)
    {
        struct xl_xact_relfilenodes *xl_relfilenodes = (struct xl_xact_relfilenodes *) data;

        parsed->nrels = xl_relfilenodes->nrels;
        parsed->xnodes = xl_relfilenodes->xnodes;

        data += MinSizeOfXactRelfilenodes;
        data += xl_relfilenodes->nrels * sizeof(struct rel_file_node);
    }

    if (parsed->xinfo & XACT_XINFO_HAS_TWOPHASE)
    {
        struct xl_xact_twophase *xl_twophase = (struct xl_xact_twophase *) data;

        parsed->twophase_xid = xl_twophase->xid;

        data += sizeof(struct xl_xact_twophase);

        if (parsed->xinfo & XACT_XINFO_HAS_GID)
        {
            strlcpy(parsed->twophase_gid, data, sizeof(parsed->twophase_gid));
            data += strlen(data) + 1;
        }
    }

    /* Note: no alignment is guaranteed after this point */

    if (parsed->xinfo & XACT_XINFO_HAS_ORIGIN)
    {
        struct xl_xact_origin xl_origin;

        /* no alignment is guaranteed, so copy onto stack */
        memcpy(&xl_origin, data, sizeof(xl_origin));

        parsed->origin_lsn = xl_origin.origin_lsn;
        parsed->origin_timestamp = xl_origin.origin_timestamp;

        data += sizeof(struct xl_xact_origin);
    }
}

/*
 * ParsePrepareRecord
 */
void
ParsePrepareRecord(uint8_t info, struct xl_xact_prepare *xlrec, xl_xact_parsed_prepare *parsed)
{
    char	   *bufptr;

    bufptr = ((char *) xlrec) + MAXALIGN(sizeof(struct xl_xact_prepare));

    memset(parsed, 0, sizeof(*parsed));

    parsed->xact_time = xlrec->prepared_at;
    parsed->origin_lsn = xlrec->origin_lsn;
    parsed->origin_timestamp = xlrec->origin_timestamp;
    parsed->twophase_xid = xlrec->xid;
    parsed->dbId = xlrec->database;
    parsed->nsubxacts = xlrec->nsubxacts;
    parsed->nrels = xlrec->ncommitrels;
    parsed->nabortrels = xlrec->nabortrels;
    parsed->nmsgs = xlrec->ninvalmsgs;

    strncpy(parsed->twophase_gid, bufptr, xlrec->gidlen);
    bufptr += MAXALIGN(xlrec->gidlen);

    parsed->subxacts = (transaction_id *) bufptr;
    bufptr += MAXALIGN(xlrec->nsubxacts * sizeof(transaction_id));

    parsed->xnodes = (struct rel_file_node *) bufptr;
    bufptr += MAXALIGN(xlrec->ncommitrels * sizeof(struct rel_file_node));

    parsed->abortnodes = (struct rel_file_node *) bufptr;
    bufptr += MAXALIGN(xlrec->nabortrels * sizeof(struct rel_file_node));

    parsed->msgs = (union shared_invalidation_message *) bufptr;
    bufptr += MAXALIGN(xlrec->ninvalmsgs * sizeof(union shared_invalidation_message));
}

void
ParseCommitRecord(uint8_t info, struct xl_xact_commit *xlrec, struct xl_xact_parsed_commit *parsed)
{
    char	   *data = ((char *) xlrec) + MinSizeOfXactCommit;

    memset(parsed, 0, sizeof(*parsed));

    parsed->xinfo = 0;			/* default, if no XLOG_XACT_HAS_INFO is
								 * present */

    parsed->xact_time = xlrec->xact_time;

    if (info & XLOG_XACT_HAS_INFO)
    {
        struct xl_xact_xinfo *xl_xinfo = (struct xl_xact_xinfo *) data;

        parsed->xinfo = xl_xinfo->xinfo;

        data += sizeof(struct xl_xact_xinfo);
    }

    if (parsed->xinfo & XACT_XINFO_HAS_DBINFO)
    {
        struct xl_xact_dbinfo *xl_dbinfo = (struct xl_xact_dbinfo *) data;

        parsed->dbId = xl_dbinfo->dbId;
        parsed->tsId = xl_dbinfo->tsId;

        data += sizeof(struct xl_xact_dbinfo);
    }

    if (parsed->xinfo & XACT_XINFO_HAS_SUBXACTS)
    {
        struct xl_xact_subxacts *xl_subxacts = (struct xl_xact_subxacts *) data;

        parsed->nsubxacts = xl_subxacts->nsubxacts;
        parsed->subxacts = xl_subxacts->subxacts;

        data += MinSizeOfXactSubxacts;
        data += parsed->nsubxacts * sizeof(transaction_id);
    }

    if (parsed->xinfo & XACT_XINFO_HAS_RELFILENODES)
    {
        struct xl_xact_relfilenodes *xl_relfilenodes = (struct xl_xact_relfilenodes *) data;

        parsed->nrels = xl_relfilenodes->nrels;
        parsed->xnodes = xl_relfilenodes->xnodes;

        data += MinSizeOfXactRelfilenodes;
        data += xl_relfilenodes->nrels * sizeof(struct rel_file_node);
    }

    if (parsed->xinfo & XACT_XINFO_HAS_INVALS)
    {
        struct xl_xact_invals *xl_invals = (struct xl_xact_invals *) data;

        parsed->nmsgs = xl_invals->nmsgs;
        parsed->msgs = xl_invals->msgs;

        data += MinSizeOfXactInvals;
        data += xl_invals->nmsgs * sizeof(union shared_invalidation_message);
    }

    if (parsed->xinfo & XACT_XINFO_HAS_TWOPHASE)
    {
        struct xl_xact_twophase *xl_twophase = (struct xl_xact_twophase *) data;

        parsed->twophase_xid = xl_twophase->xid;

        data += sizeof(struct xl_xact_twophase);

        if (parsed->xinfo & XACT_XINFO_HAS_GID)
        {
            strlcpy(parsed->twophase_gid, data, sizeof(parsed->twophase_gid));
            data += strlen(data) + 1;
        }
    }

    /* Note: no alignment is guaranteed after this point */

    if (parsed->xinfo & XACT_XINFO_HAS_ORIGIN)
    {
        struct xl_xact_origin xl_origin;

        /* no alignment is guaranteed, so copy onto stack */
        memcpy(&xl_origin, data, sizeof(xl_origin));

        parsed->origin_lsn = xl_origin.origin_lsn;
        parsed->origin_timestamp = xl_origin.origin_timestamp;

        data += sizeof(struct xl_xact_origin);
    }
}