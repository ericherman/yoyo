/* SPDX-License-Identifier: GPL-3.0-or-later */
/* faux-rogue: a test program for yoyo which pretends to hang or fail */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> and
        Brett Neumeier <brett@freesa.org> */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum action_type { succeed = 0, fail, hang };

enum action_type get_action(const char *failpath)
{
	int failcount = 0;
	if (failpath) {
		FILE *failfile = fopen(failpath, "r+");
		if (failfile) {
			if (fscanf(failfile, "%d", &failcount) != 1) {
				fprintf(stderr, "%s:%d %s", __FILE__, __LINE__,
					"failcount not found in failfile\n");
			}
			int remaining = 0;
			if (failcount > 0) {
				remaining = failcount - 1;
			} else if (failcount < 0) {
				remaining = failcount + 1;
			}

			fseek(failfile, 0, SEEK_SET);
			fprintf(failfile, "%d\n", remaining);
			fclose(failfile);
		}
	}

	enum action_type action;
	if (failcount < 0) {
		action = hang;
		fprintf(stderr, "%s:%d hang count: %d\n", __FILE__, __LINE__,
			-failcount);
	} else if (failcount > 0) {
		action = fail;
		fprintf(stderr, "%s:%d fail count: %d\n", __FILE__, __LINE__,
			failcount);
	} else {
		action = succeed;
		fprintf(stderr, "%s:%d fail/hang count: 0 (succeed)\n",
			__FILE__, __LINE__);
	}

	return action;
}

const unsigned ten_minutes = 10 * 60;

void sighandler_exit_success(int sig)
{
	fprintf(stderr, "%s:%d signal %d, exiting\n", __FILE__, __LINE__, sig);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	unsigned delay = (argc > 1) ? atoi(argv[1]) : 0;
	const char *failpath = (argc > 2) ? argv[2] : getenv("FAILCOUNT");

	signal(SIGTERM, sighandler_exit_success);

	unsigned remain = sleep(delay);

	enum action_type action = get_action(failpath);

	switch (action) {
	case succeed:
		return 0;
		break;
	case fail:
		return 127;
		break;
	case hang:
		remain = 1 + sleep(ten_minutes);
		break;
	}

	return remain > 99 ? 99 : remain;
}
