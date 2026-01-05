CC = gcc
CFLAGS = -Wall -Wextra -g  # Warnings and debug info

SRC_DIR = src
TARGET = C0_compiler
OBJ_DIR = .obj

SOURCES = $(wildcard $(SRC_DIR)/*.c)  # All .c files in src/
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@
	rm -rf $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

.PHONY: all
