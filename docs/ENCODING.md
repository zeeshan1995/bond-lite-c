# Bond CompactBinary Encoding Technical Specification

This document describes the encoding logic used in bond-lite-c for serializing and deserializing data using Microsoft Bond's CompactBinary v1 protocol.

## Table of Contents

1. [Overview](#overview)
2. [Variable-Length Integer Encoding (Varint/LEB128)](#variable-length-integer-encoding-varintleb128)
3. [ZigZag Encoding for Signed Integers](#zigzag-encoding-for-signed-integers)
4. [Type System](#type-system)
5. [Field Header Encoding](#field-header-encoding)
6. [Primitive Type Encoding](#primitive-type-encoding)
7. [String and Blob Encoding](#string-and-blob-encoding)
8. [Container Encoding](#container-encoding)
9. [Struct Encoding](#struct-encoding)
10. [Complete Example](#complete-example)

---

## Overview

CompactBinary is a compact binary serialization format designed for efficiency. Key characteristics:

- **Self-describing**: Each field carries type information
- **Variable-length encoding**: Small values use fewer bytes
- **Delta encoding**: Field IDs are encoded as deltas from the previous field
- **No schema required**: Can be decoded without schema (though field semantics need schema)

---

## Variable-Length Integer Encoding (Varint/LEB128)

Unsigned integers are encoded using **LEB128** (Little Endian Base 128), where:

- Each byte uses 7 bits for data and 1 bit (MSB) as a continuation flag
- MSB = 1 means more bytes follow; MSB = 0 means this is the last byte
- Bytes are stored in little-endian order (least significant group first)

### Encoding Algorithm

```
function encode_varint(value):
    while value >= 0x80:
        write_byte((value & 0x7F) | 0x80)  // Set continuation bit
        value >>= 7
    write_byte(value & 0x7F)               // Final byte, no continuation
```

### Decoding Algorithm

```
function decode_varint():
    result = 0
    shift = 0
    do:
        byte = read_byte()
        result |= (byte & 0x7F) << shift
        shift += 7
    while (byte & 0x80) != 0
    return result
```

### Examples

| Value | Binary Representation | Encoded Bytes |
|-------|----------------------|---------------|
| 0 | `0000 0000` | `0x00` |
| 1 | `0000 0001` | `0x01` |
| 127 | `0111 1111` | `0x7F` |
| 128 | `1000 0000` | `0x80 0x01` |
| 255 | `1111 1111` | `0xFF 0x01` |
| 300 | `1 0010 1100` | `0xAC 0x02` |
| 16383 | `11 1111 1111 1111` | `0xFF 0x7F` |
| 16384 | `100 0000 0000 0000` | `0x80 0x80 0x01` |

### Detailed Example: Encoding 300

```
300 in binary: 1 0010 1100 (9 bits)

Split into 7-bit groups (from LSB):
  Group 0: 010 1100 = 0x2C
  Group 1: 000 0010 = 0x02

Encode:
  Byte 0: 0x2C | 0x80 = 0xAC (continuation bit set)
  Byte 1: 0x02        (final byte)

Result: [0xAC, 0x02]
```

### Byte Count by Value Range

| Value Range | Bytes Required |
|-------------|----------------|
| 0 - 127 | 1 |
| 128 - 16,383 | 2 |
| 16,384 - 2,097,151 | 3 |
| 2,097,152 - 268,435,455 | 4 |
| 268,435,456 - 4,294,967,295 | 5 |

---

## ZigZag Encoding for Signed Integers

Signed integers use **ZigZag encoding** before varint encoding. This maps signed integers to unsigned integers so that small absolute values (positive or negative) produce small encoded values.

### Mapping Formula

```
Encode: zigzag(n) = (n << 1) ^ (n >> (bits - 1))
Decode: unzigzag(n) = (n >> 1) ^ -(n & 1)
```

### Mapping Table

| Signed | Unsigned (ZigZag) |
|--------|-------------------|
| 0 | 0 |
| -1 | 1 |
| 1 | 2 |
| -2 | 3 |
| 2 | 4 |
| -3 | 5 |
| 2147483647 | 4294967294 |
| -2147483648 | 4294967295 |

### Why ZigZag?

Without ZigZag, -1 as a 32-bit signed integer would be `0xFFFFFFFF`, requiring 5 bytes in varint encoding. With ZigZag, -1 maps to 1, requiring only 1 byte.

### Implementation

```c
// 32-bit ZigZag encode
uint32_t zigzag_encode32(int32_t n) {
    return (uint32_t)((n << 1) ^ (n >> 31));
}

// 32-bit ZigZag decode
int32_t zigzag_decode32(uint32_t n) {
    return (int32_t)((n >> 1) ^ -(int32_t)(n & 1));
}
```

---

## Type System

Bond defines the following primitive types:

| Type ID | Type Name | Size | Encoding |
|---------|-----------|------|----------|
| 1 | BT_BOOL | 1 byte | 0x00 or 0x01 |
| 2 | BT_INT8 | 1 byte | Fixed, ZigZag |
| 3 | BT_INT16 | varint | ZigZag + Varint |
| 4 | BT_INT32 | varint | ZigZag + Varint |
| 5 | BT_INT64 | varint | ZigZag + Varint |
| 6 | BT_UINT8 | 1 byte | Fixed |
| 7 | BT_UINT16 | varint | Varint |
| 8 | BT_UINT32 | varint | Varint |
| 9 | BT_UINT64 | varint | Varint |
| 10 | BT_FLOAT | 4 bytes | IEEE 754, little-endian |
| 11 | BT_DOUBLE | 8 bytes | IEEE 754, little-endian |
| 12 | BT_STRING | varint + data | Length-prefixed UTF-8 |
| 13 | BT_STRUCT | nested | Recursive struct encoding |
| 14 | BT_LIST | header + elements | Container encoding |
| 15 | BT_SET | header + elements | Container encoding |
| 16 | BT_MAP | header + pairs | Container encoding |
| 17 | BT_BLOB | varint + data | Length-prefixed binary |

---

## Field Header Encoding

Each field in a struct is prefixed with a **field header** that encodes the field's type and ID delta.

### Compact Header Format (1 byte)

When the field ID delta (from previous field) is ≤ 5:

```
┌─────────────────────────────────────┐
│  Byte 0                             │
├─────────┬───────────────────────────┤
│ Delta   │ Type                      │
│ (3 bits)│ (5 bits)                  │
└─────────┴───────────────────────────┘
  Bits 7-5     Bits 4-0
```

**Header byte** = `(delta << 5) | type`

### Extended Header Format (2+ bytes)

When the field ID delta is > 5, use delta value 6 as a marker:

```
┌─────────────────────────────────────┐
│  Byte 0                             │
├─────────┬───────────────────────────┤
│ 0x06    │ Type                      │
│ (3 bits)│ (5 bits)                  │
└─────────┴───────────────────────────┘
│  Bytes 1+: Field ID (varint)        │
└─────────────────────────────────────┘
```

**Header byte** = `(6 << 5) | type` = `0xC0 | type`

Followed by the **absolute field ID** as a varint.

### Struct End Marker

A struct ends with a single byte `0x00` (type = 0, which is `BT_STOP`).

### Field Header Examples

| Previous ID | Current ID | Delta | Type | Header Bytes |
|-------------|------------|-------|------|--------------|
| 0 | 0 | 0 | INT32 (4) | `0x04` |
| 0 | 1 | 1 | STRING (12) | `0x2C` |
| 1 | 2 | 1 | BOOL (1) | `0x21` |
| 2 | 5 | 3 | UINT64 (9) | `0x69` |
| 5 | 6 | 1 | FLOAT (10) | `0x2A` |
| 6 | 100 | 94 | INT32 (4) | `0xC4 0x64` |

### Encoding Algorithm

```
function write_field_header(field_id, type, prev_field_id):
    delta = field_id - prev_field_id
    
    if delta <= 5:
        write_byte((delta << 5) | type)
    else:
        write_byte((6 << 5) | type)  // 0xC0 | type
        write_varint(field_id)       // Absolute field ID
```

### Decoding Algorithm

```
function read_field_header(prev_field_id):
    byte = read_byte()
    type = byte & 0x1F
    delta = byte >> 5
    
    if type == 0:  // BT_STOP
        return (STRUCT_END, 0)
    
    if delta <= 5:
        field_id = prev_field_id + delta
    else:
        field_id = read_varint()
    
    return (type, field_id)
```

---

## Primitive Type Encoding

### Boolean (BT_BOOL)

Single byte: `0x00` for false, `0x01` for true.

```
true  → [0x01]
false → [0x00]
```

### Fixed-Width Integers (INT8, UINT8)

Single byte, stored directly.

```
uint8_t 255 → [0xFF]
int8_t -1   → [0xFF] (two's complement)
```

### Variable-Width Unsigned Integers (UINT16, UINT32, UINT64)

Encoded directly as varint.

```
uint16_t 300   → [0xAC, 0x02]
uint32_t 70000 → [0xF0, 0xA2, 0x04]
```

### Variable-Width Signed Integers (INT16, INT32, INT64)

ZigZag encoded, then varint encoded.

```
int32_t 0    → zigzag(0) = 0    → [0x00]
int32_t 1    → zigzag(1) = 2    → [0x02]
int32_t -1   → zigzag(-1) = 1   → [0x01]
int32_t -64  → zigzag(-64) = 127 → [0x7F]
int32_t 64   → zigzag(64) = 128  → [0x80, 0x01]
```

### Float (BT_FLOAT)

4 bytes, IEEE 754 single precision, little-endian.

```
float 3.14159 → [0xD0, 0x0F, 0x49, 0x40]
```

### Double (BT_DOUBLE)

8 bytes, IEEE 754 double precision, little-endian.

```
double 3.141592653589793 → [0x18, 0x2D, 0x44, 0x54, 0xFB, 0x21, 0x09, 0x40]
```

---

## String and Blob Encoding

### String (BT_STRING)

Length-prefixed UTF-8 string:

```
┌─────────────────────────────────────┐
│ Length (varint)                     │
├─────────────────────────────────────┤
│ UTF-8 Data (length bytes)           │
└─────────────────────────────────────┘
```

**Example**: "Hello"

```
Length: 5 → [0x05]
Data: "Hello" → [0x48, 0x65, 0x6C, 0x6C, 0x6F]

Encoded: [0x05, 0x48, 0x65, 0x6C, 0x6C, 0x6F]
```

**Example**: Empty string ""

```
Encoded: [0x00]
```

### Blob (BT_BLOB)

Identical to string but contains arbitrary binary data:

```
┌─────────────────────────────────────┐
│ Length (varint)                     │
├─────────────────────────────────────┤
│ Binary Data (length bytes)          │
└─────────────────────────────────────┘
```

---

## Container Encoding

### Container Header

All containers (list, set, map) begin with a header:

```
┌─────────────────────────────────────┐
│ Element Type + Count Indicator      │
├─────────────────────────────────────┤
│ Count (if not in header)            │
└─────────────────────────────────────┘
```

**First byte format**:

- Bits 0-4: Element type (5 bits)
- Bits 5-7: Count or count indicator (3 bits)

If the count fits in 3 bits (0-7), it's stored directly. Otherwise, bits 5-7 = 0 and the count follows as a varint.

### List/Set Encoding

```
┌─────────────────────────────────────┐
│ (count << 5) | element_type         │  ← if count ≤ 7
├─────────────────────────────────────┤
│ Elements...                         │
└─────────────────────────────────────┘

OR

┌─────────────────────────────────────┐
│ element_type                        │  ← count > 7 (high bits = 0)
├─────────────────────────────────────┤
│ Count (varint)                      │
├─────────────────────────────────────┤
│ Elements...                         │
└─────────────────────────────────────┘
```

**Example**: List of 3 int32 values [10, 20, 30]

```
Header: (3 << 5) | 4 = 0x64
Elements:
  10 → zigzag(10) = 20 → [0x14]
  20 → zigzag(20) = 40 → [0x28]
  30 → zigzag(30) = 60 → [0x3C]

Encoded: [0x64, 0x14, 0x28, 0x3C]
```

**Example**: List of 10 uint8 values

```
Header: (count doesn't fit) → 0x06 (type=6=UINT8)
Count: 10 → [0x0A]
Elements: [0x01, 0x02, ..., 0x0A]

Encoded: [0x06, 0x0A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A]
```

### Map Encoding

Maps have both key and value types:

```
┌─────────────────────────────────────┐
│ (count << 5) | key_type             │  ← if count ≤ 7
├─────────────────────────────────────┤
│ value_type                          │
├─────────────────────────────────────┤
│ Key-Value Pairs...                  │
└─────────────────────────────────────┘

OR

┌─────────────────────────────────────┐
│ key_type                            │  ← count > 7
├─────────────────────────────────────┤
│ value_type                          │
├─────────────────────────────────────┤
│ Count (varint)                      │
├─────────────────────────────────────┤
│ Key-Value Pairs...                  │
└─────────────────────────────────────┘
```

**Example**: Map with 2 entries: {"one": 1, "two": 2}

```
Key type: STRING (12), Value type: INT32 (4), Count: 2

Header byte 1: (2 << 5) | 12 = 0x4C
Header byte 2: 4 = 0x04

Entry 1:
  Key "one": [0x03, 0x6F, 0x6E, 0x65]
  Value 1: zigzag(1) = 2 → [0x02]

Entry 2:
  Key "two": [0x03, 0x74, 0x77, 0x6F]
  Value 2: zigzag(2) = 4 → [0x04]

Encoded: [0x4C, 0x04, 0x03, 0x6F, 0x6E, 0x65, 0x02, 0x03, 0x74, 0x77, 0x6F, 0x04]
```

---

## Struct Encoding

Structs are encoded as a sequence of field headers and values, terminated by `0x00`.

```
┌─────────────────────────────────────┐
│ Field 1 Header                      │
├─────────────────────────────────────┤
│ Field 1 Value                       │
├─────────────────────────────────────┤
│ Field 2 Header                      │
├─────────────────────────────────────┤
│ Field 2 Value                       │
├─────────────────────────────────────┤
│ ...                                 │
├─────────────────────────────────────┤
│ 0x00 (Struct End)                   │
└─────────────────────────────────────┘
```

### Nested Structs

Nested structs are encoded recursively. The field header indicates type `BT_STRUCT` (13), followed by the nested struct's encoding (including its own `0x00` terminator).

---

## Complete Example

Consider this struct:

```
struct Person {
    0: string name;
    1: int32 age;
    2: bool active;
    3: list<string> tags;
}
```

With values:
- name = "Alice"
- age = 30
- active = true
- tags = ["dev", "admin"]

### Step-by-Step Encoding

**Field 0: name = "Alice"**

```
Field header: delta=0, type=12 (STRING)
  Header: (0 << 5) | 12 = 0x0C

Value: "Alice" (5 chars)
  Length: [0x05]
  Data: [0x41, 0x6C, 0x69, 0x63, 0x65]

Bytes: [0x0C, 0x05, 0x41, 0x6C, 0x69, 0x63, 0x65]
```

**Field 1: age = 30**

```
Field header: delta=1, type=4 (INT32)
  Header: (1 << 5) | 4 = 0x24

Value: 30
  ZigZag: zigzag(30) = 60 = 0x3C
  Varint: [0x3C]

Bytes: [0x24, 0x3C]
```

**Field 2: active = true**

```
Field header: delta=1, type=1 (BOOL)
  Header: (1 << 5) | 1 = 0x21

Value: true = [0x01]

Bytes: [0x21, 0x01]
```

**Field 3: tags = ["dev", "admin"]**

```
Field header: delta=1, type=14 (LIST)
  Header: (1 << 5) | 14 = 0x2E

Container header: count=2, element_type=12 (STRING)
  Header: (2 << 5) | 12 = 0x4C

Element 1: "dev"
  Length: [0x03]
  Data: [0x64, 0x65, 0x76]

Element 2: "admin"
  Length: [0x05]
  Data: [0x61, 0x64, 0x6D, 0x69, 0x6E]

Bytes: [0x2E, 0x4C, 0x03, 0x64, 0x65, 0x76, 0x05, 0x61, 0x64, 0x6D, 0x69, 0x6E]
```

**Struct End**

```
Bytes: [0x00]
```

### Complete Encoded Output

```
0x0C 0x05 0x41 0x6C 0x69 0x63 0x65  // Field 0: name = "Alice"
0x24 0x3C                            // Field 1: age = 30
0x21 0x01                            // Field 2: active = true
0x2E 0x4C                            // Field 3: list header
     0x03 0x64 0x65 0x76             //   "dev"
     0x05 0x61 0x64 0x6D 0x69 0x6E   //   "admin"
0x00                                 // Struct end

Total: 24 bytes
```

### Hex Dump

```
00000000: 0C 05 41 6C 69 63 65 24 3C 21 01 2E 4C 03 64 65  ..Alice$<!..L.de
00000010: 76 05 61 64 6D 69 6E 00                          v.admin.
```

---

## Summary

| Concept | Technique | Purpose |
|---------|-----------|---------|
| Unsigned integers | Varint (LEB128) | Small values use fewer bytes |
| Signed integers | ZigZag + Varint | Small absolute values use fewer bytes |
| Field IDs | Delta encoding | Sequential fields compress well |
| Field headers | Compact 1-byte format | Common case optimization |
| Containers | Count in header | Small collections don't need extra bytes |
| Strings/Blobs | Length-prefixed | Efficient scanning |

This encoding achieves excellent compression for typical data patterns while remaining simple to implement and fast to process.
