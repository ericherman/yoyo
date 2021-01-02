/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org>, Brett Neumeier */

#include "yoyo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
	char fname[24];
	strcpy(fname, "temp.XXXXXX.slurp");
	int fd = mkstemps(fname, 6);
	FILE *f = fdopen(fd, "w");
	const char *expect = "foo!\n";
	fprintf(f, "%s", expect);
	fclose(f);

	unsigned errors = 0;
	char buf[80];
	char *rv;

	rv = slurp_text(NULL, 80, fname);
	if (rv) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %s but was %s\n",
			__FILE__, __func__, __LINE__, "null buf", "NULL", rv);
		++errors;
	}
	rv = slurp_text(buf, 0, fname);
	if (rv) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %s but was %s\n",
			__FILE__, __func__, __LINE__, "zero len", "NULL", rv);
		++errors;
	}
	rv = slurp_text(buf, 80, NULL);
	if (rv) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %s but was %s\n",
			__FILE__, __func__, __LINE__, "null path", "NULL", rv);
		++errors;
	}
	rv = slurp_text(buf, 80, "/bogus/file/name");
	if (rv) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %s but was %s\n",
			__FILE__, __func__, __LINE__, "bogus path", "NULL", rv);
		++errors;
	}

	rv = slurp_text(buf, 80, fname);
	if (!rv) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %p but was %p\n",
			__FILE__, __func__, __LINE__, "buf", buf, rv);
		++errors;
	} else if (strcmp(buf, expect) != 0) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %s but was %s\n",
			__FILE__, __func__, __LINE__, "contents", expect, buf);
		++errors;

	}

	remove(fname);

	return errors;
}
