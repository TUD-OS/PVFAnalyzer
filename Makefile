SHARED_OBJS = input.o
OBJS 		= main.o ptrace.o $(SHARED_OBJS)
TEST_OBJS 	= test.o $(SHARED_OBJS)
DEPFILE     = depend

CXX ?= clang++
ifeq ($(CXX),clang++)
CXXFLAGS = -Weverything -Wno-padded
endif
CXXFLAGS += -Weffc++ -std=c++0x -MMD

all : cfg test

$(DEPFILE) : $(SHARED_OBJS) $(OBJS) $(TEST_OBJS)
	cat $(OBJS:.o=.d) $(TEST_OBJS:.o=.d) >$@

cfg : $(DEPFILE) Makefile $(OBJS)
	$(CXX) $(OBJS) -o $@

test : $(TEST_OBJS) $(DEPFILE) Makefile
	$(CXX) $(TEST_OBJS) -o $@
	./test

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

-include $(DEPFILE)
	
.PHONY : clean cleanall

clean cleanall :
	$(RM) $(OBJS) test.o
	$(RM) cfg test
	$(RM) *.d $(DEPFILE)
	$(RM) *~
