#ifndef YOYO_H
#define YOYO_H

#include <stddef.h>		/* size_t */

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
	/* According to POSIX, pid_t is a signed int no wider than long */
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
	long child_pid;
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

typedef struct state_list *(*state_list_get_func) (long pid, void *context);
typedef void (*state_list_free_func)(struct state_list * l, void *context);
typedef unsigned int (*sleep_func)(unsigned int seconds, void *context);
typedef int (*kill_func)(long pid, int sig, void *context);

/* monitor a child process; signal the process if it looks hung */
void monitor_child_for_hang(long childpid, unsigned max_hangs,
			    unsigned hang_check_interval,
			    state_list_get_func get_states,
			    state_list_free_func free_states,
			    sleep_func sleep_seconds, kill_func kill_child,
			    void *context);

/* look for evidence of a hung process */
int process_looks_hung(struct state_list **next, struct state_list *previous,
		       struct state_list *current);

/* given a pid, create state_list based on the '/proc' filesystem
 * if the context object is non-null, it will be cast to a const char *
 * and prepended before '/proc' */
struct state_list *get_states_proc(long pid, void *context);

/* null-safe; frees the states as well as the state_list struct */
void state_list_free(struct state_list *l, void *context);

/* append to a string buffer, always NULL-terminates */
int appendf(char *buf, size_t bufsize, const char *format, ...);

/* print an error message, include the errno information if non-zero */
void errorf(const char *file, int line, const char *format, ...);

/* trap child exits, and capture the exit_reason information */
void exit_reason_child_trap(int sig);

/* zero the struct */
void exit_reason_clear(struct exit_reason *exit_reason);

/* populate the struct from the wait_status */
void exit_reason_set(struct exit_reason *exit_reason, long pid,
		     int wait_status);

/* populate buf with a human-friendly-ish description of the exit_reason */
void exit_reason_to_str(struct exit_reason *exit_reason, char *buf, size_t len);

void parse_command_line(struct yoyo_options *options, int argc, char **argv);

int yoyo_main(int argc, char **argv);
#endif /* YOYO_H */
