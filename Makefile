
SRCS=src/agent.cpp src/store.cpp src/database.cpp
OBJS=$(patsubst %.cpp, %.o, $(SRCS))
EXEC=build/libtracer.jnilib

CXX=clang++
CC=clang

CXXFLAGS=-Wall -O2 -fPIC -std=c++98
CFLAGS=-Wall -O2 -fPIC

OFLAGS=-Wl,-all_load -framework JavaVM
LFLAGS=-L/opt/local/lib/ -shared -lboost_thread-mt -lpthread

INC=-I. -I/opt/local/include -I/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk/System/Library/Frameworks/JavaVM.framework/Versions/A/Headers

%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(INC) $< -o $@

$(EXEC) : $(OBJS)
	$(CXX) -o $(EXEC) $(OFLAGS) $(OBJS) src/sqlite3.o $(LFLAGS)


.PHONY: clean
clean:
	@rm -f $(OBJS) $(EXEC)

sqlite:
	$(CC) -c $(CFLAGS) -I. src/sqlite3.c -o src/sqlite3.o
