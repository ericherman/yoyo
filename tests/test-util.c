#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

unsigned run_named_test(const char *name, unsigned (*func)(void))
{
	fprintf(stderr, name);
	fprintf(stderr, " ...");
	unsigned failures = func();
	if (failures) {
		fprintf(stderr, " %u failures. FAIL!\n", failures);
	} else {
		fprintf(stderr, " ok.\n");
	}
	return failures;
}

int failures_to_status(const char *name, unsigned failures)
{
	int exit_code;

	fprintf(stderr, "%s ", name);
	if (failures) {
		exit_code = EXIT_FAILURE;
		fprintf(stderr, "%u FAILURES\n", failures);
	} else {
		exit_code = EXIT_SUCCESS;
		fprintf(stderr, "SUCCESS\n");
	}

	return exit_code;
}
