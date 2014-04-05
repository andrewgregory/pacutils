all: lib src

lib:
	$(MAKE) -C $@ all

src: lib
	$(MAKE) -C $@ all

install: lib
	$(MAKE) -C lib $@
	$(MAKE) -C src $@

clean:
	$(MAKE) -C lib $@
	$(MAKE) -C src $@

.PHONY: lib src
