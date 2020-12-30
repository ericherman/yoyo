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

int print_help(FILE *out, const char *name);
int kill_pid(long pid, int sig, void *context);
unsigned int unistd_sleep(unsigned int seconds, void *context);

/* Modern versions of GCC whine about casting-away a const qualifier */
#if __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
/* Be very clear that we are discarding const knowningly. */
static void *discard_const(const void *v)
{
	return (void *)v;
}

#if __GNUC__
#pragma GCC diagnostic pop
#endif

/*************************************************************************/
/* globals */
/*************************************************************************/

/* The POSIX definition of "signal()" does not allow for a context parameter:

	#include <signal.h>

	typedef void (*sighandler_t)(int);

	sighandler_t signal(int signum, sighandler_t handler);

  Thus, we will need some global space to pass data between functions.
*/

static pid_t global_childpid;

static struct exit_reason *child_trap_exit_code_singleton;

static int global_verbose;

/*************************************************************************/
/* functions */
int yoyo_main(int argc, char **argv)
{
	if (argc < 2) {
		Errorf("no child command?");
		return EXIT_FAILURE;
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
	int child_command_line_len = options.child_command_line_len;
	int max_hangs = options.max_hangs;
	int hang_check_interval = options.hang_check_interval;
	const char *fakeroot = options.fakeroot;

	// setup globals for sharing data with signal handler
	global_verbose = options.verbose;
	if (global_verbose) {
		printf("global_verbose: %d\n", global_verbose);
	}

	struct exit_reason reason;
	child_trap_exit_code_singleton = &reason;

	// now that globals are setup, set the handler
	signal(SIGCHLD, &exit_reason_child_trap);

	for (int retries_remaining = max_retries;
	     retries_remaining > 0; --retries_remaining) {

		// reset our exit reason prior to each fork
		exit_reason_clear(&reason);

		errno = 0;
		global_childpid = fork();

		if (global_childpid < 0) {
			/* no intention of testing fork failing */
			/* LCOV_EXCL_START */
			Errorf("fork() failed?");
			return EXIT_FAILURE;
			/* LCOV_EXCL_STOP */
		} else if (global_childpid == 0) {
			// in child process
			if (global_verbose) {
				for (int i = 0; i < child_command_line_len; ++i) {
					printf("%s ", child_command_line[i]);
				}
				printf("\n");
				fflush(stdout);
			}
			return execv(child_command_line[0], child_command_line);
		}

		if (global_verbose) {
			printf("child_pid: %ld\n", (long)global_childpid);
		}

		state_list_get_func get_states = get_states_proc;
		state_list_free_func free_states = free_states_wrap;
		sleep_func sleep_seconds = unistd_sleep;
		kill_func kill_child = kill_pid;
		// context is used by get_states_proc
		void *context = discard_const(fakeroot);

		monitor_child_for_hang(global_childpid, max_hangs,
				       hang_check_interval, get_states,
				       free_states, sleep_seconds, kill_child,
				       context);

		if (reason.exit_code != 0) {
			printf("Child exited with status %d\n",
			       reason.exit_code);
		} else if (reason.exited) {
			printf("Child completed successfully\n");
			return EXIT_SUCCESS;
		} else {
			char er_buf[250];
			exit_reason_to_str(&reason, er_buf, 250);
			printf("child %ld:\n%s\n", (long)global_childpid,
			       er_buf);
		}
	}

	fflush(stdout);
	fprintf(stderr, "Retries limit reached.\n");
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
		    || (new_state.utime > (old_state.utime + 1))
		    || (new_state.stime > (old_state.stime + 1))
		    ) {
			*next = NULL;
			return 0;
		}
	}

	*next = current;
	return 1;
}

static char *pid_to_stat_pattern(char *buf, const char *fakeroot, long pid)
{
	strcpy(buf, fakeroot ? fakeroot : "");
	strcat(buf, "/proc/");
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
	return n ? buf : NULL;
}

int thread_state_from_path(struct thread_state *ts, const char *path)
{
	unsigned fields = 60;
	const size_t buf_len = FILENAME_MAX + (fields * (sizeof(long) + 1));
	char buf[buf_len];
	errno = 0;
	if (!slurp_text(buf, buf_len, path)) {
		if (global_verbose > 1 || errno != ENOENT) {
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

struct state_list *state_list_new(size_t length)
{
	size_t size = sizeof(struct state_list);
	struct state_list *sl = calloc(1, size);
	if (!sl) {
		Errorf("could not calloc (%zu bytes)?", size);
		return NULL;
	}

	sl->len = length;
	size = sizeof(struct thread_state);
	sl->states = calloc(sl->len, size);
	if (!sl->states) {
		size_t total = sl->len * size;
		Errorf("could not calloc(%zu, %zu) (%zu bytes)?", sl->len, size,
		       total);
		free(sl);
		return NULL;
	}
	return sl;
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
	if (global_verbose > 1 || errno != ENOENT) {
		Errorf("%s (%d)", epath, eerrno);
	}
	return 0;
}

struct state_list *get_states_proc(long pid, void *context)
{
	errno = 0;

	const char *fakeroot = context ? context : "";
	char pattern[FILENAME_MAX + 3];
	pid_to_stat_pattern(pattern, fakeroot, pid);

	if (global_verbose) {
		printf("pattern == '%s'\n", pattern);
	}

	glob_t threads;
	int (*errfunc)(const char *epath, int eerrno) = ignore_no_such_file;
	errno = 0;
	glob(pattern, GLOB_MARK, errfunc, &threads);

	size_t len = threads.gl_pathc;
	if (global_verbose) {
		printf("matches for %ld: %zu\n", pid, len);
	}
	struct state_list *sl = state_list_new(len);
	/* by inspection, does the right thing, no intention test */
	/* LCOV_EXCL_START */
	if (!sl) {
		exit(EXIT_FAILURE);
	}
	/* LCOV_EXCL_STOP */

	int err = 0;
	for (size_t i = 0; i < len; ++i) {
		const char *path = threads.gl_pathv[i];
		if (global_verbose) {
			printf("\t%s\n", path);
		}
		struct thread_state *ts = &sl->states[i];
		err += thread_state_from_path(ts, path);
	}

	if (err || global_verbose) {
		Errorf("get_states for pid: %ld (fakeroot:'%s')"
		       " errors: %d", (long)pid, fakeroot, err);
	}

	globfree(&threads);

	return sl;
}

void free_states_wrap(struct state_list *l, void *context)
{
	(void)context;
	state_list_free(l);
}

int kill_pid(long pid, int sig, void *context)
{
	(void)context;
	return kill(pid, sig);
}

unsigned int unistd_sleep(unsigned int seconds, void *context)
{
	(void)context;
	return sleep(seconds);
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

	if (pid == global_childpid) {
		exit_reason_set(child_trap_exit_code_singleton, pid,
				wait_status);
	}

	if (global_verbose) {
		struct exit_reason tmp;
		exit_reason_set(&tmp, pid, wait_status);
		size_t len = 255;
		char buf[len];
		exit_reason_to_str(&tmp, buf, len);
		printf("exit_reason_child_trap(%d) (%d): %s\n", sig,
		       wait_status, buf);
	}
}

void monitor_child_for_hang(long childpid, unsigned max_hangs,
			    unsigned hang_check_interval,
			    state_list_get_func get_states,
			    state_list_free_func free_states,
			    sleep_func sleep_seconds, kill_func kill_child,
			    void *context)
{
	unsigned int hang_count = 0;
	struct state_list *thread_states = NULL;
	const int sig_exists = 0;
	while (kill_child(childpid, sig_exists, context) == 0) {
		unsigned int seconds = hang_check_interval;
		unsigned int remain = sleep_seconds(seconds, context);
		(void)remain;	// ignore, maybe interrupted by a handler?
		struct state_list *previous = thread_states;
		struct state_list *current = get_states(childpid, context);
		if (process_looks_hung(&thread_states, previous, current)) {
			++hang_count;
			int sig = hang_count <= max_hangs ? SIGTERM : SIGKILL;
			errno = 0;
			int err = kill_child(childpid, sig, context);
			/* ESRCH: No such process */
			if (global_verbose || (err && (errno != ESRCH))) {
				const char *sigstr =
				    (sig == SIGTERM) ? "SIGTERM" : "SIGKILL";
				Errorf("kill(childpid, %d) returned %d", sigstr,
				       err);
			}
			sleep_seconds(1, context);
		} else {
			hang_count = 0;
			if (global_verbose) {
				printf("Child still appears to be"
				       " doing something worthwhile\n");
			}
		}
		free_states(previous, context);
		if (thread_states != current) {
			free_states(current, context);
		}
	}
	free_states(thread_states, context);
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
		{ "verbose", optional_argument, 0, 'v' },
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
			options->verbose = optarg ? atoi(optarg) : 1;
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
		options->hang_check_interval = 60;
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
	options->child_command_line_len = argc - optind;
}
