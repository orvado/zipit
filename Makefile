CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2
INCLUDES = -Isrc
LIBS = -lz

# Source directories
SRCDIR = src
COREDIR = $(SRCDIR)/core
COMMANDSDIR = $(SRCDIR)/commands
UTILSDIR = $(SRCDIR)/utils
COMPRESSIONDIR = $(SRCDIR)/compression

# Source files
SOURCES = \
	$(SRCDIR)/main.cpp \
	$(COREDIR)/ZipArchive.cpp \
	$(COREDIR)/ZipEntry.cpp \
	$(COREDIR)/ZipReader.cpp \
	$(COREDIR)/ZipWriter.cpp \
	$(COMPRESSIONDIR)/CompressionEngine.cpp \
	$(UTILSDIR)/FileUtils.cpp \
	$(UTILSDIR)/ArgumentParser.cpp \
	$(UTILSDIR)/OutputFormatter.cpp \
	$(COMMANDSDIR)/CreateCommand.cpp \
	$(COMMANDSDIR)/ExtractCommand.cpp \
	$(COMMANDSDIR)/AppendCommand.cpp \
	$(COMMANDSDIR)/CheckCommand.cpp \
	$(COMMANDSDIR)/ListCommand.cpp \
	$(COMMANDSDIR)/SearchCommand.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Target executable
TARGET = zipit

.PHONY: all clean debug test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	@echo "Running basic tests..."
	@mkdir -p test_files
	@echo "Test file content" > test_files/test.txt
	@echo "Another test file" > test_files/test2.txt
	@./$(TARGET) create test_files/test.zip test_files/test.txt test_files/test2.txt
	@./$(TARGET) list test_files/test.zip
	@./$(TARGET) extract test_files/test.zip test_extracted
	@ls -la test_extracted/
	@echo "Basic tests completed successfully!"

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all      - Build the zipit executable"
	@echo "  debug    - Build with debug symbols"
	@echo "  clean    - Remove build artifacts"
	@echo "  test     - Run basic functionality tests"
	@echo "  install  - Install to /usr/local/bin"
	@echo "  help     - Show this help message"