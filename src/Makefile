all: debug

CXXFLAGS += -Wall -Wextra -Werror -std=c++20

.PHONY: test
test:
	$(MAKE) -C utils test
	$(MAKE) -C net test

.PHONY: deploy
deploy:
	$(PROJECT_HOME)/script/deploy.sh

#.PHONY: utest
#utest:
#	$(MAKE) -C net test

.PHONY: prepare
prepare:

.PHONY: release
release:
	$(MAKE) -C utils release_bld
	$(MAKE) -C proc release_bld
	$(MAKE) -C net release_bld
	$(MAKE) -C test release_bld
	$(MAKE) -C perf release_bld

.PHONY: debug
debug:
	$(MAKE) -C utils debug_bld
	$(MAKE) -C proc debug_bld
	$(MAKE) -C net debug_bld
	$(MAKE) -C test debug_bld
	$(MAKE) -C perf debug_bld

.PHONY: test
test:
	$(MAKE) -C utils test
	$(MAKE) -C proc test
	$(MAKE) -C net test
	$(MAKE) -C test test

.PHONY: clean
clean:
	$(MAKE) -C utils clean
	$(MAKE) -C proc clean
	$(MAKE) -C net clean
	$(MAKE) -C test clean
	$(MAKE) -C perf clean
	rm -f $(PROJECT_HOME)/bin/* $(PROJECT_HOME)/bin-dbg/* $(PROJECT_HOME)/lib/* $(PROJECT_HOME)/lib-dbg/*


