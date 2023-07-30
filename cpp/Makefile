CPP = g++
CCFLAGS = -Wall -pedantic -std=c++17
LIBS = /usr/lib/x86_64-linux-gnu/libboost_program_options.so

grid-debug: grid.o main.o
	$(CPP) main.o grid.o $(LIBS) -o grid-debug

grid.o: grid.cpp grid.hpp
	$(CPP) -g -c $(CCFLAGS) grid.cpp

main.o: main.cpp grid.hpp
	$(CPP) -g -c $(CCFLAGS) main.cpp

grid-release: main.cpp grid.cpp grid.hpp
	$(CPP) -O3 -Wall -pedantic main.cpp grid.cpp $(LIBS) -o grid-release $(CCFLAGS)

clean:
	rm -f *.o grid-debug grid-release
