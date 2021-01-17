/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> and
        Brett Neumeier <brett@freesa.org> */

#include "yoyo.h"
#include "test-util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned test_slurp_tmp(void)
{
	char fname[24];
	strcpy(fname, "temp.XXXXXX.slurp");
	int fd = mkstemps(fname, 6);
	FILE *f = fdopen(fd, "w");
	const char *expect = "foo!\n";
	fprintf(f, "%s", expect);
	fclose(f);

	unsigned failures = 0;
	char buf[80];
	char *rv;

	rv = slurp_text(NULL, 80, fname);
	failures += Check(rv == NULL, "null buf, expected NULL but was %p", rv);

	rv = slurp_text(buf, 0, fname);
	failures += Check(rv == NULL, "zero len, expected NULL but was %p", rv);

	rv = slurp_text(buf, 80, NULL);
	failures += Check(rv == NULL, "no path, expected NULL but was %p", rv);

	rv = slurp_text(buf, 80, "/bogus/file/name");
	failures += Check(rv == NULL, "bad path, expected NULL but was %p", rv);

	rv = slurp_text(buf, 80, fname);
	failures += Check(rv != NULL, "expected non-NULL. (buf:'%s')", buf);
	if (rv) {
		failures +=
		    Check(strcmp(buf, expect) == 0,
			  "expected '%s' but was '%s'", expect, buf);
	}

	remove(fname);

	return failures;
}

int main(void)
{
	unsigned failures = 0;

	failures += run_test(test_slurp_tmp);

	return failures_to_status("test_slurp_text", failures);
}
