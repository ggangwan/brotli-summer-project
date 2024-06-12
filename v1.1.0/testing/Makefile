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
	@echo "Build complete. Run ./$(TARGET) to execute."

# Clean up build files
clean:
	rm -f $(TARGET)

.PHONY: all clean
