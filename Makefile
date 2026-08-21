CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -O2 -Isrc
LDFLAGS ?= -lm

SRC_DIR := src
BUILD_DIR := build
BIN := $(BUILD_DIR)/jawab

SRCS := $(SRC_DIR)/jawab.c $(SRC_DIR)/main.c
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean run audit

all: $(BIN)

$(BIN): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/jawab.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(BIN)
	./$(BIN) ask "sovereignty" corpus/seed.txt

audit: $(BIN)
	./$(BIN) audit corpus/seed.txt

clean:
	rm -rf $(BUILD_DIR)
