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

#include "wal_reader.h"
#include "wal/rm.h"
#include "pg_control.h"
#include "utils.h"
#include "wal/rm_xlog.h"
#include "pgmoneta.h"


const struct config_enum_entry wal_level_options[] = {
        {"minimal", WAL_LEVEL_MINIMAL, false},
        {"replica", WAL_LEVEL_REPLICA, false},
        {"archive", WAL_LEVEL_REPLICA, true},	/* deprecated */
        {"hot_standby", WAL_LEVEL_REPLICA, true},	/* deprecated */
        {"logical", WAL_LEVEL_LOGICAL, false},
        {NULL, 0, false}
};


char*
xlog_desc(char* buf,  struct decoded_xlog_record* record)
{
    char	   *rec = record->main_data;
    uint8_t		info = record->header.xl_info & ~XLR_INFO_MASK;
    buf = pgmoneta_append(buf, "");

    if (info == XLOG_CHECKPOINT_SHUTDOWN ||
        info == XLOG_CHECKPOINT_ONLINE)
    {
        CheckPoint *checkpoint = (CheckPoint *) rec;

        buf = pgmoneta_format_and_append(buf, "redo %X/%X; "
                              "tli %u; prev tli %u; fpw %s; xid %u:%u; oid %u; multi %u; offset %u; "
                              "oldest xid %u in DB %u; oldest multi %u in DB %u; "
                              "oldest/newest commit timestamp xid: %u/%u; "
                              "oldest running xid %u; %s",
                         LSN_FORMAT_ARGS(checkpoint->redo),
                         checkpoint->ThisTimeLineID,
                         checkpoint->PrevTimeLineID,
                         checkpoint->fullPageWrites ? "true" : "false",
                         EpochFromFullTransactionId(checkpoint->nextXid),
                         XidFromFullTransactionId(checkpoint->nextXid),
                         checkpoint->nextOid,
                         checkpoint->nextMulti,
                         checkpoint->nextMultiOffset,
                         checkpoint->oldestXid,
                         checkpoint->oldestXidDB,
                         checkpoint->oldestMulti,
                         checkpoint->oldestMultiDB,
                         checkpoint->oldestCommitTsXid,
                         checkpoint->newestCommitTsXid,
                         checkpoint->oldestActiveXid,
                         (info == XLOG_CHECKPOINT_SHUTDOWN) ? "shutdown" : "online");
    }
    else if (info == XLOG_NEXTOID)
    {
        oid			nextOid;

        memcpy(&nextOid, rec, sizeof(oid));
        buf = pgmoneta_format_and_append(buf, "%u", nextOid);
    }
    else if (info == XLOG_RESTORE_POINT)
    {
        xl_restore_point *xlrec = (xl_restore_point *) rec;

        buf = pgmoneta_format_and_append(buf, xlrec->rp_name);
    }
    else if (info == XLOG_FPI || info == XLOG_FPI_FOR_HINT)
    {
        /* no further information to print */
    }
    else if (info == XLOG_BACKUP_END)
    {
        xlog_rec_ptr	startpoint;

        memcpy(&startpoint, rec, sizeof(xlog_rec_ptr));
        buf = pgmoneta_format_and_append(buf, "%X/%X", LSN_FORMAT_ARGS(startpoint));
    }
    else if (info == XLOG_PARAMETER_CHANGE)
    {
        xl_parameter_change xlrec;
        const char *wal_level_str;
        const struct config_enum_entry *entry;

        memcpy(&xlrec, rec, sizeof(xl_parameter_change));

        /* Find a string representation for wal_level */
        wal_level_str = "?";
        for (entry = wal_level_options; entry->name; entry++)
        {
            if (entry->val == xlrec.wal_level)
            {
                wal_level_str = entry->name;
                break;
            }
        }

        buf = pgmoneta_format_and_append(buf, "max_connections=%d max_worker_processes=%d "
                              "max_wal_senders=%d max_prepared_xacts=%d "
                              "max_locks_per_xact=%d wal_level=%s "
                              "wal_log_hints=%s track_commit_timestamp=%s",
                         xlrec.MaxConnections,
                         xlrec.max_worker_processes,
                         xlrec.max_wal_senders,
                         xlrec.max_prepared_xacts,
                         xlrec.max_locks_per_xact,
                         wal_level_str,
                         xlrec.wal_log_hints ? "on" : "off",
                         xlrec.track_commit_timestamp ? "on" : "off");
    }
    else if (info == XLOG_FPW_CHANGE)
    {
        bool		fpw;

        memcpy(&fpw, rec, sizeof(bool));
        buf = pgmoneta_format_and_append(buf, fpw ? "true" : "false");
    }
    else if (info == XLOG_END_OF_RECOVERY)
    {
        xl_end_of_recovery xlrec;

        memcpy(&xlrec, rec, sizeof(xl_end_of_recovery));
        buf = pgmoneta_format_and_append(buf, "tli %u; prev tli %u; time %s",
                         xlrec.ThisTimeLineID, xlrec.PrevTimeLineID,
                         timestamptz_to_str(xlrec.end_time));
    }
    else if (info == XLOG_OVERWRITE_CONTRECORD)
    {
        xl_overwrite_contrecord xlrec;

        memcpy(&xlrec, rec, sizeof(xl_overwrite_contrecord));
        buf = pgmoneta_format_and_append(buf, "lsn %X/%X; time %s",
                         LSN_FORMAT_ARGS(xlrec.overwritten_lsn),
                         timestamptz_to_str(xlrec.overwrite_time));
    }
    return buf;
}


pg_time_t
timestamptz_to_time_t(TimestampTz t)
{
    pg_time_t	result;

    result = (pg_time_t) (t / USECS_PER_SEC +
                          ((POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * SECS_PER_DAY));
    return result;
}

const char *
timestamptz_to_str(TimestampTz dt)
{
    static char buf[MAXDATELEN + 1];
    char		ts[MAXDATELEN + 1];
    char		zone[MAXDATELEN + 1];
    time_t		result = (time_t) timestamptz_to_time_t(dt);
    struct tm  *ltime = localtime(&result);

    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", ltime);
    strftime(zone, sizeof(zone), "%Z", ltime);

    snprintf(buf, sizeof(buf), "%s.%06d %s",
             ts, (int) (dt % USECS_PER_SEC), zone);

    return buf;
}