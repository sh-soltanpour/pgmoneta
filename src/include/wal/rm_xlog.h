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


#ifndef PGMONETA_RM_XLOG_H
#define PGMONETA_RM_XLOG_H

#include "wal_reader.h"

#define MAXFNAMELEN		64
#define MAXDATELEN		128
#define INT64CONST(x)  (x##L)
#define USECS_PER_SEC	INT64CONST(1000000)
#define UNIX_EPOCH_JDATE		2440588 /* == date2j(1970, 1, 1) */
#define POSTGRES_EPOCH_JDATE	2451545 /* == date2j(2000, 1, 1) */
#define SECS_PER_DAY	86400


typedef int64_t TimestampTz;

/*
 * Information logged when we detect a change in one of the parameters
 * important for Hot Standby.
 */
struct xl_parameter_change
{
    int			MaxConnections;
    int			max_worker_processes;
    int			max_wal_senders;
    int			max_prepared_xacts;
    int			max_locks_per_xact;
    int			wal_level;
    bool		wal_log_hints;
    bool		track_commit_timestamp;
};

/* logs restore point */
struct xl_restore_point
{
    TimestampTz rp_time;
    char		rp_name[MAXFNAMELEN];
};

/* Overwrite of prior contrecord */
struct xl_overwrite_contrecord
{
    xlog_rec_ptr	overwritten_lsn;
    TimestampTz overwrite_time;
};

/* End of recovery mark, when we don't do an END_OF_RECOVERY checkpoint */
struct xl_end_of_recovery
{
    TimestampTz end_time;
    timeline_id	ThisTimeLineID; /* new TLI */
    timeline_id	PrevTimeLineID; /* previous TLI we forked off from */
};

struct config_enum_entry
{
    const char *name;
    int			val;
    bool		hidden;
};

const char *
timestamptz_to_str(TimestampTz dt);

char*
xlog_desc(char* buf, struct decoded_xlog_record* record);

#endif //PGMONETA_RM_XLOG_H
