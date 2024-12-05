# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -DLINUX
LDFLAGS := -pthread

# Try to detect OpenSSL with pkg-config if available
PKG_CONFIG := $(shell which pkg-config 2>/dev/null)
ifneq ($(PKG_CONFIG),)
    CFLAGS += $(shell pkg-config --cflags openssl)
    LIBS := $(shell pkg-config --libs openssl)
else
    # Fallback to standard paths if pkg-config is not available
    CFLAGS += -I/usr/include/openssl
    LIBS := -lssl -lcrypto
endif

LIBS += -lrt

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
all: check_env $(TARGET)

# Environment check
.PHONY: check_env
check_env:
	@echo "Checking build environment..."
	@if [ ! -f /usr/include/openssl/ssl.h ]; then \
		echo "Error: OpenSSL development files not found"; \
		echo "Please install libssl-dev (Debian/Ubuntu) or openssl-devel (RHEL/CentOS)"; \
		echo "Run:"; \
		echo "  Debian/Ubuntu: sudo apt-get install libssl-dev"; \
		echo "  RHEL/CentOS:  sudo yum install openssl-devel"; \
		exit 1; \
	fi
	@echo "OpenSSL development files found"

# Link the final binary
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LIBS)
	@echo "Build complete."

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
.PHONY: clean
clean:
	@echo "Cleaning build files..."
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*
	@echo "Clean complete."

# Debug build
.PHONY: debug
debug: CFLAGS += -g -DDEBUG
debug: clean all

# Run the program
.PHONY: run
run: $(TARGET)
	./$(TARGET)

# Help target
.PHONY: help
help:
	@echo "PhantomID Build System for Linux"
	@echo "-------------------------------"
	@echo "Available targets:"
	@echo "  all     - Build the project (default)"
	@echo "  clean   - Remove build files"
	@echo "  debug   - Build with debug symbols"
	@echo "  run     - Build and run the program"
	@echo "  help    - Show this help message"
	@echo
	@echo "Requirements:"
	@echo "  - GCC compiler"
	@echo "  - OpenSSL development files (libssl-dev)"
	@echo "  - POSIX threads support"