# ZipIt - Modern C++ Zip Archive Utility

A fast, modern, and robust command-line zip archive utility written in C++17.

## Features

- **Archive Creation** - Create zip archives from files and directories
- **Archive Extraction** - Extract files with full metadata preservation
- **Archive Appending** - Add files to existing archives
- **Archive Validation** - Check archive integrity and structure
- **Archive Listing** - Display contents in hierarchical format
- **Archive Searching** - Find files using wildcard patterns
- **Cross-Platform** - Works on Windows, Linux, and macOS
- **Modern C++** - Built with C++17 and best practices

## Installation

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 6+, MSVC 2017+)
- CMake 3.12 or higher
- zlib development library

### Building from Source

```bash
git clone https://github.com/yourusername/zipit.git
cd zipit
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

On Windows with Visual Studio:

```cmd
git clone https://github.com/yourusername/zipit.git
cd zipit
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

## Usage

### Basic Commands

```bash
# Create a new archive
zipit create archive.zip file1.txt file2.txt

# Create archive with directory (recursive)
zipit create archive.zip -r mydirectory/

# Extract archive to current directory
zipit extract archive.zip

# Extract to specific directory
zipit extract archive.zip /path/to/extract

# List archive contents
zipit list archive.zip

# Validate archive
zipit check archive.zip

# Search for files in archive
zipit search archive.zip "*.txt"

# Append files to existing archive
zipit append archive.zip newfile.txt
```

### Global Options

- `-v, --verbose` - Enable verbose output
- `-q, --quiet` - Minimal output
- `-f, --force` - Force overwrite existing files
- `-r, --recursive` - Process directories recursively
- `-h, --help` - Show help information
- `--version` - Show version information

### Command Reference

#### `create <archive.zip> <files...>`
Create a new zip archive from specified files and directories.

**Options:**
- `-r, --recursive` - Include directory contents recursively

**Examples:**
```bash
zipit create backup.zip documents/ photos/
zipit create -r project.zip src/
```

#### `extract <archive.zip> [destination]`
Extract files from a zip archive.

**Options:**
- `-f, --force` - Overwrite existing files

**Examples:**
```bash
zipit extract backup.zip
zipit extract backup.zip /tmp/restore/
```

#### `list <archive.zip>`
Display archive contents with file information.

**Output includes:**
- File name and path
- Original and compressed sizes
- Compression ratio
- Last modified timestamp

#### `check <archive.zip>`
Validate archive structure and integrity.

**Checks performed:**
- Zip file format compliance
- Central directory integrity
- File checksums
- Compression method validity

#### `search <archive.zip> <pattern>`
Search for files matching a pattern.

**Pattern syntax:**
- `*` - Match any characters
- `?` - Match single character
- Case-insensitive on Windows, case-sensitive on Unix

#### `append <archive.zip> <files...>`
Add files to an existing zip archive.

**Options:**
- `-r, --recursive` - Include directory contents recursively

## Development

### Project Structure

```
zipit/
├── CMakeLists.txt              # Build configuration
├── LICENSE                     # MIT License
├── README.md                   # This file
├── src/                        # Source code
│   ├── main.cpp                # Command-line interface
│   ├── commands/               # Command implementations
│   ├── compression/            # Compression engine
│   ├── core/                   # Core archive logic
│   └── utils/                  # Utility functions
├── include/                    # Public headers
├── tests/                      # Unit tests
├── examples/                   # Example usage
└── build/                      # Build artifacts (generated)
```

### Building for Development

```bash
# Debug build with symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

### Running Tests

```bash
# Build with tests enabled
cmake -DBUILD_TESTS=ON ..
cmake --build .

# Run tests
ctest
```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Code Style

- Follow modern C++ best practices (C++17)
- Use meaningful variable and function names
- Include comprehensive error handling
- Add unit tests for new functionality
- Document public APIs with Doxygen comments
- Ensure cross-platform compatibility

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Performance

ZipIt is optimized for:
- Large file handling
- Memory efficiency
- Fast compression/decompression
- Minimal disk I/O

Typical benchmark results:
- **Compression**: 50-100 MB/s (varies by file type)
- **Decompression**: 200-500 MB/s
- **Memory usage**: < 50MB for typical operations
- **Archive creation**: Linear time complexity

## Roadmap

- [ ] GUI interface option
- [ ] Parallel compression support
- [ ] Additional compression algorithms (LZMA, BZIP2)
- [ ] Archive encryption
- [ ] Split archive creation
- [ ] Progress indicators for large operations
- [ ] Cloud storage integration

## Support

- **Issues**: Report bugs and feature requests via [GitHub Issues](https://github.com/yourusername/zipit/issues)
- **Discussions**: Join conversations in [GitHub Discussions](https://github.com/yourusername/zipit/discussions)
- **Documentation**: See [Wiki](https://github.com/yourusername/zipit/wiki) for detailed guides

## Acknowledgments

- [zlib](https://zlib.net/) for compression functionality
- The C++ community for inspiration and best practices

---

**ZipIt** - Making zip archives simple and efficient.