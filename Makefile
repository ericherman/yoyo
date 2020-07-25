# $@ : target label
# $< : the first prerequisite after the colon
# $^ : all of the prerequisite files
# $* : wildcard matched part

default: hangcheckc

# -Wno-unused-parameter -Wno-unused-variable
CFLAGS += -g -DNDEBUG -O2 \
    -Wall -Wextra -pedantic \
    -pipe

SHELL=/bin/bash

# extracted from https://github.com/torvalds/linux/blob/master/scripts/Lindent
LINDENT=indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 -il0
# see also: https://www.kernel.org/doc/Documentation/process/coding-style.rst

hangcheckc: hangcheck.c
	$(CC) $(CFLAGS) $< -o $@

check: hangcheckc
	@echo "ruby"
	echo "0" > /tmp/failcount
	./hangcheck ./fixture 2
	@echo
	@echo "c"
	echo "0" > /tmp/failcount
	./hangcheckc ./fixture 2
	@echo
	@echo "ruby"
	echo "1" > /tmp/failcount
	./hangcheck ./fixture 2
	@echo
	@echo "c"
	echo "1" > /tmp/failcount
	./hangcheckc ./fixture 2

clean:
	rm -rvf hangcheckc `cat .gitignore | sed -e 's/#.*//'`

mrproper:
	git clean -dffx
	git submodule foreach --recursive git clean -dffx

tidy:
	$(LINDENT) *.c
