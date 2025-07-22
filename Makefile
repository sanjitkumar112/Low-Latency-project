CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -pthread -march=native -mtune=native -I.
TARGET = ring_buffer_demo
SOURCES = main.cpp src/network_sim/tcp_sim.cpp src/network_sim/udp_sim.cpp src/network_sim/shm_sim.cpp
HEADERS = order.hpp ring_buffer.hpp batcher.hpp

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Run the demo
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build
release: $(TARGET)

# Install dependencies (macOS)
install-deps:
	brew install cmake

# Install dependencies (Ubuntu/Debian)
install-deps-ubuntu:
	sudo apt-get update
	sudo apt-get install -y build-essential cmake

.PHONY: all clean run debug release install-deps install-deps-ubuntu 