CC=gcc
FLAGS=-I/usr/X11/include -L/usr/X11/lib -lX11

all: src/pop.h
	$(CC) $(FLAGS) src/pop.c -o pop
