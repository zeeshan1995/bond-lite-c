# Tests CMakeLists.txt

# Fetch Unity testing framework
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)

# Enable Unity double precision support
target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)

# Test executables
add_executable(test_buffer test_buffer.c)
target_link_libraries(test_buffer bond_lite_c unity m)

add_executable(test_encoding test_encoding.c)
target_link_libraries(test_encoding bond_lite_c unity m)

add_executable(test_writer_reader test_writer_reader.c)
target_link_libraries(test_writer_reader bond_lite_c unity m)

# Register tests with CTest
add_test(NAME test_buffer COMMAND test_buffer)
add_test(NAME test_encoding COMMAND test_encoding)
add_test(NAME test_writer_reader COMMAND test_writer_reader)

# Set test properties
set_tests_properties(test_buffer test_encoding test_writer_reader
    PROPERTIES
    TIMEOUT 30
)
