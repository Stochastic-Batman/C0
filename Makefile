CC = gcc
CFLAGS = -Wall -Wextra -g  # Warnings and debug info

SRC_DIR = src
BIN_DIR = bin
TARGET = $(BIN_DIR)/compiler

SOURCES = $(wildcard $(SRC_DIR)/*.c)  # All .c files in src/
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(OBJECTS) -o $@

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

test_scanner: $(BIN_DIR)/main.o $(BIN_DIR)/scanner.o | $(BIN_DIR)
	$(CC) $^ -o $(BIN_DIR)/test_scanner

clean:
	rm -rf $(BIN_DIR)

test: $(TARGET)
	./$(TARGET) tests/simple.c0 > output.asm
