# $@ : target label
# $< : the first prerequisite after the colon
# $^ : all of the prerequisite files
# $* : wildcard matched part

BROWSER=firefox

YOYO_BIN=yoyo

FIXTURE_SLEEP ?= 2
WAIT_INTERVAL ?= 5
FIXTURE_SLEEP_LONG ?= 8

COMMON_CFLAGS += -g -Wall -Wextra -pedantic -Werror -I./src $(CFLAGS)

BUILD_CFLAGS += -DNDEBUG -O2 $(COMMON_CFLAGS)

DEBUG_CFLAGS += -DDEBUG -O0 $(COMMON_CFLAGS) \
	-fno-inline-small-functions \
	-fkeep-inline-functions \
	-fkeep-static-functions \
	--coverage

SHELL=/bin/bash

# extracted from https://github.com/torvalds/linux/blob/master/scripts/Lindent
LINDENT=indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 -il0
# see also: https://www.kernel.org/doc/Documentation/process/coding-style.rst

default: build/$(YOYO_BIN)

build/yoyo.o: src/yoyo.c src/yoyo.h
	mkdir -pv build
	$(CC) -c $(BUILD_CFLAGS) $< -o $@

debug/yoyo.o: src/yoyo.c src/yoyo.h
	mkdir -pv debug
	$(CC) -c $(DEBUG_CFLAGS) $< -o $@

build/$(YOYO_BIN): build/yoyo.o src/yoyo-main.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@
	ls -l build/$(YOYO_BIN)

debug/$(YOYO_BIN): debug/yoyo.o src/yoyo-main.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@
	ls -l debug/$(YOYO_BIN)

build/test_yoyo_parse_command_line: build/yoyo.o \
		tests/test_yoyo_parse_command_line.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_build_yoyo_parse_command_line: build/test_yoyo_parse_command_line
	./build/test_yoyo_parse_command_line

debug/test_yoyo_parse_command_line: debug/yoyo.o \
		tests/test_yoyo_parse_command_line.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

check_debug_yoyo_parse_command_line: debug/test_yoyo_parse_command_line
	./debug/test_yoyo_parse_command_line

build/test_process_looks_hung: build/yoyo.o \
		tests/test_process_looks_hung.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_build_process_looks_hung: build/test_process_looks_hung
	./build/test_process_looks_hung

debug/test_process_looks_hung: debug/yoyo.o \
		tests/test_process_looks_hung.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

check_debug_process_looks_hung: debug/test_process_looks_hung
	./debug/test_process_looks_hung

build/test_monitor_child_for_hang: build/yoyo.o \
		tests/test_monitor_child_for_hang.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_build_monitor_child_for_hang: build/test_monitor_child_for_hang
	./build/test_monitor_child_for_hang

debug/test_monitor_child_for_hang: debug/yoyo.o \
		tests/test_monitor_child_for_hang.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

check_debug_monitor_child_for_hang: debug/test_monitor_child_for_hang
	./debug/test_monitor_child_for_hang

build/test_slurp_text: build/yoyo.o \
		tests/test_slurp_text.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_build_slurp_text: build/test_slurp_text
	./build/test_slurp_text

debug/test_slurp_text: debug/yoyo.o \
		tests/test_slurp_text.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

check_debug_slurp_text: debug/test_slurp_text
	./debug/test_slurp_text

build/test_state_list_new: build/yoyo.o \
		tests/test_state_list_new.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_build_state_list_new: build/test_state_list_new
	./build/test_state_list_new

debug/test_state_list_new: debug/yoyo.o \
		tests/test_state_list_new.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

check_debug_state_list_new: debug/test_state_list_new
	./debug/test_state_list_new

build/test_yoyo_main: build/yoyo.o \
		tests/test_yoyo_main.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_build_yoyo_main: build/test_yoyo_main
	./build/test_yoyo_main

debug/test_yoyo_main: debug/yoyo.o \
		tests/test_yoyo_main.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

check_debug_yoyo_main: debug/test_yoyo_main
	./debug/test_yoyo_main

build/test_exit_reason: build/yoyo.o \
		tests/test_exit_reason.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

check_build_exit_reason: build/test_exit_reason
	./build/test_exit_reason

debug/test_exit_reason: debug/yoyo.o \
		tests/test_exit_reason.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@

check_debug_exit_reason: debug/test_exit_reason
	./debug/test_exit_reason

check-build-unit: check_build_yoyo_parse_command_line \
		check_build_exit_reason \
		check_build_yoyo_main \
		check_build_slurp_text \
		check_build_state_list_new \
		check_build_process_looks_hung \
		check_build_monitor_child_for_hang
	@echo "SUCCESS! ($@)"

check-debug-unit: check_debug_yoyo_parse_command_line \
		check_debug_exit_reason \
		check_debug_yoyo_main \
		check_debug_slurp_text \
		check_debug_state_list_new \
		check_debug_process_looks_hung \
		check_debug_monitor_child_for_hang
	@echo "SUCCESS! ($@)"

build/faux-rogue: tests/faux-rogue.c
	$(CC) $(BUILD_CFLAGS) $^ -o $@

debug/faux-rogue: tests/faux-rogue.c
	$(CC) $(DEBUG_CFLAGS) $^ -o $@


build--version: ./build/$(YOYO_BIN)
	@echo
	./build/$(YOYO_BIN) --version
	@echo "SUCCESS! ($@)"

debug-version: ./debug/$(YOYO_BIN)
	@echo
	./debug/$(YOYO_BIN) --version
	@echo "SUCCESS! ($@)"

build--help:./build/$(YOYO_BIN)
	@echo
	./build/$(YOYO_BIN) --help
	@echo "SUCCESS! ($@)"

debug--help: ./debug/$(YOYO_BIN)
	@echo
	./debug/$(YOYO_BIN) --help
	@echo "SUCCESS! ($@)"

check-build-acceptance-succeed-first-try: build/$(YOYO_BIN) build/faux-rogue
	@echo
	@echo "succeed first try"
	echo "0" > tmp.$@.failcount
	FAILCOUNT=tmp.$@.failcount \
		./build/$(YOYO_BIN) \
		--wait-interval=$(WAIT_INTERVAL) \
		$(YOYO_OPTS) ./build/faux-rogue $(FIXTURE_SLEEP)
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-debug-acceptance-succeed-first-try: debug/$(YOYO_BIN) debug/faux-rogue
	@echo
	@echo "succeed first try"
	echo "0" > tmp.$@.failcount
	FAILCOUNT=tmp.$@.failcount \
		./debug/$(YOYO_BIN) \
		--wait-interval=$(WAIT_INTERVAL) \
		--verbose=2 \
		$(YOYO_OPTS) ./debug/faux-rogue $(FIXTURE_SLEEP)
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-build-acceptance-fail-one-then-succeed: build/$(YOYO_BIN) build/faux-rogue
	@echo
	@echo "fail once, then succeed"
	echo "1" > tmp.$@.failcount
	FAILCOUNT=tmp.$@.failcount \
		./build/$(YOYO_BIN) \
		--wait-interval=$(WAIT_INTERVAL) \
		$(YOYO_OPTS) ./build/faux-rogue $(FIXTURE_SLEEP)
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-debug-acceptance-fail-one-then-succeed: debug/$(YOYO_BIN) debug/faux-rogue
	@echo
	@echo "fail once, then succeed"
	echo "1" > tmp.$@.failcount
	FAILCOUNT=tmp.$@.failcount
		./debug/$(YOYO_BIN) \
		--wait-interval=$(WAIT_INTERVAL) \
		--verbose=2 \
		$(YOYO_OPTS) ./debug/faux-rogue $(FIXTURE_SLEEP)
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-build-acceptance-succeed-after-long-time: build/$(YOYO_BIN) build/faux-rogue
	@echo
	@echo "succeed after a long time"
	echo "0" > tmp.$@.failcount
	FAILCOUNT=tmp.$@.failcount
		./build/$(YOYO_BIN) \
		--wait-interval=$(WAIT_INTERVAL) \
		$(YOYO_OPTS) ./build/faux-rogue $(FIXTURE_SLEEP_LONG)
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-debug-acceptance-succeed-after-long-time: debug/$(YOYO_BIN) debug/faux-rogue
	@echo
	@echo "succeed after a long time"
	echo "0" > tmp.$@.failcount
	FAILCOUNT=tmp.$@.failcount
		./debug/$(YOYO_BIN) \
		--wait-interval=$(WAIT_INTERVAL) \
		--verbose=2 \
		$(YOYO_OPTS) ./debug/faux-rogue $(FIXTURE_SLEEP_LONG)
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-build-acceptance-hang-twice-then-succeed: build/$(YOYO_BIN) build/faux-rogue
	echo "./build/faux-rogue will hang twice"
	echo "-2" > tmp.$@.failcount
	FAILCOUNT=tmp.$@.failcount \
		./build/$(YOYO_BIN) \
		--wait-interval=$(WAIT_INTERVAL) \
		$(YOYO_OPTS) ./build/faux-rogue $(FIXTURE_SLEEP)
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-debug-acceptance-hang-twice-then-succeed: debug/$(YOYO_BIN) debug/faux-rogue
	echo "./debug/faux-rogue will hang twice"
	echo "-2" > tmp.$@.failcount
	FAILCOUNT=tmp.$@.failcount \
		./debug/$(YOYO_BIN) \
		--wait-interval=$(WAIT_INTERVAL) \
		--verbose=2 \
		$(YOYO_OPTS) ./debug/faux-rogue $(FIXTURE_SLEEP)
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-build-acceptance-fail-every-time: build/$(YOYO_BIN) build/faux-rogue
	@echo
	echo "now ./build/faux-rogue will hang every time"
	echo "100" > tmp.$@.failcount
	-( FAILCOUNT=tmp.$@.failcount \
		./build/$(YOYO_BIN) \
		--wait-interval=$(WAIT_INTERVAL) \
		--max-hangs=3 --max-retries=2 \
		$(YOYO_OPTS) ./build/faux-rogue $(FIXTURE_SLEEP) )
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-debug-acceptance-fail-every-time: debug/$(YOYO_BIN) debug/faux-rogue
	@echo
	echo "now ./debug/faux-rogue will hang every time"
	echo "100" > tmp.$@.failcount
	-( FAILCOUNT=tmp.$@.failcount \
		./debug/$(YOYO_BIN) \
		--wait-interval=$(WAIT_INTERVAL) \
		--max-hangs=3 --max-retries=2 \
		--verbose=2 \
		$(YOYO_OPTS) ./debug/faux-rogue $(FIXTURE_SLEEP) )
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-build-acceptance-hang-every-time: build/$(YOYO_BIN) build/faux-rogue
	@echo
	echo "now ./build/faux-rogue will hang every time"
	echo "-100" > tmp.$@.failcount
	-( FAILCOUNT=tmp.$@.failcount \
		./build/$(YOYO_BIN) \
		--wait-interval=$(WAIT_INTERVAL) \
		--max-hangs=3 --max-retries=2 \
		$(YOYO_OPTS) ./build/faux-rogue $(FIXTURE_SLEEP) )
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-debug-acceptance-hang-every-time: debug/$(YOYO_BIN) debug/faux-rogue
	@echo
	echo "now ./debug/faux-rogue will hang every time"
	echo "-100" > tmp.$@.failcount
	-( FAILCOUNT=tmp.$@.failcount \
		./debug/$(YOYO_BIN) \
		--wait-interval=$(WAIT_INTERVAL) \
		--max-hangs=3 --max-retries=2 \
		--verbose=2 \
		$(YOYO_OPTS) ./debug/faux-rogue $(FIXTURE_SLEEP) )
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-build-acceptance: \
		build--version \
		build--help \
		check-build-acceptance-succeed-first-try \
		check-build-acceptance-fail-one-then-succeed \
		check-debug-acceptance-succeed-after-long-time \
		check-build-acceptance-hang-twice-then-succeed \
		check-build-acceptance-fail-every-time \
		check-build-acceptance-hang-every-time
	@echo "SUCCESS! ($@)"

check-debug-acceptance: \
		debug-version \
		debug--help \
		check-debug-acceptance-succeed-first-try \
		check-debug-acceptance-fail-one-then-succeed \
		check-debug-acceptance-succeed-after-long-time \
		check-debug-acceptance-hang-twice-then-succeed \
		check-debug-acceptance-fail-every-time \
		check-debug-acceptance-hang-every-time
	@echo "SUCCESS! ($@)"

coverage.info: check-debug-unit check-debug-acceptance
	lcov    --checksum \
		--capture \
		--base-directory . \
		--directory . \
		--output-file coverage.info

html-report: coverage.info
	mkdir -pv ./coverage_html
	genhtml coverage.info --output-directory coverage_html

coverage: html-report
	$(BROWSER) ./coverage_html/src/yoyo.c.gcov.html

check: check-build-unit check-build-acceptance
	@echo "SUCCESS! ($@)"

debug-check: check-debug-unit check-debug-acceptance
	@echo "SUCCESS! ($@)"

check-all: check html-report check-ruby
	@echo "SUCCESS! ($@)"

build/globdemo:
	mkdir -pv build
	$(CC) $(CFLAGS) $< -o build/$@

globdemo: build/globdemo
	./build/globdemo

check-ruby-success: ruby/yoyo ruby/fixture
	@echo "ruby"
	echo "0" > tmp.$@.failcount
	FAILCOUNT=tmp.$@.failcount ruby/yoyo ruby/fixture $(FIXTURE_SLEEP)
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-ruby-fail-once: ruby/yoyo ruby/fixture
	@echo
	echo "1" > tmp.$@.failcount
	FAILCOUNT=tmp.$@.failcount ruby/yoyo ruby/fixture $(FIXTURE_SLEEP)
	rm -fv tmp.$@.failcount
	@echo "SUCCESS! ($@)"

check-ruby: check-ruby-success check-ruby-fail-once
	@echo "SUCCESS! ($@)"

tidy:
	$(LINDENT) -T FILE src/*.c src/*.h tests/*.c

clean:
	rm -rvf build/* debug/* `cat .gitignore | sed -e 's/#.*//'`
	pushd src; rm -rvf `cat ../.gitignore | sed -e 's/#.*//'`; popd
	pushd tests; rm -rvf `cat ../.gitignore | sed -e 's/#.*//'`; popd

mrproper:
	git clean -dffx
	git submodule foreach --recursive git clean -dffx
