CFLAGS += -Wall -Wextra -I../lib
LDFLAGS += -L../lib
LDLIBS += -lpacutils -lalpm -larchive

PROVE ?= prove

TESTS += \
		 10-basename.t \
		 10-config-basic.t \
		 10-filelist_contains_path.t \
		 10-log-action-parse.t \
		 10-log-transaction-parse.t \
		 10-log-reader-basic.t \
		 10-mtree-basic.t \
		 10-parse-datetime.t \
		 10-pathcmp.t \
		 10-strreplace.t \
		 20-config-includes.t \
		 20-config-root-inheritance.t \
		 40-ui-cb-download-progress.t \
		 99-pu_list_shift.t

%.t: %.c ../lib/libpacutils.so
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

check: tests
	LD_LIBRARY_PATH=../lib $(PROVE) $(TESTS)

tests: $(TESTS)

all: tests

gcov: CC = gcc
gcov: CFLAGS += -fprofile-arcs -ftest-coverage
gcov: check
	gcov $(TESTS)

clean:
	$(RM) $(TESTS)
	$(RM) *.gcda *.gcno *.gcov

.PHONY: all clean check tests
