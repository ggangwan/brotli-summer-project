# v0.4.0/testing/Makefile

# Compiler
CXX = g++

# Directories
ENC_DIR = ../enc
DEC_DIR = ../dec

# Include paths
INCLUDES = -I$(ENC_DIR) -I$(DEC_DIR)

# Object files
ENC_OBJS = $(wildcard $(ENC_DIR)/*.o)
DEC_OBJS = $(wildcard $(DEC_DIR)/*.o)

# Source and target
TARGET = final_test
SRCS = final_test.cpp

# Default target
all: $(TARGET)

# Link the program
$(TARGET): $(SRCS)
	$(CXX) -o $@ $^ $(INCLUDES) $(ENC_OBJS) $(DEC_OBJS)
	@echo "Build complete -> Brotli v0.4.0"
	@echo "Usage: ./final_test -f <file_path> -c <compression_quality> -w <window_bits> -m <mode>"
	@echo "  -f <file_path>              : Path to the input file"
	@echo "  -c <compression_quality>    : Compression quality (1 to 11)"
	@echo "  -w <window_bits>            : Number of window bits (10 to 24)"
	@echo "  -m <mode>                   : Mode ('compress', 'decompress', 'both')"


# Clean up build files
clean:
	rm -f $(TARGET)

.PHONY: all clean
