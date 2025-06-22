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
	$(MAKE) -C net release_bld

.PHONY: debug
debug:
	$(MAKE) -C utils debug_bld
	$(MAKE) -C net debug_bld

.PHONY: clean
clean:
	$(MAKE) -C utils clean
	$(MAKE) -C net clean
	rm -f $(PROJECT_HOME)/bin/* $(PROJECT_HOME)/bin-dbg/* $(PROJECT_HOME)/lib/* $(PROJECT_HOME)/lib-dbg/*


