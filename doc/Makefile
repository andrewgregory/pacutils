-include ../configure.mk
-include configure.mk

PREFIX      ?= /usr/local
DATAROOTDIR ?= $(PREFIX)/share
MANDIR      ?= $(DATAROOTDIR)/man
MAN1DIR     ?= $(MANDIR)/man1
MAN1EXT     ?= .1
MAN3DIR     ?= $(MANDIR)/man3
MAN3EXT     ?= .3
MAN7DIR     ?= $(MANDIR)/man7
MAN7EXT     ?= .7

MAN1PAGES = \
						paccapability$(MAN1EXT) \
						paccheck$(MAN1EXT) \
						pacconf$(MAN1EXT) \
						pacfile$(MAN1EXT) \
						pacinfo$(MAN1EXT) \
						pacini$(MAN1EXT) \
						paclog$(MAN1EXT) \
						pacrepairdb$(MAN1EXT) \
						pacrepairfile$(MAN1EXT) \
						pacreport$(MAN1EXT) \
						pacsift$(MAN1EXT) \
						pacsync$(MAN1EXT) \
						pactrans$(MAN1EXT)

MAN3PAGES = \
						pacutils-mtree$(MAN3EXT)

MAN7PAGES = \
						pacutils-sysroot$(MAN7EXT)

all: doc

%$(MAN1EXT): %.pod
	pod2man --center $* --release pacutils --section 1  $< $@

%$(MAN3EXT): %.pod
	pod2man --center $* --release pacutils --section 3  $< $@

%$(MAN7EXT): %.pod
	pod2man --center $* --release pacutils --section 7  $< $@

pacinstall$(MAN1EXT): | pactrans$(MAN1EXT)
	ln -fs $| $@

pacremove$(MAN1EXT): | pactrans$(MAN1EXT)
	ln -fs $| $@

doc: $(MAN1PAGES) $(MAN3PAGES) $(MAN7PAGES) pacinstall$(MAN1EXT) pacremove$(MAN1EXT)

install: doc
	install -d "${DESTDIR}${MAN1DIR}"
	install -d "${DESTDIR}${MAN3DIR}"
	install -d "${DESTDIR}${MAN7DIR}"
	install -m 644 $(MAN1PAGES) "${DESTDIR}${MAN1DIR}"
	install -m 644 $(MAN3PAGES) "${DESTDIR}${MAN3DIR}"
	install -m 644 $(MAN7PAGES) "${DESTDIR}${MAN7DIR}"
	cp -d pacinstall$(MAN1EXT) "${DESTDIR}${MAN1DIR}"
	cp -d pacremove$(MAN1EXT)  "${DESTDIR}${MAN1DIR}"

clean:
	$(RM) $(MAN1PAGES) $(MAN3PAGES) $(MAN7PAGES) pacinstall$(MAN1EXT) pacremove$(MAN1EXT)

.PHONY: all clean doc install
