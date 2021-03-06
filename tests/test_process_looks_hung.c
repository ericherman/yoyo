/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> */

#include "yoyo.h"
#include "test-util.h"

#include <stdio.h>
#include <string.h>

unsigned test_previous_is_null_next_sleeping(void)
{
	struct thread_state three_sleeping[3] = {
		{.pid = 10007,.state = 'S',.utime = 3217,.stime = 3259 },
		{.pid = 10009,.state = 'S',.utime = 6733,.stime = 5333 },
		{.pid = 10037,.state = 'S',.utime = 0,.stime = 0 }
	};
	struct state_list all_sleeping = {.states = three_sleeping,.len = 3 };

	struct state_list *next = NULL;
	struct state_list *previous = NULL;
	struct state_list *current = &all_sleeping;

	int hung = process_looks_hung(&next, previous, current);

	unsigned failures = 0;

	failures += Check(hung == 0, "expected 0 but was %d", hung);

	failures +=
	    Check(next == current, "expected %p but was %p", current, next);

	return failures;
}

unsigned test_next_not_sleeping(void)
{
	struct thread_state three_sleeping[3] = {
		{.pid = 10007,.state = 'S',.utime = 3217,.stime = 3259 },
		{.pid = 10009,.state = 'S',.utime = 6733,.stime = 5333 },
		{.pid = 10037,.state = 'S',.utime = 0,.stime = 0 }
	};
	struct state_list all_sleeping = {.states = three_sleeping,.len = 3 };

	struct thread_state one_awake[3] = {
		{.pid = 10007,.state = 'S',.utime = 3217,.stime = 3259 },
		{.pid = 10009,.state = 'R',.utime = 6733,.stime = 5333 },
		{.pid = 10037,.state = 'S',.utime = 0,.stime = 0 }
	};
	struct state_list two_sleeping_one_up = {.states = one_awake,.len = 3 };

	struct state_list *next = NULL;
	struct state_list *previous = &all_sleeping;
	struct state_list *current = &two_sleeping_one_up;

	int hung = process_looks_hung(&next, previous, current);

	unsigned failures = 0;

	failures += Check(hung == 0, "expected 0 but was %d", hung);

	failures += Check(next == NULL, "expected NULL but was %p", next);

	return failures;
}

unsigned test_all_sleeping_different_length(void)
{
	struct thread_state three_sleeping[3] = {
		{.pid = 10007,.state = 'S',.utime = 3217,.stime = 3259 },
		{.pid = 10009,.state = 'S',.utime = 6733,.stime = 5333 },
		{.pid = 10037,.state = 'S',.utime = 0,.stime = 0 }
	};
	struct state_list all_sleeping3 = {.states = three_sleeping,.len = 3 };

	struct thread_state four_sleeping[4] = {
		{.pid = 10007,.state = 'S',.utime = 3217,.stime = 3259 },
		{.pid = 10009,.state = 'S',.utime = 6733,.stime = 5333 },
		{.pid = 10037,.state = 'S',.utime = 0,.stime = 0 },
		{.pid = 10039,.state = 'S',.utime = 0,.stime = 0 }
	};
	struct state_list all_sleeping4 = {.states = four_sleeping,.len = 4 };

	struct state_list *next = NULL;
	struct state_list *previous = &all_sleeping3;
	struct state_list *current = &all_sleeping4;

	int hung = process_looks_hung(&next, previous, current);

	unsigned failures = 0;

	failures += Check(hung == 0, "expected 0 but was %d", hung);

	failures +=
	    Check(next == current, "expected %p but was %p", current, next);

	return failures;
}

unsigned test_times_increment_by_only_one(void)
{
	struct thread_state three_sleeping[3] = {
		{.pid = 10007,.state = 'S',.utime = 3217,.stime = 3259 },
		{.pid = 10009,.state = 'S',.utime = 6733,.stime = 5333 },
		{.pid = 10037,.state = 'S',.utime = 0,.stime = 0 }
	};
	struct state_list all_sleeping = {.states = three_sleeping,.len = 3 };

	struct thread_state all_inc_by_one[3] = {
		{.pid = 10007,.state = 'S',.utime = 3218,.stime = 3260 },
		{.pid = 10009,.state = 'S',.utime = 6734,.stime = 5334 },
		{.pid = 10037,.state = 'S',.utime = 1,.stime = 1 },
	};
	struct state_list still_sleeping = {.states = all_inc_by_one,.len = 3 };

	struct state_list *next = NULL;
	struct state_list *previous = &all_sleeping;
	struct state_list *current = &still_sleeping;

	int hung = process_looks_hung(&next, previous, current);

	unsigned failures = 0;

	failures += Check(hung != 0, "expected non-zero");

	failures +=
	    Check(next == current, "expected %p but was %p", current, next);

	return failures;
}

unsigned test_sleeping_times_increment_by_17(void)
{
	struct thread_state three_sleeping[3] = {
		{.pid = 10007,.state = 'S',.utime = 3217,.stime = 3259 },
		{.pid = 10009,.state = 'S',.utime = 6733,.stime = 5333 },
		{.pid = 10037,.state = 'S',.utime = 0,.stime = 0 }
	};
	struct state_list all_sleeping = {.states = three_sleeping,.len = 3 };

	struct thread_state all_inc_by_one[3] = {
		{.pid = 10007,.state = 'S',.utime = 3217,.stime = 3259 },
		{.pid = 10009,.state = 'S',.utime = 6733,.stime = 5333 },
		{.pid = 10037,.state = 'S',.utime = 1,.stime = 17 },
	};
	struct state_list still_sleeping = {.states = all_inc_by_one,.len = 3 };

	struct state_list *next = NULL;
	struct state_list *previous = &all_sleeping;
	struct state_list *current = &still_sleeping;

	int hung = process_looks_hung(&next, previous, current);

	unsigned failures = 0;

	failures += Check(hung == 0, "expected 0 but was %d", hung);

	failures += Check(next == NULL, "expected NULL but was %p", next);

	return failures;
}

int main(void)
{
	unsigned failures = 0;

	failures += run_test(test_previous_is_null_next_sleeping);
	failures += run_test(test_next_not_sleeping);
	failures += run_test(test_all_sleeping_different_length);
	failures += run_test(test_times_increment_by_only_one);
	failures += run_test(test_sleeping_times_increment_by_17);

	return failures_to_status("test_process_looks_hung", failures);
}
