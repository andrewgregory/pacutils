PROVE ?= prove

GIT ?= git

CFLAGS += -Wall -Wextra -Wpedantic -g

override CPPFLAGS += -I..

TESTS = \
		01-sanity.t \
		10-escape_pattern.t \
		10-is_pattern.t \
		10-split_pattern.t \
		20-globat.t \
		30-globdir.t \
		40-glob_append.t \
		40-glob_dooffs.t \
		40-glob_err.t \
		40-glob_mark.t \
		40-glob_nocheck.t \
		40-glob_noescape.t \
		40-glob_nosort.t

01-sanity.t: CFLAGS += -pedantic -Werror

%.t: %.c ../globdir.c ../globdir.h globdir_test.h ../ext/tap.c/tap.c Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

check: tests
	$(PROVE) --exec="./runtest.sh" $(TESTS)

valgrind: tests
	$(PROVE) --exec="./runtest.sh -v" $(TESTS)

Weverything: CC = clang
Weverything: CFLAGS += -Weverything -Wno-padded
Weverything: check

gcov: CC = gcc
gcov: CFLAGS += -fprofile-arcs -ftest-coverage
gcov: check
	gcov $(TESTS)

gprof: CC = gcc
gprof: CFLAGS += -pg
gprof: check

tests: $(TESTS)

all: tests

clean:
	$(RM) $(TESTS)
	$(RM) *.gcov *.gcda *.gcno gmon.out

.PHONY: all clean check gcov gprof tests Weverything
