OBJS = main.o ptrace.o

CXX = clang++
CXXFLAGS = -Weverything -Weffc++

all : cfg

cfg : $(OBJS) Makefile
	$(CXX) $(OBJS) -o $@

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	
.PHONY : clean

clean :
	$(RM) $(OBJS)
	$(RM) cfg
