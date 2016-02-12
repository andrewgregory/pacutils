-include ../configure.mk
-include configure.mk

CFLAGS ?= -Wall -Wextra -Werror -I../lib -g
LDFLAGS += -L../lib
LDLIBS += -lpacutils -lalpm -larchive

PREFIX      ?= /usr/local
EXEC_PREFIX ?= ${PREFIX}
BINDIR      ?= ${EXEC_PREFIX}/bin
INCLUDEDIR  ?= ${PREFIX}/include
LIBDIR      ?= ${EXEC_PREFIX}/lib

FILESDBEXT ?= .files

CPPFLAGS   += -DFILESDBEXT=\"$(FILESDBEXT)\"

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
