# Makefile for compiling StarScout

# Compiler settings - Can change cc and CFLAGS to whatever is required
CC = g++
CFLAGS = -Wall

# Target executable name
TARGET = StarScout

# Build target
$(TARGET): StarScout.cpp
	$(CC) $(CFLAGS) StarScout.cpp -o $(TARGET)

# Clean the built files
clean:
	rm -f $(TARGET)
