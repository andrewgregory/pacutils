CFLAGS += -Wall -Wextra -I../lib
LDFLAGS += -L../lib
LDLIBS += -lpacutils -lalpm -larchive

TESTS += \
		 10-basename.t \
		 10-config-basic.t \
		 10-filelist_contains_path.t \
		 10-pathcmp.t \
		 10-strreplace.t \
		 10-strtrim.t \
		 20-config-includes.t \
		 20-config-root-inheritance.t

%.t: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

check: tests
	LD_LIBRARY_PATH=../lib prove .

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
