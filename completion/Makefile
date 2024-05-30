PREFIX      ?= /usr/local
DATAROOTDIR ?= $(PREFIX)/share
ZSH_COMPLETION_DIR ?= $(DATAROOTDIR)/zsh/site-functions

ZSH_COMPLETION_FILES = \
	zsh/_paccapability \
	zsh/_paccheck \
	zsh/_pacconf \
	zsh/_pacfile \
	zsh/_pacinfo \
	zsh/_pacini \
	zsh/_paclock \
	zsh/_paclog \
	zsh/_pacrepairdb \
	zsh/_pacrepairfile \
	zsh/_pacreport \
	zsh/_pacsift \
	zsh/_pacsync \
	zsh/_pactrans

install:
	install -d "${DESTDIR}${ZSH_COMPLETION_DIR}"
	install -m 644 $(ZSH_COMPLETION_FILES) "${DESTDIR}${ZSH_COMPLETION_DIR}"

.PHONY: install
