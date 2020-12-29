# $@ : target label
# $< : the first prerequisite after the colon
# $^ : all of the prerequisite files
# $* : wildcard matched part

default: check

# -Wno-unused-parameter -Wno-unused-variable
CFLAGS += -g -DNDEBUG -O2 \
    -Wall -Wextra -pedantic \
    -pipe

SHELL=/bin/bash
FAILCOUNT:=$(shell mktemp --tmpdir=. --suffix=.failcount)

# extracted from https://github.com/torvalds/linux/blob/master/scripts/Lindent
LINDENT=indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 -il0
# see also: https://www.kernel.org/doc/Documentation/process/coding-style.rst

yoyoc: yoyo.c yoyo-main.c yoyo.h
	$(CC) $(CFLAGS) yoyo.c yoyo-main.c -o $@

test_yoyo_parse_command_line: yoyo.c yoyo.h test_yoyo_parse_command_line.c
	$(CC) $(CFLAGS) yoyo.c test_yoyo_parse_command_line.c -o $@

check_yoyo_parse_command_line: test_yoyo_parse_command_line
	./test_yoyo_parse_command_line

test_process_looks_hung: yoyo.c yoyo.h test_process_looks_hung.c
	$(CC) $(CFLAGS) yoyo.c test_process_looks_hung.c -o $@

check_process_looks_hung: test_process_looks_hung
	./test_process_looks_hung

check-yoyoc: check_yoyo_parse_command_line \
		check_process_looks_hung
	@echo check-yoyoc SUCCESS

check: check-yoyoc yoyoc
	@echo "ruby"
	echo "0" > $(FAILCOUNT)
	FAILCOUNT=$(FAILCOUNT) ./yoyo ./fixture 2
	@echo
	@echo "c"
	echo "0" > $(FAILCOUNT)
	FAILCOUNT=$(FAILCOUNT) ./yoyoc ./fixture 2
	@echo
	@echo "ruby"
	echo "1" > $(FAILCOUNT)
	FAILCOUNT=$(FAILCOUNT) ./yoyo ./fixture 2
	@echo
	@echo "c"
	echo "1" > $(FAILCOUNT)
	FAILCOUNT=$(FAILCOUNT) ./yoyoc ./fixture 2
	@echo
	rm -fv $(FAILCOUNT)

globdemo: globdemo.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rvf yoyoc *.failcount `cat .gitignore | sed -e 's/#.*//'`

mrproper:
	git clean -dffx
	git submodule foreach --recursive git clean -dffx

tidy:
	$(LINDENT) -T FILE *.c *.h
