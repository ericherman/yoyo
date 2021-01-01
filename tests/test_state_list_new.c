#include "yoyo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yoyo_verbose;
extern void *(*yoyo_calloc)(size_t nmemb, size_t size);
extern void (*yoyo_free)(void *ptr);

struct error_injecting_mem_context {
	unsigned long allocs;
	unsigned long alloc_bytes;
	unsigned long frees;
	unsigned long free_bytes;
	unsigned long fails;
	unsigned long attempts;
	unsigned long attempts_to_fail_bitmask;
};

struct error_injecting_mem_context *global_mem_context = NULL;

void *err_calloc(size_t nmemb, size_t size)
{
	struct error_injecting_mem_context *ctx = global_mem_context;

	if (0x01 & (ctx->attempts_to_fail_bitmask >> ctx->attempts++)) {
		return NULL;
	}

	size_t wide = sizeof(size_t) + (nmemb * size);
	unsigned char *wide_buffer = (unsigned char *)malloc(wide);
	if (!wide_buffer) {
		++ctx->fails;
		return NULL;
	}

	memcpy(wide_buffer, &size, sizeof(size_t));
	++ctx->allocs;
	ctx->alloc_bytes += size;

	void *ptr = (void *)(wide_buffer + sizeof(size_t));
	return ptr;
}

void err_free(void *ptr)
{
	struct error_injecting_mem_context *ctx = global_mem_context;

	if (ptr == NULL) {
		return;
	}

	unsigned char *wide_buffer = ((unsigned char *)ptr) - sizeof(size_t);
	size_t size = 0;
	memcpy(&size, wide_buffer, sizeof(size_t));

	free(wide_buffer);

	ctx->free_bytes += size;
	++ctx->frees;
}

unsigned test_state_list_new_no_errors(void)
{
	unsigned failures = 0;

	struct error_injecting_mem_context ctx;
	memset(&ctx, 0x00, sizeof(struct error_injecting_mem_context));
	global_mem_context = &ctx;

	size_t len = 17;
	struct state_list *sl = state_list_new(len);

	if (!sl) {
		fprintf(stderr, "%s:%s:%d FAIL: state_list_new returned null\n",
			__FILE__, __func__, __LINE__);
		++failures;
	}

	if (!sl->states) {
		fprintf(stderr, "%s:%s:%d FAIL: sl->states is null\n",
			__FILE__, __func__, __LINE__);
		++failures;
	}

	if (sl->len != len) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %zu but was %zu\n",
			__FILE__, __func__, __LINE__, "sl->len", len, sl->len);
		++failures;
	}

	state_list_free(sl);

	if (!ctx.attempts) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %zu but was 0\n",
			__FILE__, __func__, __LINE__, "attempts", (size_t)2);
		++failures;
	}

	if (ctx.frees != ctx.allocs) {
		fprintf(stderr, "%s:%s:%d FAIL: %s (frees %zu, allocs %zu)\n",
			__FILE__, __func__, __LINE__, "frees != allocs",
			ctx.frees, ctx.allocs);
		++failures;
	}

	if (!ctx.alloc_bytes) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected non-zero\n",
			__FILE__, __func__, __LINE__, "alloc_bytes");
		++failures;
	}
	if (ctx.free_bytes != ctx.alloc_bytes) {
		fprintf(stderr,
			"%s:%s:%d FAIL: %s (free_bytes %zu, alloc_bytes %zu)\n",
			__FILE__, __func__, __LINE__,
			"free_bytes != alloc_bytes", ctx.free_bytes,
			ctx.alloc_bytes);
		++failures;
	}

	return failures;
}

unsigned test_state_list_new_first_error(void)
{
	unsigned failures = 0;

	struct error_injecting_mem_context ctx;
	memset(&ctx, 0x00, sizeof(struct error_injecting_mem_context));
	global_mem_context = &ctx;
	ctx.attempts_to_fail_bitmask = 0x01;

	size_t len = 17;
	struct state_list *sl = state_list_new(len);

	if (sl) {
		fprintf(stderr, "%s:%s:%d FAIL: %s exptected NULL, was %p\n",
			__FILE__, __func__, __LINE__, "state_list_new",
			(void *)sl);
		++failures;
	}

	state_list_free(sl);

	if (!ctx.attempts) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %zu but was 0\n",
			__FILE__, __func__, __LINE__, "attempts", (size_t)1);
		++failures;
	}

	if (ctx.frees != ctx.allocs) {
		fprintf(stderr, "%s:%s:%d FAIL: %s (frees %zu, allocs %zu)\n",
			__FILE__, __func__, __LINE__, "frees != allocs",
			ctx.frees, ctx.allocs);
		++failures;
	}

	if (ctx.free_bytes != ctx.alloc_bytes) {
		fprintf(stderr,
			"%s:%s:%d FAIL: %s (free_bytes %zu, alloc_bytes %zu)\n",
			__FILE__, __func__, __LINE__,
			"free_bytes != alloc_bytes", ctx.free_bytes,
			ctx.alloc_bytes);
		++failures;
	}

	return failures;
}

unsigned test_state_list_new_second_error(void)
{
	unsigned failures = 0;

	struct error_injecting_mem_context ctx;
	memset(&ctx, 0x00, sizeof(struct error_injecting_mem_context));
	global_mem_context = &ctx;
	ctx.attempts_to_fail_bitmask = (0x01 << 1);

	size_t len = 17;
	struct state_list *sl = state_list_new(len);

	if (sl) {
		fprintf(stderr, "%s:%s:%d FAIL: %s exptected NULL, was %p\n",
			__FILE__, __func__, __LINE__, "state_list_new",
			(void *)sl);
		++failures;
	}

	state_list_free(sl);

	if (!ctx.attempts) {
		fprintf(stderr, "%s:%s:%d FAIL: %s expected %zu but was 0\n",
			__FILE__, __func__, __LINE__, "attempts", (size_t)2);
		++failures;
	}

	if (ctx.frees != ctx.allocs) {
		fprintf(stderr, "%s:%s:%d FAIL: %s (frees %zu, allocs %zu)\n",
			__FILE__, __func__, __LINE__, "frees != allocs",
			ctx.frees, ctx.allocs);
		++failures;
	}

	if (ctx.free_bytes != ctx.alloc_bytes) {
		fprintf(stderr,
			"%s:%s:%d FAIL: %s (free_bytes %zu, alloc_bytes %zu)\n",
			__FILE__, __func__, __LINE__,
			"free_bytes != alloc_bytes", ctx.free_bytes,
			ctx.alloc_bytes);
		++failures;
	}
	return failures;
}

int main(void)
{
	unsigned failures = 0;

	yoyo_calloc = err_calloc;
	yoyo_free = err_free;

	failures += test_state_list_new_no_errors();

	yoyo_verbose = -1;
	failures += test_state_list_new_first_error();
	failures += test_state_list_new_second_error();

	return failures ? 1 : 0;
}
