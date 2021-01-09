/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2021 Eric Herman <eric@freesa.org> and
	Brett Neumeier <brett@freesa.org> */

#include "yoyo.h"
#include "test-util.h"

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

const unsigned hang_check_interval = 60;
const char *fakeroot = NULL;
const pid_t childpid = 174993;

const size_t qemu_state_0_len = 10;
struct thread_state qemu_state_0[] = {
	{.pid = 174993,.state = 'S',.utime = 42833,.stime = 125400 },
	{.pid = 174994,.state = 'S',.utime = 12376,.stime = 1809 },
	{.pid = 175000,.state = 'S',.utime = 4306567,.stime = 219019 },
	{.pid = 175001,.state = 'S',.utime = 4197739,.stime = 220016 },
	{.pid = 175002,.state = 'S',.utime = 4492751,.stime = 220595 },
	{.pid = 175003,.state = 'S',.utime = 4194664,.stime = 222319 },
	{.pid = 175004,.state = 'S',.utime = 4197278,.stime = 222898 },
	{.pid = 175005,.state = 'S',.utime = 4213710,.stime = 222763 },
	{.pid = 175006,.state = 'S',.utime = 4178210,.stime = 226003 },
	{.pid = 175007,.state = 'S',.utime = 4091083,.stime = 221455 }
};

struct state_list qemu_state_list_0 = {
	.states = qemu_state_0,
	.len = qemu_state_0_len
};

const size_t qemu_state_1_len = 10;
struct thread_state qemu_state_1[] = {
	{.pid = 174993,.state = 'S',.utime = 42835,.stime = 125400 },
	{.pid = 174994,.state = 'S',.utime = 12376,.stime = 1809 },
	{.pid = 175000,.state = 'S',.utime = 4306567,.stime = 219019 },
	{.pid = 175001,.state = 'S',.utime = 4197739,.stime = 220016 },
	{.pid = 175002,.state = 'S',.utime = 4492752,.stime = 220595 },
	{.pid = 175003,.state = 'S',.utime = 4194668,.stime = 222319 },
	{.pid = 175004,.state = 'S',.utime = 4197278,.stime = 222898 },
	{.pid = 175005,.state = 'S',.utime = 4213710,.stime = 222763 },
	{.pid = 175006,.state = 'S',.utime = 4178210,.stime = 226003 },
	{.pid = 175007,.state = 'S',.utime = 4091083,.stime = 221455 }
};

struct state_list qemu_state_list_1 = {
	.states = qemu_state_1,
	.len = qemu_state_1_len
};

const size_t qemu_state_2_len = 10;
struct thread_state qemu_state_2[] = {
	{.pid = 174993,.state = 'S',.utime = 42837,.stime = 125400 },
	{.pid = 174994,.state = 'S',.utime = 12376,.stime = 1809 },
	{.pid = 175000,.state = 'S',.utime = 4306567,.stime = 219019 },
	{.pid = 175001,.state = 'S',.utime = 4197739,.stime = 220016 },
	{.pid = 175002,.state = 'S',.utime = 4492753,.stime = 220595 },
	{.pid = 175003,.state = 'S',.utime = 4194671,.stime = 222319 },
	{.pid = 175004,.state = 'S',.utime = 4197278,.stime = 222898 },
	{.pid = 175005,.state = 'S',.utime = 4213710,.stime = 222763 },
	{.pid = 175006,.state = 'S',.utime = 4178210,.stime = 226003 },
	{.pid = 175007,.state = 'S',.utime = 4091084,.stime = 221455 }
};

struct state_list qemu_state_list_2 = {
	.states = qemu_state_2,
	.len = qemu_state_2_len
};

const size_t qemu_state_3_len = 10;
struct thread_state qemu_state_3[] = {
	{.pid = 174993,.state = 'S',.utime = 42839,.stime = 125400 },
	{.pid = 174994,.state = 'S',.utime = 12376,.stime = 1809 },
	{.pid = 175000,.state = 'S',.utime = 4306567,.stime = 219019 },
	{.pid = 175001,.state = 'S',.utime = 4197739,.stime = 220016 },
	{.pid = 175002,.state = 'S',.utime = 4492753,.stime = 220595 },
	{.pid = 175003,.state = 'S',.utime = 4194674,.stime = 222319 },
	{.pid = 175004,.state = 'S',.utime = 4197278,.stime = 222898 },
	{.pid = 175005,.state = 'S',.utime = 4213710,.stime = 222763 },
	{.pid = 175006,.state = 'S',.utime = 4178210,.stime = 226003 },
	{.pid = 175007,.state = 'S',.utime = 4091084,.stime = 221455 }
};

struct state_list qemu_state_list_3 = {
	.states = qemu_state_3,
	.len = qemu_state_3_len
};

const size_t qemu_state_4_len = 10;
struct thread_state qemu_state_4[] = {
	{.pid = 174993,.state = 'S',.utime = 42841,.stime = 125400 },
	{.pid = 174994,.state = 'S',.utime = 12376,.stime = 1809 },
	{.pid = 175000,.state = 'S',.utime = 4306567,.stime = 219019 },
	{.pid = 175001,.state = 'S',.utime = 4197739,.stime = 220016 },
	{.pid = 175002,.state = 'S',.utime = 4492754,.stime = 220595 },
	{.pid = 175003,.state = 'S',.utime = 4194678,.stime = 222319 },
	{.pid = 175004,.state = 'S',.utime = 4197278,.stime = 222898 },
	{.pid = 175005,.state = 'S',.utime = 4213710,.stime = 222763 },
	{.pid = 175006,.state = 'S',.utime = 4178210,.stime = 226003 },
	{.pid = 175007,.state = 'S',.utime = 4091084,.stime = 221455 }
};

struct state_list qemu_state_list_4 = {
	.states = qemu_state_4,
	.len = qemu_state_4_len
};

const size_t hung_qemu_frames_len = 5;
struct state_list *hung_qemu_frames[] = {
	&qemu_state_list_0,
	&qemu_state_list_1,
	&qemu_state_list_2,
	&qemu_state_list_3,
	&qemu_state_list_4
};

struct monitor_child_context {
	unsigned *failures;
	struct state_list **templates;
	size_t templates_len;
	unsigned sleep_count;
	size_t current_state;
	unsigned looks_hung;
};

struct monitor_child_context *ctx = NULL;

/* some function prototypes */
void copy_templates(struct monitor_child_context *dup,
		    struct state_list **templates, size_t templates_len);
void free_templates(struct monitor_child_context *dup);
size_t max_hangs_for_len(size_t len);

/* Test Functions */
unsigned test_qemu_hung(void)
{
	unsigned failures = 0;

	struct monitor_child_context mctx;
	memset(&mctx, 0x00, sizeof(struct monitor_child_context));

	mctx.failures = &failures;

	copy_templates(&mctx, hung_qemu_frames, hung_qemu_frames_len);
	ctx = &mctx;

	unsigned max_hangs = max_hangs_for_len(ctx->templates_len);

	monitor_child_for_hang(childpid, max_hangs, hang_check_interval,
			       fakeroot);

	if (!ctx->looks_hung) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected non-zero\n",
			__FILE__, __func__, __LINE__, "ctx->looks_hung");
		++failures;
	}

	free_templates(ctx);
	return failures;
}

unsigned test_qemu_active_state_4(void)
{
	unsigned failures = 0;

	struct monitor_child_context mctx;
	memset(&mctx, 0x00, sizeof(struct monitor_child_context));

	mctx.failures = &failures;

	copy_templates(&mctx, hung_qemu_frames, hung_qemu_frames_len);
	mctx.templates[4]->states[9].utime += 10;
	ctx = &mctx;

	unsigned max_hangs = max_hangs_for_len(ctx->templates_len);

	monitor_child_for_hang(childpid, max_hangs, hang_check_interval,
			       fakeroot);

	if (ctx->looks_hung) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected 0 but was %u\n",
			__FILE__, __func__, __LINE__, "ctx->looks_hung",
			ctx->looks_hung);
		++failures;
	}

	free_templates(ctx);
	return failures;
}

/* Test Fixture Functions */

#define Calloc_or_die(pptr, nmemb, size) do { \
	size_t _cod_nmeb = (nmemb); \
	size_t _cod_size = (size); \
	void *_cod_ptr = *(pptr) = calloc(_cod_nmeb, _cod_size); \
	if (!_cod_ptr) { \
		size_t _cod_tot = (_cod_nmeb * _cod_size); \
		fprintf(stderr, "%s:%s:%d could not calloc(%zu, %zu)" \
				" (%zu bytes) for %s?\n", \
			__FILE__, __func__, __LINE__, _cod_nmeb, _cod_size, \
				_cod_tot, #pptr); \
		exit(1); \
	} \
} while (0)

void copy_templates(struct monitor_child_context *dup,
		    struct state_list **templates, size_t templates_len)
{
	dup->templates_len = templates_len;
	size_t nmemb = dup->templates_len;
	size_t size = sizeof(struct state_list *);
	Calloc_or_die(&dup->templates, nmemb, size);

	for (size_t i = 0; i < dup->templates_len; ++i) {
		nmemb = 1;
		size = sizeof(struct state_list);
		Calloc_or_die(&(dup->templates[i]), nmemb, size);

		dup->templates[i]->len = templates[i]->len;
		nmemb = dup->templates[i]->len;
		size = sizeof(struct thread_state);
		Calloc_or_die(&dup->templates[i]->states, nmemb, size);

		for (size_t j = 0; j < dup->templates[i]->len; ++j) {
			dup->templates[i]->states[j] = templates[i]->states[j];
		}
	}
}

void free_templates(struct monitor_child_context *dup)
{
	for (size_t i = 0; i < dup->templates_len; ++i) {
		free(dup->templates[i]->states);
		dup->templates[i]->states = NULL;
		dup->templates[i]->len = 0;

		free(dup->templates[i]);
		dup->templates[i] = NULL;
	}
	free(dup->templates);
	dup->templates = NULL;
	dup->templates_len = 0;
	dup = NULL;
}

size_t max_hangs_for_len(size_t len)
{
	if (len < 2) {
		fprintf(stderr, "%s:%s:%d ERROR! len: %zu\n",
			__FILE__, __func__, __LINE__, len);
		fprintf(stderr, "The first check is _never_ considered hung\n");
		fprintf(stderr, " and there must be a 'next' to compare to.\n");
		fprintf(stderr, "Thus the max_hangs for any size is the\n");
		fprintf(stderr, " length minus the first AND last.\n");
		exit(EXIT_FAILURE);
	}
	return len - 2;
}

struct state_list empty_state_list = {.states = NULL,.len = 0 };

struct state_list *faux_get_states(long pid, const char *fakeroot)
{
	(void)pid;
	(void)fakeroot;

	return (ctx->current_state < ctx->templates_len)
	    ? ctx->templates[ctx->current_state]
	    : &empty_state_list;
}

void faux_free_states(struct state_list *l)
{
	(void)l;
}

int faux_kill(pid_t pid, int sig)
{
	(void)pid;

	/* record data about what it is doing */
	if (sig != 0) {
		++(ctx->looks_hung);
	}

	return ctx->current_state < ctx->templates_len ? 0 : -1;
}

unsigned int faux_sleep(unsigned int seconds)
{
	if (seconds) {
		if (ctx->sleep_count) {
			++ctx->current_state;
		}
		++ctx->sleep_count;
	}

	size_t threshold = ctx->templates_len;
	if (ctx->current_state > threshold) {
		fprintf(stderr, "%s:%s:%d sleep(%u) threshold %zu exceeded\n",
			__FILE__, __func__, __LINE__, seconds, threshold);
		exit(EXIT_FAILURE);
	}

	return ctx->looks_hung ? 1 : 0;
}

extern int (*yoyo_kill)(pid_t pid, int sig);
extern unsigned int (*yoyo_sleep)(unsigned int seconds);
extern struct state_list *(*get_states) (long pid, const char *fakeroot);
extern void (*free_states)(struct state_list *l);

int main(void)
{
	unsigned failures = 0;

	yoyo_kill = faux_kill;
	yoyo_sleep = faux_sleep;
	get_states = faux_get_states;
	free_states = faux_free_states;

	failures += run_test(test_qemu_hung);
	failures += run_test(test_qemu_active_state_4);

	return failures_to_status("test_qemu_states", failures);
}
