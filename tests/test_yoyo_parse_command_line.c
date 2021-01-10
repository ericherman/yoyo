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

	failures +=
	    Check((strcmp(options.child_command_line[0], "./fixture") == 0),
		  "was: %s", options.child_command_line[0]);

	failures +=
	    Check((strcmp(options.child_command_line[1], "2") == 0), "was: %s",
		  options.child_command_line[1]);

	failures +=
	    Check(options.verbose == 0, "expected 0 but was %d",
		  options.verbose);

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

	failures +=
	    Check(options.verbose == 2, "expected 2 but was %d",
		  options.verbose);

	failures +=
	    Check(options.hang_check_interval == 30, "expected 30 but was %d",
		  options.hang_check_interval);

	failures +=
	    Check(options.max_hangs == 3, "expected 3 but was %d",
		  options.max_hangs);

	failures +=
	    Check(options.max_retries == 6, "expected 6 but was %d",
		  options.max_retries);

	failures +=
	    Check((strcmp(options.fakeroot, "./fake") == 0),
		  "was: %s", options.child_command_line[0]);

	failures +=
	    Check((strcmp(options.child_command_line[0], "ls") == 0),
		  "was: %s", options.child_command_line[0]);

	failures +=
	    Check((strcmp(options.child_command_line[1], "-l") == 0),
		  "was: %s", options.child_command_line[1]);

	return failures;
}

int main(void)
{
	unsigned failures = 0;

	failures += run_test(test_fixture_2);
	failures += run_test(test_ls_l);

	return failures_to_status("test_yoyo_parse_command_line", failures);
}
