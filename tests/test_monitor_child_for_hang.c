/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> */

#include "yoyo.h"
#include "test-util.h"

#include <sys/types.h>

extern int (*yoyo_kill)(pid_t pid, int sig);
extern unsigned int (*yoyo_sleep)(unsigned int seconds);
extern struct state_list *(*get_states) (long pid);
extern void (*free_states)(struct state_list *l);

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

struct monitor_child_context {
	unsigned *failures;
	long childpid;
	struct state_list *templates;
	size_t template_len;
	unsigned has_exited;
	unsigned sleep_count;
	unsigned get_states_count;
	unsigned get_states_sleeping_after;
	unsigned get_states_exit_at;
	unsigned free_states_count;
	unsigned sig_term_count;
	unsigned sig_term_count_to_set_exited;
	unsigned sig_kill_count;
	unsigned sig_kill_count_to_set_exited;
};

struct monitor_child_context *ctx = NULL;

unsigned test_monitor_and_exit_after_4(void)
{
	const long childpid = 10007;
	struct thread_state three_states_a[3] = {
		{.pid = 10007,.state = 'S',.utime = 3217,.stime = 3259 },
		{.pid = 10009,.state = 'R',.utime = 6733,.stime = 5333 },
		{.pid = 10037,.state = 'R',.utime = 0,.stime = 0 }
	};
	struct state_list template = {.states = three_states_a,.len = 3 };

	unsigned failures = 0;

	struct monitor_child_context context;
	memset(&context, 0x00, sizeof(struct monitor_child_context));
	ctx = &context;

	ctx->failures = &failures;
	ctx->childpid = childpid;
	ctx->templates = &template;
	ctx->template_len = 1;
	ctx->get_states_exit_at = 4;
	ctx->get_states_sleeping_after = 2;

	unsigned max_hangs = 3;
	unsigned hang_check_interval = 60;

	monitor_child_for_hang(childpid, max_hangs, hang_check_interval);

	failures +=
	    Check(ctx->sig_term_count == 0, "expected 0 but was %u",
		  ctx->sig_term_count);

	failures +=
	    Check(ctx->sig_kill_count == 0, "expected 0 but was %u",
		  ctx->sig_kill_count);

	failures +=
	    Check(ctx->free_states_count == ctx->get_states_count,
		  "expected %u but was %u", ctx->get_states_count,
		  ctx->free_states_count);

	return failures;
}

unsigned test_monitor_requires_sigkill(void)
{
	const long childpid = 10007;
	struct thread_state three_states_a[3] = {
		{.pid = 10007,.state = 'S',.utime = 3217,.stime = 3259 },
		{.pid = 10009,.state = 'R',.utime = 6733,.stime = 5333 },
		{.pid = 10037,.state = 'R',.utime = 0,.stime = 0 }
	};
	struct state_list template = {.states = three_states_a,.len = 3 };

	unsigned failures = 0;

	struct monitor_child_context context;
	memset(&context, 0x00, sizeof(struct monitor_child_context));
	ctx = &context;

	ctx->failures = &failures;
	ctx->childpid = childpid;
	ctx->templates = &template;
	ctx->template_len = 1;
	ctx->get_states_sleeping_after = 2;
	ctx->sig_kill_count_to_set_exited = 1;

	unsigned max_hangs = 3;
	unsigned hang_check_interval = 60;

	monitor_child_for_hang(childpid, max_hangs, hang_check_interval);

	failures += Check(ctx->sig_term_count, "expected term");

	failures += Check(ctx->sig_kill_count, "expected kill");

	failures +=
	    Check(ctx->free_states_count == ctx->get_states_count,
		  "expected %u but was %u", ctx->get_states_count,
		  ctx->free_states_count);

	return failures;
}

/* Test Fixture functions */
int check_for_proc_end(void)
{
	if (ctx->sig_kill_count_to_set_exited &&
	    ctx->sig_kill_count_to_set_exited <= ctx->sig_kill_count) {
		ctx->has_exited = 1;
		return 1;
	}

	if (ctx->sig_term_count_to_set_exited &&
	    ctx->sig_term_count_to_set_exited <= ctx->sig_term_count) {
		ctx->has_exited = 1;
		return 1;
	}

	if (ctx->get_states_exit_at &&
	    ctx->get_states_exit_at <= ctx->get_states_count) {
		ctx->has_exited = 1;
		return 1;
	}

	return 0;
}

static size_t _zmin(size_t a, size_t b)
{
	return a < b ? a : b;
}

struct state_list *faux_get_states(long pid)
{
	++ctx->get_states_count;

	if (pid != ctx->childpid) {
		++(*ctx->failures);
		fprintf(stderr, "%s:%s:%d WHAT? Expected pid %ld but was %ld\n",
			__FILE__, __func__, __LINE__, ctx->childpid, pid);
	}

	check_for_proc_end();

	size_t len = ctx->has_exited ? 0 : ctx->templates->len;
	size_t idx = _zmin(ctx->get_states_count, ctx->template_len) - 1;
	struct state_list *template = ctx->templates + idx;

	struct state_list *new_list = state_list_new(len);
	if (!new_list) {
		exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < len; ++i) {
		/* some values may have incremented during faux_sleep */
		new_list->states[i] = template->states[i];
	}

	return new_list;
}

void faux_free_states(struct state_list *l)
{
	if (l) {
		++ctx->free_states_count;
		state_list_free(l);
	}
}

int faux_kill(pid_t pid, int sig)
{
	int err = 0;

	if (pid != ctx->childpid) {
		++err;
		fprintf(stderr, "%s:%s:%d WHAT? Expected pid %ld but was %ld\n",
			__FILE__, __func__, __LINE__, ctx->childpid, (long)pid);
	}

	if (sig == 0) {
		return ctx->has_exited ? -1 : 0;
	} else if (sig == SIGTERM) {
		++ctx->sig_term_count;
	} else if (sig == SIGKILL) {
		++ctx->sig_kill_count;
	} else {
		++err;
		fprintf(stderr, "%s:%s:%d WHAT? Did not expected signal %d\n",
			__FILE__, __func__, __LINE__, sig);
	}

	(*ctx->failures) += err;

	return err ? -1 : 0;
}

unsigned int faux_sleep(unsigned int seconds)
{
	const unsigned threshold = 1000;
	if (ctx->sleep_count++ > threshold) {
		fprintf(stderr, "%s:%s:%d sleep(%u) threshold %d exceeded\n",
			__FILE__, __func__, __LINE__, seconds, threshold);
		exit(EXIT_FAILURE);
	}

	if (check_for_proc_end()) {
		return 1;
	}

	/* maybe update some counters */
	if ((ctx->get_states_count >= ctx->templates->len) &&
	    !ctx->sig_term_count && !ctx->sig_kill_count) {
		size_t pos = (ctx->sleep_count % (ctx->templates->len + 2));
		if (pos < ctx->templates->len) {
			if (ctx->get_states_count % 2) {
				ctx->templates->states[pos].utime++;
			} else {
				ctx->templates->states[pos].stime++;
			}
		}
	}

	if (ctx->get_states_sleeping_after &&
	    ctx->get_states_sleeping_after < ctx->get_states_count) {
		for (size_t i = 0; i < ctx->templates->len; ++i) {
			ctx->templates->states[i].state = 'S';
		}
	}

	return 0;
}

int main(void)
{
	unsigned failures = 0;

	yoyo_kill = faux_kill;
	yoyo_sleep = faux_sleep;
	get_states = faux_get_states;
	free_states = faux_free_states;

	failures += run_test(test_monitor_and_exit_after_4);
	failures += run_test(test_monitor_requires_sigkill);

	return failures_to_status("test_monitor_child_for_hang", failures);
}
