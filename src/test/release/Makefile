all: utest

CXXFLAGS += -g -O2
STATIC_LIBS += $(PROJECT_HOME)/lib/libproc.a \
               $(PROJECT_HOME)/lib/libnet.a \
               $(PROJECT_HOME)/lib/libutils.a

%.o : ../%.cpp
	$(CXX) $(CXXFLAGS) $(INCS) $< -o $@ -MMD

utest: $(UTEST_MAIN) $(TEST_OBJS) $(STATIC_LIBS)
	$(CXX) $(CXXLFLAGS) $(INCS) -o $@ $^ $(STATIC_LIBS) $(LIBS)

test: utest
	../utest.sh

clean:
	rm -f *.o
	rm -f *.d
	rm -f *.a
	rm -f utest
	rm -f work

-include $(subst .cpp,.d,$(SOURCES))
-include $(subst .cpp,.d,$(TEST_SOURCES))

