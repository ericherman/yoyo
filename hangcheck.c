/* freestanding headers */
#include <float.h>
#include <limits.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* hosted headers */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>		/* strerror */
#include <sys/types.h>		/* pid_t */
#include <sys/wait.h>		/* waitpid */
#include <unistd.h>		/* execv, fork */

#ifndef HANG_CHECK_INTERVAL
#define HANG_CHECK_INTERVAL 5
#endif

#ifndef MAX_HANGS
#define MAX_HANGS 5
#endif

#ifndef MAX_RETRIES
#define MAX_RETRIES 5
#endif

#define Errnof(format, ...) \
	errnof(__FILE__, __LINE__, format __VA_OPT__(,) __VA_ARGS__)

struct exit_reason {
	pid_t child_pid;
	int wait_status;
	int exited;
	int exit_code;
	int signaled;
	int termsig;
	int coredump;
	int stopped;
	int stopsig;
	int continued;
};

/* function prototypes */
/* append to a string buffer, always NULL-terminates */
int appendf(char *buf, size_t bufsize, const char *format, ...);

/* print an error message, include the errno information */
void errnof(const char *file, int line, const char *format, ...);

/* trap child exits, and capture the exit_reason information */
void exit_reason_child_trap(int sig);

/* zero the struct */
void exit_reason_clear(struct exit_reason *exit_reason);

/* populate the struct from the wait_status */
void exit_reason_set(struct exit_reason *exit_reason, pid_t pid,
		     int wait_status);

/* populate buf with a human-friendly-ish description of the exit_reason */
void exit_reason_to_str(struct exit_reason *exit_reason, char *buf, size_t len);

/* monitor a child process; signal the process if it looks hung */
static void monitor_child_for_hang(pid_t childpid, struct exit_reason *reason,
				   unsigned max_hangs,
				   unsigned hang_check_interval);

/* look for evidence of a hung process, not yet implemented */
int process_looks_hung(pid_t pid);

/* globals */
/* The POSIX definition of "signal()" does not allow for a context parameter:

	#include <signal.h>

	typedef void (*sighandler_t)(int);

	sighandler_t signal(int signum, sighandler_t handler);

  Thus, we will need some global space to pass data between functions.
*/
static struct exit_reason *child_trap_exit_code_singleton;
static int global_verbose;

/* functions */
int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "no child command?\n");
		exit(EXIT_FAILURE);
	}

	// setup globals for sharing data with signal handler
	char *verbosenv = getenv("VERBOSE");
	global_verbose = verbosenv ? atoi(verbosenv) : 0;

	struct exit_reason reason;
	child_trap_exit_code_singleton = &reason;

	// now that globals are setup, set the handler
	signal(SIGCHLD, &exit_reason_child_trap);

	for (int retries_remaining = MAX_RETRIES;
	     retries_remaining > 0; --retries_remaining) {

		// reset our exit reason prior to each fork
		exit_reason_clear(&reason);

		pid_t childpid = fork();

		if (childpid < 0) {
			Errnof("fork() failed?");
			exit(EXIT_FAILURE);
		} else if (childpid == 0) {
			// in child process
			// shift-off this program and exec the rest
			// of the commandline
			char **rest = argv + 1;
			int rv = execv(rest[0], rest);
			exit(rv);
		}

		monitor_child_for_hang(childpid, &reason, MAX_RETRIES,
				       HANG_CHECK_INTERVAL);

		assert(reason.exited);

		if (reason.exit_code != 0) {
			printf("Child exited with status %d\n",
			       reason.exit_code);
		} else {
			printf("Child completed successfully\n");
			exit(EXIT_SUCCESS);
		}
	}

	fflush(stdout);
	fprintf(stderr, "Retries limit reached.\n");
	return EXIT_FAILURE;
}

// FIXXXME: TODO
int process_looks_hung(pid_t pid)
{
	return 0;
}

void exit_reason_child_trap(int sig)
{
	if (global_verbose) {
		printf("exit_reason_child_trap(%d)\n", sig);
	}

	pid_t any_child = -1;
	int wait_status = 0;
	int options = 0;
	pid_t pid = waitpid(any_child, &wait_status, options);

	exit_reason_set(child_trap_exit_code_singleton, pid, wait_status);

	if (global_verbose) {
		size_t len = 255;
		char buf[len];
		exit_reason_to_str(child_trap_exit_code_singleton, buf, len);
		printf("%s\n", buf);
	}
}

static void monitor_child_for_hang(pid_t childpid, struct exit_reason *reason,
				   unsigned max_hangs,
				   unsigned hang_check_interval)
{
	unsigned int hang_count = 0;
	while (!reason->exited) {
		unsigned int seconds = hang_check_interval;
		unsigned int remain = sleep(seconds);
		(void)remain;	// ignore, maybe interrupted by a handler?

		if (process_looks_hung(childpid)) {
			++hang_count;
			int sig = hang_count > max_hangs ? SIGTERM : SIGKILL;
			int err = kill(childpid, sig);
			if (err) {
				Errnof("kill(childpid, %d) returned %d?", sig,
				       err);
			}
		} else {
			hang_count = 0;
			printf("Child still appears to be"
			       " doing something worthwhile\n");
		}
	}
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

void errnof(const char *file, int line, const char *format, ...)
{
	int _errnof_save = errno;

	fflush(stdout);
	fprintf(stderr, "%s:%d ", file, line);

	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fprintf(stderr, " errno %d: %s\n", _errnof_save,
		strerror(_errnof_save));
}

void exit_reason_clear(struct exit_reason *reason)
{
	memset(reason, 0x00, sizeof(struct exit_reason));
}

void exit_reason_set(struct exit_reason *reason, pid_t pid, int wait_status)
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
		if (reason->coredump) {
			appendf(buf, bufsize, " produced a core dump");
		}
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
