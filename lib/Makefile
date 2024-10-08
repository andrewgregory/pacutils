-include ../configure.mk
-include configure.mk

ALPM_CFLAGS ?= $(shell pkg-config libalpm --cflags)

CFLAGS ?= -Wall -Wextra -Wpedantic -Werror -g

override CFLAGS += $(ALPM_CFLAGS)
override LDLIBS += -lalpm

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

override CPPFLAGS += \
	-DCACHEDIR=\"$(CACHEDIR)\" \
	-DDBEXT=\"$(DBEXT)\" \
	-DDBPATH=\"$(DBPATH)\" \
	-DGPGDIR=\"$(GPGDIR)\" \
	-DHOOKDIR=\"$(HOOKDIR)\" \
	-DLOCALSTATEDIR=\"$(LOCALSTATEDIR)\" \
	-DLOGFILE=\"$(LOGFILE)\" \
	-DROOTDIR=\"$(ROOTDIR)\" \
	-DSYSCONFDIR=\"$(SYSCONFDIR)\"

GITTAG = $(shell git describe --abbrev=0 2>/dev/null)
ifneq ($(GITTAG),)
	GITVER = $(subst $(GITTAG)-,,$(shell git describe --dirty))
	ifneq ($(GITTAG),$(GITVER))
		override CPPFLAGS += -DGITVER=\"$(GITVER)\"
	endif
endif

HEADERS = \
					pacutils.h \
					pacutils/config.h \
					pacutils/depends.h \
					pacutils/log.h \
					pacutils/mtree.h \
					pacutils/ui.h \
					pacutils/uix.h \
					pacutils/util.h

SOURCES = \
					../ext/globdir.c/globdir.c \
					../ext/mini.c/mini.c \
					pacutils.c \
					pacutils/config.c \
					pacutils/depends.c \
					pacutils/log.c \
					pacutils/mtree.c \
					pacutils/ui.c \
					pacutils/uix.c \
					pacutils/util.c

all: libpacutils.so

libpacutils.so: ${SOURCES}
	$(CC) $(CPPFLAGS) -shared -fPIC $(CFLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS)

install: libpacutils.so
	install -d "${DESTDIR}${INCLUDEDIR}/pacutils"
	for h in ${HEADERS}; do install -m 644 $$h "${DESTDIR}${INCLUDEDIR}/$$h"; done
	install -d "${DESTDIR}${LIBDIR}"
	install -m 644 libpacutils.so "${DESTDIR}${LIBDIR}/libpacutils.so"

clean:
	$(RM) *.o *.so*

.PHONY: all clean install
