/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> */

#include "yoyo.h"
#include "test-util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

extern int yoyo_verbose;
extern void *(*yoyo_calloc)(size_t nmemb, size_t size);
extern void (*yoyo_free)(void *ptr);
extern struct state_list *(*get_states) (long pid);
extern void (*free_states)(struct state_list *l);
extern int yoyo_verbose;
extern FILE *yoyo_stdout;
extern FILE *yoyo_stderr;

struct error_injecting_mem_context {
	unsigned long allocs;
	unsigned long alloc_bytes;
	unsigned long frees;
	unsigned long free_bytes;
	unsigned long fails;
	unsigned long attempts;
	unsigned long attempts_to_fail_bitmask;
};
struct error_injecting_mem_context global_mem_context;
struct error_injecting_mem_context *mem = &global_mem_context;

void *err_calloc(size_t nmemb, size_t size)
{
	if (0x01 & (mem->attempts_to_fail_bitmask >> mem->attempts++)) {
		return NULL;
	}

	size_t wide = sizeof(size_t) + (nmemb * size);
	unsigned char *wide_buffer = (unsigned char *)malloc(wide);
	if (!wide_buffer) {
		++mem->fails;
		return NULL;
	}

	memcpy(wide_buffer, &size, sizeof(size_t));
	++mem->allocs;
	mem->alloc_bytes += size;

	void *ptr = (void *)(wide_buffer + sizeof(size_t));
	return ptr;
}

void err_free(void *ptr)
{
	if (ptr == NULL) {
		return;
	}

	unsigned char *wide_buffer = ((unsigned char *)ptr) - sizeof(size_t);
	size_t size = 0;
	memcpy(&size, wide_buffer, sizeof(size_t));

	free(wide_buffer);

	mem->free_bytes += size;
	++mem->frees;
}

unsigned test_state_list_new_no_errors(void)
{
	unsigned failures = 0;

	memset(mem, 0x00, sizeof(struct error_injecting_mem_context));

	size_t len = 17;
	struct state_list *sl = state_list_new(len);

	failures += Check(sl, "state_list_new returned null");

	if (sl) {
		failures += Check(sl->states, "sl->states is null");
		failures +=
		    Check(sl->len == len, "expected %zu but was %zu", len,
			  sl->len);
	}

	state_list_free(sl);

	failures += Check(mem->attempts, "expected alloc attempts");

	failures +=
	    Check(mem->frees == mem->allocs, "(frees %zu, allocs %zu)",
		  mem->frees, mem->allocs);

	failures += Check(mem->alloc_bytes, "expected non-zero");

	failures +=
	    Check(mem->free_bytes == mem->alloc_bytes,
		  "(free_bytes %zu, alloc_bytes %zu)", mem->free_bytes,
		  mem->alloc_bytes);

	return failures;
}

unsigned test_state_list_pid(void)
{
	unsigned failures = 0;

	memset(mem, 0x00, sizeof(struct error_injecting_mem_context));

	pid_t pid = getpid();

	char buf[250];
	memset(buf, 0x00, 250);
	FILE *fbuf = fmemopen(buf, 250, "w");
	yoyo_verbose = 2;
	yoyo_stdout = fbuf;
	yoyo_stderr = fbuf;

	struct state_list *sl = get_states(pid);

	fflush(fbuf);
	fclose(fbuf);
	yoyo_verbose = 0;
	yoyo_stdout = NULL;
	yoyo_stderr = NULL;

	failures += Check(sl, "state_list_new returned null");

	if (sl) {
		failures += Check(sl->states, "sl->states is null");
	}
	if (sl && sl->states) {
		failures +=
		    Check(sl->states[0].state == 'R', "expected %c but was %c",
			  'R', sl->states[0].state);
	}

	state_list_free(sl);

	failures +=
	    Check(mem->frees == mem->allocs, "(frees %zu, allocs %zu)",
		  mem->frees, mem->allocs);

	failures +=
	    Check(mem->free_bytes == mem->alloc_bytes,
		  "(free_bytes %zu, alloc_bytes %zu)", mem->free_bytes,
		  mem->alloc_bytes);

	return failures;
}

unsigned test_state_list_len_zero(void)
{
	unsigned failures = 0;

	memset(mem, 0x00, sizeof(struct error_injecting_mem_context));

	size_t len = 0;
	struct state_list *sl = state_list_new(len);

	failures += Check(sl, "state_list_new returned null");

	state_list_free(sl);

	failures +=
	    Check(mem->frees == mem->allocs, "(frees %zu, allocs %zu)",
		  mem->frees, mem->allocs);

	failures +=
	    Check(mem->free_bytes == mem->alloc_bytes,
		  "(free_bytes %zu, alloc_bytes %zu)", mem->free_bytes,
		  mem->alloc_bytes);

	return failures;
}

unsigned test_state_list_pid_not_exists(void)
{
	unsigned failures = 0;

	memset(mem, 0x00, sizeof(struct error_injecting_mem_context));

	pid_t bogus_pid = -1;

	char buf[250];
	memset(buf, 0x00, 250);
	FILE *fbuf = fmemopen(buf, 250, "w");
	yoyo_verbose = 2;
	yoyo_stdout = fbuf;
	yoyo_stderr = fbuf;

	struct state_list *sl = get_states(bogus_pid);

	fflush(fbuf);
	fclose(fbuf);
	yoyo_verbose = 0;
	yoyo_stdout = NULL;
	yoyo_stderr = NULL;

	const char *expect = "No such file or directory";
	failures +=
	    Check(strstr(buf, expect), "'%s' not in:\n%s\n", buf, expect);

	failures += Check(sl, "state_list_new returned null");

	state_list_free(sl);

	failures +=
	    Check(mem->frees == mem->allocs, "(frees %zu, allocs %zu)",
		  mem->frees, mem->allocs);

	failures +=
	    Check(mem->free_bytes == mem->alloc_bytes,
		  "(free_bytes %zu, alloc_bytes %zu)", mem->free_bytes,
		  mem->alloc_bytes);

	return failures;
}

unsigned test_state_list_new_first_error(void)
{
	unsigned failures = 0;

	memset(mem, 0x00, sizeof(struct error_injecting_mem_context));
	mem->attempts_to_fail_bitmask = 0x01;

	size_t len = 17;
	struct state_list *sl = state_list_new(len);

	failures += Check(sl == NULL, "expected NULL, was %p", sl);

	state_list_free(sl);

	failures += Check(mem->attempts, "expected alloc-attemts");

	failures +=
	    Check(mem->frees == mem->allocs, "(frees %zu, allocs %zu)",
		  mem->frees, mem->allocs);

	failures +=
	    Check(mem->free_bytes == mem->alloc_bytes,
		  "(free_bytes %zu, alloc_bytes %zu)", mem->free_bytes,
		  mem->alloc_bytes);

	return failures;
}

unsigned test_state_list_new_second_error(void)
{
	unsigned failures = 0;

	memset(mem, 0x00, sizeof(struct error_injecting_mem_context));
	mem->attempts_to_fail_bitmask = (0x01 << 1);

	size_t len = 17;
	struct state_list *sl = state_list_new(len);

	failures += Check(sl == NULL, "expected NULL, was %p", sl);

	state_list_free(sl);

	failures += Check(mem->attempts, "expected alloc-attemts");

	failures +=
	    Check(mem->frees == mem->allocs, "(frees %zu, allocs %zu)",
		  mem->frees, mem->allocs);

	failures +=
	    Check(mem->free_bytes == mem->alloc_bytes,
		  "(free_bytes %zu, alloc_bytes %zu)", mem->free_bytes,
		  mem->alloc_bytes);

	return failures;
}

int main(void)
{
	unsigned failures = 0;

	yoyo_calloc = err_calloc;
	yoyo_free = err_free;

	failures += run_test(test_state_list_new_no_errors);
	failures += run_test(test_state_list_pid);
	failures += run_test(test_state_list_len_zero);
	failures += run_test(test_state_list_pid_not_exists);

	yoyo_verbose = -1;
	failures += run_test(test_state_list_new_first_error);
	failures += run_test(test_state_list_new_second_error);

	return failures_to_status("test_state_list_new", failures);
}
