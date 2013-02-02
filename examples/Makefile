CFLAGS += -I..
LDFLAGS += -L..
LDLIBS += -lpacutils -lalpm

PREFIX      ?= /usr/local
EXEC_PREFIX ?= ${PREFIX}
BINDIR      ?= ${EXEC_PREFIX}/bin
INCLUDEDIR  ?= ${PREFIX}/include
LIBDIR      ?= ${EXEC_PREFIX}/lib

OBJECTS = pacsize pacinfo pacinstall pacown pacconf

all: $(OBJECTS)

install: all
	install -d ${DESTDIR}${BINDIR}
	install -m 755 pacsize ${DESTDIR}${BINDIR}/pacsize
	install -m 755 pacinfo ${DESTDIR}${BINDIR}/pacinfo
	install -m 755 pacinstall ${DESTDIR}${BINDIR}/pacinstall
	install -m 755 pacown ${DESTDIR}${BINDIR}/pacown
	install -m 755 pacconf ${DESTDIR}${BINDIR}/pacconf

clean:
	$(RM) $(OBJECTS)
