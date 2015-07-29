PREFIX      ?= /usr/local
DATAROOTDIR ?= $(PREFIX)/share
MANDIR      ?= $(DATAROOTDIR)/man
MAN8DIR     ?= $(MANDIR)/man8
MAN8EXT     ?= .8

MANPAGES = \
		   paccheck$(MAN8EXT) \
		   pacconf$(MAN8EXT) \
		   pacfile$(MAN8EXT) \
		   pacinfo$(MAN8EXT) \
		   pacini$(MAN8EXT) \
		   paclog$(MAN8EXT) \
		   pacreport$(MAN8EXT) \
		   pacsift$(MAN8EXT) \
		   pacsync$(MAN8EXT) \
		   pactrans$(MAN8EXT)

all: doc

%$(MAN8EXT): %.pod
	pod2man --center $* --release pacutils --section 8  $< $@

pacinstall$(MAN8EXT): | pactrans$(MAN8EXT)
	ln -fs $| $@

pacremove$(MAN8EXT): | pactrans$(MAN8EXT)
	ln -fs $| $@

doc: $(MANPAGES) pacinstall$(MAN8EXT) pacremove$(MAN8EXT)

install: doc
	install -d "${DESTDIR}${MAN8DIR}"
	install -m 644 $(MANPAGES) "${DESTDIR}${MAN8DIR}"
	cp -d pacinstall$(MAN8EXT) "${DESTDIR}${MAN8DIR}"
	cp -d pacremove$(MAN8EXT)  "${DESTDIR}${MAN8DIR}"

clean:
	$(RM) $(MANPAGES) pacinstall$(MAN8EXT) pacremove$(MAN8EXT)

.PHONY: all doc
