/* SPDX-License-Identifier: GPL-3.0-or-later */
/* yoyo: a program to launch and re-launch another program if it hangs */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> and
        Brett Neumeier <brett@freesa.org> */

#include "yoyo.h"

/* freestanding headers */
#include <float.h>
#include <limits.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* hosted headers */
#include <errno.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>		/* strerror */
#include <sys/types.h>		/* pid_t */
#include <sys/wait.h>		/* waitpid */
#include <unistd.h>		/* execv, fork */

const char *yoyo_version = "0.99.2";

const int default_hang_check_interval = 60;
const int default_max_hangs = 5;
const int default_max_retries = 5;

/*************************************************************************/
/* globals */
/*************************************************************************/

/* The POSIX definition of "signal()" does not allow for a context parameter:

	#include <signal.h>

	typedef void (*sighandler_t)(int);

	sighandler_t signal(int signum, sighandler_t handler);

  Thus, we will need some global space to pass data between functions.
*/
struct exit_reason global_exit_reason;

/* global verbose, can be set via commandline options, or directly in tests */
int yoyo_verbose = 0;

/* global pointers to stdout, stderr if tests wish to capture these */
FILE *yoyo_stdout = NULL;
#define Ystdout (yoyo_stdout ? yoyo_stdout : stdout)
FILE *yoyo_stderr = NULL;
#define Ystderr (yoyo_stderr ? yoyo_stderr : stderr)

#define Y_USE_PREFIX 1
#define Ylog(log_level, format, ...) \
	yoyo_log(log_level, Y_USE_PREFIX, __FILE__, __LINE__, __func__, \
		format __VA_OPT__(,) __VA_ARGS__);

#define Y_SKIP_PREFIX 0
#define Ylog_append(log_level, format, ...) \
	yoyo_log(log_level, Y_SKIP_PREFIX, __FILE__, __LINE__, __func__, \
		format __VA_OPT__(,) __VA_ARGS__);

/* global pointers to calloc(), free() provided for testing OOM and such */
void *(*yoyo_calloc)(size_t nmemb, size_t size) = calloc;
void (*yoyo_free)(void *ptr) = free;

/* global pointers to fork, execv, kill, sleep provided for testing */
pid_t (*yoyo_fork)(void) = fork;
int (*yoyo_execv)(const char *pathname, char *const argv[]) = execv;
int (*yoyo_kill)(pid_t pid, int sig) = kill;
unsigned int (*yoyo_sleep)(unsigned int seconds) = sleep;

/* TODO: replace calls to "signal" with "sigaction" */
/* global pointers to signal, waitpid provided for tests */
typedef void (*sighandler_func)(int);
sighandler_func (*yoyo_signal)(int signum, sighandler_func handler) = signal;
pid_t (*yoyo_waitpid)(pid_t pid, int *wstatus, int options) = waitpid;

/* global pointers to internal functions */
struct state_list *(*get_states) (long pid) = get_states_proc;
void (*free_states)(struct state_list *l) = state_list_free;
void (*monitor_for_hang)(long child_pid, unsigned max_hangs,
			 unsigned hang_check_interval) = monitor_child_for_hang;

/*************************************************************************/
/* functions */
int print_help(FILE *out);

int yoyo_env_default(int default_val, const char *env_var_name)
{
	char *ev = getenv(env_var_name);
	return ev ? atoi(ev) : default_val;
}

int yoyo(int argc, char **argv)
{
	if (argc < 2) {
		print_help(Ystderr);
		return EXIT_FAILURE;
	} else if (strcmp(argv[1], "--help") == 0) {
		print_help(Ystdout);
		return EXIT_SUCCESS;
	} else if (strcmp(argv[1], "--version") == 0) {
		fprintf(Ystdout, "yoyo %s\n", yoyo_version);
		return EXIT_SUCCESS;
	}
	int max_retries = yoyo_env_default(default_max_retries,
					   "YOYO_MAX_RETRIES");
	int max_hangs = yoyo_env_default(default_max_retries,
					 "YOYO_MAX_HANGS");
	int hang_check_interval = yoyo_env_default(default_hang_check_interval,
						   "YOYO_HANG_CHECK_INTERVAL");

	char **child_command_line = argv + 1;
	int child_command_line_len = argc - 1;

	size_t buflen = 80;
	char buf[buflen];
	const size_t summary_len = (max_retries + 2) * (buflen + 1);
	char summary[summary_len];
	memset(summary, 0x00, summary_len);
	strcat(summary, "yoyo result summary:\n");

	// setup globals
	yoyo_verbose = yoyo_env_default(yoyo_verbose, "YOYO_VERBOSE");
	Ylog(1, "yoyo_verbose: %d\n", yoyo_verbose);

	// setup global for sharing data with signal handler
	exit_reason_clear(&global_exit_reason);

	// now that globals are setup, set the handler
	yoyo_signal(SIGCHLD, &exit_reason_child_trap);

	int max_tries = max_retries + 1;
	for (int i = 0; i < max_tries; ++i) {
		// reset our exit reason prior to each fork
		exit_reason_clear(&global_exit_reason);

		errno = 0;
		global_exit_reason.child_pid = yoyo_fork();

		if (global_exit_reason.child_pid < 0) {
			Ylog(0, "fork() failed?\n");
			return EXIT_FAILURE;
		} else if (global_exit_reason.child_pid == 0) {
			// in child process
			Ylog(1, "%s", child_command_line[0]);
			for (int i = 1; i < child_command_line_len; ++i) {
				Ylog_append(1, " %s", child_command_line[i]);
			}
			Ylog_append(1, "\n");
			return yoyo_execv(child_command_line[0],
					  child_command_line);
		}

		Ylog(1, "'%s' child_pid: %ld\n", child_command_line[0],
		     (long)global_exit_reason.child_pid);

		monitor_for_hang(global_exit_reason.child_pid, max_hangs,
				 hang_check_interval);

		if (global_exit_reason.exit_code != 0) {
			snprintf(buf, buflen,
				 "Child '%s' exited with status %d\n",
				 child_command_line[0],
				 global_exit_reason.exit_code);
			Ylog(0, "%s", buf);
			strcat(summary, buf);
		} else if (global_exit_reason.exited) {
			snprintf(buf, buflen,
				 "Child '%s' completed successfully\n",
				 child_command_line[0]);
			Ylog(0, "%s", buf);
			strcat(summary, buf);
			Ylog_append(0, "%s", summary);
			return EXIT_SUCCESS;
		} else {
			char er_buf[250];
			exit_reason_to_str(&global_exit_reason, er_buf, 250);
			Ylog(0, "exit reason: %s\n", er_buf);

			snprintf(buf, buflen, "Child '%s' killed\n",
				 child_command_line[0]);
			Ylog(0, "%s", buf);
			strcat(summary, buf);
		}
	}
	Ylog(0, "'%s' failed.\n", child_command_line[0]);
	Ylog_append(0, "%s", summary);
	Ylog_append(0, "Retries limit reached.\n");
	return EXIT_FAILURE;
}

// "next" is an OUT parameter
int process_looks_hung(struct state_list **next, struct state_list *previous,
		       struct state_list *current)
{
	for (size_t i = 0; i < current->len; ++i) {
		if (current->states[i].state != 'S') {
			*next = NULL;
			return 0;
		}
	}

	if (!previous || previous->len != current->len) {
		*next = current;
		return 0;
	}

	for (size_t i = 0; i < current->len; ++i) {
		struct thread_state old_state = previous->states[i];
		struct thread_state new_state = current->states[i];

		if ((old_state.pid != new_state.pid)
		    || (new_state.utime > (old_state.utime + 5))
		    || (new_state.stime > (old_state.stime + 5))
		    ) {
			*next = NULL;
			return 0;
		}
	}

	*next = current;
	return 1;
}

static char *pid_to_stat_pattern(char *buf, long pid)
{
	strcpy(buf, "/proc/");
	sprintf(buf + strlen(buf), "%ld", pid);
	strcat(buf, "/task/*/stat");
	return buf;
}

char *slurp_text(char *buf, size_t buflen, const char *path)
{
	if (!buf || !buflen) {
		return NULL;
	}
	memset(buf, 0x00, buflen);

	if (!path) {
		return NULL;
	}

	FILE *f = fopen(path, "r");
	if (!f) {
		return NULL;
	}

	errno = 0;
	size_t n = fread(buf, 1, buflen - 1, f);
	fclose(f);

	Ylog(2, "slurp_text: '%s'\n", buf);

	return n ? buf : NULL;
}

int thread_state_from_path(struct thread_state *ts, const char *path)
{
	unsigned fields = 60;
	const size_t buf_len = FILENAME_MAX + (fields * (sizeof(long) + 1));
	char buf[buf_len];
	errno = 0;
	char *rbuf = slurp_text(buf, buf_len, path);
	int log_level = (!rbuf && errno != ENOENT) ? 0 : 2;
	Ylog(log_level, "slurp_text returned %p\n", rbuf);

	const char *stat_scanf =
	    "%d %*s %c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %lu %lu";
	errno = 0;
	int matched = sscanf(buf, stat_scanf, &ts->pid, &ts->state, &ts->utime,
			     &ts->stime);

	log_level = (matched != 4) ? 0 : 2;
	Ylog(log_level, "scanf matched %d of 4 fields for %s\n", matched, path);

	return (4 - matched);
}

struct state_list *state_list_new(size_t length)
{
	size_t size = sizeof(struct state_list);
	struct state_list *sl = yoyo_calloc(1, size);
	if (!sl) {
		Ylog(0, "could not alloc (%zu bytes)?\n", size);
		return NULL;
	}

	sl->len = length;
	size = sizeof(struct thread_state);
	sl->states = yoyo_calloc(sl->len, size);
	if (!sl->states) {
		size_t total = sl->len * size;
		Ylog(0, "could not alloc(%zu, %zu) (%zu bytes)?\n", sl->len,
		     size, total);
		yoyo_free(sl);
		return NULL;
	}
	return sl;
}

void state_list_free(struct state_list *l)
{
	if (l) {
		yoyo_free(l->states);
		yoyo_free(l);
	}
}

/* errno ENOENT: No such file or directory */
int ignore_no_such_file(const char *epath, int eerrno)
{
	int log_level = (errno != ENOENT) ? 0 : 2;
	Ylog(log_level, "%s (%d)\n", epath, eerrno);

	return 0;
}

struct state_list *get_states_proc(long pid)
{
	errno = 0;

	char pattern[FILENAME_MAX + 3];
	pid_to_stat_pattern(pattern, pid);

	Ylog(1, "pattern == '%s'\n", pattern);

	glob_t threads;
	int (*errfunc)(const char *epath, int eerrno) = ignore_no_such_file;
	errno = 0;
	glob(pattern, GLOB_MARK, errfunc, &threads);

	size_t len = threads.gl_pathc;
	Ylog(1, "matches for %ld: %zu\n", pid, len);
	struct state_list *sl = state_list_new(len);
	/* by inspection, does the right thing (exits on malloc failure),
	 * no intention test --eric.herman 2020-12-31 */
	/* LCOV_EXCL_START */
	if (!sl) {
		exit(EXIT_FAILURE);
	}
	/* LCOV_EXCL_STOP */

	int err = 0;
	for (size_t i = 0; i < len; ++i) {
		const char *path = threads.gl_pathv[i];
		Ylog(1, "\t%s\n", path);
		struct thread_state *ts = &sl->states[i];
		err += thread_state_from_path(ts, path);
	}

	int log_level = err ? 0 : 1;
	Ylog(log_level, "get_states for pid: %ld errors: %d\n", (long)pid, err);

	globfree(&threads);

	return sl;
}

void exit_reason_child_trap(int sig)
{
	Ylog(1, "exit_reason_child_trap(%d)\n", sig);

	pid_t any_child = -1;
	int wait_status = 0;
	int options = 0;

	pid_t pid = yoyo_waitpid(any_child, &wait_status, options);

	if (pid == global_exit_reason.child_pid) {
		exit_reason_set(&global_exit_reason, pid, wait_status);
	}

	if (yoyo_verbose >= 1) {
		struct exit_reason tmp;
		exit_reason_set(&tmp, pid, wait_status);
		size_t len = 255;
		char buf[len];
		exit_reason_to_str(&tmp, buf, len);
		Ylog(1, "exit_reason_child_trap(%d) (%d): %s\n", sig,
		     wait_status, buf);
	}
}

int pid_exists(long pid)
{
	/*
	 * If sig is 0, then no signal is sent, but existence and permission
	 * checks are still performed; this can be used to check for the
	 * existence of a process ID or process group ID that the caller is
	 * permitted to signal.
	 */
	const int sig_exists = 0;

	/*
	 * On success, zero is returned. On error, -1 is returned, and errno
	 * is set appropriately.
	 */
	int rv = yoyo_kill(pid, sig_exists);

	return (rv == 0);
}

void monitor_child_for_hang(long child_pid, unsigned max_hangs,
			    unsigned hang_check_interval)
{
	unsigned int hang_count = 0;
	struct state_list *thread_states = NULL;
	while (pid_exists(child_pid)) {
		unsigned int seconds = hang_check_interval;
		unsigned int seconds_remaining = yoyo_sleep(seconds);
		if (seconds_remaining) {
			Ylog(1, "Interrupted with %u seconds remaining.\n",
			     seconds_remaining);
		}
		struct state_list *previous = thread_states;
		struct state_list *current = get_states(child_pid);
		if (process_looks_hung(&thread_states, previous, current)) {
			++hang_count;
			if (hang_count > max_hangs) {
				int need_dash_9 = hang_count > (max_hangs + 1);
				int sig = need_dash_9 ? SIGKILL : SIGTERM;
				errno = 0;
				int err = yoyo_kill(child_pid, sig);
				/* ESRCH: No such process */
				int log_level = (err && (errno != ESRCH))
				    ? 0 : 1;
				const char *sigstr = (sig == SIGKILL)
				    ? "SIGKILL" : "SIGTERM";

				Ylog(log_level,
				     "kill(child_pid, %s) returned %d\n",
				     sigstr, err);

				yoyo_sleep(0);	// yield after kill
			}
		} else {
			hang_count = 0;
			Ylog(1, "Child still appears to be"
			     " doing something worthwhile\n");
		}
		free_states(previous);
		if (thread_states != current) {
			free_states(current);
		}
	}
	free_states(thread_states);
}

int appendf(char *buf, size_t bufsize, const char *format, ...)
{
	size_t used = strlen(buf);
	size_t max = bufsize - used;
	char *start = buf + used;

	va_list args;

	va_start(args, format);
	int printed = vsnprintf(start, max, format, args);
	va_end(args);

	buf[bufsize - 1] = '\0';
	return printed;
}

void yoyo_log(int loglevel, int prefix, const char *file, int line,
	      const char *func, const char *format, ...)
{
	int _errorf_save = errno;
	errno = 0;
	if (yoyo_verbose < loglevel) {
		return;
	}

	fflush(Ystdout);

	if (prefix) {
		fprintf(Ystderr, "%s:%d %s(): ", file, line, func);
	}

	if (_errorf_save) {
		fprintf(Ystderr, "errno %d (%s): ", _errorf_save,
			strerror(_errorf_save));
	}

	va_list args;

	va_start(args, format);
	vfprintf(Ystderr, format, args);
	va_end(args);
}

void exit_reason_clear(struct exit_reason *reason)
{
	memset(reason, 0x00, sizeof(struct exit_reason));
}

void exit_reason_set(struct exit_reason *reason, long pid, int wait_status)
{
	exit_reason_clear(reason);

	reason->child_pid = pid;
	reason->wait_status = wait_status;

	reason->exited = WIFEXITED(reason->wait_status);
	if (reason->exited) {
		reason->exit_code = WEXITSTATUS(reason->wait_status);
	}

	reason->signaled = WIFSIGNALED(reason->wait_status);
	if (reason->signaled) {
		reason->termsig = WTERMSIG(reason->wait_status);
#ifdef WCOREDUMP
		reason->coredump = WCOREDUMP(reason->wait_status);
#endif
	}

	reason->stopped = WIFSTOPPED(reason->wait_status);
	if (reason->stopped) {
		reason->stopsig = WSTOPSIG(reason->wait_status);
	}

	reason->continued = WIFCONTINUED(reason->wait_status);
}

void exit_reason_to_str(struct exit_reason *reason, char *buf, size_t bufsize)
{
	memset(buf, 0x00, bufsize);
	appendf(buf, bufsize, "child pid %d", reason->child_pid);

	if (reason->exited) {
		appendf(buf, bufsize, " terminated normally");
		appendf(buf, bufsize, " exit code: %d", reason->exit_code);
	}

	if (reason->signaled) {
		appendf(buf, bufsize, " terminated by a signal");
		if (reason->termsig) {
			appendf(buf, bufsize, " %d", reason->termsig);
		}
		/* we really do not need to test that coredump is in
		 * the string  --eric.herman 2020-12-31 */
		/* LCOV_EXCL_START */
		if (reason->coredump) {
			appendf(buf, bufsize, " produced a core dump");
		}
		/* LCOV_EXCL_STOP */
	}

	if (reason->stopped) {
		appendf(buf, bufsize, " stopped (WUNTRACED? ptrace?)");
		if (reason->stopsig) {
			appendf(buf, bufsize, " stop signal: %d",
				reason->stopsig);
		}
	}

	if (reason->continued) {
		appendf(buf, bufsize, " was resumed by SIGCONT");
	}
}

int print_help(FILE *out)
{
	fprintf(out, "yoyo runs a program and monitors /proc. ");
	fprintf(out, "If the process looks hung, based\n");
	fprintf(out, "on activity observed in /proc, yoyo ");
	fprintf(out, "will kill and restart it. If the\n");
	fprintf(out, "program terminates with an error status, ");
	fprintf(out, "yoyo will run it again.\n");
	fprintf(out, "\n");
	fprintf(out, "Usage: yoyo program program-args...\n");
	fprintf(out, "or\n");
	fprintf(out, "  --version                  ");
	fprintf(out, "print version (%s) and exit\n", yoyo_version);
	fprintf(out, "  --help                     ");
	fprintf(out, "print this message and exit\n");
	return 0;
}
