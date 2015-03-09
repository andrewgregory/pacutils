CFLAGS += -Wall -fPIC -shared -g
LDLIBS += -lalpm

PREFIX      = /usr/local
EXEC_PREFIX = ${PREFIX}
BINDIR      = ${EXEC_PREFIX}/bin
INCLUDEDIR  = ${PREFIX}/include
LIBDIR      = ${EXEC_PREFIX}/lib

HEADERS = \
		  pacutils.h \
		  pacutils/config.h \
		  pacutils/log.h

SOURCES = \
		  pacutils.c \
		  pacutils/config.c \
		  pacutils/log.c

all: libpacutils.so

libpacutils.so: ${SOURCES}
	$(CC) $(CFLAGS) -o $@ $^

install: libpacutils.so
	install -d ${DESTDIR}${INCLUDEDIR}/pacutils
	for h in ${HEADERS}; do install -m 644 $$h "${DESTDIR}${INCLUDEDIR}/$$h"; done
	install -d ${DESTDIR}${LIBDIR}
	install -m 644 libpacutils.so ${DESTDIR}${LIBDIR}/libpacutils.so

clean:
	$(RM) *.o *.so*

.PHONY: all clean install
