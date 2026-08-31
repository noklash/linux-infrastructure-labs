CC      = gcc

CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2 -g

LDFLAGS =

SRC_DIR = src

OBJ_DIR = obj

BIN     = procwatch

SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/proc.c $(SRC_DIR)/display.c $(SRC_DIR)/util.c

OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all clean test install

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN)

test: $(BIN)
	./tests/test_basic.sh

install: $(BIN)
	install -m 755 $(BIN) /usr/local/bin/