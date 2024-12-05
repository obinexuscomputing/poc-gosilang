# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -Werror -O2
LDFLAGS := -pthread
LIBS := -lssl -lcrypto -lrt

# Directories
SRC_DIR := .
OBJ_DIR := obj
BIN_DIR := bin

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Binary name
TARGET := $(BIN_DIR)/phantomid

# Header dependencies
DEPS := $(wildcard *.h)

# Create directories if they don't exist
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
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Install the binary to system
.PHONY: install
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall the binary
.PHONY: uninstall
uninstall:
	rm -f /usr/local/bin/phantomid

# Run tests (placeholder)
.PHONY: test
test: $(TARGET)
	@echo "Running tests..."
	# Add test commands here

# Generate documentation (placeholder)
.PHONY: docs
docs:
	@echo "Generating documentation..."
	# Add documentation generation commands here

# Debug build
.PHONY: debug
debug: CFLAGS += -g -DDEBUG
debug: clean all

# Memory check with valgrind
.PHONY: memcheck
memcheck: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(TARGET)

# Format source code using clang-format
.PHONY: format
format:
	clang-format -i $(SRCS) $(DEPS)

# Static analysis using cppcheck
.PHONY: check
check:
	cppcheck --enable=all --suppress=missingIncludeSystem $(SRCS)

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all       - Build the project (default)"
	@echo "  clean     - Remove build files"
	@echo "  install   - Install the binary to /usr/local/bin"
	@echo "  uninstall - Remove the installed binary"
	@echo "  test      - Run tests"
	@echo "  docs      - Generate documentation"
	@echo "  debug     - Build with debug symbols"
	@echo "  memcheck  - Run with valgrind memory checker"
	@echo "  format    - Format source code"
	@echo "  check     - Run static analysis"
	@echo "  help      - Show this help message"