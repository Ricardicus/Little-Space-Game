CC=gcc
LIBS=X11 pthread

all: src/pop.h
	$(CC) $(FLAGS) src/pop.c -o pop $(addprefix -l,$(LIBS))
