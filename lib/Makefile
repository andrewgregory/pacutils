CFLAGS += -Wall -fPIC -shared -g
LDLIBS += -lalpm

PREFIX      = /usr/local
EXEC_PREFIX = ${PREFIX}
BINDIR      = ${EXEC_PREFIX}/bin
INCLUDEDIR  = ${PREFIX}/include
LIBDIR      = ${EXEC_PREFIX}/lib

all: libpacutils.so

libpacutils.so: pacutils.c pacutils/log.c
	$(CC) $(CFLAGS) -o $@ $^

install: libpacutils.so
	install -d ${DESTDIR}${INCLUDEDIR}/pacutils
	install -m 644 pacutils.h ${DESTDIR}${INCLUDEDIR}/pacutils.h
	install -m 644 pacutils/log.h ${DESTDIR}${INCLUDEDIR}/pacutils/log.h
	install -d ${DESTDIR}${LIBDIR}
	install -m 644 libpacutils.so ${DESTDIR}${LIBDIR}/libpacutils.so

clean:
	$(RM) *.o *.so*
