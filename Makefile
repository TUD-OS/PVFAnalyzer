SHARED_OBJS = input.o
OBJS 		= main.o ptrace.o $(SHARED_OBJS)
TEST_OBJS 	= test.o $(SHARED_OBJS)

CXX ?= clang++
ifeq ($(CXX),clang++)
CXXFLAGS = -Weverything
endif
CXXFLAGS += -Weffc++ -std=c++0x

all : cfg test

cfg : $(OBJS) Makefile
	$(CXX) $(OBJS) -o $@

test : $(TEST_OBJS) Makefile
	$(CXX) $(TEST_OBJS) -o $@
	./test

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	
.PHONY : clean cleanall

clean cleanall :
	$(RM) $(OBJS) test.o
	$(RM) cfg test
	$(RM) *~
