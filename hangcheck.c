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

#ifndef VERBOSE
#define VERBOSE 0
#endif

#define Errnof(format, ...) \
	errnof(__FILE__, __LINE__, format __VA_OPT__(,) __VA_ARGS__)

/* prototypes */
static void do_parent(unsigned int *retries_remaining, pid_t childpid);

int process_looks_hung(pid_t pid);

int exit_reason(pid_t child_pid, int wait_status, int *exit_code, char *buf,
		size_t buf_len);

void errnof(const char *file, int line, const char *format, ...);

int appendf(char *buf, size_t bufsize, const char *format, ...);

/* the POSIX definition of "signal()" does not allow for a context parameter:
	#include <signal.h>

       typedef void (*sighandler_t)(int);

       sighandler_t signal(int signum, sighandler_t handler);

  thus, we will need a global variable to pass data between functions. */
/* globals */

static int child_trap_exit_code_singleton;

/* functions */
int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "no child command?\n");
		exit(EXIT_FAILURE);
	}

	unsigned int retries_remaining = MAX_RETRIES;
	while (retries_remaining > 0) {
		child_trap_exit_code_singleton = INT_MIN;
		pid_t childpid = fork();

		if (childpid < 0) {
			Errnof("fork() failed?");
			exit(EXIT_FAILURE);
		} else if (childpid > 0) {
			do_parent(&retries_remaining, childpid);
		} else {
			// shift-off this program and exec the rest:
			char **rest = argv + 1;
			int rv = execv(rest[0], rest);
			exit(rv);
		}
	}

	fflush(stdout);
	fprintf(stderr, "Retries limit reached.\n");
	return EXIT_FAILURE;
}

int process_looks_hung(pid_t pid)
{
	return 0;
}

void child_trap(int sig)
{
	if (VERBOSE) {
		printf("child_trap(%d)\n", sig);
	}
	int wstatus = 0;
	int options = 0;
	pid_t any_child = -1;
	pid_t pid = waitpid(any_child, &wstatus, options);

	size_t len = 255;
	char buf[len];
	int *exit_code_ptr = &child_trap_exit_code_singleton;
	int exited = exit_reason(pid, wstatus, exit_code_ptr, buf, len);

	if (VERBOSE) {
		printf("%s\n", buf);
	}

	if (exited) {
		if (child_trap_exit_code_singleton == 0) {
			printf("Child completed successfully\n");
			exit(EXIT_SUCCESS);
		} else {
			printf("Child exited with status %d\n",
			       child_trap_exit_code_singleton);
		}
	}
}

static void do_parent(unsigned int *retries_remaining, pid_t childpid)
{
	signal(SIGCHLD, &child_trap);
	int hang_count = 0;
	for (;;) {
		if (child_trap_exit_code_singleton != INT_MIN) {
			--(*retries_remaining);
			break;
		}
		unsigned int seconds = HANG_CHECK_INTERVAL;
		unsigned int remain = sleep(seconds);
		(void)remain;	// ignore
		if (process_looks_hung(childpid)) {
			++hang_count;
			int sig = hang_count > MAX_HANGS ? SIGTERM : SIGKILL;
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

int exit_reason(pid_t child_pid, int wait_status, int *exit_code, char *buf,
		size_t bufsize)
{
	memset(buf, 0x00, bufsize);
	appendf(buf, bufsize, "child pid %d", child_pid);

	int exited = WIFEXITED(wait_status);
	if (exited) {
		appendf(buf, bufsize, " terminated normally");
		*exit_code = WEXITSTATUS(wait_status);
		appendf(buf, bufsize, " exit code: %d", *exit_code);
	}

	int signaled = WIFSIGNALED(wait_status);
	if (signaled) {
		appendf(buf, bufsize, " terminated by a signal");
		int termsig = WTERMSIG(wait_status);
		if (termsig) {
			appendf(buf, bufsize, " %d", termsig);
		}
#ifdef WCOREDUMP
		if (WCOREDUMP(wait_status)) {
			appendf(buf, bufsize, " produced a core dump");
		}
#endif
	}

	if (WIFSTOPPED(wait_status)) {
		appendf(buf, bufsize, " stopped (WUNTRACED? ptrace?)");
		int stopsig = WSTOPSIG(wait_status);
		if (stopsig) {
			appendf(buf, bufsize, " stop signal: %d", stopsig);
		}
	}

	if (WIFCONTINUED(wait_status)) {
		appendf(buf, bufsize, " was resumed by SIGCONT");
	}

	return exited;
}
