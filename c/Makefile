grid-debug:  main.c fit.c fit.h
	gcc -Wall -g main.c fit.c -lgsl -lm `pkg-config --cflags --libs glib-2.0` `pkg-config --cflags --libs useful` -o fit

# fit: fit.c
#	gcc-12 -Wall -g fit.c -o fit -lgsl -lm

#fit-release: fit.c
#	gcc-12 -Wall -O3 fit.c -o fit -lgsl -lm
