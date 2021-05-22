all: lib src

lib:
	$(MAKE) -C $@ all

src: lib
	$(MAKE) -C $@ all

check: lib src
	$(MAKE) -C t/ check

doc:
	$(MAKE) -C doc/ $@

tidy:
	astyle --options=.astylerc --exclude=ext --recursive "*.c,*.h" --suffix=none

install: lib
	$(MAKE) -C doc/ $@
	$(MAKE) -C lib/ $@
	$(MAKE) -C src/ $@

clean:
	$(MAKE) -C doc/ $@
	$(MAKE) -C lib/ $@
	$(MAKE) -C src/ $@
	$(MAKE) -C t/ $@

.PHONY: all check clean doc install lib src tidy
