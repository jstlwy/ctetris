CC := gcc
CFLAGS := -Wall

OS := $(shell uname)
ifeq ($(OS), Darwin)
LDFLAGS := -lncurses
else
LDFLAGS := -lncurses -lbsd
endif

.PHONY: all tetris clean

bin := tetris

all: $(bin)

$(bin): main.o
	$(CC) $^ -o $@ $(LDFLAGS)

# Generic object file creation rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(bin)
