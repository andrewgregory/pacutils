CFLAGS += -Wall -I..
LDFLAGS += -L..
LDLIBS += -lpacutils -lalpm -larchive

PREFIX      ?= /usr/local
EXEC_PREFIX ?= ${PREFIX}
BINDIR      ?= ${EXEC_PREFIX}/bin
INCLUDEDIR  ?= ${PREFIX}/include
LIBDIR      ?= ${EXEC_PREFIX}/lib

OBJECTS = pacsize pacinfo pacinstall pacfile pacconf pacsync pactrans

all: $(OBJECTS)

install: all
	install -d ${DESTDIR}${BINDIR}
	install -m 755 pacsize    ${DESTDIR}${BINDIR}/pacsize
	install -m 755 pacinfo    ${DESTDIR}${BINDIR}/pacinfo
	install -m 755 pacfile    ${DESTDIR}${BINDIR}/pacfile
	install -m 755 pacconf    ${DESTDIR}${BINDIR}/pacconf
	install -m 755 pacsync    ${DESTDIR}${BINDIR}/pacsync
	install -m 755 pactrans   ${DESTDIR}${BINDIR}/pactrans
	install -m 755 pacinstall ${DESTDIR}${BINDIR}/pacinstall

clean:
	$(RM) $(OBJECTS)
