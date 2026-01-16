cmake_minimum_required(VERSION 3.10)
project(bond_lite_c VERSION 1.0.0 LANGUAGES C)

# C11 standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Fetch Unity testing framework
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)

# Test executable - encoding
add_executable(test_encoding
    src/bond_encoding.c
    tests/test_encoding.c
)
target_link_libraries(test_encoding unity)

# Test executable - buffer
add_executable(test_buffer
    src/bond_buffer.c
    tests/test_buffer.c
)
target_link_libraries(test_buffer unity)

# Test executable - writer
add_executable(test_writer
    src/bond_buffer.c
    src/bond_encoding.c
    src/bond_writer.c
    tests/test_writer_reader.c
)
target_link_libraries(test_writer unity)

# Test executable - reader
add_executable(test_reader
    src/bond_buffer.c
    src/bond_encoding.c
    src/bond_reader.c
    tests/test_reader.c
)
target_link_libraries(test_reader unity)

# Test executable - roundtrip
add_executable(test_roundtrip
    src/bond_buffer.c
    src/bond_encoding.c
    src/bond_writer.c
    src/bond_reader.c
    tests/test_roundtrip.c
)
target_link_libraries(test_roundtrip unity)

enable_testing()
add_test(NAME test_encoding COMMAND test_encoding)
add_test(NAME test_buffer COMMAND test_buffer)
add_test(NAME test_writer COMMAND test_writer)
add_test(NAME test_reader COMMAND test_reader)
add_test(NAME test_roundtrip COMMAND test_roundtrip)
