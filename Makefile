OBJS = main.o ptrace.o

CXX = clang++
CXXFLAGS = -Weverything -Weffc++ -std=c++0x

all : cfg test

cfg : $(OBJS) Makefile
	$(CXX) $(OBJS) -o $@

test : test.o Makefile
	$(CXX) test.o -o $@
	./test

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	
.PHONY : clean cleanall

clean cleanall :
	$(RM) $(OBJS) test.o
	$(RM) cfg test
