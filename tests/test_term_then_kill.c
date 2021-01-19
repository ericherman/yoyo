/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> */

#include "yoyo.h"
#include "test-util.h"

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>

extern int (*yoyo_kill)(pid_t pid, int sig);

const long child_pid = 10007;

struct kill_context {
	unsigned *failures;
	unsigned persist_after_term;
	unsigned sig_term_count;
	unsigned sig_kill_count;
};

struct kill_context ctx;

unsigned test_term_then_kill(void)
{

	unsigned failures = 0;

	ctx.failures = &failures;

	ctx.persist_after_term = 0;
	ctx.sig_term_count = 0;
	ctx.sig_kill_count = 0;
	unsigned killed = term_then_kill(child_pid);

	failures += Check(killed == 1, "expected 1 but was %u", killed);

	failures +=
	    Check(ctx.sig_term_count == 1, "expected 1 but was %u",
		  ctx.sig_term_count);

	failures +=
	    Check(ctx.sig_kill_count == 0, "expected 0 but was %u",
		  ctx.sig_kill_count);

	ctx.persist_after_term = 1;
	ctx.sig_term_count = 0;
	ctx.sig_kill_count = 0;
	killed = term_then_kill(child_pid);

	failures += Check(killed == 2, "expected 1 but was %u", killed);

	failures +=
	    Check(ctx.sig_term_count == 1, "expected 1 but was %u",
		  ctx.sig_term_count);

	failures +=
	    Check(ctx.sig_kill_count == 1, "expected 0 but was %u",
		  ctx.sig_kill_count);

	return failures;
}

/* Test Fixture functions */
int faux_kill(pid_t pid, int sig)
{
	int err = 0;

	if (pid != child_pid) {
		++err;
		fprintf(stderr, "%s:%s:%d WHAT? Expected pid %ld but was %ld\n",
			__FILE__, __func__, __LINE__, child_pid, (long)pid);
	}

	if (sig == 0) {
		return ctx.persist_after_term ? 0 : -1;
	} else if (sig == SIGTERM) {
		++ctx.sig_term_count;
	} else if (sig == SIGKILL) {
		++ctx.sig_kill_count;
	} else {
		++err;
		fprintf(stderr, "%s:%s:%d WHAT? Did not expected signal %d\n",
			__FILE__, __func__, __LINE__, sig);
	}

	(*ctx.failures) += err;

	return err ? -1 : 0;
}

int main(void)
{
	unsigned failures = 0;

	yoyo_kill = faux_kill;

	failures += run_test(test_term_then_kill);

	return failures_to_status("test_term_then_kill", failures);
}
