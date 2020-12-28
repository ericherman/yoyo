#include "yoyo.h"

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

	return failures;
}

unsigned test_ls_l(void)
{
	const int fake_argc = 5;
	char *fake_args[6] = {
		"./yoyo",
		"--wait-interval=60",
		"--",
		"ls",
		"-l",
		NULL
	};

	struct yoyo_options options;

	parse_command_line(&options, fake_argc, fake_args);

	unsigned failures = 0;

	if (options.hang_check_interval != 60) {
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

	failures += test_fixture_2();
	failures += test_ls_l();

	return failures ? 1 : 0;
}
