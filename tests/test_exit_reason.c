/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> */

#include "yoyo.h"

#include <stdio.h>
#include <string.h>

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
	if (!strstr(buf, expect1)) {
		fprintf(stderr, "%s:%s:%d FAIL: %s no '%s' in: %s\n",
			__FILE__, __func__, __LINE__, "pid", expect1, buf);
		++failures;
	}

	const char *expect2 = "exit code: 0";
	if (!strstr(buf, expect2)) {
		fprintf(stderr, "%s:%s:%d FAIL: %s no '%s' in: %s\n",
			__FILE__, __func__, __LINE__, "exit code",
			expect2, buf);
		++failures;
	}

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
	if (!strstr(buf, expect)) {
		fprintf(stderr, "%s:%s:%d FAIL: %s no '%s' in: %s\n",
			__FILE__, __func__, __LINE__, "expect", expect, buf);
		++failures;
	}
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
	if (!strstr(buf, expect)) {
		fprintf(stderr, "%s:%s:%d FAIL: %s no '%s' in: %s\n",
			__FILE__, __func__, __LINE__, "expect", expect, buf);
		++failures;
	}
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
	if (!strstr(buf, expect)) {
		fprintf(stderr, "%s:%s:%d FAIL: %s no '%s' in: %s\n",
			__FILE__, __func__, __LINE__, "expect", expect, buf);
		++failures;
	}
	return failures;
}

int main(void)
{
	unsigned failures = 0;

	failures += test_wait_status_0();
	failures += test_wait_status_1();
	failures += test_wait_status_2943();
	failures += test_wait_status_ffff();

	return failures ? 1 : 0;
}
