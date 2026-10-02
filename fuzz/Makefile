# Makefile for fuzzing roaring buffer functions without PostgreSQL deps
#
# Usage:
#   make clean
#   make
#   ./fuzz_buffer_reader -max_total_time=60 corpus/

# Configuration
FUZZER_NAME = fuzz_buffer_reader
CORPUS_DIR = corpus
SRC_DIR = ..

# Try to find clang
CC ?= $(shell which clang)

# Fuzzing flags
FUZZ_CFLAGS = -fsanitize=fuzzer,address,undefined -g -O1 -fno-omit-frame-pointer

# Additional flags
CFLAGS = $(FUZZ_CFLAGS) \
         -I$(SRC_DIR) \
         -std=c11 \
         -Wno-unused-function \
         -Wno-unused-variable \
         -Wno-missing-prototypes

# Source files
FUZZ_SOURCES = $(FUZZER_NAME).c

# Build targets
.PHONY: all clean corpus test quick_fuzz help

all: $(FUZZER_NAME)

# Build the fuzzer binary
$(FUZZER_NAME): $(FUZZ_SOURCES)
	@echo "Compiling roaring buffer fuzzer..."
	$(CC) $(CFLAGS) -o $@ $^

# Test compilation without running
test: $(FUZZER_NAME)
	@echo "✓ Roaring buffer fuzzer compiled successfully: $(FUZZER_NAME)"
	@echo "Run with: ./$(FUZZER_NAME) -max_total_time=60 $(CORPUS_DIR)/"

# Clean build artifacts
clean:
	@rm -f $(FUZZER_NAME)
	@echo "Cleaned build artifacts"

# Quick fuzzing session (60 seconds)
quick_fuzz: $(FUZZER_NAME) corpus
	@echo "Starting 60-second fuzzing session..."
	./$(FUZZER_NAME) -max_total_time=60 -print_final_stats=1 $(CORPUS_DIR)/

# Long fuzzing session (10 minutes)
long_fuzz: $(FUZZER_NAME) corpus
	@echo "Starting 10-minute fuzzing session..."
	./$(FUZZER_NAME) -max_total_time=600 -print_final_stats=1 $(CORPUS_DIR)/

# Debug run with single input
debug: $(FUZZER_NAME)
	@echo "Debug run with minimal input..."
	@printf '\x3a\x30\x00\x00\x00\x00\x00\x00' | ./$(FUZZER_NAME)

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build the minimal fuzzer"
	@echo "  test         - Build and test compilation"
	@echo "  quick_fuzz   - Run 60-second fuzzing session"
	@echo "  long_fuzz    - Run 10-minute fuzzing session"
	@echo "  debug        - Debug run with minimal input"
	@echo "  clean        - Clean build artifacts"
	@echo "  help         - Show this help"