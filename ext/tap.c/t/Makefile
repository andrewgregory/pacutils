CFLAGS += -Wall -Wextra -Wpedantic -Werror -g

override CPPFLAGS += -I..

TESTS = 01-sanity.t 10-basic.t

01-sanity.t: CFLAGS += -std=c99 -pedantic -Werror

%.t: %.c ../tap.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

check: tests
	prove .

tests: $(TESTS)

all: tests

Weverything: CC = clang
Weverything: CFLAGS += -Weverything
Weverything: check

gcov: CC = gcc
gcov: CFLAGS += -fprofile-arcs -ftest-coverage
gcov: check
	gcov $(TESTS)

clean:
	$(RM) $(TESTS)
	$(RM) tmp_*
	$(RM) *.gcov *.gcda *.gcno

.PHONY: all clean check gcov tests Weverything
