# Top-level Makefile for Linux (SAMD20 + CMake + OpenOCD)

# --- Configuration ---
BUILD_DIR := build
CMAKE := cmake
TOOLCHAIN_FILE := cmake/arm-none-eabi.cmake
BUILD_TYPE ?= Debug

# --- Phony targets ---
.PHONY: all configure build flash debug erase clean

# Default target
all: build

# --- Configure build directory ---
configure:
	@echo "Creating build directory and running initial CMake configure..."
	@mkdir -p $(BUILD_DIR)
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_FILE) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

# --- Build project ---
build: configure
	$(CMAKE) --build $(BUILD_DIR)

# --- Flash via OpenOCD ---
flash: build
	$(CMAKE) --build $(BUILD_DIR) --target flash

# --- Debug via GDB ---
debug: build
	$(CMAKE) --build $(BUILD_DIR) --target debug

# --- Erase device ---
erase:
	$(CMAKE) --build $(BUILD_DIR) --target erase

unlock:
	$(CMAKE) --build $(BUILD_DIR) --target unlock
# --- Clean build directory ---
clean:
	@echo "Removing build directory..."
	rm -rf $(BUILD_DIR)

