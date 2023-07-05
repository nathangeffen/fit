CPP = g++

# FLAGS = `pkg-config --cflags --libs glib-2.0`

grid-debug: grid.cpp
	$(CPP) -g -Wall grid.cpp -o grid-debug $(FLAGS)

grid-release: grid.cpp
	$(CPP) -O3 -Wall grid.cpp -o grid-release
