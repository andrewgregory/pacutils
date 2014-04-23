all: lib src

lib:
	$(MAKE) -C $@ all

src: lib
	$(MAKE) -C $@ all

check: lib src
	$(MAKE) -C t/ check

install: lib
	$(MAKE) -C lib/ $@
	$(MAKE) -C src/ $@

clean:
	$(MAKE) -C lib/ $@
	$(MAKE) -C src/ $@
	$(MAKE) -C t/ $@

.PHONY: lib src check
