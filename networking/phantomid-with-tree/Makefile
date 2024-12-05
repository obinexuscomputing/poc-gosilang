# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -I/usr/include/openssl
LDFLAGS := -pthread
LIBS := -lssl -lcrypto -lrt

# Directories
SRC_DIR := .
OBJ_DIR := obj
BIN_DIR := bin

# Source files and objects
SRCS := main.c network.c phantomid.c
OBJS := $(SRCS:%.c=$(OBJ_DIR)/%.o)

# Binary name
TARGET := $(BIN_DIR)/phantomid

# Header files
DEPS := network.h phantomid.h

# Create directories
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# Default target
.PHONY: all
all: $(TARGET)

# Link the final binary
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LIBS)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

# Debug build
.PHONY: debug
debug: CFLAGS += -g -DDEBUG
debug: clean all

# Run the program
.PHONY: run
run: $(TARGET)
	$(TARGET)

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all    - Build the project (default)"
	@echo "  clean  - Remove build files"
	@echo "  debug  - Build with debug symbols"
	@echo "  run    - Build and run the program"
	@echo "  help   - Show this help message"