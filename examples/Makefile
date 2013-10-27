CFLAGS += -Wall -I..
LDFLAGS += -L..
LDLIBS += -lpacutils -lalpm -larchive

PREFIX      ?= /usr/local
EXEC_PREFIX ?= ${PREFIX}
BINDIR      ?= ${EXEC_PREFIX}/bin
INCLUDEDIR  ?= ${PREFIX}/include
LIBDIR      ?= ${EXEC_PREFIX}/lib

OBJECTS = pacsize pacinfo pacinstall pacremove pacfile pacconf pacsync pactrans pacsift

all: $(OBJECTS)

pacremove: | pactrans
	ln -fs $< $@

pacinstall: | pactrans
	ln -fs $< $@

install: all
	install -d ${DESTDIR}${BINDIR}
	install -m 755 pacsize    ${DESTDIR}${BINDIR}/pacsize
	install -m 755 pacinfo    ${DESTDIR}${BINDIR}/pacinfo
	install -m 755 pacfile    ${DESTDIR}${BINDIR}/pacfile
	install -m 755 pacconf    ${DESTDIR}${BINDIR}/pacconf
	install -m 755 pacsync    ${DESTDIR}${BINDIR}/pacsync
	install -m 755 pacsift    ${DESTDIR}${BINDIR}/pacsift
	install -m 755 pactrans   ${DESTDIR}${BINDIR}/pactrans
	cp -d          pacinstall ${DESTDIR}${BINDIR}/pacinstall
	cp -d          pacremove  ${DESTDIR}${BINDIR}/pacremove

clean:
	$(RM) $(OBJECTS)
