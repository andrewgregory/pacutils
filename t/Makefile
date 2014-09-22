CFLAGS += -Wall -Wextra -I../lib
LDFLAGS += -L../lib
LDLIBS += -lpacutils -lalpm -larchive

TESTS += \
		 10-config-basic.t \
		 10-pathcmp.t \
		 10-strtrim.t \
		 20-config-root-inheritance.t

%.t: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

check: tests
	LD_LIBRARY_PATH=../lib prove .

tests: $(TESTS)

all: tests

clean:
	$(RM) $(TESTS)

.PHONY: all clean check tests
