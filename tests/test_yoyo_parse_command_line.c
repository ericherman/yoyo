/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> */

#include "yoyo.h"
#include "test-util.h"

#include <stdio.h>
#include <string.h>

unsigned test_fixture_2(void)
{
	const int fake_argc = 3;
	char *fake_args[4] = {
		"./yoyoc",
		"./fixture",
		"2",
		NULL
	};

	struct yoyo_options options;

	parse_command_line(&options, fake_argc, fake_args);

	unsigned failures = 0;

	if (strcmp(options.child_command_line[0], "./fixture") != 0) {
		++failures;
	}
	if (strcmp(options.child_command_line[1], "2") != 0) {
		++failures;
	}
	if (options.verbose) {
		++failures;
	}

	return failures;
}

unsigned test_ls_l(void)
{
	const int fake_argc = 9;
	char *fake_args[10] = {
		"./yoyo",
		"--verbose=2",
		"--wait-interval=30",
		"--max-hangs=3",
		"--max-retries=6",
		"--fakeroot=./fake",
		"--",
		"ls",
		"-l",
		NULL
	};

	struct yoyo_options options;

	parse_command_line(&options, fake_argc, fake_args);

	unsigned failures = 0;

	if (options.verbose != 2) {
		++failures;
	}
	if (options.hang_check_interval != 30) {
		++failures;
	}
	if (options.max_hangs != 3) {
		++failures;
	}
	if (options.max_retries != 6) {
		++failures;
	}
	if (strcmp(options.fakeroot, "./fake") != 0) {
		++failures;
	}
	if (strcmp(options.child_command_line[0], "ls") != 0) {
		++failures;
	}
	if (strcmp(options.child_command_line[1], "-l") != 0) {
		++failures;
	}

	return failures;
}

int main(void)
{
	unsigned failures = 0;

	failures += run_test(test_fixture_2);
	failures += run_test(test_ls_l);

	return failures_to_status("test_yoyo_parse_command_line", failures);
}
