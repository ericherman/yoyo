# $@ : target label
# $< : the first prerequisite after the colon
# $^ : all of the prerequisite files
# $* : wildcard matched part

BROWSER=firefox

default: check

ifndef DEBUG
DEBUG=0
endif

ifeq ($(DEBUG), 0)
CFLAGS += -DNDEBUG -O2
else
CFLAGS += -DDEBUG -O0 \
	-fno-inline-small-functions \
	-fkeep-inline-functions \
	-fkeep-static-functions \
	--coverage
endif

CFLAGS += -g -Wall -Wextra -pedantic -Werror

SHELL=/bin/bash
FAILCOUNT:=$(shell mktemp --tmpdir=. --suffix=.failcount)

# extracted from https://github.com/torvalds/linux/blob/master/scripts/Lindent
LINDENT=indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 -il0
# see also: https://www.kernel.org/doc/Documentation/process/coding-style.rst

yoyoc: yoyo.o yoyo-main.c
	$(CC) $(CFLAGS) $^ -o $@

yoyo.o: yoyo.c yoyo.h
	$(CC) -c $(CFLAGS) -o $@ $<

test_yoyo_parse_command_line: yoyo.o test_yoyo_parse_command_line.c
	$(CC) $(CFLAGS) $^ -o $@

check_yoyo_parse_command_line: test_yoyo_parse_command_line
	./test_yoyo_parse_command_line

test_process_looks_hung: yoyo.o test_process_looks_hung.c
	$(CC) $(CFLAGS) $^ -o $@

check_process_looks_hung: test_process_looks_hung
	./test_process_looks_hung

test_monitor_child_for_hang: yoyo.o test_monitor_child_for_hang.c
	$(CC) $(CFLAGS) $^ -o $@

check_monitor_child_for_hang: test_monitor_child_for_hang
	./test_monitor_child_for_hang

check-yoyoc: check_yoyo_parse_command_line \
		check_process_looks_hung \
		check_monitor_child_for_hang
	@echo "$@ SUCCESS"

faux-rogue: faux-rogue.c
	$(CC) $(CFLAGS) $^ -o $@

FIXTURE_SLEEP ?= 2
WAIT_INTERVAL ?= 4
FIXTURE_SLEEP_LONG ?= 6

check-ruby:
	@echo "ruby"
	echo "0" > $(FAILCOUNT)
	FAILCOUNT=$(FAILCOUNT) ./yoyo ./fixture $(FIXTURE_SLEEP)
	@echo
	echo "1" > $(FAILCOUNT)
	FAILCOUNT=$(FAILCOUNT) ./yoyo ./fixture $(FIXTURE_SLEEP)
	@echo "$@ SUCCESS"

check: check-yoyoc yoyoc faux-rogue
	@echo
	./yoyoc --version
	@echo
	./yoyoc --help
	@echo
	@echo "succeed first try succeed"
	echo "0" > $(FAILCOUNT)
	FAILCOUNT=$(FAILCOUNT) ./yoyoc \
		--wait-interval=$(WAIT_INTERVAL) \
		$(YOYO_OPTS) ./faux-rogue $(FIXTURE_SLEEP)
	@echo
	@echo "fail once, then succeed"
	echo "1" > $(FAILCOUNT)
	FAILCOUNT=$(FAILCOUNT) ./yoyoc \
		--wait-interval=$(WAIT_INTERVAL) \
		$(YOYO_OPTS) ./faux-rogue $(FIXTURE_SLEEP)
	@echo
	@echo "succeed after a long time"
	echo "0" > $(FAILCOUNT)
	FAILCOUNT=$(FAILCOUNT) ./yoyoc \
		--wait-interval=$(WAIT_INTERVAL) \
		$(YOYO_OPTS) ./faux-rogue $(FIXTURE_SLEEP_LONG)
	@echo
	echo "./faux-rogue will hang twice"
	echo "-2" > $(FAILCOUNT)
	FAILCOUNT=$(FAILCOUNT) ./yoyoc \
		--wait-interval=$(WAIT_INTERVAL) \
		$(YOYO_OPTS) ./faux-rogue $(FIXTURE_SLEEP)
	@echo
	echo "now ./faux-rogue will hang every time"
	echo "-100" > $(FAILCOUNT)
	-( FAILCOUNT=$(FAILCOUNT) ./yoyoc \
		--wait-interval=$(WAIT_INTERVAL) \
		--max-hangs=3 --max-retries=2 \
		$(YOYO_OPTS) ./faux-rogue $(FIXTURE_SLEEP) )
	@echo
	rm -fv $(FAILCOUNT)
	@echo "$@ SUCCESS"

check-all: check check-ruby
	@echo "$@ SUCCESS"

globdemo: globdemo.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rvf yoyoc *.failcount `cat .gitignore | sed -e 's/#.*//'`

mrproper:
	git clean -dffx
	git submodule foreach --recursive git clean -dffx

tidy:
	$(LINDENT) -T FILE *.c *.h

lcov: check
	lcov    --checksum \
		--capture \
		--base-directory . \
		--directory . \
		--output-file coverage.info

html-report: lcov
	mkdir -pv ./coverage_html
	genhtml coverage.info --output-directory coverage_html

coverage: html-report
	$(BROWSER) ./coverage_html/yoyo/yoyo.c.gcov.html
