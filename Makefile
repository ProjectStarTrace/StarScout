# Simplified Makefile to compile and link StarScout.cpp

# Compiler settings - Customized for cross-compilation.
CXX = aarch64-linux-gnu-g++
CXXFLAGS = -std=c++17 -I$(HOME)/vcpkg/installed/arm64-linux/include
LDFLAGS = -L$(HOME)/vcpkg/installed/arm64-linux/lib -lcivetweb -lpthread

# Target executable name
TARGET = StarScout

# Source file
SRC = $(TARGET).cpp

# Object file
OBJ = $(SRC:.cpp=.o)

# Compile and link the program
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

$(OBJ): $(SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean
