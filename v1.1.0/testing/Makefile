# v1.1.0/testing/Makefile

# Compiler
CXX = g++

# Directories
INCLUDE_DIR = ../include
LIB_DIR = ../lib

# Include and library paths
INCLUDES = -I$(INCLUDE_DIR)
LIBS = -L$(LIB_DIR) -lbrotlicommon.1.1.0 -lbrotlidec.1.1.0 -lbrotlienc.1.1.0

# Source and target
TARGET = final_test
SRCS = final_test.cpp

# Default target
all: $(TARGET)

# Link the program
$(TARGET): $(SRCS)
	$(CXX) -o $@ $^ $(INCLUDES) $(LIBS)
	@echo "Build complete -> Brotli v1.1.0"
	@echo "Usage: ./final_test -f <file_path> -c <compression_quality> -w <window_bits> -m <mode>"
	@echo "  -f <file_path>              : Path to the input file"
	@echo "  -c <compression_quality>    : Compression quality (1 to 11)"
	@echo "  -w <window_bits>            : Number of window bits (10 to 24)"
	@echo "  -m <mode>                   : Mode ('compress', 'decompress', 'both')"
	@echo "  -d <dictionary_path>        : Path to the dictionary (optional)"

# Clean up build files
clean:
	rm -f $(TARGET)

.PHONY: all clean
