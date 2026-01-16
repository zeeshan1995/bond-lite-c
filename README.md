# Bond-Lite-C

A lightweight, pure C11 implementation of Microsoft Bond's CompactBinary v1 serialization protocol.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Overview

Bond-Lite-C provides efficient binary serialization compatible with [Microsoft Bond](https://github.com/microsoft/bond) CompactBinary v1 format, implemented in pure C11 with no external dependencies. It's designed for embedded systems, IoT devices, and any environment where a minimal footprint is required.

### Features

- **Pure C11** - Maximum portability across compilers and platforms
- **Zero Dependencies** - Only requires the standard C library
- **Small Footprint** - Suitable for embedded and resource-constrained environments
- **Cross-Platform** - Works on Linux, macOS, Windows, and embedded systems
- **CompactBinary v1** - Compatible with Bond's compact binary wire format
- **Full Type Support** - All Bond primitive types and containers
- **Comprehensive Tests** - Unit tests for all modules

### Supported Types

| Type | Description |
|------|-------------|
| `bool` | Boolean (1 byte) |
| `int8`, `int16`, `int32`, `int64` | Signed integers (zigzag encoded) |
| `uint8`, `uint16`, `uint32`, `uint64` | Unsigned integers (varint encoded) |
| `float`, `double` | IEEE 754 floating point |
| `string` | UTF-8 string with length prefix |
| `list<T>` | Homogeneous list |
| `set<T>` | Homogeneous set |
| `map<K,V>` | Key-value map |
| `struct` | Nested structures |

## Building

### Prerequisites

- CMake 3.14 or later
- C11-compatible compiler (GCC, Clang, MSVC)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/zeeshanakhter/bond-lite-c.git
cd bond-lite-c

# Create build directory and build
mkdir build && cd build
cmake ..
make

# Run tests
ctest --output-on-failure

# Build and run examples
make simple_struct nested_struct enum_example
./examples/simple_struct
./examples/nested_struct
./examples/enum_example
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | ON | Build unit tests |
| `BUILD_EXAMPLES` | ON | Build example programs |

```bash
# Build without tests and examples
cmake -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF ..
```

### Windows (Visual Studio)

```batch
mkdir build
cd build
cmake -G "Visual Studio 17 2022" ..
cmake --build . --config Release
ctest -C Release --output-on-failure
```

## Usage

### Simple Struct Example

```c
#include "bond_lite.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    // Create a buffer for serialization
    bond_buffer buffer;
    bond_buffer_init(&buffer, 256);
    
    // Serialize a Person struct: { name: string, age: int32, email: string }
    bond_writer_write_struct_begin(&buffer);
    bond_writer_write_field_string(&buffer, 0, "Alice", 5);    // field 0: name
    bond_writer_write_field_int32(&buffer, 1, 30);             // field 1: age
    bond_writer_write_field_string(&buffer, 2, "alice@example.com", 17);  // field 2: email
    bond_writer_write_struct_end(&buffer);
    
    printf("Serialized %zu bytes\n", buffer.size);
    
    // Deserialize
    bond_reader reader;
    bond_reader_init(&reader, &buffer);
    
    uint8_t field_type;
    uint16_t field_id;
    
    while (bond_reader_read_field_header(&reader, &field_type, &field_id) == BOND_OK) {
        if (field_type == BOND_TYPE_STRUCT_END) break;
        
        switch (field_id) {
            case 0: case 2: {  // string fields
                char str[256];
                uint32_t len;
                bond_reader_read_string_value(&reader, str, sizeof(str), &len);
                printf("Field %u: %s\n", field_id, str);
                break;
            }
            case 1: {  // int32 field
                int32_t age;
                bond_reader_read_int32_value(&reader, &age);
                printf("Field %u: %d\n", field_id, age);
                break;
            }
            default:
                bond_reader_skip(&reader, field_type);
        }
    }
    
    bond_buffer_destroy(&buffer);
    return 0;
}
```

### Working with Containers

```c
// Serialize a list of 3 integers
bond_writer_write_field_header(&buffer, BOND_TYPE_LIST, 0);
bond_writer_write_container_header(&buffer, BOND_TYPE_INT32, 3);
bond_writer_write_int32_value(&buffer, 10);
bond_writer_write_int32_value(&buffer, 20);
bond_writer_write_int32_value(&buffer, 30);

// Serialize a map<string, int32>
bond_writer_write_field_header(&buffer, BOND_TYPE_MAP, 1);
bond_writer_write_map_header(&buffer, BOND_TYPE_STRING, BOND_TYPE_INT32, 2);
bond_writer_write_string_value(&buffer, "one", 3);
bond_writer_write_int32_value(&buffer, 1);
bond_writer_write_string_value(&buffer, "two", 3);
bond_writer_write_int32_value(&buffer, 2);
```

### Enums

Bond enums are serialized as int32 with zigzag encoding:

```c
typedef enum { STATUS_PENDING = 0, STATUS_ACTIVE = 1, STATUS_COMPLETED = 2 } Status;

// Write enum
bond_writer_write_field_int32(&buffer, 0, (int32_t)STATUS_ACTIVE);

// Read enum
int32_t value;
bond_reader_read_int32_value(&reader, &value);
Status status = (Status)value;
```

### Error Handling

All functions return `bond_error` which can be:

| Error Code | Description |
|------------|-------------|
| `BOND_OK` | Success |
| `BOND_ERROR_NULL_POINTER` | Null pointer passed |
| `BOND_ERROR_OUT_OF_MEMORY` | Memory allocation failed |
| `BOND_ERROR_BUFFER_OVERFLOW` | Buffer overflow |
| `BOND_ERROR_INVALID_TYPE` | Invalid Bond type |
| `BOND_ERROR_EOF` | Unexpected end of buffer |

## API Reference

### Buffer Functions

| Function | Description |
|----------|-------------|
| `bond_buffer_init(buffer, capacity)` | Initialize a new buffer with given capacity |
| `bond_buffer_init_from_data(buffer, data, size)` | Initialize buffer from existing data (for reading) |
| `bond_buffer_destroy(buffer)` | Free buffer resources |
| `bond_buffer_reset(buffer)` | Reset buffer for reuse |
| `bond_buffer_ensure_capacity(buffer, n)` | Ensure buffer can hold n more bytes |
| `bond_buffer_write_byte(buffer, byte)` | Write a single byte |
| `bond_buffer_write_bytes(buffer, data, len)` | Write multiple bytes |

### Writer Functions

| Function | Description |
|----------|-------------|
| `bond_writer_write_struct_begin(buffer)` | Begin a struct |
| `bond_writer_write_struct_end(buffer)` | End a struct (writes 0x00 marker) |
| `bond_writer_write_field_header(buffer, type, id)` | Write field header |
| `bond_writer_write_field_bool(buffer, id, value)` | Write bool field |
| `bond_writer_write_field_int8/16/32/64(...)` | Write signed integer fields |
| `bond_writer_write_field_uint8/16/32/64(...)` | Write unsigned integer fields |
| `bond_writer_write_field_float/double(...)` | Write floating point fields |
| `bond_writer_write_field_string(buffer, id, str, len)` | Write string field |
| `bond_writer_write_container_header(buffer, type, count)` | Write list/set header |
| `bond_writer_write_map_header(buffer, kt, vt, count)` | Write map header |

### Reader Functions

| Function | Description |
|----------|-------------|
| `bond_reader_init(reader, buffer)` | Initialize reader from buffer |
| `bond_reader_read_field_header(reader, type, id)` | Read field header |
| `bond_reader_read_*_value(reader, out)` | Read typed values |
| `bond_reader_read_container_header(reader, type, count)` | Read list/set header |
| `bond_reader_read_map_header(reader, kt, vt, count)` | Read map header |
| `bond_reader_skip(reader, type)` | Skip a value of given type |

### Encoding Functions

| Function | Description |
|----------|-------------|
| `bond_varint_encode(value, out)` | Encode uint64 as varint, returns byte count |
| `bond_varint_decode(data, len, out)` | Decode varint to uint64, returns bytes consumed |
| `bond_zigzag_encode32/64(value)` | Encode signed int with zigzag |
| `bond_zigzag_decode32/64(value)` | Decode zigzag to signed int |

## CompactBinary Wire Format

Bond-Lite-C implements the CompactBinary v1 protocol:

- **Variable-length integers**: Uses LEB128 encoding for compact integer representation
- **ZigZag encoding**: Signed integers use ZigZag encoding for efficient small value representation
- **Compact field headers**: Field IDs with small deltas (≤5) are encoded in the type byte
- **Containers**: Lists, sets, and maps include element type and count headers

## Compatibility

This library produces output compatible with:
- Microsoft Bond C++
- Microsoft Bond C#
- Microsoft Bond Python
- Other CompactBinary v1 implementations

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Microsoft Bond](https://github.com/microsoft/bond) - The original Bond serialization framework
- Inspired by the need for a lightweight C implementation for embedded systems
