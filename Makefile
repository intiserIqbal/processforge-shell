CC=gcc
CFLAGS=-Wall -Wextra -Werror -g -O0
LDFLAGS=-lreadline -lncurses

SRC=src/main.c src/parser.c src/executor.c src/builtins.c src/jobs.c src/signals.c src/logging.c src/scheduler.c
OUT=build/processforge

all:
	mkdir -p build
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -rf build