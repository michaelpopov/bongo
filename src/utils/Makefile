all: debug_bld

CXXFLAGS += -c -Wall -Wextra -Werror -std=c++20
INCS := -I$(PROJECT_HOME)/src -I$(PROJECT_HOME)/include

SOURCES := pipe_queue.cpp data_buffer.cpp
OBJS := $(subst .cpp,.o,$(SOURCES))

UTEST_MAIN=$(PROJECT_HOME)/src/utils/utest_main.cpp
TEST_SOURCES := utest_data_buffer.cpp utest_pipe_queue.cpp
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

PHONY: clean
clean:
	$(MAKE) -C debug clean
	$(MAKE) -C release clean

