/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> */

#include "yoyo.h"
#include "test-util.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>

extern struct exit_reason global_exit_reason;
extern pid_t (*yoyo_waitpid)(pid_t pid, int *wstatus, int options);
extern int yoyo_verbose;
extern FILE *yoyo_stdout;
extern FILE *yoyo_stderr;

unsigned test_wait_status_0(void)
{
	struct exit_reason reason;
	exit_reason_clear(&reason);

	long pid = 10007;
	int wait_status = 0;

	exit_reason_set(&reason, pid, wait_status);

	char buf[250];
	exit_reason_to_str(&reason, buf, 250);

	unsigned failures = 0;

	const char *expect1 = "10007";
	failures += Check(strstr(buf, expect1), "no '%s' in: %s", expect1, buf);

	const char *expect2 = "exit code: 0";
	failures += Check(strstr(buf, expect2), "no '%s' in: %s", expect2, buf);

	return failures;
}

unsigned test_wait_status_1(void)
{
	struct exit_reason reason;
	exit_reason_clear(&reason);

	long pid = 23;
	int wait_status = 1;

	exit_reason_set(&reason, pid, wait_status);

	char buf[250];
	exit_reason_to_str(&reason, buf, 250);

	unsigned failures = 0;

	const char *expect = "terminated by a signal 1";
	failures += Check(strstr(buf, expect), "no '%s' in: %s", expect, buf);

	return failures;
}

unsigned test_wait_status_15(void)
{
	struct exit_reason reason;
	exit_reason_clear(&reason);

	long pid = 23;
	int wait_status = 15;

	exit_reason_set(&reason, pid, wait_status);

	char buf[250];
	exit_reason_to_str(&reason, buf, 250);

	unsigned failures = 0;

	const char *expect = "terminated by a signal 15";
	failures += Check(strstr(buf, expect), "no '%s' in: %s", expect, buf);

	return failures;
}

unsigned test_wait_status_2943(void)
{
	struct exit_reason reason;
	exit_reason_clear(&reason);

	long pid = 4969;
	int wait_status = 2943;

	exit_reason_set(&reason, pid, wait_status);

	char buf[250];
	exit_reason_to_str(&reason, buf, 250);

	unsigned failures = 0;

	const char *expect = "stopped";
	failures += Check(strstr(buf, expect), "no '%s' in: %s", expect, buf);

	return failures;
}

unsigned test_wait_status_ffff(void)
{
	struct exit_reason reason;
	exit_reason_clear(&reason);

	long pid = 4973;
	int wait_status = 0xffff;

	exit_reason_set(&reason, pid, wait_status);

	char buf[250];
	exit_reason_to_str(&reason, buf, 250);

	unsigned failures = 0;

	const char *expect = "resumed";
	failures += Check(strstr(buf, expect), "no '%s' in: %s", expect, buf);

	return failures;
}

unsigned test_wait_status_32512(void)
{
	struct exit_reason reason;
	exit_reason_clear(&reason);

	long pid = 4973;
	int wait_status = 32512;

	exit_reason_set(&reason, pid, wait_status);

	char buf[250];
	exit_reason_to_str(&reason, buf, 250);

	unsigned failures = 0;

	const char *expect = "terminated normally exit code: 127";
	failures += Check(strstr(buf, expect), "no '%s' in: %s", expect, buf);

	return failures;
}

pid_t faux_wait_return_pid;
int faux_wait_status;

pid_t faux_waitpid(pid_t pid, int *wstatus, int options)
{
	(void)pid;
	(void)options;

	*wstatus = faux_wait_status;
	return faux_wait_return_pid;
}

unsigned test_exit_reason_child_trap(void)
{
	unsigned failures = 0;

	exit_reason_clear(&global_exit_reason);
	const size_t buflen = 80 * 24;
	char buf[80 * 24];
	memset(buf, 0x00, buflen);

	yoyo_verbose = 1;
	FILE *fbuf = fmemopen(buf, buflen, "w");
	yoyo_stdout = fbuf;
	yoyo_stderr = fbuf;

	global_exit_reason.child_pid = 10003;

	faux_wait_return_pid = global_exit_reason.child_pid + 1;
	faux_wait_status = 9;
	int signal = 17;

	yoyo_waitpid = faux_waitpid;

	exit_reason_child_trap(signal);
	fflush(fbuf);
	fclose(fbuf);
	fbuf = NULL;

	failures +=
	    Check(!global_exit_reason.termsig, "expected 0 but was %d",
		  global_exit_reason.termsig);

	const char *expect = "10004 terminated by a signal 9";
	failures +=
	    Check(strstr(buf, expect), "'%s' not found in: %s\n", expect, buf);

	memset(buf, 0x00, buflen);
	fbuf = fmemopen(buf, buflen, "w");
	yoyo_stdout = fbuf;
	yoyo_stderr = fbuf;

	faux_wait_return_pid = global_exit_reason.child_pid;
	exit_reason_child_trap(signal);

	fflush(fbuf);
	fclose(fbuf);
	fbuf = NULL;

	expect = "10003";
	failures +=
	    Check(strstr(buf, expect), "'%s' not found in: %s\n", expect, buf);
	failures +=
	    Check(global_exit_reason.termsig, "expected exited but was %d\n",
		  global_exit_reason.termsig);

	yoyo_stdout = NULL;
	yoyo_stderr = NULL;
	yoyo_waitpid = waitpid;

	return failures;
}

int main(void)
{
	unsigned failures = 0;

	failures += run_test(test_wait_status_0);
	failures += run_test(test_wait_status_1);
	failures += run_test(test_wait_status_15);
	failures += run_test(test_wait_status_2943);
	failures += run_test(test_wait_status_ffff);
	failures += run_test(test_wait_status_32512);
	failures += run_test(test_exit_reason_child_trap);

	return failures_to_status("test_exit_reason", failures);
}
