/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> */

#include "yoyo.h"
#include "test-util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

extern FILE *yoyo_stdout;
extern FILE *yoyo_stderr;

typedef pid_t (*fork_func)(void);
extern fork_func yoyo_fork;

typedef int (*execv_func)(const char *pathname, char *const argv[]);
extern execv_func yoyo_execv;

FILE *dev_null;

unsigned fork_count;
pid_t fake_counting_fork_rv;
pid_t fake_counting_fork(void)
{
	++fork_count;
	return fake_counting_fork_rv;
}

unsigned execv_count;
const char *stash_pathname;
char *const *stash_argv;
int fake_counting_execv(const char *pathname, char *const *argv)
{
	++execv_count;
	stash_pathname = pathname;
	stash_argv = argv;
	return 0;
}

unsigned test_fake_fork(void)
{
	fork_count = 0;
	execv_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = 0;

	yoyo_fork = fake_counting_fork;
	yoyo_execv = fake_counting_execv;

	unsigned failures = 0;

	char *argv0 = "./yoyo";
	char *argv1 = "--verbose=2";
	char *child_argv0 = "./faux-rogue";
	char *child_argv1 = "2";
	char *argv[5] = { argv0, argv1, child_argv0, child_argv1, NULL };
	const int argc = 4;

	const size_t buflen = 80 * 24;
	char buf[80 * 24];
	memset(buf, 0x00, buflen);
	FILE *fbuf = fmemopen(buf, buflen, "w");
	yoyo_stdout = fbuf;
	yoyo_stderr = fbuf;

	int exit_val = yoyo_main(argc, argv);

	fflush(fbuf);
	fclose(fbuf);
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	if (!strstr(buf, child_argv0)) {
		fprintf(stderr, "%s:%s:%d FAIL: %s no '%s' in: %s\n",
			__FILE__, __func__, __LINE__, "child_argv0",
			child_argv0, buf);
		++failures;
	}
	if (!strstr(buf, child_argv1)) {
		fprintf(stderr, "%s:%s:%d FAIL: %s no '%s' in: %s\n",
			__FILE__, __func__, __LINE__, "child_argv0",
			child_argv0, buf);
		++failures;
	}

	if (fork_count != 1) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__, "fork_count", 1U,
			fork_count);
		++failures;
	}

	if (execv_count != 1) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__, "execv_count", 1U,
			execv_count);
		++failures;
	}

	if (!stash_pathname || strcmp(stash_pathname, child_argv0) != 0) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %s but was %s\n",
			__FILE__, __func__, __LINE__, "pathname", child_argv0,
			stash_pathname);
		++failures;
	}

	if (exit_val != 0) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %d but was %d\n",
			__FILE__, __func__, __LINE__, "exit_val", 0, exit_val);
		++failures;
	}

	return failures;
}

unsigned test_help(void)
{
	fork_count = 0;
	execv_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = 0;

	yoyo_fork = fake_counting_fork;
	yoyo_execv = fake_counting_execv;

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

	yoyo_main(argc, argv);

	fflush(fbuf);
	fclose(fbuf);
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	if (!strstr(buf, "max-hangs")) {
		fprintf(stderr, "%s:%s:%d FAIL: %s no '%s' in: %s\n",
			__FILE__, __func__, __LINE__, "help", "max-hangs", buf);
		++failures;
	}

	if (fork_count) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__, "fork_count", 0U,
			fork_count);
		++failures;
	}

	if (execv_count) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__, "execv_count", 0U,
			execv_count);
		++failures;
	}

	return failures;
}

unsigned test_version(void)
{
	fork_count = 0;
	execv_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = 0;

	yoyo_fork = fake_counting_fork;
	yoyo_execv = fake_counting_execv;

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

	yoyo_main(argc, argv);

	fflush(fbuf);
	fclose(fbuf);
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	const char *expect = "0.0.1";
	if (!strstr(buf, expect)) {
		fprintf(stderr, "%s:%s:%d FAIL: %s no '%s' in: %s\n",
			__FILE__, __func__, __LINE__, "version", expect, buf);
		++failures;
	}

	if (fork_count) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__, "fork_count", 0U,
			fork_count);
		++failures;
	}

	if (execv_count) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__, "execv_count", 0U,
			execv_count);
		++failures;
	}

	return failures;
}

unsigned test_failing_fork(void)
{
	fork_count = 0;
	execv_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = -1;

	yoyo_fork = fake_counting_fork;
	yoyo_execv = fake_counting_execv;
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	unsigned failures = 0;

	char *argv0 = "./yoyo";
	char *child_argv0 = "./faux-rogue";
	char *child_argv1 = "2";
	char *argv[4] = { argv0, child_argv0, child_argv1, NULL };
	const int argc = 3;

	int exit_val = yoyo_main(argc, argv);

	if (fork_count != 1) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__, "fork_count", 1U,
			fork_count);
		++failures;
	}

	if (execv_count) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected 0 but was %u\n",
			__FILE__, __func__, __LINE__, "execv_count",
			execv_count);
		++failures;
	}

	if (stash_pathname) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %s but was %s\n",
			__FILE__, __func__, __LINE__, "pathname", "NULL",
			stash_pathname);
		++failures;
	}

	if (exit_val == 0) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected non-zero\n",
			__FILE__, __func__, __LINE__, "exit_val");
		++failures;
	}

	return failures;
}

unsigned test_do_not_even_try_if_no_child(void)
{
	fork_count = 0;
	execv_count = 0;
	stash_pathname = NULL;
	stash_argv = NULL;
	fake_counting_fork_rv = 0;

	yoyo_fork = fake_counting_fork;
	yoyo_execv = fake_counting_execv;
	yoyo_stdout = dev_null;
	yoyo_stderr = dev_null;

	unsigned failures = 0;

	const int argc = 2;
	char *argv[3] = { "./yoyo", "--verbose=1", NULL };

	int exit_val = yoyo_main(argc, argv);

	if (fork_count != 0) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__, "fork_count", 0U,
			fork_count);
		++failures;
	}

	if (execv_count != 0) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %u but was %u\n",
			__FILE__, __func__, __LINE__, "execv_count", 0U,
			execv_count);
		++failures;
	}

	if (stash_pathname) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %s but was %s\n",
			__FILE__, __func__, __LINE__, "pathname", "NULL",
			stash_pathname);
		++failures;
	}

	if (exit_val == 0) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected non-zero\n",
			__FILE__, __func__, __LINE__, "exit_val");
		++failures;
	}

	return failures;
}

int main(void)
{
	dev_null = fopen("/dev/null", "w");
	if (!dev_null) {
		fprintf(stderr, "%s: can't open /dev/null?\n", __FILE__);
		return EXIT_FAILURE;
	}

	unsigned failures = 0;

	failures += run_test(test_fake_fork);
	failures += run_test(test_failing_fork);
	failures += run_test(test_do_not_even_try_if_no_child);
	failures += run_test(test_help);
	failures += run_test(test_version);

	fflush(dev_null);
	fclose(dev_null);

	return failures_to_status("test_yoyo_main", failures);
}
