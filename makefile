CC=gcc
FLAGS=-I/usr/X11R6/include -L/usr/X11R6/lib -lX11

all: pop.h
	$(CC) $(FLAGS) pop.c -o pop