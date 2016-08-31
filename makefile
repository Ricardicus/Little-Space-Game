CC=gcc
FLAGS=-I/usr/X11R6/include -L/usr/X11R6/lib -lX11

all: src/pop.h
	$(CC) $(FLAGS) src/pop.c -o pop