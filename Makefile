# Makefile for compiling StarScout

# Compiler settings - Can change cc and CFLAGS to whatever is required
CC = g++
CFLAGS = -Wall

# Target executable name
TARGET = StarScout

# Source files
SOURCES = StarScout.cpp ScoutNetworkUtilities.cpp

# Build target
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

# Clean the built files
clean:
	rm -f $(TARGET)
