all: debug_bld

CXXFLAGS += -c -Wall -Wextra -Werror -std=c++20
INCS := -I$(PROJECT_HOME)/src -I$(PROJECT_HOME)/include

SOURCES := perf_main.cpp config.cpp
OBJS := $(subst .cpp,.o,$(SOURCES))

LIBS := -lpthread
STATIC_LIBS := 

export

.PHONY: release_bld
release_bld:
	$(MAKE) -C release

.PHONY: debug_bld
debug_bld:
	$(MAKE) -C debug

.PHONY: test
test:
	$(MAKE) -C debug test

.PHONY: run
run:
	$(MAKE) -C debug run

PHONY: clean
clean:
	$(MAKE) -C debug clean
	$(MAKE) -C release clean

