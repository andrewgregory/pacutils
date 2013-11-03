CFLAGS += -Wall -fPIC -shared -g
LDLIBS += -lalpm

PREFIX      = /usr/local
EXEC_PREFIX = ${PREFIX}
BINDIR      = ${EXEC_PREFIX}/bin
INCLUDEDIR  = ${PREFIX}/include
LIBDIR      = ${EXEC_PREFIX}/lib

all: libpacutils.so src

libpacutils.so: pacutils.c
	$(CC) $(CFLAGS) -o $@ $^

src: libpacutils.so
	$(MAKE) -C $@ all

install: libpacutils.so
	install -d ${DESTDIR}${INCLUDEDIR}
	install -m 644 pacutils.h ${DESTDIR}${INCLUDEDIR}/pacutils.h
	install -d ${DESTDIR}${LIBDIR}
	install -m 644 libpacutils.so ${DESTDIR}${LIBDIR}/libpacutils.so

clean:
	$(RM) *.o *.so*
	$(MAKE) -C src $@

.PHONY: src
