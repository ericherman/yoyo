/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2021 Eric Herman <eric@freesa.org> and
	Brett Neumeier <brett@freesa.org> */

#include "yoyo.h"
#include "test-util.h"

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

const unsigned hang_check_interval = 60;
const size_t max_hangs = 5;

const pid_t child_pid = 1754993;

const size_t qemu_state_0_len = 10;
struct thread_state qemu_state_0[] = {
	{.pid = 1754993,.state = 'S',.utime = 42825,.stime = 125398 },
	{.pid = 1754994,.state = 'S',.utime = 12376,.stime = 1809 },
	{.pid = 1755000,.state = 'S',.utime = 4306567,.stime = 219019 },
	{.pid = 1755001,.state = 'S',.utime = 4197739,.stime = 220016 },
	{.pid = 1755002,.state = 'S',.utime = 4492748,.stime = 220595 },
	{.pid = 1755003,.state = 'S',.utime = 4194648,.stime = 222319 },
	{.pid = 1755004,.state = 'S',.utime = 4197278,.stime = 222898 },
	{.pid = 1755005,.state = 'S',.utime = 4213710,.stime = 222763 },
	{.pid = 1755006,.state = 'S',.utime = 4178210,.stime = 226003 },
	{.pid = 1755007,.state = 'S',.utime = 4091083,.stime = 221455 }
};

struct state_list qemu_state_list_0 = {
	.states = qemu_state_0,
	.len = qemu_state_0_len
};

const size_t qemu_state_1_len = 10;
struct thread_state qemu_state_1[] = {
	{.pid = 1754993,.state = 'S',.utime = 42825,.stime = 125400 },
	{.pid = 1754994,.state = 'S',.utime = 12376,.stime = 1809 },
	{.pid = 1755000,.state = 'S',.utime = 4306567,.stime = 219019 },
	{.pid = 1755001,.state = 'S',.utime = 4197739,.stime = 220016 },
	{.pid = 1755002,.state = 'S',.utime = 4492749,.stime = 220595 },
	{.pid = 1755003,.state = 'S',.utime = 4194651,.stime = 222319 },
	{.pid = 1755004,.state = 'S',.utime = 4197278,.stime = 222898 },
	{.pid = 1755005,.state = 'S',.utime = 4213710,.stime = 222763 },
	{.pid = 1755006,.state = 'S',.utime = 4178210,.stime = 226003 },
	{.pid = 1755007,.state = 'S',.utime = 4091083,.stime = 221455 }
};

struct state_list qemu_state_list_1 = {
	.states = qemu_state_1,
	.len = qemu_state_1_len
};

const size_t qemu_state_2_len = 10;
struct thread_state qemu_state_2[] = {
	{.pid = 1754993,.state = 'S',.utime = 42827,.stime = 125400 },
	{.pid = 1754994,.state = 'S',.utime = 12376,.stime = 1809 },
	{.pid = 1755000,.state = 'S',.utime = 4306567,.stime = 219019 },
	{.pid = 1755001,.state = 'S',.utime = 4197739,.stime = 220016 },
	{.pid = 1755002,.state = 'S',.utime = 4492749,.stime = 220595 },
	{.pid = 1755003,.state = 'S',.utime = 4194654,.stime = 222319 },
	{.pid = 1755004,.state = 'S',.utime = 4197278,.stime = 222898 },
	{.pid = 1755005,.state = 'S',.utime = 4213710,.stime = 222763 },
	{.pid = 1755006,.state = 'S',.utime = 4178210,.stime = 226003 },
	{.pid = 1755007,.state = 'S',.utime = 4091083,.stime = 221455 }
};

struct state_list qemu_state_list_2 = {
	.states = qemu_state_2,
	.len = qemu_state_2_len
};

const size_t qemu_state_3_len = 10;
struct thread_state qemu_state_3[] = {
	{.pid = 1754993,.state = 'S',.utime = 42829,.stime = 125400 },
	{.pid = 1754994,.state = 'S',.utime = 12376,.stime = 1809 },
	{.pid = 1755000,.state = 'S',.utime = 4306567,.stime = 219019 },
	{.pid = 1755001,.state = 'S',.utime = 4197739,.stime = 220016 },
	{.pid = 1755002,.state = 'S',.utime = 4492750,.stime = 220595 },
	{.pid = 1755003,.state = 'S',.utime = 4194658,.stime = 222319 },
	{.pid = 1755004,.state = 'S',.utime = 4197278,.stime = 222898 },
	{.pid = 1755005,.state = 'S',.utime = 4213710,.stime = 222763 },
	{.pid = 1755006,.state = 'S',.utime = 4178210,.stime = 226003 },
	{.pid = 1755007,.state = 'S',.utime = 4091083,.stime = 221455 }
};

struct state_list qemu_state_list_3 = {
	.states = qemu_state_3,
	.len = qemu_state_3_len
};

const size_t qemu_state_4_len = 10;
struct thread_state qemu_state_4[] = {
	{.pid = 1754993,.state = 'S',.utime = 42831,.stime = 125400 },
	{.pid = 1754994,.state = 'S',.utime = 12376,.stime = 1809 },
	{.pid = 1755000,.state = 'S',.utime = 4306567,.stime = 219019 },
	{.pid = 1755001,.state = 'S',.utime = 4197739,.stime = 220016 },
	{.pid = 1755002,.state = 'S',.utime = 4492751,.stime = 220595 },
	{.pid = 1755003,.state = 'S',.utime = 4194661,.stime = 222319 },
	{.pid = 1755004,.state = 'S',.utime = 4197278,.stime = 222898 },
	{.pid = 1755005,.state = 'S',.utime = 4213710,.stime = 222763 },
	{.pid = 1755006,.state = 'S',.utime = 4178210,.stime = 226003 },
	{.pid = 1755007,.state = 'S',.utime = 4091083,.stime = 221455 }
};

struct state_list qemu_state_list_4 = {
	.states = qemu_state_4,
	.len = qemu_state_4_len
};

const size_t qemu_state_5_len = 10;
struct thread_state qemu_state_5[] = {
	{.pid = 1754993,.state = 'S',.utime = 42833,.stime = 125400 },
	{.pid = 1754994,.state = 'S',.utime = 12376,.stime = 1809 },
	{.pid = 1755000,.state = 'S',.utime = 4306567,.stime = 219019 },
	{.pid = 1755001,.state = 'S',.utime = 4197739,.stime = 220016 },
	{.pid = 1755002,.state = 'S',.utime = 4492751,.stime = 220595 },
	{.pid = 1755003,.state = 'S',.utime = 4194664,.stime = 222319 },
	{.pid = 1755004,.state = 'S',.utime = 4197278,.stime = 222898 },
	{.pid = 1755005,.state = 'S',.utime = 4213710,.stime = 222763 },
	{.pid = 1755006,.state = 'S',.utime = 4178210,.stime = 226003 },
	{.pid = 1755007,.state = 'S',.utime = 4091083,.stime = 221455 }
};

struct state_list qemu_state_list_5 = {
	.states = qemu_state_5,
	.len = qemu_state_5_len
};

const size_t qemu_state_6_len = 10;
struct thread_state qemu_state_6[] = {
	{.pid = 1754993,.state = 'S',.utime = 42835,.stime = 125400 },
	{.pid = 1754994,.state = 'S',.utime = 12376,.stime = 1809 },
	{.pid = 1755000,.state = 'S',.utime = 4306567,.stime = 219019 },
	{.pid = 1755001,.state = 'S',.utime = 4197739,.stime = 220016 },
	{.pid = 1755002,.state = 'S',.utime = 4492752,.stime = 220595 },
	{.pid = 1755003,.state = 'S',.utime = 4194668,.stime = 222319 },
	{.pid = 1755004,.state = 'S',.utime = 4197278,.stime = 222898 },
	{.pid = 1755005,.state = 'S',.utime = 4213710,.stime = 222763 },
	{.pid = 1755006,.state = 'S',.utime = 4178210,.stime = 226003 },
	{.pid = 1755007,.state = 'S',.utime = 4091083,.stime = 221455 }
};

struct state_list qemu_state_list_6 = {
	.states = qemu_state_6,
	.len = qemu_state_6_len
};

const size_t hung_qemu_frames_len = 7;
struct state_list *hung_qemu_frames[] = {
	&qemu_state_list_0,	/* first never looks hung */
	&qemu_state_list_1,	/* looks hung 1 */
	&qemu_state_list_2,	/* looks hung 2 */
	&qemu_state_list_3,	/* looks hung 3 */
	&qemu_state_list_4,	/* looks hung 4 */
	&qemu_state_list_5,	/* looks hung 5 == 5 (max_hangs) */
	&qemu_state_list_6,	/* is hung 6 > 5 (exceeds max_hangs!) */
};

struct monitor_child_context {
	unsigned *failures;
	struct state_list **state_lists;
	size_t state_lists_len;
	unsigned sleep_count;
	size_t current_state;
	unsigned looks_hung;
};

struct monitor_child_context *ctx = NULL;

/* some function prototypes */
void copy_qemu_states_to_global_context(unsigned *failures);
void free_global_context(void);

/* Test Functions */
unsigned test_qemu_hung(void)
{
	unsigned failures = 0;

	copy_qemu_states_to_global_context(&failures);

	monitor_child_for_hang(child_pid, max_hangs, hang_check_interval);

	/* fail if it does _not_ look hung */
	failures += Check(ctx->looks_hung, "expected non-zero hung");

	free_global_context();
	return failures;
}

unsigned test_qemu_last_state_big_counter_increase(void)
{
	unsigned failures = 0;

	copy_qemu_states_to_global_context(&failures);

	/* make the last state active enough to not look hung */
	size_t last = ctx->state_lists_len - 1;
	ctx->state_lists[last]->states[9].utime += 10;

	monitor_child_for_hang(child_pid, max_hangs, hang_check_interval);

	/* fail if it _does_ look hung */
	failures +=
	    Check(!ctx->looks_hung, "expected not-hung, but was %u",
		  ctx->looks_hung);

	free_global_context();
	return failures;
}

/* Test Fixture Functions */
void *calloc_or_die(const char *file, const char *func, int line, size_t nmemb,
		    size_t size)
{
	void *ptr = calloc(nmemb, size);
	if (!ptr) {
		size_t total = (nmemb * size);
		fprintf(stderr, "%s:%s:%d could not calloc(%zu, %zu)"
			" (%zu bytes)?\n",
			file, func, line, nmemb, size, total);
		exit(EXIT_FAILURE);
	}
	return ptr;
}

#define Calloc_or_die(nmemb, size) \
	calloc_or_die(__FILE__, __func__, __LINE__, nmemb, size)

void copy_qemu_states_to_global_context(unsigned *failures)
{
	size_t nmemb = 1;
	size_t size = sizeof(struct monitor_child_context);
	ctx = Calloc_or_die(nmemb, size);
	ctx->failures = failures;

	ctx->state_lists_len = hung_qemu_frames_len;
	nmemb = ctx->state_lists_len;
	size = sizeof(struct state_list *);
	ctx->state_lists = Calloc_or_die(nmemb, size);

	for (size_t i = 0; i < ctx->state_lists_len; ++i) {
		nmemb = 1;
		size = sizeof(struct state_list);
		ctx->state_lists[i] = Calloc_or_die(nmemb, size);

		struct state_list *sl_i = hung_qemu_frames[i];

		ctx->state_lists[i]->len = sl_i->len;
		nmemb = ctx->state_lists[i]->len;
		size = sizeof(struct thread_state);
		ctx->state_lists[i]->states = Calloc_or_die(nmemb, size);

		for (size_t j = 0; j < ctx->state_lists[i]->len; ++j) {
			ctx->state_lists[i]->states[j] = sl_i->states[j];
		}
	}
}

void free_global_context(void)
{
	for (size_t i = 0; i < ctx->state_lists_len; ++i) {
		free(ctx->state_lists[i]->states);
		ctx->state_lists[i]->states = NULL;
		ctx->state_lists[i]->len = 0;

		free(ctx->state_lists[i]);
		ctx->state_lists[i] = NULL;
	}

	free(ctx->state_lists);
	ctx->state_lists = NULL;
	ctx->state_lists_len = 0;

	free(ctx);
	ctx = NULL;
}

struct state_list empty_state_list = {.states = NULL,.len = 0 };

struct state_list *faux_get_states(long pid)
{
	(void)pid;

	if (ctx->current_state > ctx->state_lists_len) {
		/* Why is the code trying to fetch the states more than once
		 * after the end of known states? Something is broken. */
		++(*ctx->failures);
		fprintf(stderr, "%s:%s:%d asking for state[%zu] (len=%zu)\n",
			__FILE__, __func__, __LINE__, ctx->current_state,
			ctx->state_lists_len);
	}

	return (ctx->current_state < ctx->state_lists_len)
	    ? ctx->state_lists[ctx->current_state]
	    : &empty_state_list;
}

void faux_free_states(struct state_list *l)
{
	(void)l;
}

int faux_kill(pid_t pid, int sig)
{
	(void)pid;

	/* sig of 0 is only checking for process running */
	/* sig of TERM or KILL indicates that it looks hung */
	if (sig == SIGTERM || sig == SIGKILL) {
		++(ctx->looks_hung);
	}

	return ctx->current_state < ctx->state_lists_len ? 0 : -1;
}

unsigned int faux_sleep(unsigned int seconds)
{
	if (seconds) {
		if (ctx->sleep_count) {
			++ctx->current_state;
		}
		++ctx->sleep_count;
	}

	size_t threshold = ctx->state_lists_len;
	if (ctx->current_state > threshold) {
		fprintf(stderr, "%s:%s:%d sleep(%u) threshold %zu exceeded\n",
			__FILE__, __func__, __LINE__, seconds, threshold);
		exit(EXIT_FAILURE);
	}

	return ctx->looks_hung ? 1 : 0;
}

extern int (*yoyo_kill)(pid_t pid, int sig);
extern unsigned int (*yoyo_sleep)(unsigned int seconds);
extern struct state_list *(*get_states) (long pid);
extern void (*free_states)(struct state_list *l);

int main(void)
{
	unsigned failures = 0;

	yoyo_kill = faux_kill;
	yoyo_sleep = faux_sleep;
	get_states = faux_get_states;
	free_states = faux_free_states;

	failures += run_test(test_qemu_hung);
	failures += run_test(test_qemu_last_state_big_counter_increase);

	return failures_to_status("test_qemu_states", failures);
}
