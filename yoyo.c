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
#include <glob.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>		/* strerror */
#include <sys/types.h>		/* pid_t */
#include <sys/wait.h>		/* waitpid */
#include <unistd.h>		/* execv, fork */

#define YOYO_NAME "yoyo"
#define YOYO_VERSION "0.0.1"

#define Errorf(format, ...) \
	errorf(__FILE__, __LINE__, format __VA_OPT__(,) __VA_ARGS__)

struct yoyo_options {
	int version;
	int help;
	int verbose;
	int hang_check_interval;
	int max_hangs;
	int max_retries;
	const char *fakeroot;
	char **child_command_line;
};

struct thread_state {
	long pid;
	char state;
	unsigned long utime;
	unsigned long stime;
};

struct state_list {
	struct thread_state *states;
	size_t len;
};

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

/* look for evidence of a hung process */
int process_looks_hung(pid_t pid, const char *fakeroot);

/* given a pid, create state_list */
struct state_list *get_states(pid_t pid, const char *fakeroot, int *err);

/* null-safe; frees the states as well as the state_list struct */
void state_list_free(struct state_list *l);

/* append to a string buffer, always NULL-terminates */
int appendf(char *buf, size_t bufsize, const char *format, ...);

/* print an error message, include the errno information if non-zero */
void errorf(const char *file, int line, const char *format, ...);

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
				   unsigned hang_check_interval,
				   const char *fakeroot);

void parse_command_line(struct yoyo_options *options, int argc, char **argv);

int print_help(FILE *stream, const char *name);

/*************************************************************************/
/* globals */
/*************************************************************************/

/* The POSIX definition of "signal()" does not allow for a context parameter:

	#include <signal.h>

	typedef void (*sighandler_t)(int);

	sighandler_t signal(int signum, sighandler_t handler);

  Thus, we will need some global space to pass data between functions.
*/
static struct exit_reason *child_trap_exit_code_singleton;

static int global_verbose;

struct state_list *threadStates = NULL;

/*************************************************************************/
/* functions */
int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "no child command?\n");
		exit(EXIT_FAILURE);
	}

	struct yoyo_options options;
	parse_command_line(&options, argc, argv);

	if (options.version) {
		printf("%s version %s\n", YOYO_NAME, YOYO_VERSION);
		return 0;
	} else if (options.help) {
		return print_help(stdout, argv[0]);
	}
	int max_retries = options.max_retries;
	char **child_command_line = options.child_command_line;
	int max_hangs = options.max_hangs;
	int hang_check_interval = options.hang_check_interval;
	const char *fakeroot = options.fakeroot;

	// setup globals for sharing data with signal handler
	global_verbose = options.verbose;

	struct exit_reason reason;
	child_trap_exit_code_singleton = &reason;

	// now that globals are setup, set the handler
	signal(SIGCHLD, &exit_reason_child_trap);

	int rv = EXIT_FAILURE;
	for (int retries_remaining = max_retries;
	     retries_remaining > 0; --retries_remaining) {

		// reset our exit reason prior to each fork
		exit_reason_clear(&reason);

		errno = 0;
		pid_t childpid = fork();

		if (childpid < 0) {
			Errorf("fork() failed?");
			rv = EXIT_FAILURE;
			goto end_main;
		} else if (childpid == 0) {
			// in child process
			rv = execv(child_command_line[0], child_command_line);
			goto end_main;
		}

		monitor_child_for_hang(childpid, &reason, max_hangs,
				       hang_check_interval, fakeroot);

		assert(reason.exited);

		if (reason.exit_code != 0) {
			printf("Child exited with status %d\n",
			       reason.exit_code);
		} else {
			printf("Child completed successfully\n");
			rv = EXIT_SUCCESS;
			goto end_main;
		}
	}

	fflush(stdout);
	fprintf(stderr, "Retries limit reached.\n");
	rv = EXIT_FAILURE;

end_main:
	state_list_free(threadStates);
	threadStates = NULL;
	return rv;
}

int process_looks_hung(pid_t pid, const char *fakeroot)
{
	errno = 0;
	int err = 0;
	struct state_list *currentThreadStates =
	    get_states(pid, fakeroot, &err);
	if (err) {
		Errorf("get_states for pid:%ld (fakeroot:'%s')"
		       " returned error: %d", (long)pid, fakeroot, err);
	}

	for (size_t i = 0; i < currentThreadStates->len; ++i) {
		if (currentThreadStates->states[i].state != 'S') {
			state_list_free(currentThreadStates);
			state_list_free(threadStates);
			threadStates = NULL;
			return 0;
		}
	}

	if (!threadStates || threadStates->len != currentThreadStates->len) {
		state_list_free(threadStates);
		threadStates = currentThreadStates;
		return 0;
	}

	for (size_t i = 0; i < currentThreadStates->len; ++i) {
		struct thread_state old_state = threadStates->states[i];
		struct thread_state new_state = currentThreadStates->states[i];

		if ((old_state.pid != new_state.pid)
		    || (new_state.utime > (old_state.utime + 1))
		    || (new_state.stime > (old_state.stime + 1))
		    ) {
			state_list_free(currentThreadStates);
			state_list_free(threadStates);
			threadStates = NULL;
			return 0;
		}
	}

	state_list_free(threadStates);
	threadStates = currentThreadStates;
	return 1;
}

static char *pid_to_stat_pattern(char *buf, const char *fakeroot, pid_t pid)
{
	strcpy(buf, fakeroot ? fakeroot : "");
	strcat(buf, "/proc/");
	/* According to POSIX, pid_t is a signed int no wider than long */
	sprintf(buf + strlen(buf), "%ld", (long)pid);
	strcat(buf, "/task/*/stat");
	return buf;
}

static char *slurp_text(char *buf, size_t buflen, const char *path)
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
	if (fseek(f, 0, SEEK_END)) {
		Errorf("seek(f, 0, SEEK_END) failure?");
	}

	errno = 0;
	long length = ftell(f);
	if (length <= 0) {
		Errorf("ftell returned %ld?", length);
		buf = NULL;
	} else {
		unsigned long ulength = (unsigned long)length;
		size_t bytes = (ulength < buflen) ? ulength : buflen - 1;

		errno = 0;
		if (fseek(f, 0, SEEK_SET)) {
			Errorf("seek(f, 0, SEEK_SET) failure?");
		}

		errno = 0;
		size_t n = fread(buf, 1, bytes, f);
		if (n != ulength) {
			Errorf("only read %zu of %lu bytes from %s", n, ulength,
			       path);
			buf = NULL;
		}
	}

	fclose(f);
	return buf;
}

int thread_state_from_path(struct thread_state *ts, const char *path)
{
	unsigned fields = 60;
	const size_t buf_len = FILENAME_MAX + (fields * (sizeof(long) + 1));
	char buf[buf_len];
	errno = 0;
	if (!slurp_text(buf, buf_len, path)) {
		if (global_verbose || errno != ENOENT) {
			Errorf("slurp_text failed?");
		}
	}

	const char *stat_scanf =
	    "%d %*s %c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %lu %lu";
	errno = 0;
	int matched = sscanf(buf, stat_scanf, &ts->pid, &ts->state, &ts->utime,
			     &ts->stime);
	if (matched != 4) {
		Errorf("only matched %d for %s", matched, path);
	}

	return (4 - matched);
}

void state_list_free(struct state_list *l)
{
	if (l) {
		free(l->states);
		free(l);
	}
}

/* errno ENOENT: No such file or directory */
int ignore_no_such_file(const char *epath, int eerrno)
{
	if (global_verbose || errno != ENOENT) {
		Errorf("%s (%d)", epath, eerrno);
	}
	return 0;
}

struct state_list *get_states(pid_t pid, const char *fakeroot, int *err)
{
	size_t size = sizeof(struct state_list);
	struct state_list *sl = calloc(1, size);
	if (!sl) {
		Errorf("could not calloc (%zu bytes)?", size);
		exit(EXIT_FAILURE);
	}

	char pattern[FILENAME_MAX + 3];
	pid_to_stat_pattern(pattern, fakeroot, pid);

	glob_t threads;
	int (*errfunc)(const char *epath, int eerrno) = ignore_no_such_file;
	errno = 0;
	glob(pattern, GLOB_MARK, errfunc, &threads);

	sl->len = threads.gl_pathc;
	size = sizeof(struct thread_state);
	sl->states = calloc(sl->len, size);
	if (!sl->states) {
		size_t total = sl->len * size;
		Errorf("could not calloc(%zu, %zu) (%zu bytes)?", sl->len, size,
		       total);
		exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < threads.gl_pathc; ++i) {
		const char *path = threads.gl_pathv[i];
		struct thread_state *ts = &sl->states[i];
		*err += thread_state_from_path(ts, path);
	}

	globfree(&threads);

	return sl;
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

static void monitor_child_for_hang(pid_t childpid,
				   struct exit_reason *reason,
				   unsigned max_hangs,
				   unsigned hang_check_interval,
				   const char *fakeroot)
{
	unsigned int hang_count = 0;
	while (!reason->exited) {
		unsigned int seconds = hang_check_interval;
		unsigned int remain = sleep(seconds);
		(void)remain;	// ignore, maybe interrupted by a handler?
		if (process_looks_hung(childpid, fakeroot)) {
			++hang_count;
			int sig = hang_count > max_hangs ? SIGTERM : SIGKILL;
			errno = 0;
			int err = kill(childpid, sig);
			/* ESRCH: No such process */
			if (err && (global_verbose || errno != ESRCH)) {
				const char *sigstr =
				    (sig == SIGTERM) ? "SIGTERM" : "SIGKILL";
				Errorf("kill(childpid, %d) returned %d", sigstr,
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

void errorf(const char *file, int line, const char *format, ...)
{
	int _errorf_save = errno;
	errno = 0;

	fflush(stdout);
	fprintf(stderr, "%s:%d ", file, line);

	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	if (_errorf_save) {
		fprintf(stderr, " errno %d: %s", _errorf_save,
			strerror(_errorf_save));
	}
	fprintf(stderr, "\n");
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

int print_help(FILE *out, const char *name)
{
	fprintf(out, "The %s runs a program, and monitors /proc\n"
		"Based on counters in /proc if the process looks hung,\n"
		"%s will kill and restart it.\n\n", YOYO_NAME, YOYO_NAME);
	fprintf(out, "Usage: %s [OPTION] program program-args...\n", name);
	fprintf(out,
		"  -V, --version                  "
		"print version (%s)\n", YOYO_VERSION);
	fprintf(out,
		"  -h, --help                     " "print this message\n");
	fprintf(out,
		"  -v, --verbose                  "
		"output addition error information\n");
	fprintf(out,
		"  -w, --wait-interval[=seconds]  "
		"seconds to sleep between checks\n"
		"                                 "
		"    if < 1, defaults is 5\n");
	fprintf(out,
		"  -m, --max-hangs[=num]          "
		"number of hang checks to tolerate\n"
		"                                 "
		"    if < 1, defaults is 5\n");
	fprintf(out,
		"  -r, --max-retry[=num]          "
		"max iterations to retry after hang\n"
		"                                 "
		"    if < 1, defaults is 5\n");
	fprintf(out,
		"  -f, --fakeroot[=path]          "
		"path to look for /proc files\n");
	return 0;
}

/* getopt/getoplong is horrrible */
void parse_command_line(struct yoyo_options *options, int argc, char **argv)
{
	/* yes, optstirng is horrible */
	const char *optstring = "Vhvw::m::r::f::";

	struct option long_options[] = {
		{ "version", no_argument, 0, 'V' },
		{ "help", no_argument, 0, 'h' },
		{ "verbose", no_argument, 0, 'v' },
		{ "wait-interval", optional_argument, 0, 'w' },
		{ "max-hangs", optional_argument, 0, 'm' },
		{ "max-retries", optional_argument, 0, 'r' },
		{ "fakeroot", optional_argument, 0, 'f' },
		{ 0, 0, 0, 0 }
	};

	memset(options, 0x00, sizeof(struct yoyo_options));

	char *verboseenv = getenv("VERBOSE");
	if (verboseenv) {
		options->verbose = atoi(verboseenv);
	}

	// This is intended to be useful for some forms of testing
	char *fakerootenv = getenv("YOYO_FAKE_ROOT");
	if (fakerootenv) {
		options->fakeroot = fakerootenv;
	}

	while (1) {
		int option_index = 0;
		int opt_char = getopt_long(argc, argv, optstring, long_options,
					   &option_index);

		/* Detect the end of the options. */
		if (opt_char == -1)
			break;

		switch (opt_char) {
		case 0:
			break;
		case 'V':	/* --version | -V */
			options->version = 1;
			break;
		case 'h':	/* --help | -h */
			options->help = 1;
			break;
		case 'v':	/* --verbose | -v */
			options->version = 1;
			break;
		case 'w':	/* --wait-interval | -w */
			options->hang_check_interval = atoi(optarg);
			break;
		case 'm':	/* --max-hangs | -m */
			options->max_hangs = atoi(optarg);
			break;
		case 'r':	/* --max-retries | -m */
			options->max_retries = atoi(optarg);
			break;
		case 'f':	/* --fakeroot | -f */
			options->fakeroot = optarg;
			break;
		}
	}

	if (options->hang_check_interval < 1) {
		options->hang_check_interval = 5;
	}

	if (options->max_hangs < 1) {
		options->max_hangs = 5;
	}

	if (options->max_retries < 1) {
		options->max_retries = 5;
	}

	if (!options->fakeroot) {
		options->fakeroot = "";
	}

	// shift-off this program + options, and exec the rest
	// of the commandline
	options->child_command_line = argv + optind;
}
