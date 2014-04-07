CFLAGS += -Wall -I../lib
LDFLAGS += -L../lib
LDLIBS += -lpacutils -lalpm -larchive

PREFIX      ?= /usr/local
EXEC_PREFIX ?= ${PREFIX}
BINDIR      ?= ${EXEC_PREFIX}/bin
INCLUDEDIR  ?= ${PREFIX}/include
LIBDIR      ?= ${EXEC_PREFIX}/lib

OBJECTS = pacinfo pacinstall pacremove pacfile pacconf pacsync pactrans pacsift paclog pacreport

all: $(OBJECTS)

pacremove: | pactrans
	ln -fs $| $@

pacinstall: | pactrans
	ln -fs $| $@

install: all
	install -d ${DESTDIR}${BINDIR}
	install -m 755 pacinfo    ${DESTDIR}${BINDIR}/pacinfo
	install -m 755 pacfile    ${DESTDIR}${BINDIR}/pacfile
	install -m 755 pacconf    ${DESTDIR}${BINDIR}/pacconf
	install -m 755 pacsync    ${DESTDIR}${BINDIR}/pacsync
	install -m 755 pacsift    ${DESTDIR}${BINDIR}/pacsift
	install -m 755 paclog     ${DESTDIR}${BINDIR}/paclog
	install -m 755 pactrans   ${DESTDIR}${BINDIR}/pactrans
	install -m 755 pacreport  ${DESTDIR}${BINDIR}/pacreport
	cp -d          pacinstall ${DESTDIR}${BINDIR}/pacinstall
	cp -d          pacremove  ${DESTDIR}${BINDIR}/pacremove

clean:
	$(RM) $(OBJECTS)
