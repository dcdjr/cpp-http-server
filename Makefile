# Compiler
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread -O2 -Iinclude

# Directories
SRC_DIR = src
BIN_DIR = bin

# Target binary
TARGET = $(BIN_DIR)/server

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:.cpp=.o)

# Default rule
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

# Compile
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run server
run: all
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(SRC_DIR)/*.o
	rm 	-f $(TARGET)

.PHONY: all run clean

