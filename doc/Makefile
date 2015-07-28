PREFIX      ?= /usr/local
DATAROOTDIR ?= $(PREFIX)/share
MANDIR      ?= $(DATAROOTDIR)/man
MAN8DIR     ?= $(MANDIR)/man8
MAN8EXT     ?= .8

MANPAGES =

all: doc

%$(MAN8EXT): %.pod
	pod2man --center $* --release pacutils --section 8  $< $@

doc: $(MANPAGES)

install: $(MANPAGES)
	install -d "${DESTDIR}${MAN8DIR}"
	install -m 644 $^ "${DESTDIR}${MAN8DIR}"

clean:
	$(RM) $(MANPAGES)

.PHONY: all doc
