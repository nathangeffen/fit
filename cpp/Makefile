CPP = g++
CCFLAGS := -Wall -pedantic -std=c++17
LIB_BOOST := $(shell ldconfig -p  | grep libboost_program_options.so | awk  '{print $$4}' | tail -1)
LIB_GSL := $(shell pkg-config --cflags --libs gsl)
LIBS := $(LIB_BOOST) $(LIB_GSL)

fit-debug: fit.o main.o
	$(CPP) main.o fit.o $(LIBS) -o fit-debug

fit.o: fit.cpp fit.hpp
	$(CPP) -g -c $(CCFLAGS) fit.cpp

main.o: main.cpp fit.hpp
	$(CPP) -g -c $(CCFLAGS) main.cpp

fit-release: main.cpp fit.cpp fit.hpp
	$(CPP) -O3 -Wall -pedantic main.cpp fit.cpp $(LIBS) -o fit-release $(CCFLAGS)

clean:
	rm -f *.o fit-debug fit-release

