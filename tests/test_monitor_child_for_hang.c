#include "yoyo.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

struct monitor_child_context {
	unsigned *failures;
	long childpid;
	struct state_list *template_sl;
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

int check_for_proc_end(struct monitor_child_context *ctx)
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

struct state_list *faux_get_states(long pid, void *context)
{
	struct monitor_child_context *ctx = context;

	++ctx->get_states_count;

	if (pid != ctx->childpid) {
		++(*ctx->failures);
		fprintf(stderr, "%s:%s:%d WHAT? Expected pid %ld but was %ld\n",
			__FILE__, __func__, __LINE__, ctx->childpid, pid);
	}

	check_for_proc_end(ctx);

	size_t len = ctx->has_exited ? 0 : ctx->template_sl->len;
	struct state_list *template = ctx->template_sl;

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

void faux_free_states(struct state_list *l, void *context)
{
	struct monitor_child_context *ctx = context;

	if (l) {
		++ctx->free_states_count;
		state_list_free(l);
	}
}

int faux_kill(long pid, int sig, void *context)
{
	struct monitor_child_context *ctx = context;

	int err = 0;

	if (pid != ctx->childpid) {
		++err;
		fprintf(stderr, "%s:%s:%d WHAT? Expected pid %ld but was %ld\n",
			__FILE__, __func__, __LINE__, ctx->childpid, pid);
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

unsigned int faux_sleep(unsigned int seconds, void *context)
{
	struct monitor_child_context *ctx = context;

	const unsigned threshold = 1000;
	if (ctx->sleep_count++ > threshold) {
		fprintf(stderr, "%s:%s:%d sleep(%u) threshold %d exceeded\n",
			__FILE__, __func__, __LINE__, seconds, threshold);
		exit(EXIT_FAILURE);
	}

	if (check_for_proc_end(ctx)) {
		return 1;
	}

	/* maybe update some counters */
	if (!ctx->sig_term_count && !ctx->sig_kill_count) {
		size_t pos = (ctx->sleep_count % (ctx->template_sl->len + 2));
		if (pos < ctx->template_sl->len) {
			if (ctx->get_states_count % 2) {
				ctx->template_sl->states[pos].utime++;
			} else {
				ctx->template_sl->states[pos].stime++;
			}
		}
	}

	if (ctx->get_states_sleeping_after &&
	    ctx->get_states_sleeping_after < ctx->get_states_count) {
		for (size_t i = 0; i < ctx->template_sl->len; ++i) {
			ctx->template_sl->states[i].state = 'S';
		}
	}

	return 0;
}

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

	struct monitor_child_context ctx;
	memset(&ctx, 0x00, sizeof(struct monitor_child_context));

	ctx.failures = &failures;
	ctx.childpid = childpid;
	ctx.template_sl = &template;
	ctx.get_states_exit_at = 4;
	ctx.get_states_sleeping_after = 2;

	unsigned max_hangs = 3;
	unsigned hang_check_interval = 60;

	monitor_child_for_hang(childpid, max_hangs, hang_check_interval,
			       faux_get_states, faux_free_states, faux_sleep,
			       faux_kill, &ctx);

	if (ctx.sig_term_count) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__, "ctx.sig_term_count", 0,
			ctx.sig_term_count);
		++failures;
	}

	if (ctx.sig_kill_count) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__, "ctx.sig_kill_count", 0,
			ctx.sig_kill_count);
		++failures;
	}

	if (ctx.free_states_count != ctx.get_states_count) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__,
			"ctx.free_states_count != ctx.get_states_count",
			ctx.get_states_count, ctx.free_states_count);
		++failures;
	}

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

	struct monitor_child_context ctx;
	memset(&ctx, 0x00, sizeof(struct monitor_child_context));

	ctx.failures = &failures;
	ctx.childpid = childpid;
	ctx.template_sl = &template;
	ctx.get_states_sleeping_after = 2;
	ctx.sig_kill_count_to_set_exited = 1;

	unsigned max_hangs = 3;
	unsigned hang_check_interval = 60;

	monitor_child_for_hang(childpid, max_hangs, hang_check_interval,
			       faux_get_states, faux_free_states, faux_sleep,
			       faux_kill, &ctx);

	if (ctx.sig_term_count < max_hangs) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__, "ctx.sig_term_count",
			max_hangs, ctx.sig_term_count);
		++failures;
	}

	if (!ctx.sig_kill_count) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__, "ctx.sig_kill_count", 1,
			ctx.sig_kill_count);
		++failures;
	}

	if (ctx.free_states_count != ctx.get_states_count) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__,
			"ctx.free_states_count != ctx.get_states_count",
			ctx.get_states_count, ctx.free_states_count);
		++failures;
	}

	return failures;
}

int main(void)
{
	unsigned failures = 0;

	failures += test_monitor_and_exit_after_4();
	failures += test_monitor_requires_sigkill();

	return failures ? 1 : 0;
}