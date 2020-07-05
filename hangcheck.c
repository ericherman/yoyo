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

#ifndef VERBOSE
#define VERBOSE 0
#endif

int exit_reason(pid_t child_pid, int wait_status, int *exit_code, char *buf,
		size_t buf_len);

int process_looks_hung(pid_t pid)
{
	return 0;
}

void child_trap(int sig)
{
	printf("Child died (or stopped but let's ignore that)\n");
	int wstatus = 0;
	int options = 0;
	pid_t any_child = -1;
	pid_t pid = waitpid(any_child, &wstatus, options);
	int exit_code = INT_MIN;

	char buf[255];
	int exited = exit_reason(pid, wstatus, &exit_code, buf, 255);
	if (VERBOSE) {
		printf("%s\n", buf);
	}

	if (exited) {
		printf("Child process %d status: %d\n", pid, exit_code);
		exit(EXIT_SUCCESS);
	}
}

int main(int argc, char **argv)
{
	pid_t childpid = fork();

	if (childpid > 0) {	// WE IS PARENT
		signal(SIGCHLD, &child_trap);
		int hang_count = 0;
		for (;;) {
			unsigned int seconds = HANG_CHECK_INTERVAL;
			unsigned int remain = sleep(seconds);
			if (process_looks_hung(childpid)) {
				++hang_count;
				int sig = hang_count > 3 ? SIGTERM : SIGKILL;
				int err = kill(childpid, sig);
				if (err) {
					int save_errno = errno;
					fflush(stdout);
					fprintf(stderr,
						"kill(%d) failed?"
						" errno %d: %s\n", sig,
						save_errno,
						strerror(save_errno));
				}
			} else {
				hang_count = 0;
				printf("Child still appears to be"
				       " doing something worthwhile\n");
			}
		}
	} else if (childpid == 0) {	// WE IS CHILD
		const char *path = argv[1];
		char **argv2 = argv + 1;
		int rv = execv(path, argv2);
		exit(rv);
	} else {
		int save_errno = errno;
		fflush(stdout);
		fprintf(stderr, "fork failed? errno %d: %s\n", save_errno,
			strerror(save_errno));
		exit(EXIT_FAILURE);
	}

	fflush(stdout);
	fprintf(stderr, "how did we get here?\n");
	return 1;
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
