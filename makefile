CC=gcc
LIBS=X11 pthread

X11INC?=
X11LIB?=

SRC_FOLDER=src
SOURCES=pop.c

.PHONY: pop

all: pop

pop: $(addprefix $(SRC_FOLDER)/,$(SOURCES))
	$(CC) -o $@ $^ $(addprefix -I,$(X11INC)) $(addprefix -L,$(X11LIB)) $(addprefix -l, $(LIBS))
