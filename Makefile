# SPDX-License-Identifier: GPL-3.0-or-later
# Makefile for yoyo
#  a program to launch and re-launch another program if it hangs
# Copyright (C) 2020, 2021 Eric Herman <eric@freesa.org>

# $@ : target label
# $< : the first prerequisite after the colon
# $^ : all of the prerequisite files
# $* : wildcard matched part

BROWSER=firefox

FIXTURE_SLEEP ?= 1
WAIT_INTERVAL ?= 3
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


build/test_process_looks_hung: build/yoyo.o build/test-util.o \
		tests/test_process_looks_hung.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_process_looks_hung: build/test_process_looks_hung
	./build/test_process_looks_hung
	@echo "SUCCESS! ($@)"

debug/test_process_looks_hung: debug/yoyo.o debug/test-util.o \
		tests/test_process_looks_hung.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

valgrind_check_process_looks_hung: debug/test_process_looks_hung
	$(VALGRIND) ./debug/test_process_looks_hung >$@.out 2>&1
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f $@.out
	@echo "SUCCESS! ($@)"

build/test_qemu_states: build/yoyo.o build/test-util.o \
		tests/test_qemu_states.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_qemu_states: build/test_qemu_states
	./build/test_qemu_states
	@echo "SUCCESS! ($@)"

debug/test_qemu_states: debug/yoyo.o debug/test-util.o \
		tests/test_qemu_states.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

valgrind_check_qemu_states: debug/test_qemu_states
	$(VALGRIND) ./debug/test_qemu_states >$@.out 2>&1
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f $@.out
	@echo "SUCCESS! ($@)"

build/test_monitor_child_for_hang: build/yoyo.o build/test-util.o \
		tests/test_monitor_child_for_hang.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_monitor_child_for_hang: build/test_monitor_child_for_hang
	./build/test_monitor_child_for_hang
	@echo "SUCCESS! ($@)"

debug/test_monitor_child_for_hang: debug/yoyo.o debug/test-util.o \
		tests/test_monitor_child_for_hang.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

valgrind_check_monitor_child_for_hang: debug/test_monitor_child_for_hang
	$(VALGRIND) ./debug/test_monitor_child_for_hang >$@.out 2>&1
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f $@.out
	@echo "SUCCESS! ($@)"

build/test_slurp_text: build/yoyo.o build/test-util.o \
		tests/test_slurp_text.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_slurp_text: build/test_slurp_text
	./build/test_slurp_text
	@echo "SUCCESS! ($@)"

debug/test_slurp_text: debug/yoyo.o debug/test-util.o \
		tests/test_slurp_text.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

valgrind_check_slurp_text: debug/test_slurp_text
	$(VALGRIND) ./debug/test_slurp_text >$@.out 2>&1
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f $@.out
	@echo "SUCCESS! ($@)"

build/test_state_list_new: build/yoyo.o build/test-util.o \
		tests/test_state_list_new.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_state_list_new: build/test_state_list_new
	./build/test_state_list_new
	@echo "SUCCESS! ($@)"

debug/test_state_list_new: debug/yoyo.o debug/test-util.o \
		tests/test_state_list_new.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

valgrind_check_state_list_new: debug/test_state_list_new
	$(VALGRIND) ./debug/test_state_list_new >$@.out 2>&1
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f $@.out
	@echo "SUCCESS! ($@)"

build/test_yoyo_main: build/yoyo.o build/test-util.o \
		tests/test_yoyo_main.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_yoyo_main: build/test_yoyo_main
	./build/test_yoyo_main
	@echo "SUCCESS! ($@)"

debug/test_yoyo_main: debug/yoyo.o debug/test-util.o \
		tests/test_yoyo_main.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

valgrind_check_yoyo_main: debug/test_yoyo_main
	$(VALGRIND) ./debug/test_yoyo_main >$@.out 2>&1
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f $@.out
	@echo "SUCCESS! ($@)"

build/test_exit_reason: build/yoyo.o build/test-util.o \
		tests/test_exit_reason.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_exit_reason: build/test_exit_reason
	./build/test_exit_reason
	@echo "SUCCESS! ($@)"

debug/test_exit_reason: debug/yoyo.o debug/test-util.o \
		tests/test_exit_reason.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

valgrind_check_exit_reason: debug/test_exit_reason
	$(VALGRIND) ./debug/test_exit_reason >$@.out 2>&1
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -fv $@.out
	@echo "SUCCESS! ($@)"

check-unit: \
		check_exit_reason \
		check_yoyo_main \
		check_slurp_text \
		check_state_list_new \
		check_process_looks_hung \
		check_qemu_states \
		check_monitor_child_for_hang
	@echo "SUCCESS! ($@)"

valgrind-check-unit: \
		valgrind_check_exit_reason \
		valgrind_check_yoyo_main \
		valgrind_check_slurp_text \
		valgrind_check_state_list_new \
		valgrind_check_process_looks_hung \
		valgrind_check_qemu_states \
		valgrind_check_monitor_child_for_hang
	@echo "SUCCESS! ($@)"

build/faux-rogue: tests/faux-rogue.c
	mkdir -pv build
	$(CC) $(BUILD_CFLAGS) $^ -o $@

debug/faux-rogue: tests/faux-rogue.c
	mkdir -pv debug
	$(CC) $(DEBUG_CFLAGS) $^ -o $@


yoyo-version: ./build/yoyo
	@echo
	./build/yoyo --version >$@.out 2>&1
	grep -q '$(YOYO_VERSION)' $@.out
	rm -f $@.out
	@echo "SUCCESS! ($@)"

valgrind-yoyo-version: ./debug/yoyo
	@echo
	$(VALGRIND) ./debug/yoyo --version >$@.out 2>&1
	grep -q '$(YOYO_VERSION)' $@.out
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f $@.out
	@echo "SUCCESS! ($@)"

yoyo-help:./build/yoyo
	@echo
	./build/yoyo --help >$@.out 2>&1
	grep -q 'wait-interval' $@.out
	grep -q 'max-hangs' $@.out
	grep -q 'max-retries' $@.out
	grep -q 'verbose' $@.out
	rm -f $@.out
	@echo "SUCCESS! ($@)"

valgrind-yoyo-help: ./debug/yoyo
	@echo
	$(VALGRIND) ./debug/yoyo --help >$@.out 2>&1
	grep -q 'wait-interval' $@.out
	grep -q 'max-hangs' $@.out
	grep -q 'max-retries' $@.out
	grep -q 'verbose' $@.out
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f $@.out
	@echo "SUCCESS! ($@)"

check-acceptance-succeed-first-try: build/yoyo build/faux-rogue
	@echo
	@echo "succeed first try"
	echo "0" > tmp.$@.failcount
	WAIT_INTERVAL=$(WAIT_INTERVAL) \
	./build/yoyo \
		./build/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1
	grep -q '(succeed)' $@.out
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

valgrind-check-acceptance-succeed-first-try: debug/yoyo debug/faux-rogue
	@echo
	@echo "succeed first try"
	echo "0" > tmp.$@.failcount
	WAIT_INTERVAL=$(WAIT_INTERVAL) \
	VERBOSE=2 \
	$(VALGRIND) ./debug/yoyo \
		./debug/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1
	grep -q '(succeed)' $@.out
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

check-acceptance-fail-one-then-succeed: build/yoyo build/faux-rogue
	@echo
	@echo "fail once, then succeed"
	echo "1" > tmp.$@.failcount
	WAIT_INTERVAL=$(WAIT_INTERVAL) \
	./build/yoyo \
		./build/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1
	grep -q 'exited with status 127' $@.out
	grep -q '(succeed)' $@.out
	grep -q '0' tmp.$@.failcount
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

valgrind-check-acceptance-fail-one-then-succeed: debug/yoyo debug/faux-rogue
	@echo
	@echo "fail once, then succeed"
	echo "1" > tmp.$@.failcount
	WAIT_INTERVAL=$(WAIT_INTERVAL) \
	VERBOSE=2 \
	$(VALGRIND) ./debug/yoyo \
		./debug/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1
	grep -q 'exited with status 127' $@.out
	grep -q '(succeed)' $@.out
	grep -q '0' tmp.$@.failcount
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

check-acceptance-succeed-after-long-time: build/yoyo build/faux-rogue
	@echo
	@echo "succeed after a long time"
	echo "0" > tmp.$@.failcount
	WAIT_INTERVAL=$(WAIT_INTERVAL) \
	./build/yoyo \
		./build/faux-rogue $(FIXTURE_SLEEP_LONG) tmp.$@.failcount \
		>$@.out 2>&1
	grep -q '(succeed)' $@.out
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

valgrind-check-acceptance-succeed-after-long-time: debug/yoyo debug/faux-rogue
	@echo
	@echo "succeed after a long time"
	echo "0" > tmp.$@.failcount
	WAIT_INTERVAL=$(WAIT_INTERVAL) \
	VERBOSE=2 \
	$(VALGRIND) ./debug/yoyo \
		./debug/faux-rogue $(FIXTURE_SLEEP_LONG) tmp.$@.failcount \
		>$@.out 2>&1
	grep -q '(succeed)' $@.out
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

check-acceptance-hang-twice-then-succeed: build/yoyo build/faux-rogue
	echo "./build/faux-rogue will hang twice"
	echo "-2" > tmp.$@.failcount
	WAIT_INTERVAL=$(WAIT_INTERVAL) \
	./build/yoyo \
		./build/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1
	grep -q 'terminated by a signal 15' $@.out
	grep -q '(succeed)' $@.out
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

valgrind-check-acceptance-hang-twice-then-succeed: debug/yoyo debug/faux-rogue
	echo "./debug/faux-rogue will hang twice"
	echo "-2" > tmp.$@.failcount
	WAIT_INTERVAL=$(WAIT_INTERVAL) \
	VERBOSE=2 \
	$(VALGRIND) ./debug/yoyo \
		./debug/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1
	grep -q 'terminated by a signal 15' $@.out
	grep -q '(succeed)' $@.out
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

check-acceptance-fail-every-time: build/yoyo build/faux-rogue
	@echo
	echo "now ./build/faux-rogue will hang every time"
	echo "100" > tmp.$@.failcount
	-( WAIT_INTERVAL=$(WAIT_INTERVAL) \
	   VERBOSE=2 \
	   MAX_HANGS=3 \
	   MAX_RETRIES=2 \
	   ./build/yoyo \
		./build/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1 )
	grep -q 'Retries limit reached' $@.out
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

valgrind-check-acceptance-fail-every-time: debug/yoyo debug/faux-rogue
	@echo
	echo "now ./debug/faux-rogue will hang every time"
	echo "100" > tmp.$@.failcount
	-( WAIT_INTERVAL=$(WAIT_INTERVAL) \
	   VERBOSE=2 \
	   MAX_HANGS=3 \
	   MAX_RETRIES=2 \
		./debug/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1 )
	grep -q 'Retries limit reached' $@.out
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

check-acceptance-hang-every-time: build/yoyo build/faux-rogue
	@echo
	echo "now ./build/faux-rogue will hang every time"
	echo "-100" > tmp.$@.failcount
	-( WAIT_INTERVAL=$(WAIT_INTERVAL) \
	   VERBOSE=2 \
	   MAX_HANGS=3 \
	   MAX_RETRIES=2 \
	   ./build/yoyo \
		./build/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1 )
	grep -q 'Retries limit reached' $@.out
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

valgrind-check-acceptance-hang-every-time: debug/yoyo debug/faux-rogue
	@echo
	echo "now ./debug/faux-rogue will hang every time"
	echo "-100" > tmp.$@.failcount
	-( WAIT_INTERVAL=$(WAIT_INTERVAL) \
	   VERBOSE=2 \
	   MAX_HANGS=3 \
	   MAX_RETRIES=2 \
	   $(VALGRIND) ./debug/yoyo \
		./debug/faux-rogue $(FIXTURE_SLEEP) tmp.$@.failcount \
		>$@.out 2>&1 )
	grep -q 'Retries limit reached' $@.out
	if [ $$(grep -c 'definitely lost' $@.out) -eq 0 ]; \
		then true; else false; fi
	rm -f tmp.$@.failcount $@.out
	@echo "SUCCESS! ($@)"

check-acceptance: \
		yoyo-version \
		yoyo-help \
		check-acceptance-succeed-first-try \
		check-acceptance-fail-one-then-succeed \
		check-acceptance-succeed-after-long-time \
		check-acceptance-hang-twice-then-succeed \
		check-acceptance-fail-every-time \
		check-acceptance-hang-every-time
	@echo "SUCCESS! ($@)"

valgrind-check-acceptance: \
		valgrind-yoyo-version \
		valgrind-yoyo-help \
		valgrind-check-acceptance-succeed-first-try \
		valgrind-check-acceptance-fail-one-then-succeed \
		valgrind-check-acceptance-succeed-after-long-time \
		valgrind-check-acceptance-hang-twice-then-succeed \
		valgrind-check-acceptance-fail-every-time \
		valgrind-check-acceptance-hang-every-time
	@echo "SUCCESS! ($@)"

coverage.info: valgrind-check-unit
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

check: default check-unit check-code-coverage
	@echo "SUCCESS! ($@)"

check-all: check check-acceptance html-report
	@echo "SUCCESS! ($@)"

tidy:
	$(LINDENT) \
		-T FILE -T pid_t \
		-T error_injecting_mem_context \
		-T exit_reason \
		-T monitor_child_context \
		-T state_list \
		-T thread_state \
		-T yoyo_options \
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
