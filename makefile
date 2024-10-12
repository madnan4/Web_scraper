# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++11 -Wall

# Libraries
LIBS = -lcurl -lgumbo

# Executable name
EXEC = scraper

# Source files
SRCS = main.cpp compare.cpp fetch.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(EXEC)

# Link the executable
$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean
