cmake_minimum_required(VERSION 3.12)
project(ZipIt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find zlib (optional for now)
find_package(ZLIB QUIET)

# If zlib not found, try to use vcpkg or provide fallback
if(NOT ZLIB_FOUND)
    message(STATUS "ZLIB not found, trying alternative methods...")
    
    # Try to find zlib in common locations
    find_path(ZLIB_INCLUDE_DIR zlib.h
        PATHS
        /usr/include
        /usr/local/include
        C:/vcpkg/installed/x64-windows/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/zlib/include
    )
    
    find_library(ZLIB_LIBRARY
        NAMES zlib zlibstatic z
        PATHS
        /usr/lib
        /usr/local/lib
        C:/vcpkg/installed/x64-windows/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/zlib/lib
    )
    
    if(ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)
        set(ZLIB_FOUND TRUE)
        set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
        set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
        message(STATUS "Found ZLIB: ${ZLIB_LIBRARY}")
    else()
        message(WARNING "ZLIB not found. Building without compression support.")
        message(WARNING "To enable compression, install zlib:")
        message(WARNING "  On Windows with vcpkg: vcpkg install zlib:x64-windows")
        message(WARNING "  On Ubuntu/Debian: sudo apt-get install zlib1g-dev")
        message(WARNING "  On macOS: brew install zlib")
        
        # Create stub definitions for building without zlib
        add_definitions(-DZIPIT_NO_ZLIB)
        set(ZLIB_LIBRARIES "")
        set(ZLIB_INCLUDE_DIRS "")
    endif()
endif()

# Include directories
include_directories(src)

# Source files
set(SOURCES
    src/main.cpp
    src/core/ZipArchive.cpp
    src/core/ZipEntry.cpp
    src/core/ZipReader.cpp
    src/core/ZipWriter.cpp
    src/compression/CompressionEngine.cpp
    src/utils/FileUtils.cpp
    src/utils/ArgumentParser.cpp
    src/utils/OutputFormatter.cpp
    src/commands/CreateCommand.cpp
    src/commands/ExtractCommand.cpp
    src/commands/AppendCommand.cpp
    src/commands/CheckCommand.cpp
    src/commands/ListCommand.cpp
    src/commands/SearchCommand.cpp
)

# Header files
set(HEADERS
    src/core/ZipArchive.h
    src/core/ZipEntry.h
    src/core/ZipReader.h
    src/core/ZipWriter.h
    src/compression/CompressionEngine.h
    src/utils/FileUtils.h
    src/utils/ArgumentParser.h
    src/utils/OutputFormatter.h
    src/commands/CreateCommand.h
    src/commands/ExtractCommand.h
    src/commands/AppendCommand.h
    src/commands/CheckCommand.h
    src/commands/ListCommand.h
    src/commands/SearchCommand.h
)

# Create executable
add_executable(zipit ${SOURCES} ${HEADERS})

# Link zlib
target_link_libraries(zipit ${ZLIB_LIBRARIES})

# Add zlib include directories
if(ZLIB_INCLUDE_DIRS)
    target_include_directories(zipit PRIVATE ${ZLIB_INCLUDE_DIRS})
endif()

# Platform-specific settings
if(WIN32)
    target_compile_definitions(zipit PRIVATE _WIN32_WINNT=0x0601)
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(zipit PRIVATE /W4)
else()
    target_compile_options(zipit PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Debug/Release configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(zipit PRIVATE DEBUG)
endif()

# Install target
install(TARGETS zipit DESTINATION bin)