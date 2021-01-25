# SPDX-License-Identifier: GPL-3.0-or-later
# Makefile for yoyo
#  a program to launch and re-launch another program if it hangs
# Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org> and
#    Brett Neumeier <brett@freesa.org>

# $@ : target label
# $< : the first prerequisite after the colon
# $^ : all of the prerequisite files
# $* : wildcard matched part
# Target-specific Variable syntax:
# https://www.gnu.org/software/make/manual/html_node/Target_002dspecific.html
#
# patsubst : $(patsubst pattern,replacement,text)
#	https://www.gnu.org/software/make/manual/html_node/Text-Functions.html

BROWSER=firefox

FIXTURE_SLEEP ?= 1
HANG_CHECK_INTERVAL ?= 3
FIXTURE_SLEEP_LONG ?= 4

COMMON_CFLAGS += -g -Wall -Wextra -pedantic -Werror -I./src $(CFLAGS)

BUILD_CFLAGS += -DNDEBUG -O2 $(COMMON_CFLAGS)

DEBUG_CFLAGS += -DDEBUG -O0 $(COMMON_CFLAGS) \
	-fno-inline-small-functions \
	-fkeep-inline-functions \
	-fkeep-static-functions \
	--coverage

SHELL=/bin/bash

VALGRIND=$(shell which valgrind)

YOYO_VERSION=$(shell \
	     grep '^const char \*yoyo_version =' src/yoyo.c \
	     | cut -f2 -d'"')

# extracted from https://github.com/torvalds/linux/blob/master/scripts/Lindent
LINDENT=indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 -il0
# see also: https://www.kernel.org/doc/Documentation/process/coding-style.rst

default: build/faux-rogue build/yoyo

UNIT_TEST_BASE_NAMES = exit_reason \
	yoyo_main \
	slurp_text \
	state_list_new \
	process_looks_hung \
	qemu_states \
	term_then_kill \
	monitor_child_for_hang

# Target-specific Variables:
check-acceptance-%: BUILD_DIR = build
valgrind-acceptance-%: BUILD_DIR = debug
check-acceptance-%: WRAPPER =
valgrind-acceptance-%: WRAPPER = YOYO_VERBOSE=2 $(VALGRIND)
check%: EXTRA_CHECK = true
valgrind%: EXTRA_CHECK = if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi && \
		if [ $$(grep -c 'probably lost' $@.out) -eq 0 ]; \
		then true; else false; fi

tests: $(patsubst %, build/test_%, $(UNIT_TEST_BASE_NAMES)) \
	$(patsubst %, debug/test_%, $(UNIT_TEST_BASE_NAMES))

UNIT_TEST_TARGETS: $(patsubst %, check_%, $(UNIT_TEST_BASE_NAMES))
VALGRIND_UNIT_TEST_TARGETS: $(patsubst %, valgrind_%, $(UNIT_TEST_BASE_NAMES))


build/yoyo.o: src/yoyo.c src/yoyo.h
	mkdir -pv build
	$(CC) -c $(BUILD_CFLAGS) $< -o $@

debug/yoyo.o: src/yoyo.c src/yoyo.h
	mkdir -pv debug
	$(CC) -c $(DEBUG_CFLAGS) $< -o $@

build/yoyo: build/yoyo.o src/yoyo-main.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@
	ls -l build/yoyo

debug/yoyo: debug/yoyo.o src/yoyo-main.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@
	ls -l debug/yoyo


build/test-util.o: tests/test-util.c tests/test-util.h
	mkdir -pv build
	$(CC) -c $(BUILD_CFLAGS) $< -o $@

debug/test-util.o: tests/test-util.c tests/test-util.h
	mkdir -pv debug
	$(CC) -c $(DEBUG_CFLAGS) $< -o $@

build/test_%: build/yoyo.o build/test-util.o tests/test_%.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

debug/test_%: debug/yoyo.o debug/test-util.o tests/test_%.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

check_%: build/test_%
	./$<
	@echo "SUCCESS! ($@)"

valgrind_%: debug/test_%
	$(VALGRIND) ./$< >$@.out 2>&1
	$(EXTRA_CHECK)
	@echo "SUCCESS! ($@)"

check-unit: UNIT_TEST_TARGETS
	@echo "SUCCESS! ($@)"

valgrind-unit: VALGRIND_UNIT_TEST_TARGETS
	@echo "SUCCESS! ($@)"


build/faux-rogue: tests/faux-rogue.c
	mkdir -pv build
	$(CC) $(BUILD_CFLAGS) $^ -o $@

debug/faux-rogue: tests/faux-rogue.c
	mkdir -pv debug
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

ACCEPTANCE_DEPS = build/yoyo build/faux-rogue \
		debug/yoyo debug/faux-rogue

check-acceptance-yoyo-version valgrind-acceptance-yoyo-version: \
		$(ACCEPTANCE_DEPS)
	@echo
	$(WRAPPER) $(BUILD_DIR)/yoyo \
		--version \
		>$@.out 2>&1
	grep -q '$(YOYO_VERSION)' $@.out
	$(EXTRA_CHECK)
	rm -f $@.out
	@echo "SUCCESS! ($@)"

check-acceptance-yoyo-help valgrind-acceptance-yoyo-help: \
		$(ACCEPTANCE_DEPS)
	@echo
	$(WRAPPER) $(BUILD_DIR)/yoyo \
		--help \
		>$@.out 2>&1
	grep -q 'help' $@.out
	grep -q 'version' $@.out
	$(EXTRA_CHECK)
	rm -f $@.out
	@echo "SUCCESS! ($@)"

check-acceptance-succeed-first-try valgrind-acceptance-succeed-first-try: \
		$(ACCEPTANCE_DEPS)
	@echo
	@echo "succeed first try"
	echo "0" > tmp.$@.failcount
	YOYO_HANG_CHECK_INTERVAL=$(HANG_CHECK_INTERVAL) \
	$(WRAPPER) $(BUILD_DIR)/yoyo \
		$(BUILD_DIR)/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1
	grep -q '(succeed)' $@.out
	$(EXTRA_CHECK)
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

check-acceptance-fail-one-then-succeed valgrind-acceptance-fail-one-then-succeed: \
		$(ACCEPTANCE_DEPS)
	@echo
	@echo "fail once, then succeed"
	echo "1" > tmp.$@.failcount
	YOYO_HANG_CHECK_INTERVAL=$(HANG_CHECK_INTERVAL) \
	$(WRAPPER) $(BUILD_DIR)/yoyo \
		$(BUILD_DIR)/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1
	grep -q 'exited with status 127' $@.out
	grep -q '(succeed)' $@.out
	grep -q '0' tmp.$@.failcount
	$(EXTRA_CHECK)
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

check-acceptance-succeed-after-long-time valgrind-acceptance-succeed-after-long-time: \
		$(ACCEPTANCE_DEPS)
	@echo
	@echo "succeed after a long time"
	echo "0" > tmp.$@.failcount
	YOYO_HANG_CHECK_INTERVAL=$(HANG_CHECK_INTERVAL) \
	$(WRAPPER) $(BUILD_DIR)/yoyo \
		$(BUILD_DIR)/faux-rogue $(FIXTURE_SLEEP_LONG) tmp.$@.failcount \
		>$@.out 2>&1
	grep -q '(succeed)' $@.out
	$(EXTRA_CHECK)
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

check-acceptance-hang-twice-then-succeed valgrind-acceptance-hang-twice-then-succeed: \
		$(ACCEPTANCE_DEPS)
	echo "$(BUILD_DIR)/faux-rogue will hang twice"
	echo "-2" > tmp.$@.failcount
	YOYO_HANG_CHECK_INTERVAL=$(HANG_CHECK_INTERVAL) \
	$(WRAPPER) $(BUILD_DIR)/yoyo \
		$(BUILD_DIR)/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1
	if [ $$(grep -c "^Child '$(BUILD_DIR)/faux-rogue' killed" $@.out) -eq 2 ]; \
		then true; else false; fi
	grep -q '(succeed)' $@.out
	$(EXTRA_CHECK)
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

check-acceptance-fail-every-time valgrind-acceptance-fail-every-time: \
		$(ACCEPTANCE_DEPS)
	@echo
	echo "now $(BUILD_DIR)/faux-rogue will fail every time"
	echo "100" > tmp.$@.failcount
	-( YOYO_HANG_CHECK_INTERVAL=$(HANG_CHECK_INTERVAL) \
	   MAX_HANGS=3 \
	   YOYO_MAX_RETRIES=2 \
	   $(WRAPPER) $(BUILD_DIR)/yoyo \
		$(BUILD_DIR)/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1 )
	grep -q 'Retries limit reached' $@.out
	$(EXTRA_CHECK)
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

check-acceptance-hang-every-time valgrind-acceptance-hang-every-time: \
		$(ACCEPTANCE_DEPS)
	@echo
	echo "now $(BUILD_DIR)/faux-rogue will hang every time"
	echo "-100" > tmp.$@.failcount
	-( YOYO_HANG_CHECK_INTERVAL=$(HANG_CHECK_INTERVAL) \
	   YOYO_MAX_HANGS=3 \
	   YOYO_MAX_RETRIES=2 \
	   $(WRAPPER) $(BUILD_DIR)/yoyo \
		$(BUILD_DIR)/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1 )
	grep -q 'Retries limit reached' $@.out
	$(EXTRA_CHECK)
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

check-acceptance: \
		check-acceptance-yoyo-version \
		check-acceptance-yoyo-help \
		check-acceptance-succeed-first-try \
		check-acceptance-fail-one-then-succeed \
		check-acceptance-succeed-after-long-time \
		check-acceptance-hang-twice-then-succeed \
		check-acceptance-fail-every-time \
		check-acceptance-hang-every-time
	@echo "SUCCESS! ($@)"

valgrind-acceptance: \
		valgrind-acceptance-yoyo-version \
		valgrind-acceptance-yoyo-help \
		valgrind-acceptance-succeed-first-try \
		valgrind-acceptance-fail-one-then-succeed \
		valgrind-acceptance-succeed-after-long-time \
		valgrind-acceptance-hang-twice-then-succeed \
		valgrind-acceptance-fail-every-time \
		valgrind-acceptance-hang-every-time
	@echo "SUCCESS! ($@)"

coverage.info: valgrind-unit
	lcov    --checksum \
		--capture \
		--base-directory . \
		--directory . \
		--output-file coverage.info
	ls -l coverage.info

html-report: coverage.info
	mkdir -pv ./coverage_html
	genhtml coverage.info --output-directory coverage_html
	ls -l ./coverage_html/src/yoyo.c.gcov.html

check-code-coverage: html-report
	# expect two: one for lines, one for functions
	if [ $$(grep -c 'headerCovTableEntryHi">100.0' \
		./coverage_html/src/yoyo.c.gcov.html) -eq 2 ]; \
		then true; else false; fi
	@echo "SUCCESS! ($@)"

coverage: html-report
	$(BROWSER) ./coverage_html/src/yoyo.c.gcov.html

valgrind: valgrind-unit valgrind-acceptance
	@echo "SUCCESS! ($@)"

check: default check-unit
	@echo "SUCCESS! ($@)"

check-dev: check valgrind-unit check-code-coverage
	@echo "SUCCESS! ($@)"

check-all: check-dev check-acceptance
	@echo "SUCCESS! ($@)"

tidy:
	$(LINDENT) \
		-T FILE -T pid_t \
		-T error_injecting_mem_context \
		-T exit_reason \
		-T monitor_child_context \
		-T state_list \
		-T thread_state \
		-T fork_func \
		-T execv_func \
		-T sighandler_func \
		-T signal_func \
		-T monitor_for_hang_func \
		src/*.c src/*.h tests/*.c tests/*.h

clean:
	rm -rvf build debug `cat .gitignore | sed -e 's/#.*//'`
	pushd src; rm -rvf `cat ../.gitignore | sed -e 's/#.*//'`; popd
	pushd tests; rm -rvf `cat ../.gitignore | sed -e 's/#.*//'`; popd

mrproper:
	git clean -dffx
	git submodule foreach --recursive git clean -dffx
