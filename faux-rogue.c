
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* defined in yoyo.h */

enum action_type { succeed = 0, fail, hang };

enum action_type get_action(const char *failpath)
{
	int failcount = 0;
	if (failpath) {
		FILE *failfile = fopen(failpath, "r+");
		if (failfile) {
			fscanf(failfile, "%d", &failcount);
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
	if (failcount < 0) {
		fprintf(stderr, "hang count: %d\n", -failcount);
	} else {
		fprintf(stderr, "fail count: %d\n", failcount);
	}

	enum action_type action = succeed;
	if (failcount > 0) {
		action = fail;
	} else if (failcount < 0) {
		action = hang;
	}

	return action;
}

const unsigned ten_minutes = 10 * 60;

int main(int argc, char **argv)
{
	unsigned delay = (argc > 1) ? atoi(argv[1]) : 0;
	const char *failpath = (argc > 2) ? argv[2] : getenv("FAILCOUNT");

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
