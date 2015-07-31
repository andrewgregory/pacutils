PREFIX      ?= /usr/local
DATAROOTDIR ?= $(PREFIX)/share
MANDIR      ?= $(DATAROOTDIR)/man
MAN1DIR     ?= $(MANDIR)/man1
MAN1EXT     ?= .1
MAN3DIR     ?= $(MANDIR)/man3
MAN3EXT     ?= .3

MANPAGES = \
		   paccheck$(MAN1EXT) \
		   pacconf$(MAN1EXT) \
		   pacfile$(MAN1EXT) \
		   pacinfo$(MAN1EXT) \
		   pacini$(MAN1EXT) \
		   paclog$(MAN1EXT) \
		   pacreport$(MAN1EXT) \
		   pacsift$(MAN1EXT) \
		   pacsync$(MAN1EXT) \
		   pactrans$(MAN1EXT) \
		   pacutils-mtree$(MAN3EXT)

all: doc

%$(MAN1EXT): %.pod
	pod2man --center $* --release pacutils --section 1  $< $@

%$(MAN3EXT): %.pod
	pod2man --center $* --release pacutils --section 3  $< $@

pacinstall$(MAN1EXT): | pactrans$(MAN1EXT)
	ln -fs $| $@

pacremove$(MAN1EXT): | pactrans$(MAN1EXT)
	ln -fs $| $@

doc: $(MANPAGES) pacinstall$(MAN1EXT) pacremove$(MAN1EXT)

install: doc
	install -d "${DESTDIR}${MAN1DIR}"
	install -m 644 $(MANPAGES) "${DESTDIR}${MAN1DIR}"
	cp -d pacinstall$(MAN1EXT) "${DESTDIR}${MAN1DIR}"
	cp -d pacremove$(MAN1EXT)  "${DESTDIR}${MAN1DIR}"

clean:
	$(RM) $(MANPAGES) pacinstall$(MAN1EXT) pacremove$(MAN1EXT)

.PHONY: all doc
