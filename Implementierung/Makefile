CFLAGS= -O3  -Wall -Wextra -Wpedantic -std=gnu11 -g
LDFLAGS=-lm

.PHONY: all clean

all: sqrt2
sqrt2: main.c sqrt2.c operations.c operations.S 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
clean:
	rm -f sqrt2
