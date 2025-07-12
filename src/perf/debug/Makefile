all: http_perf

CXXFLAGS += -g
STATIC_LIBS += $(PROJECT_HOME)/lib-dbg/libproc.a \
               $(PROJECT_HOME)/lib-dbg/libnet.a \
               $(PROJECT_HOME)/lib-dbg/libutils.a

%.o : ../%.cpp
	$(CXX) $(CXXFLAGS) $(INCS) $< -o $@ -MMD

http_perf: $(OBJS) $(STATIC_LIBS)
	$(CXX) $(CXXLFLAGS) $(INCS) -o $@ $^ $(STATIC_LIBS) $(LIBS) && cp $@ $(PROJECT_HOME)/bin-dbg/.

clean:
	rm -f http_perf
	rm -f *.o
	rm -f *.d
	rm -f *.a
	rm -f utest
	rm -f work

-include $(subst .cpp,.d,$(SOURCES))
-include $(subst .cpp,.d,$(TEST_SOURCES))

