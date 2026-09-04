CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2 -g
DEBUG_CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O0 -g \
               -fsanitize=address -fsanitize=undefined
LDFLAGS =
DEBUG_LDFLAGS = -fsanitize=address -fsanitize=undefined

SRC_DIR = src
BUILD_DIR = build

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/cli.c \
       $(SRC_DIR)/util.c \
       $(SRC_DIR)/memory.c \
       $(SRC_DIR)/maps.c \
       $(SRC_DIR)/smaps.c \
       $(SRC_DIR)/proc.c \
       $(SRC_DIR)/analyzer.c \
       $(SRC_DIR)/display.c

OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
TARGET = memmap

.PHONY: all clean debug examples

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

debug: CFLAGS = $(DEBUG_CFLAGS)
debug: LDFLAGS = $(DEBUG_LDFLAGS)
debug: clean $(TARGET)

examples: examples/memory_lab

examples/memory_lab: examples/memory_lab.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR) $(TARGET) examples/memory_lab