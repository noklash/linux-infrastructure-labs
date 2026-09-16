CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c17

TARGET = fswatch
SRC = src/main.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

test: $(TARGET)
	./tests/test_info.sh

clean:
	rm -f $(TARGET)