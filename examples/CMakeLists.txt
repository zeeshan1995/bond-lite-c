# Examples CMakeLists.txt

# Simple struct example
add_executable(simple_struct simple_struct.c)
target_link_libraries(simple_struct bond_lite)

# Nested struct with containers example
add_executable(nested_struct nested_struct.c)
target_link_libraries(nested_struct bond_lite)

# Enum example
add_executable(enum_example enum_example.c)
target_link_libraries(enum_example bond_lite)

# Set output directory for examples
set_target_properties(simple_struct nested_struct enum_example
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
)
