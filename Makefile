CC=gcc
CFLAGS=-Wall -g

SRC=src/main.c src/parser.c src/executor.c src/builtins.c src/signals.c
OUT=build/processforge

all:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -rf build