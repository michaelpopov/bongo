all: libutils.a utest

CXXFLAGS += -g
STATIC_LIBS += $(PROJECT_HOME)/lib-dbg/libutils.a

%.o : ../%.cpp
	$(CXX) $(CXXFLAGS) $(INCS) $< -o $@ -MMD

libutils.a: $(OBJS)
	ar rcs $@ $(OBJS) && cp $@ $(PROJECT_HOME)/lib-dbg/.

utest: $(UTEST_MAIN) $(TEST_OBJS) $(STATIC_LIBS)
	$(CXX) $(CXXLFLAGS) $(INCS) -o $@ $^ $(STATIC_LIBS) $(LIBS)

test: utest
	./utest

clean:
	rm -rf *.o
	rm -rf *.d
	rm -rf *.a
	rm -rf utest
	rm -rf work

-include $(subst .cpp,.d,$(SOURCES))
-include $(subst .cpp,.d,$(TEST_SOURCES))

