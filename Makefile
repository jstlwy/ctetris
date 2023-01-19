CC := gcc
CFLAGS := -Wall
LDFLAGS := -lncurses
.PHONY: all tetris clean

bin := tetris

all: $(bin)

$(bin): main.o
	$(CC) $(LDFLAGS) $^ -o $@

# Generic object file creation rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(bin)
