all: libnet.a utest

CXXFLAGS += -g
STATIC_LIBS += $(PROJECT_HOME)/lib-dbg/libproc.a \
               $(PROJECT_HOME)/lib-dbg/libnet.a \
               $(PROJECT_HOME)/lib-dbg/libutils.a

%.o : ../%.cpp
	$(CXX) $(CXXFLAGS) $(INCS) $< -o $@ -MMD

libnet.a: $(OBJS)
	ar rcs $@ $(OBJS) && cp $@ $(PROJECT_HOME)/lib-dbg/.

utest: $(UTEST_MAIN) $(TEST_OBJS) $(STATIC_LIBS)
	$(CXX) $(CXXLFLAGS) $(INCS) -o $@ $^ $(STATIC_LIBS) $(LIBS)

test: utest
	./utest

clean:
	rm -f $(PROJECT_HOME)/lib-dbg/libnet.a
	rm -f *.o
	rm -f *.d
	rm -f *.a
	rm -f utest
	rm -f work

-include $(subst .cpp,.d,$(SOURCES))
-include $(subst .cpp,.d,$(TEST_SOURCES))

