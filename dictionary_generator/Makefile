# Compiler and flags
CC = g++
CFLAGS = -Wall -g -std=c++11 -Iinclude  # Enable C++11
CFILES = divsufsort.c sssort.c trsort.c                  # C source file
CPPFILES = dict_gen.cc deorummolae.cc durchschlag.cc sieve.cc  # C++ source files
OBJFILES = $(CFILES:.c=.o) $(CPPFILES:.cc=.o)  # Object files
TARGET = dict_gen                     # Name of the final executable

# Build target
all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $(OBJFILES)

%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJFILES) $(TARGET)
