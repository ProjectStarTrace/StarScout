# Makefile for compiling StarScout

# Compiler settings - Can change cc and CFLAGS to whatever is required
CC = g++
CFLAGS = -Wall
LDFLAGS = -lcurl # Linking flags, including libcurl


# Target executable name
TARGET = StarScout

# Source files
SOURCES = StarScout.cpp ScoutNetworkUtilities.cpp FirebaseUploader.cpp


# Build target
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

# Clean the built files
clean:
	rm -f $(TARGET)
	rm -f *.o
