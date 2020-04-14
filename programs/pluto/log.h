/* logging declarations
 *
 * Copyright (C) 1998-2001  D. Hugh Redelmeier.
 * Copyright (C) 2004 Michael Richardson <mcr@xelerance.com>
 * Copyright (C) 2012-2013 Paul Wouters <paul@libreswan.org>
 * Copyright (C) 2013 D. Hugh Redelmeier <hugh@mimosa.com>
 * Copyright (C) 2019 Andrew Cagney <cagney@gnu.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <https://www.gnu.org/licenses/gpl2.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#ifndef _PLUTO_LOG_H
#define _PLUTO_LOG_H

#include <libreswan.h>

#include "lswcdefs.h"
#include "lswlog.h"
#include "fd.h"
#include "ip_endpoint.h"

struct state;
struct connection;
struct msg_digest;
struct pending;

/* moved common code to library file */
#include "libreswan/passert.h"

extern bool
	log_with_timestamp,     /* prefix timestamp */
	log_append,
	log_to_audit;

extern bool log_to_syslog;          /* should log go to syslog? */
extern char *pluto_log_file;
extern char *pluto_stats_binary;

/* Context for logging.
 *
 * Global variables: must be carefully adjusted at transaction boundaries!
 * All are to be left in RESET condition and will be checked.
 * There are several pairs of routines to set and reset them.
 * If the context provides a whack file descriptor, messages
 * should be copied to it -- see whack_log()
 */
extern const struct fd *whack_log_fd;                        /* only set during whack_handle() */

extern bool whack_prompt_for(struct state *st, const char *prompt,
			     bool echo, char *ansbuf, size_t ansbuf_len);

/* for pushing state to other subsystems */
#define binlog_refresh_state(st) binlog_state((st), (st)->st_state->kind)
#define binlog_fake_state(st, new_state) binlog_state((st), (new_state))
extern void binlog_state(struct state *st, enum state_kind state);

extern void set_debugging(lset_t deb);

extern void log_reset_globals(where_t where);
#define reset_globals() log_reset_globals(HERE)

extern void log_pexpect_reset_globals(where_t where);
#define pexpect_reset_globals() log_pexpect_reset_globals(HERE)

struct connection *log_push_connection(struct connection *c, where_t where);
void log_pop_connection(struct connection *c, where_t where);

#define push_cur_connection(C) log_push_connection(C, HERE)
#define pop_cur_connection(C) log_pop_connection(C, HERE)

so_serial_t log_push_state(struct state *st, where_t where);
void log_pop_state(so_serial_t serialno, where_t where);

#define push_cur_state(ST) log_push_state(ST, HERE)
#define pop_cur_state(ST) log_pop_state(ST, HERE)

#define set_cur_connection(C) push_cur_connection(C)
#define reset_cur_connection() pop_cur_connection(NULL)
bool is_cur_connection(const struct connection *c);
#define set_cur_state(ST) push_cur_state(ST)
#define reset_cur_state() pop_cur_state(SOS_NOBODY)

extern ip_address log_push_from(ip_address new_from, where_t where);
extern void log_pop_from(ip_address old_from, where_t where);

#define push_cur_from(NEW) log_push_from(NEW, HERE)
#define pop_cur_from(OLD) log_pop_from(OLD, HERE)

struct logger cur_logger(void);

/*
 * Broadcast a log message.
 *
 * By default send it to the log file and any attached whacks (both
 * globally and the object).
 *
 * If any *_STREAM flag is specified then only send the message to
 * that stream.
 *
 * log_message() is a catch-all for code that may or may not have ST.
 * For instance a responder decoding a message may not yet have
 * created the state.  It will will use ST, MD, or nothing as the
 * prefix, and logs to ST's whackfd when possible.
 */

typedef void jam_prefix_fn(jambuf_t *buf, const void *object);

jam_prefix_fn jam_global_prefix;
jam_prefix_fn jam_from_prefix;
jam_prefix_fn jam_message_prefix;
jam_prefix_fn jam_connection_prefix;
jam_prefix_fn jam_state_prefix;
jam_prefix_fn jam_string_prefix;

typedef bool suppress_log_fn(const void *object);

suppress_log_fn suppress_connection_log;
suppress_log_fn suppress_state_log;
suppress_log_fn always_suppress_log;
suppress_log_fn never_suppress_log;

struct logger {
	const struct fd *global_whackfd;
	const struct fd *object_whackfd;
	const void *object;
	jam_prefix_fn *jam_prefix;
	where_t where;
	/* used by timing to nest its logging output */
	int timing_level;
	/*
	 * When opportunistic encryption or the initial responder, for
	 * instance, some logging is suppressed.
	 */
	suppress_log_fn *suppress_log;
};

#define GLOBAL_LOGGER(WHACKFD) (struct logger)			\
	{							\
		.where = HERE,					\
		.global_whackfd = WHACKFD,			\
		.jam_prefix = jam_global_prefix,		\
		.suppress_log = never_suppress_log,		\
	}
#define FROM_LOGGER(FROM) (struct logger)			\
	{							\
		.where = HERE,					\
		.global_whackfd = null_fd,			\
		.jam_prefix = jam_from_prefix,			\
		.object = FROM,					\
		.suppress_log = always_suppress_log,		\
	}
#define MESSAGE_LOGGER(MD) (struct logger)			\
	{							\
		.where = HERE,					\
		.global_whackfd = null_fd,			\
		.jam_prefix = jam_message_prefix,		\
		.object = MD,					\
		.suppress_log = always_suppress_log,		\
	}
#define CONNECTION_LOGGER(CONNECTION, WHACKFD) (struct logger)	\
	{							\
		.where = HERE,					\
		.global_whackfd = WHACKFD,			\
		.jam_prefix = jam_connection_prefix,		\
		.object = CONNECTION,				\
		.suppress_log = suppress_connection_log,	\
	}
#define PENDING_LOGGER(PENDING) (struct logger)			\
	{							\
		.where = HERE,					\
		.global_whackfd = whack_log_fd,			\
		.jam_prefix = jam_connection_prefix,		\
		.object = (PENDING)->connection,		\
		.object_whackfd = (PENDING)->whack_sock,	\
		.suppress_log = suppress_connection_log,	\
	}
#define STATE_LOGGER(STATE) (struct logger)			\
	{							\
		.where = HERE,					\
		.global_whackfd = whack_log_fd,			\
		.jam_prefix = jam_state_prefix,			\
		.object = STATE,				\
		.object_whackfd = (STATE)->st_whack_sock,	\
		.timing_level = (STATE)->st_timing.level,	\
		.suppress_log = suppress_state_log,		\
	}

struct logger *clone_logger(const struct logger *stack);
void free_logger(struct logger **logp);

void log_message(lset_t rc_flags,
		 const struct logger *log,
		 const char *format, ...) PRINTF_LIKE(3);

void jambuf_to_log(jambuf_t *buf, const struct logger *logger, lset_t rc_flags);

#define LOG_MESSAGE(RC_FLAGS, LOGGER, BUF)				\
	LSWLOG_(true, BUF,						\
		(LOGGER)->jam_prefix(BUF, (LOGGER)->object),		\
		jambuf_to_log(BUF, (LOGGER), RC_FLAGS))

void log_pending(lset_t rc_flags,
		 const struct pending *pending,
		 const char *format, ...) PRINTF_LIKE(3);

void log_state(lset_t rc_flags,
	       const struct state *st,
	       const char *format, ...)	PRINTF_LIKE(3);

/*
 * Log with no context.
 *
 * plog_global() pluto-log only; loglog_global() pluto and whack (if
 * attached).
 */

#define plog_global(MESSAGE, ...)					\
	{								\
		struct logger log_ = GLOBAL_LOGGER(null_fd);		\
		log_message(LOG_STREAM, &log_,				\
			    MESSAGE,##__VA_ARGS__);			\
	}

#define loglog_global(RC, WHACKFD, MESSAGE, ...)			\
	{								\
		struct logger log_ = GLOBAL_LOGGER(WHACKFD);		\
		log_message(RC,	&log_,					\
			    MESSAGE,##__VA_ARGS__);			\
	}

/*
 * XXX: log_md() should never be called directly - *log_md() is only
 * useful when in the packet (MD) event handler.  Since this means it
 * isn't in the whack event handler there can't be a whack calling
 * log_md(RC) is useless.
 */

void log_md(lset_t rc_flags,
	    const struct msg_digest *md,
	    const char *format, ...) PRINTF_LIKE(3);
#define plog_md(MD, MESSAGE, ...) log_md(LOG_STREAM, MD, MESSAGE,##__VA_ARGS__)
#define dbg_md(MD, MESSAGE, ...)					\
	{								\
		if (DBGP(DBG_BASE)) {					\
			log_md(DEBUG_STREAM, MD, MESSAGE,##__VA_ARGS__); \
		}							\
	}

/*
 * Log with a connection context.
 *
 * Unlike state and pending, connections do not have an attached
 * WHACKFD.  Instead connection operations only log to whack when
 * being called by the whack event handler (where WHACKFD is passed
 * down).  If whack needs to remain attached after the whack event
 * handler returns then the WHACKFD parameter is duped into to either
 * a state or pending struct.
 */

void log_connection(lset_t rc_flags, struct fd *whackfd,
		    const struct connection *c,
		    const char *format, ...) PRINTF_LIKE(4);

#define plog_connection(C, MESSAGE, ...)				\
	log_connection(LOG_STREAM, null_fd, C, MESSAGE,##__VA_ARGS__)

/*
 * Wrappers.
 *
 * XXX: do these help or hinder - would calling log_state() directly
 * be better (if slightly more text)?  For the moment stick with the
 * wrappers so changing the underlying implementation is easier.
 *
 * XXX: what about dbg_state() et.al.?  Since these always add a
 * prefix the debate is open.  However, when cur_state is deleted
 * (sure ...), the debug-prefix macro will break.
 *
 * XXX: what about whack_log()?  That only sends messages to the
 * global whack (and never the objects whack).  Likely easier to stick
 * with whack_log() and manually add the prefix as needed.
 */

#define plog_state(ST, MESSAGE, ...) log_state(LOG_STREAM, ST, MESSAGE,##__VA_ARGS__);

/*
 * rate limited logging
 */
void rate_log(const struct msg_digest *md,
	      const char *message, ...) PRINTF_LIKE(2);

/*
 * Log 'cur' directly (without setting it first).
 */

void jam_log_prefix(struct lswlog *buf,
		    const struct state *st,
		    const struct connection *c,
		    const ip_address *from);

extern void pluto_init_log(void);
void init_rate_log(void);
extern void close_log(void);
extern void exit_log(const char *message, ...) PRINTF_LIKE(1) NEVER_RETURNS;


/*
 * Whack only logging.
 *
 * None of these functions add a contex prefix (such as connection
 * name).  If that's really really needed then use
 * log_*(WHACK_STREAM,...) above.
 *
 * whack_print() output completely suppresses the 'NNN ' prefix.  It
 * also requires a valid whackfd.  It should only be used by raw-print
 * commands, namely 'show global-stats'.
 *
 * whack_comment() output includes the '000 ' prefix (RC_COMMENT).  It
 * also requires a valid whackfd.  It should only be used by show
 * commands.
 */

void whack_log(enum rc_type rc, const struct fd *whackfd, const char *message, ...) PRINTF_LIKE(3);
void whack_print(const struct fd *whackfd, const char *message, ...) PRINTF_LIKE(2);
void whack_comment(const struct fd *whackfd, const char *message, ...) PRINTF_LIKE(2);
void jambuf_to_whack(jambuf_t *buf, const struct fd *whackfd, enum rc_type rc);

#define WHACK_LOG(RC, WHACKFD, BUF)					\
	LSWLOG_(true, BUF,						\
		/*NO-PREFIX*/,						\
		jambuf_to_whack(BUF, WHACKFD, RC))

extern void show_status(const struct fd *whackfd);
extern void show_setup_plutomain(const struct fd *whackfd);
extern void show_setup_natt(const struct fd *whackfd);
extern void show_global_status(const struct fd *whackfd);

enum linux_audit_kind {
	LAK_PARENT_START,
	LAK_CHILD_START,
	LAK_PARENT_DESTROY,
	LAK_CHILD_DESTROY,
	LAK_PARENT_FAIL,
	LAK_CHILD_FAIL
};
extern void linux_audit_conn(const struct state *st, enum linux_audit_kind);

#ifdef USE_LINUX_AUDIT
extern void linux_audit_init(int do_audit);
# include <libaudit.h>	/* from audit-libs devel */
# define AUDIT_LOG_SIZE 256
/* should really be in libaudit.h */
# define AUDIT_RESULT_FAIL 0
# define AUDIT_RESULT_OK 1
# ifndef AUDIT_CRYPTO_IKE_SA
#  define AUDIT_CRYPTO_IKE_SA 2408
# endif
# ifndef AUDIT_CRYPTO_IPSEC_SA
#  define AUDIT_CRYPTO_IPSEC_SA 2409
# endif
#endif

#endif /* _PLUTO_LOG_H */
