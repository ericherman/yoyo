/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> and
        Brett Neumeier <brett@freesa.org> */

#include "yoyo.h"
#include "test-util.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

extern const int default_max_retries;
extern int yoyo_verbose;

extern struct exit_reason global_exit_reason;
extern FILE *yoyo_stdout;
extern FILE *yoyo_stderr;

typedef pid_t (*fork_func)(void);
extern fork_func yoyo_fork;

typedef int (*execvp_func)(const char *pathname, char *const argv[]);
extern execvp_func yoyo_execvp;

typedef int (*sigaction_func)(int signum, const struct sigaction * act,
			      struct sigaction * oldact);
extern sigaction_func yoyo_sigaction;

typedef unsigned (*monitor_for_hang_func)(long child_pid, unsigned max_hangs,
					  unsigned hang_check_interval);
extern monitor_for_hang_func monitor_for_hang;

FILE *dev_null;

unsigned fork_count;
pid_t fake_counting_fork_rv;
pid_t fake_counting_fork(void)
{
	++fork_count;
	return fake_counting_fork_rv;
}

unsigned execvp_count;
const char *stash_pathname;
char *const *stash_argv;
int fake_counting_execvp(const char *pathname, char *const *argv)
{
	++execvp_count;
	stash_pathname = pathname;
	stash_argv = argv;
	return 0;
}

int stashed_signum;
const struct sigaction *stash_act;
const struct sigaction *stash_oldact;
int stash_sigaction(int signum, const struct sigaction *act,
		    struct sigaction *oldact)
{
	stashed_signum = signum;
	stash_act = act;
	stash_oldact = oldact;
	return 0;
}

unsigned monitor_for_hang_count;
int *fake_wait_status;
size_t fake_wait_status_len;
unsigned fake_monitor_for_hang_func(long child_pid, unsigned max_hangs,
				    unsigned hang_check_interval)
{
	(void)max_hangs;
	(void)hang_check_interval;

	int wait_status = -1;
	if (monitor_for_hang_count < fake_wait_status_len) {
		wait_status = fake_wait_status[monitor_for_hang_count];
	}

	++monitor_for_hang_count;

	exit_reason_set(&global_exit_reason, child_pid, wait_status);

	return wait_status == 9 ? 2 : 0;
}

unsigned test_fake_fork(void)
{
	fork_count = 0;
	execvp_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = 0;
	stashed_signum = 0;
	stash_act = NULL;
	stash_oldact = NULL;
	monitor_for_hang_count = 0;
	fake_wait_status_len = 0;

	yoyo_fork = fake_counting_fork;
	yoyo_execvp = fake_counting_execvp;
	yoyo_sigaction = stash_sigaction;
	monitor_for_hang = fake_monitor_for_hang_func;

	unsigned failures = 0;

	char *argv0 = "./yoyo";
	char *child_argv0 = "./faux-rogue";
	char *child_argv1 = "1";
	char *argv[4] = { argv0, child_argv0, child_argv1, NULL };
	const int argc = 3;

	const size_t buflen = 80 * 24;
	char buf[80 * 24];
	memset(buf, 0x00, buflen);
	FILE *fbuf = fmemopen(buf, buflen, "w");
	yoyo_stdout = fbuf;
	yoyo_stderr = fbuf;
	yoyo_verbose = 2;

	int exit_val = yoyo(argc, argv);

	fflush(fbuf);
	fclose(fbuf);
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	failures +=
	    Check(strstr(buf, child_argv0), "expected '%s' in: %s", child_argv0,
		  buf);
	failures +=
	    Check(strstr(buf, child_argv1), "expected '%s' in: %s", child_argv1,
		  buf);

	failures += Check(fork_count == 1, "expected 1 but was %u", fork_count);

	failures +=
	    Check(execvp_count == 1, "expected 1 but was %u", execvp_count);

	failures += Check(stash_pathname
			  && strcmp(stash_pathname, child_argv0) == 0,
			  "expected %s but was %s", child_argv0,
			  stash_pathname);

	failures +=
	    Check(stashed_signum == SIGCHLD, "expected SIGCHLD (%d) but was %d",
		  SIGCHLD, stashed_signum);
	failures +=
	    Check(stash_act->sa_handler == exit_reason_child_trap,
		  "expected &exit_reason_child_trap (%p) but was %p",
		  exit_reason_child_trap, stash_act->sa_handler);
	failures +=
	    Check(stash_oldact == NULL, "expected NULL but was %p",
		  stash_oldact);

	failures +=
	    Check(monitor_for_hang_count == 0, "expected 0 but was %u",
		  monitor_for_hang_count);

	failures += Check(exit_val == 0, "expected 0 but was %d", exit_val);

	return failures;
}

unsigned test_help(void)
{
	fork_count = 0;
	execvp_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = 0;

	yoyo_fork = fake_counting_fork;
	yoyo_execvp = fake_counting_execvp;

	unsigned failures = 0;

	char *argv0 = "./yoyo";
	char *argv1 = "--help";
	char *argv[3] = { argv0, argv1, NULL };
	const int argc = 2;

	const size_t buflen = 80 * 24;
	char buf[80 * 24];
	memset(buf, 0x00, buflen);
	FILE *fbuf = fmemopen(buf, buflen, "w");
	yoyo_stdout = fbuf;
	yoyo_stderr = fbuf;

	int exit_val = yoyo(argc, argv);

	fflush(fbuf);
	fclose(fbuf);
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	failures += Check(!exit_val, "expected 0 but was: %d", exit_val);

	failures += Check(fork_count == 0, "expected 0 but was %u", fork_count);

	failures +=
	    Check(execvp_count == 0, "expected 0 but was %u", execvp_count);
	return failures;
}

unsigned test_version(void)
{
	fork_count = 0;
	execvp_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = 0;

	yoyo_fork = fake_counting_fork;
	yoyo_execvp = fake_counting_execvp;

	unsigned failures = 0;

	char *argv0 = "./yoyo";
	char *argv1 = "--version";
	char *argv[3] = { argv0, argv1, NULL };
	const int argc = 2;

	const size_t buflen = 80 * 24;
	char buf[80 * 24];
	memset(buf, 0x00, buflen);
	FILE *fbuf = fmemopen(buf, buflen, "w");
	yoyo_stdout = fbuf;
	yoyo_stderr = fbuf;

	int exit_val = yoyo(argc, argv);

	fflush(fbuf);
	fclose(fbuf);
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	failures += Check(!exit_val, "expected 0 but was: %d", exit_val);

	failures +=
	    Check(strstr(buf, yoyo_version), "expected '%s' in: %s",
		  yoyo_version, buf);

	failures += Check(fork_count == 0, "expected 0 but was %u", fork_count);

	failures +=
	    Check(execvp_count == 0, "expected 0 but was %u", execvp_count);

	return failures;
}

unsigned test_failing_fork(void)
{
	fork_count = 0;
	execvp_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = -1;

	yoyo_fork = fake_counting_fork;
	yoyo_execvp = fake_counting_execvp;
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	unsigned failures = 0;

	char *argv0 = "./yoyo";
	char *child_argv0 = "./faux-rogue";
	char *child_argv1 = "2";
	char *argv[4] = { argv0, child_argv0, child_argv1, NULL };
	const int argc = 3;

	int exit_val = yoyo(argc, argv);

	failures += Check(fork_count == 1, "expected 1 but was %u", fork_count);

	failures +=
	    Check(execvp_count == 0, "expected 0 but was %u", execvp_count);

	failures +=
	    Check(stash_pathname == NULL, "expected NULL but was %d",
		  stash_pathname);

	failures += Check(exit_val != 0, "expected non-zero");

	return failures;
}

unsigned test_child_works_first_time(void)
{
	fork_count = 0;
	execvp_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = 2111;
	monitor_for_hang_count = 0;

	int wait_statuses[1] = { 0 };
	fake_wait_status = wait_statuses;
	fake_wait_status_len = 1;

	yoyo_fork = fake_counting_fork;
	yoyo_execvp = fake_counting_execvp;
	yoyo_sigaction = stash_sigaction;
	monitor_for_hang = fake_monitor_for_hang_func;

	const size_t buflen = 80 * 24;
	char buf[80 * 24];
	memset(buf, 0x00, buflen);
	FILE *fbuf = fmemopen(buf, buflen, "w");
	yoyo_stdout = fbuf;
	yoyo_stderr = fbuf;

	unsigned failures = 0;

	char *argv0 = "./yoyo";
	char *child_argv0 = "./bogus";
	char *argv[3] = { argv0, child_argv0, NULL };
	const int argc = 2;

	int exit_val = yoyo(argc, argv);

	fflush(fbuf);
	fclose(fbuf);
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	failures += Check(fork_count == 1, "expected 1 but was %u", fork_count);

	failures +=
	    Check(execvp_count == 0, "expected 0 but was %u", execvp_count);

	failures +=
	    Check(monitor_for_hang_count == 1, "expected 1 but was %u",
		  monitor_for_hang_count);

	failures += Check(exit_val == 0, "expected zero, but was %d", exit_val);

	const char *expect = "completed successfully";
	failures +=
	    Check(strstr(buf, expect), "'%s' not in: %s\n", expect, buf);

	return failures;
}

unsigned test_child_works_last_time(void)
{
	fork_count = 0;
	execvp_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = 2111;
	monitor_for_hang_count = 0;

	int wait_statuses[5] = { 9, 9, 9, 9, 0 };
	fake_wait_status = wait_statuses;
	fake_wait_status_len = 5;

	yoyo_fork = fake_counting_fork;
	yoyo_execvp = fake_counting_execvp;
	yoyo_sigaction = stash_sigaction;
	monitor_for_hang = fake_monitor_for_hang_func;

	const size_t buflen = 80 * 24;
	char buf[80 * 24];
	memset(buf, 0x00, buflen);
	FILE *fbuf = fmemopen(buf, buflen, "w");
	yoyo_stdout = fbuf;
	yoyo_stderr = fbuf;

	unsigned failures = 0;

	char *argv0 = "./yoyo";
	char *child_argv0 = "./bogus";
	char *argv[3] = { argv0, child_argv0, NULL };
	const int argc = 2;

	int exit_val = yoyo(argc, argv);

	fflush(fbuf);
	fclose(fbuf);
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	failures += Check(fork_count == 5, "expected 5 but was %u", fork_count);

	failures +=
	    Check(execvp_count == 0, "expected 0 but was %u", execvp_count);

	failures +=
	    Check(monitor_for_hang_count == 5, "expected 5 but was %u",
		  monitor_for_hang_count);

	failures += Check(exit_val == 0, "expected zero, but was %d", exit_val);

	const char *expect = "completed successfully";
	failures +=
	    Check(strstr(buf, expect), "'%s' not in: %s\n", expect, buf);

	return failures;
}

unsigned test_child_hangs_every_time(void)
{
	fork_count = 0;
	execvp_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = 2111;
	monitor_for_hang_count = 0;

	int wait_statuses[6] = { 32512, 32512, 32512, 32512, 32512, 32512 };
	fake_wait_status = wait_statuses;
	fake_wait_status_len = 6;

	yoyo_fork = fake_counting_fork;
	yoyo_execvp = fake_counting_execvp;
	yoyo_sigaction = stash_sigaction;
	monitor_for_hang = fake_monitor_for_hang_func;

	const size_t buflen = 80 * 24;
	char buf[80 * 24];
	memset(buf, 0x00, buflen);
	FILE *fbuf = fmemopen(buf, buflen, "w");
	yoyo_stdout = fbuf;
	yoyo_stderr = fbuf;

	unsigned failures = 0;

	char *argv0 = "./yoyo";
	char *child_argv0 = "./bogus";
	char *argv[3] = { argv0, child_argv0, NULL };
	const int argc = 2;

	int exit_val = yoyo(argc, argv);

	fflush(fbuf);
	fclose(fbuf);
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	const unsigned max_tries = default_max_retries + 1;

	failures +=
	    Check(fork_count == max_tries, "expected %u but was %u", max_tries,
		  fork_count);

	failures +=
	    Check(execvp_count == 0, "expected 0 but was %u", execvp_count);

	failures +=
	    Check(monitor_for_hang_count == max_tries, "expected %u but was %u",
		  max_tries, monitor_for_hang_count);

	failures += Check(exit_val == 1, "expected 1, but was %d", exit_val);

	const char *expect = "Retries limit reached";
	failures +=
	    Check(strstr(buf, expect), "'%s' not in: %s\n", expect, buf);

	return failures;
}

unsigned test_child_killed_every_time(void)
{
	fork_count = 0;
	execvp_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = 2111;
	monitor_for_hang_count = 0;

	int wait_statuses[6] = { 15, 15, 15, 15, 15, 15 };
	fake_wait_status = wait_statuses;
	fake_wait_status_len = 6;

	yoyo_fork = fake_counting_fork;
	yoyo_execvp = fake_counting_execvp;
	yoyo_sigaction = stash_sigaction;
	monitor_for_hang = fake_monitor_for_hang_func;

	const size_t buflen = 80 * 24;
	char buf[80 * 24];
	memset(buf, 0x00, buflen);
	FILE *fbuf = fmemopen(buf, buflen, "w");
	yoyo_stdout = fbuf;
	yoyo_stderr = fbuf;

	unsigned failures = 0;

	char *argv0 = "./yoyo";
	char *child_argv0 = "./bogus";
	char *argv[3] = { argv0, child_argv0, NULL };
	const int argc = 2;

	int exit_val = yoyo(argc, argv);

	fflush(fbuf);
	fclose(fbuf);
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	const unsigned max_tries = default_max_retries + 1;

	failures +=
	    Check(fork_count == max_tries, "expected %u but was %u", max_tries,
		  fork_count);

	failures +=
	    Check(execvp_count == 0, "expected 0 but was %u", execvp_count);

	failures +=
	    Check(monitor_for_hang_count == max_tries, "expected %u but was %u",
		  max_tries, monitor_for_hang_count);

	failures += Check(exit_val == 1, "expected 1, but was %d", exit_val);

	const char *expect = "Retries limit reached";
	failures +=
	    Check(strstr(buf, expect), "'%s' not in: %s\n", expect, buf);

	return failures;
}

unsigned test_do_not_even_try_if_no_child(void)
{
	fork_count = 0;
	execvp_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = 0;

	yoyo_fork = fake_counting_fork;
	yoyo_execvp = fake_counting_execvp;
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	unsigned failures = 0;

	const int argc = 1;
	char *argv[2] = { "./yoyo", NULL };

	int exit_val = yoyo(argc, argv);

	failures += Check(fork_count == 0, "expected 0 but was %u", fork_count);
	failures +=
	    Check(execvp_count == 0, "expected 0 but was %u", execvp_count);
	failures +=
	    Check(stash_pathname == NULL, "expected NULL but was %d",
		  stash_pathname);

	failures += Check(exit_val != 0, "expected non-zero");

	return failures;
}

int main(void)
{
	dev_null = fopen("/dev/null", "w");
	if (!dev_null) {
		fprintf(stderr, "\n%s: can't open /dev/null?\n", __FILE__);
		return EXIT_FAILURE;
	}

	unsigned failures = 0;

	failures += run_test(test_fake_fork);
	failures += run_test(test_failing_fork);
	failures += run_test(test_child_works_first_time);
	failures += run_test(test_child_works_last_time);
	failures += run_test(test_child_hangs_every_time);
	failures += run_test(test_child_killed_every_time);
	failures += run_test(test_do_not_even_try_if_no_child);
	failures += run_test(test_help);
	failures += run_test(test_version);

	fflush(dev_null);
	fclose(dev_null);

	return failures_to_status("test_yoyo_main", failures);
}
