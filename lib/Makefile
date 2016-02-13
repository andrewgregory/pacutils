-include ../configure.mk
-include configure.mk

CFLAGS ?= -Wall -Wextra -Wpedantic -Werror -g
LDLIBS += -lalpm

PREFIX        ?= /usr/local
EXEC_PREFIX   ?= ${PREFIX}
BINDIR        ?= ${EXEC_PREFIX}/bin
INCLUDEDIR    ?= ${PREFIX}/include
LIBDIR        ?= ${EXEC_PREFIX}/lib
LOCALSTATEDIR ?= ${PREFIX}/var
SYSCONFDIR    ?= ${PREFIX}/etc

CACHEDIR   ?= ${LOCALSTATEDIR}/cache/pacman/pkg
DBEXT      ?= .db
DBPATH     ?= ${LOCALSTATEDIR}/lib/pacman
GPGDIR     ?= ${SYSCONFDIR}/pacman.d/gnupg
HOOKDIR    ?= ${SYSCONFDIR}/pacman.d/hooks
LOGFILE    ?= ${LOCALSTATEDIR}/log/pacman.log
ROOTDIR    ?= /

CPPFLAGS   += \
			  -DCACHEDIR=\"$(CACHEDIR)\" \
			  -DDBEXT=\"$(DBEXT)\" \
			  -DDBPATH=\"$(DBPATH)\" \
			  -DGPGDIR=\"$(GPGDIR)\" \
			  -DHOOKDIR=\"$(HOOKDIR)\" \
			  -DLOGFILE=\"$(LOGFILE)\" \
			  -DROOTDIR=\"$(ROOTDIR)\"

HEADERS = \
		  pacutils.h \
		  pacutils/config.h \
		  pacutils/log.h \
		  pacutils/mtree.h \
		  pacutils/ui.h \
		  pacutils/util.h

SOURCES = \
		  ../ext/mini.c/mini.c \
		  pacutils.c \
		  pacutils/config.c \
		  pacutils/log.c \
		  pacutils/mtree.c \
		  pacutils/ui.c \
		  pacutils/util.c

all: libpacutils.so

libpacutils.so: ${SOURCES}
	$(CC) $(CPPFLAGS) -shared -fPIC $(CFLAGS) -o $@ $^ $(LDLIBS)

install: libpacutils.so
	install -d "${DESTDIR}${INCLUDEDIR}/pacutils"
	for h in ${HEADERS}; do install -m 644 $$h "${DESTDIR}${INCLUDEDIR}/$$h"; done
	install -d "${DESTDIR}${LIBDIR}"
	install -m 644 libpacutils.so "${DESTDIR}${LIBDIR}/libpacutils.so"

clean:
	$(RM) *.o *.so*

.PHONY: all clean install
