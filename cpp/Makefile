CPP = g++
CCFLAGS=-std=c++17

grid-debug: grid.cpp main.cpp
	$(CPP) -g -pedantic -Wall -c grid.cpp $(CCFLAGS)
	$(CPP) -g -pedantic -Wall -c main.cpp $(CCFLAGS)
	$(CPP) main.o grid.o -o grid-debug

grid-release: grid.cpp
	$(CPP) -O3 -Wall -pedantic main.cpp grid.cpp -o grid-release $(CCFLAGS)
