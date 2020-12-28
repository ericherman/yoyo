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

yoyoc: yoyo.c yoyo-main.c
	$(CC) $(CFLAGS) yoyo.c yoyo-main.c -o $@

globdemo: globdemo.c
	$(CC) $(CFLAGS) $< -o $@

check: yoyoc
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

clean:
	rm -rvf yoyoc *.failcount `cat .gitignore | sed -e 's/#.*//'`

mrproper:
	git clean -dffx
	git submodule foreach --recursive git clean -dffx

tidy:
	$(LINDENT) -T FILE *.c
