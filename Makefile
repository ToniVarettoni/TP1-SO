CC = gcc
CFLAGS = -Wall -O0 -lrt -lm -std=c99 -D_POSIX_C_SOURCE=200809L -g
SRC_DIR = include
COMMON_SRCS = $(SRC_DIR)/shared_memory.c $(SRC_DIR)/process_manager.c $(SRC_DIR)/game_utils.c

BINARIES = MAX view master

all: $(BINARIES)

MAX: maxPlayer.c $(COMMON_SRCS)
	$(CC) $^ -o $@ $(CFLAGS)

view: view.c $(COMMON_SRCS)
	$(CC) $^ -o $@ $(CFLAGS)

master: master.c $(COMMON_SRCS)
	$(CC) $^ -o $@ $(CFLAGS)

run-docker:
	docker run -v "${PWD}:/root" --privileged -ti agodio/itba-so-multi-platform:3.0

clean:
	rm -f $(BINARIES)

.PHONY: all clean run-docker