cmake_minimum_required(VERSION 3.16)

project(SMWTestROMGenerator
    VERSION 1.0.0
    DESCRIPTION "SMW Test ROM Generator - Creates synthetic ROMs for testing"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(SMW_TEST_ROM_SOURCES
    SMWTestROMGenerator.cpp
)

set(SMW_TEST_ROM_HEADERS
    SMWTestROMGenerator.h
)

# Create static library
add_library(SMWTestROMGenerator STATIC
    ${SMW_TEST_ROM_SOURCES}
    ${SMW_TEST_ROM_HEADERS}
)

target_include_directories(SMWTestROMGenerator
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create demo executable
add_executable(SMWTestROMGeneratorDemo main.cpp)

target_link_libraries(SMWTestROMGeneratorDemo
    PRIVATE
        SMWTestROMGenerator
)

# Set output directories
set_target_properties(SMWTestROMGenerator SMWTestROMGeneratorDemo
    PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)