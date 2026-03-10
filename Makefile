CC=gcc
CFLAGS=-Wall -Wextra -Werror -g -O0
SRC=src/main.c src/shell.c
OUT=build/myshell

all:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -Iinclude -o $(OUT)

clean:
	rm -rf build