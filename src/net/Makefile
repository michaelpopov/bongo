all: debug_bld

CXXFLAGS += -c -Wall -Wextra -Werror -std=c++20
INCS := -I$(PROJECT_HOME)/src -I$(PROJECT_HOME)/include

SOURCES := block_conn.cpp nonblock_conn.cpp net_session.cpp
OBJS := $(subst .cpp,.o,$(SOURCES))

UTEST_MAIN=$(PROJECT_HOME)/src/utils/utest_main.cpp
TEST_SOURCES := utest_block_conn.cpp utest_nonblock_conn.cpp
TEST_OBJS := $(subst .cpp,.o,$(TEST_SOURCES))

LIBS :=  -lgtest -lpthread
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

