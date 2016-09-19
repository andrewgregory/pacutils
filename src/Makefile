-include ../configure.mk
-include configure.mk

ALPM_CFLAGS ?= $(shell pkg-config libalpm --cflags)

CFLAGS ?= -Wall -Wextra -Wpedantic -Werror -g

override CFLAGS   += $(ALPM_CFLAGS)
override CPPFLAGS += -I../lib
override LDFLAGS  += -L../lib
override LDLIBS   += -lpacutils -lalpm -larchive

PREFIX      ?= /usr/local
EXEC_PREFIX ?= ${PREFIX}
BINDIR      ?= ${EXEC_PREFIX}/bin
INCLUDEDIR  ?= ${PREFIX}/include
LIBDIR      ?= ${EXEC_PREFIX}/lib
SYSCONFDIR  ?= ${PREFIX}/etc

FILESDBEXT ?= .files
PACMANCONF ?= ${SYSCONFDIR}/pacman.conf

override CPPFLAGS += \
					 -DFILESDBEXT=\"$(FILESDBEXT)\" \
					 -DPACMANCONF=\"$(PACMANCONF)\" \
					 -DSYSCONFDIR=\"$(SYSCONFDIR)\"

GITTAG = $(shell git describe --abbrev=0 2>/dev/null)
ifneq ($(GITTAG),)
    GITVER = $(subst $(GITTAG)-,,$(shell git describe --dirty))
    ifneq ($(GITTAG),$(GITVER))
        override CPPFLAGS += -DGITVER=\"$(GITVER)\"
    endif
endif

OBJECTS = \
		  paccheck \
		  pacconf \
		  pacfile \
		  pacinfo \
		  pacini \
		  paclog \
		  pacreport \
		  pacsift \
		  pacsync \
		  pactrans

all: $(OBJECTS) pacinstall pacremove

pacsift: LDLIBS += -lm

pacremove: | pactrans
	ln -fs $| $@

pacinstall: | pactrans
	ln -fs $| $@

install: all
	install -d "${DESTDIR}${BINDIR}"
	install -m 755 $(OBJECTS) "${DESTDIR}${BINDIR}/"
	cp -d          pacinstall "${DESTDIR}${BINDIR}/pacinstall"
	cp -d          pacremove  "${DESTDIR}${BINDIR}/pacremove"

clean:
	$(RM) $(OBJECTS) pacinstall pacremove

.PHONY: all clean install
