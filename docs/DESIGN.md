# Bond-Lite-C Design Document

## Overview

A lightweight, pure C11 implementation of Microsoft Bond's CompactBinary serialization protocol.

**Goals:**
- Zero external dependencies (only standard C library)
- Minimal memory footprint
- Easy integration into embedded systems and C projects
- Wire-compatible with official Bond implementations

**Non-Goals:**
- Code generation from .bond schema files
- FastBinary or SimpleBinary protocols
- Runtime schema introspection

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        User Application                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ┌─────────────────┐              ┌─────────────────┐          │
│   │     Writer      │              │     Reader      │          │
│   │  (Serializer)   │              │ (Deserializer)  │          │
│   └────────┬────────┘              └────────┬────────┘          │
│            │                                │                   │
│            ▼                                ▼                   │
│   ┌─────────────────────────────────────────────────────┐       │
│   │                      Buffer                          │       │
│   │            (Dynamic byte array)                      │       │
│   └─────────────────────────────────────────────────────┘       │
│                              │                                  │
│                              ▼                                  │
│   ┌─────────────────────────────────────────────────────┐       │
│   │                    Encoding                          │       │
│   │     (Varint, ZigZag, Float/Double primitives)        │       │
│   └─────────────────────────────────────────────────────┘       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Module Breakdown

### 1. Encoding (`bond_encoding.c`) ✅ COMPLETE

Low-level encoding/decoding primitives.

| Function | Purpose |
|----------|---------|
| `bond_encode_varint16/32/64` | Variable-length unsigned int (LEB128) |
| `bond_decode_varint16/32/64` | Decode varint back to int |
| `bond_zigzag_encode16/32/64` | Signed → unsigned mapping |
| `bond_zigzag_decode16/32/64` | Unsigned → signed mapping |
| `bond_encode_float/double` | IEEE 754 little-endian |
| `bond_decode_float/double` | Decode float/double |

**Key Design Decisions:**
- Pure functions, no state
- Work on raw `uint8_t*` buffers
- Caller manages buffer space
- Little-endian assumed (matches Bond spec)

---

### 2. Buffer (`bond_buffer.c`) ✅ COMPLETE

Dynamic byte array for accumulating/consuming serialized data.

```c
typedef struct {
    uint8_t *data;      // Byte array
    size_t size;        // Bytes written
    size_t capacity;    // Allocated space
    size_t read_pos;    // Read cursor
    bool owns_memory;   // Ownership flag
} bond_buffer;
```

**Operations:**
- `init` / `init_from` / `destroy` — Lifecycle
- `write` / `write_byte` — Append data
- `read` / `read_byte` / `peek` — Consume data
- `reserve` — Ensure capacity
- `clear` / `rewind` — Reset state

**Key Design Decisions:**
- Doubles capacity on growth (amortized O(1))
- Can wrap external memory (for parsing received data)
- Not thread-safe (caller synchronizes)

---

### 3. Writer (`bond_writer.c`) ⬜ TODO

High-level serialization API. Writes Bond CompactBinary v1 format.

```c
typedef struct {
    bond_buffer *buffer;
} bond_writer;
```

**Operations:**
- `bond_writer_init(writer, buffer)` — Attach to buffer
- `bond_write_struct_begin/end()` — Struct delimiters
- `bond_write_field_begin/end(id, type)` — Field headers
- `bond_write_int32(value)` — Write varint + zigzag
- `bond_write_string(str, len)` — Length-prefixed UTF-8
- `bond_write_container_begin(size, element_type)` — List/set/map

**Key Design Decisions:**
- Minimal state (just buffer pointer)
- v1 format (matches TelInstaller, v2 in backlog)
- Absolute field IDs (id 0-5 = 1 byte, 6-255 = 2 bytes, 256+ = 3 bytes)

---

### 4. Reader (`bond_reader.c`) ⬜ TODO

High-level deserialization API. Reads Bond CompactBinary v1 format.

```c
typedef struct {
    bond_buffer *buffer;
} bond_reader;
```

**Operations:**
- `bond_reader_init(reader, buffer)` — Attach to buffer
- `bond_read_struct_begin/end()` — Enter/exit struct
- `bond_read_field_begin(id, type)` — Read next field header
- `bond_read_int32(value)` — Decode varint + zigzag
- `bond_read_string(str, max_len)` — Read length-prefixed string
- `bond_read_skip(type)` — Skip unknown fields

**Key Design Decisions:**
- Forward-only parsing (no seeking back)
- Returns error on malformed data
- Allows skipping unknown fields (forward compatibility)

---

### 5. Types (`bond_types.h`) ✅ COMPLETE

Bond type definitions and constants.

```c
// Bond type IDs (5 bits, 0-31)
typedef enum {
    BOND_TYPE_BOOL   = 2,
    BOND_TYPE_UINT8  = 3,
    BOND_TYPE_UINT16 = 4,
    BOND_TYPE_UINT32 = 5,
    BOND_TYPE_UINT64 = 6,
    BOND_TYPE_FLOAT  = 7,
    BOND_TYPE_DOUBLE = 8,
    BOND_TYPE_STRING = 9,
    BOND_TYPE_STRUCT = 10,
    BOND_TYPE_LIST   = 11,
    BOND_TYPE_SET    = 12,
    BOND_TYPE_MAP    = 13,
    BOND_TYPE_INT8   = 14,
    BOND_TYPE_INT16  = 15,
    BOND_TYPE_INT32  = 16,
    BOND_TYPE_INT64  = 17,
    BOND_TYPE_WSTRING = 18,
} bond_type;

// Special markers
#define BOND_BT_STOP      0   // End of struct
#define BOND_BT_STOP_BASE 1   // End of base struct
```

---

## Wire Format (CompactBinary v1)

### Struct Layout

```
┌────────┬────────┬─────────────┬─────────┐
│ field₁ │ field₂ │     ...     │ BT_STOP │
└────────┴────────┴─────────────┴─────────┘
```

Note: v1 has no length prefix (v2 adds varint length before fields).

### Field Header Encoding

Field headers combine type (5 bits) and ID (variable):

```
ID ∈ [0, 5]:     1 byte   [id:3][type:5]
ID ∈ [6, 255]:   2 bytes  [110][type:5] [id:8]
ID ∈ [256,65535]: 3 bytes [111][type:5] [id:16 LE]
```

**Escape Codes:** The top 3 bits can hold 0-7, but values 6 and 7 are escape
codes that signal extended encoding:

```
3-bit value:  Meaning:
-----------   --------
  0-5         Field ID is this value (1 byte total)
  6 (110)     Escape: actual ID (6-255) in next 1 byte (2 bytes total)
  7 (111)     Escape: actual ID (256-65535) in next 2 bytes LE (3 bytes total)
```

**Why Raw Bytes (Not Varint)?** Headers use raw bytes for performance:
- Parsed for every field, must be fast
- Read 1 byte → top 3 bits tell you exactly how many more bytes to read
- No looping to check continuation bits like varint
- Field IDs are bounded (max 65535), no space savings from variable encoding

### Value Encoding

**Varint is only used for values, not headers:**

| Type | Encoding |
|------|----------|
| bool | 1 byte (0 or 1) |
| int8/uint8 | 1 byte raw |
| int16/32/64 | ZigZag + Varint |
| uint16/32/64 | Varint |
| float | 4 bytes LE |
| double | 8 bytes LE |
| string | Varint length + UTF-8 bytes |
| list/set | [type:1] [count:varint] [elements...] |
| map | [key_type:1] [val_type:1] [count:varint] [pairs...] |
| struct | Recursive, ends with BT_STOP |

---

## Error Handling Strategy

### Return Codes

```c
// Success
#define BOND_OK           0

// Errors
#define BOND_ERR_ALLOC   -1   // Memory allocation failed
#define BOND_ERR_EOF     -2   // Unexpected end of buffer
#define BOND_ERR_CORRUPT -3   // Malformed data
#define BOND_ERR_OVERFLOW -4  // Value too large for target type
```

### Error Propagation

- Functions return error code or -1
- Caller checks and handles
- No exceptions (C doesn't have them)
- No global errno (keep it simple)

---

## Memory Management

### Principles

1. **No hidden allocations** — User controls all memory
2. **Explicit ownership** — `owns_memory` flag
3. **Cleanup functions** — Always call `destroy()`
4. **Fail gracefully** — Check malloc returns

### Typical Usage

```c
// Serialization
bond_buffer buf;
bond_buffer_init(&buf, 256);  // User allocates
// ... write data ...
send(sock, buf.data, buf.size);
bond_buffer_destroy(&buf);    // User frees

// Deserialization
uint8_t received[1024];
recv(sock, received, sizeof(received));
bond_buffer buf;
bond_buffer_init_from(&buf, received, len);  // No allocation
// ... read data ...
bond_buffer_destroy(&buf);  // No-op, doesn't free received
```

---

## File Structure

```
bond-lite-c/
├── include/
│   ├── bond_lite.h          # Public API header
│   ├── bond_buffer.h        # ✅ Buffer struct/API
│   └── bond_types.h         # ✅ Type definitions
├── src/
│   ├── bond_encoding.c      # ✅ Varint, ZigZag, Float
│   ├── bond_buffer.c        # ✅ Dynamic buffer
│   ├── bond_writer.c        # ⬜ Serialization (v1)
│   ├── bond_reader.c        # ⬜ Deserialization (v1)
│   └── bond_types.c         # ⬜ Type utilities
├── tests/
│   ├── test_encoding.c      # ✅ 17 tests
│   ├── test_buffer.c        # ✅ 15 tests
│   ├── test_writer.c        # ⬜ Writer tests
│   └── test_reader.c        # ⬜ Reader tests
├── docs/
│   ├── ENCODING.md          # Encoding reference
│   ├── BUFFER_DESIGN.md     # Buffer module design
│   └── DESIGN.md            # This document
└── examples/
    └── simple_struct.c      # ⬜ Example usage
```

---

## Build System

CMake with FetchContent for Unity test framework.

```cmake
cmake_minimum_required(VERSION 3.10)
project(bond_lite_c C)

# Fetch Unity test framework
FetchContent_Declare(unity ...)

# Library
add_library(bond_lite
    src/bond_encoding.c
    src/bond_buffer.c
    src/bond_writer.c
    src/bond_reader.c
)

# Tests
add_executable(test_all
    tests/test_encoding.c
    tests/test_buffer.c
    ...
)
target_link_libraries(test_all bond_lite unity)
```

---

## Implementation Roadmap

### Phase 1: Primitives ✅
- [x] Varint encode/decode (16/32/64)
- [x] ZigZag encode/decode (16/32/64)
- [x] Float/Double encode/decode
- [x] Unit tests

### Phase 2: Buffer 🔄
- [x] Design document
- [ ] Core implementation
- [ ] Unit tests

### Phase 3: Writer
- [ ] Struct serialization
- [ ] Field headers
- [ ] All primitive types
- [ ] Containers (list, set, map)
- [ ] Nested structs
- [ ] Unit tests

### Phase 4: Reader
- [ ] Struct deserialization
- [ ] Field parsing
- [ ] Type decoding
- [ ] Skip unknown fields
- [ ] Unit tests

### Phase 5: Integration
- [ ] Public header consolidation
- [ ] Example programs
- [ ] Interop tests with official Bond
- [ ] Documentation

---

## Compatibility Notes

### Platform Requirements

- **C11** standard (for `bool`, `static_assert`, etc.)
- **Little-endian** CPU (x86, x64, ARM LE)
- **Standard library** only (stdlib.h, string.h, stdint.h)

### Tested On

- Linux x86-64 (GCC, Clang)
- TODO: Linux ARM64
- TODO: macOS
- TODO: Windows (MSVC)

### Wire Compatibility

Goal: Produce bytes identical to official Bond C++ implementation.
- CompactBinary v1: Supported
- CompactBinary v2: Supported (default)
- FastBinary: Not supported
- SimpleBinary: Not supported

---

## References

- [Bond GitHub](https://github.com/microsoft/bond)
- [Bond Manual](https://microsoft.github.io/bond/)
- [CompactBinary Format](https://microsoft.github.io/bond/manual/bond_cpp.html#compact-binary)
- [LEB128 Encoding](https://en.wikipedia.org/wiki/LEB128)
- [ZigZag Encoding](https://developers.google.com/protocol-buffers/docs/encoding#signed-ints)
