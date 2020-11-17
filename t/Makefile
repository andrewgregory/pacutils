CFLAGS += -Wall -Wextra -Wpedantic -g

override CPPFLAGS += -I.. -I../ext

TESTS = 01-sanity.t 10-basic.t

01-sanity.t: CFLAGS += -std=c99 -pedantic -Werror -D_POSIX_C_SOURCE=200809L

%.t: %.c ../mGarbageCollector.c ../mGarbageCollector.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

check: tests
	prove .

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
