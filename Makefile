CC := gcc
CFLAGS := -Wall -Wextra -Werror

OS := $(shell uname)
ifeq ($(OS), Darwin)
LDFLAGS := -lncurses
else
LDFLAGS := -lncurses -lbsd
endif

.PHONY: all clean

bin := tetris

all: $(bin)

$(bin): main.o
	$(CC) $(LDFLAGS) $^ -o $@

main.o: main.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f main.o $(bin)
