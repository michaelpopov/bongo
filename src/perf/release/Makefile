all: http_perf

CXXFLAGS += -g
STATIC_LIBS += $(PROJECT_HOME)/lib/libproc.a \
               $(PROJECT_HOME)/lib/libnet.a \
               $(PROJECT_HOME)/lib/libutils.a

%.o : ../%.cpp
	$(CXX) $(CXXFLAGS) $(INCS) $< -o $@ -MMD

http_perf: $(OBJS) $(STATIC_LIBS)
	$(CXX) $(CXXLFLAGS) $(INCS) -o $@ $^ $(STATIC_LIBS) $(LIBS)  && cp $@ $(PROJECT_HOME)/bin/.

clean:
	rm -f http_perf
	rm -f *.o
	rm -f *.d
	rm -f *.a
	rm -f utest
	rm -f work

-include $(subst .cpp,.d,$(SOURCES))
-include $(subst .cpp,.d,$(TEST_SOURCES))

