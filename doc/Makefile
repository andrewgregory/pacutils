PREFIX      ?= /usr/local
DATAROOTDIR ?= $(PREFIX)/share
MANDIR      ?= $(DATAROOTDIR)/man
MAN8DIR     ?= $(MANDIR)/man8
MAN8EXT     ?= .8

MANPAGES = \
		   pacsift$(MAN8EXT) \
		   pactrans$(MAN8EXT)

all: doc pacinstall$(MAN8EXT) pacremove$(MAN8EXT)

%$(MAN8EXT): %.pod
	pod2man --center $* --release pacutils --section 8  $< $@

pacinstall$(MAN8EXT): | pactrans$(MAN8EXT)
	ln -fs $| $@

pacremove$(MAN8EXT): | pactrans$(MAN8EXT)
	ln -fs $| $@

doc: $(MANPAGES)

install: $(MANPAGES)
	install -d "${DESTDIR}${MAN8DIR}"
	install -m 644 $^ "${DESTDIR}${MAN8DIR}"

clean:
	$(RM) $(MANPAGES) pacinstall$(MAN8EXT) pacremove$(MAN8EXT)

.PHONY: all doc
