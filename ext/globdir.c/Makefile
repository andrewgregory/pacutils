check:                         ## build and run test suite
	$(MAKE) -C t/ check

clean:                         ## clean all built artifacts
	$(MAKE) -C t/ $@

update-ext:                    ## pull and commit updates to external modules
	git subtree pull --squash --prefix=ext/tap.c https://github.com/andrewgregory/tap.c.git master

help:                          ## show this help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

.PHONY: check clean update-ext help
