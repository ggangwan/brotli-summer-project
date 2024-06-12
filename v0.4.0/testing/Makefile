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
	@echo "Build complete. Run ./$(TARGET) to execute."

# Clean up build files
clean:
	rm -f $(TARGET)

.PHONY: all clean
