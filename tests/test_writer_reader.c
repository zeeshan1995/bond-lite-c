/**
 * @file test_writer_reader.c
 * @brief Unit tests for Bond CompactBinary v1 Writer
 */

#include <unity.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "bond_buffer.h"
#include "bond_writer.h"
#include "bond_types.h"

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// Helper Macros
// ============================================================================

#define INIT_WRITER(capacity)           \
    bond_buffer buffer;                 \
    bond_writer writer;                 \
    bond_buffer_init(&buffer, capacity); \
    bond_writer_init(&writer, &buffer)

#define CLEANUP() bond_buffer_destroy(&buffer)

// ============================================================================
// Struct Control Tests
// ============================================================================

void test_struct_end_writes_stop(void)
{
    INIT_WRITER(16);
    
    bond_writer_struct_end(&writer);
    
    TEST_ASSERT_EQUAL(1, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(BOND_TYPE_STOP, buffer.data[0]);
    
    CLEANUP();
}

// ============================================================================
// Field Header Tests
// ============================================================================

void test_field_header_id_0(void)
{
    INIT_WRITER(16);
    
    // Field ID 0, type UINT32 (5)
    // Format: [id:3][type:5] = [000][00101] = 0x05
    bond_writer_write_field_header(&writer, 0, BOND_TYPE_UINT32);
    
    TEST_ASSERT_EQUAL(1, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x05, buffer.data[0]);
    
    CLEANUP();
}

void test_field_header_id_5(void)
{
    INIT_WRITER(16);
    
    // Field ID 5, type BOOL (2)
    // Format: [id:3][type:5] = [101][00010] = 0xA2
    bond_writer_write_field_header(&writer, 5, BOND_TYPE_BOOL);
    
    TEST_ASSERT_EQUAL(1, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0xA2, buffer.data[0]);
    
    CLEANUP();
}

void test_field_header_id_6_escape(void)
{
    INIT_WRITER(16);
    
    // Field ID 6, type STRING (9)
    // Escape code 6: [110][type:5][id:8] = 0xC9, 0x06
    bond_writer_write_field_header(&writer, 6, BOND_TYPE_STRING);
    
    TEST_ASSERT_EQUAL(2, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0xC9, buffer.data[0]);  // 0xC0 | 9
    TEST_ASSERT_EQUAL_HEX8(0x06, buffer.data[1]);
    
    CLEANUP();
}

void test_field_header_id_255_escape(void)
{
    INIT_WRITER(16);
    
    // Field ID 255, type INT32 (16)
    // Escape code 6: [110][type:5][id:8] = 0xD0, 0xFF
    bond_writer_write_field_header(&writer, 255, BOND_TYPE_INT32);
    
    TEST_ASSERT_EQUAL(2, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0xD0, buffer.data[0]);  // 0xC0 | 16
    TEST_ASSERT_EQUAL_HEX8(0xFF, buffer.data[1]);
    
    CLEANUP();
}

void test_field_header_id_256_escape(void)
{
    INIT_WRITER(16);
    
    // Field ID 256, type UINT64 (6)
    // Escape code 7: [111][type:5][id_lo:8][id_hi:8] = 0xE6, 0x00, 0x01
    bond_writer_write_field_header(&writer, 256, BOND_TYPE_UINT64);
    
    TEST_ASSERT_EQUAL(3, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0xE6, buffer.data[0]);  // 0xE0 | 6
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[1]);  // lo byte
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer.data[2]);  // hi byte
    
    CLEANUP();
}

void test_field_header_id_max(void)
{
    INIT_WRITER(16);
    
    // Field ID 65535, type FLOAT (7)
    // Escape code 7: 0xE7, 0xFF, 0xFF
    bond_writer_write_field_header(&writer, 65535, BOND_TYPE_FLOAT);
    
    TEST_ASSERT_EQUAL(3, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0xE7, buffer.data[0]);  // 0xE0 | 7
    TEST_ASSERT_EQUAL_HEX8(0xFF, buffer.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, buffer.data[2]);
    
    CLEANUP();
}

// ============================================================================
// Primitive Writer Tests (with field header)
// ============================================================================

void test_write_bool_true(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_bool(&writer, 1, true);
    
    // Field ID 1, type BOOL (2): [001][00010] = 0x22
    // Value: 0x01
    TEST_ASSERT_EQUAL(2, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x22, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer.data[1]);
    
    CLEANUP();
}

void test_write_bool_false(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_bool(&writer, 2, false);
    
    // Field ID 2, type BOOL (2): [010][00010] = 0x42
    // Value: 0x00
    TEST_ASSERT_EQUAL(2, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x42, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[1]);
    
    CLEANUP();
}

void test_write_uint8(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_uint8(&writer, 0, 0xAB);
    
    // Field ID 0, type UINT8 (3): [000][00011] = 0x03
    // Value: 0xAB (raw byte)
    TEST_ASSERT_EQUAL(2, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x03, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAB, buffer.data[1]);
    
    CLEANUP();
}

void test_write_uint16_small(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_uint16(&writer, 0, 127);
    
    // Field ID 0, type UINT16 (4): [000][00100] = 0x04
    // Value: 127 = 0x7F (1-byte varint)
    TEST_ASSERT_EQUAL(2, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x04, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x7F, buffer.data[1]);
    
    CLEANUP();
}

void test_write_uint16_large(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_uint16(&writer, 0, 300);
    
    // Field ID 0, type UINT16 (4): 0x04
    // Value: 300 = 0x012C → varint: 0xAC, 0x02
    TEST_ASSERT_EQUAL(3, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x04, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAC, buffer.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x02, buffer.data[2]);
    
    CLEANUP();
}

void test_write_uint32(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_uint32(&writer, 1, 16384);
    
    // Field ID 1, type UINT32 (5): [001][00101] = 0x25
    // Value: 16384 = 0x4000 → varint: 0x80, 0x80, 0x01
    TEST_ASSERT_EQUAL(4, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x25, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x80, buffer.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x80, buffer.data[2]);
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer.data[3]);
    
    CLEANUP();
}

void test_write_uint64(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_uint64(&writer, 0, 0);
    
    // Field ID 0, type UINT64 (6): [000][00110] = 0x06
    // Value: 0 = 0x00
    TEST_ASSERT_EQUAL(2, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x06, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[1]);
    
    CLEANUP();
}

void test_write_int8(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_int8(&writer, 0, -1);
    
    // Field ID 0, type INT8 (14): [000][01110] = 0x0E
    // Value: -1 = 0xFF (two's complement)
    TEST_ASSERT_EQUAL(2, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x0E, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, buffer.data[1]);
    
    CLEANUP();
}

void test_write_int16(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_int16(&writer, 0, -100);
    
    // Field ID 0, type INT16 (15): [000][01111] = 0x0F
    // Value: -100 as uint16 = 0xFF9C → varint
    TEST_ASSERT_GREATER_THAN(1, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x0F, buffer.data[0]);
    
    CLEANUP();
}

void test_write_int32(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_int32(&writer, 0, 42);
    
    // Field ID 0, type INT32 (16): [000][10000] = 0x10
    // Value: 42 = 0x2A (varint)
    TEST_ASSERT_EQUAL(2, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x10, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x2A, buffer.data[1]);
    
    CLEANUP();
}

void test_write_int64(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_int64(&writer, 0, 0);
    
    // Field ID 0, type INT64 (17): [000][10001] = 0x11
    // Value: 0
    TEST_ASSERT_EQUAL(2, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x11, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[1]);
    
    CLEANUP();
}

void test_write_float(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_float(&writer, 0, 1.0f);
    
    // Field ID 0, type FLOAT (7): [000][00111] = 0x07
    // Value: 1.0f = 0x3F800000 (little-endian: 00 00 80 3F)
    TEST_ASSERT_EQUAL(5, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x07, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[2]);
    TEST_ASSERT_EQUAL_HEX8(0x80, buffer.data[3]);
    TEST_ASSERT_EQUAL_HEX8(0x3F, buffer.data[4]);
    
    CLEANUP();
}

void test_write_double(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_double(&writer, 0, 1.0);
    
    // Field ID 0, type DOUBLE (8): [000][01000] = 0x08
    // Value: 1.0 = 0x3FF0000000000000 (little-endian)
    TEST_ASSERT_EQUAL(9, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x08, buffer.data[0]);
    // IEEE 754: 1.0 = 00 00 00 00 00 00 F0 3F
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[3]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[4]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[5]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[6]);
    TEST_ASSERT_EQUAL_HEX8(0xF0, buffer.data[7]);
    TEST_ASSERT_EQUAL_HEX8(0x3F, buffer.data[8]);
    
    CLEANUP();
}

void test_write_string_empty(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_string(&writer, 0, "");
    
    // Field ID 0, type STRING (9): [000][01001] = 0x09
    // Length: 0 (varint)
    // No characters
    TEST_ASSERT_EQUAL(2, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x09, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[1]);
    
    CLEANUP();
}

void test_write_string(void)
{
    INIT_WRITER(32);
    
    bond_writer_write_string(&writer, 1, "hello");
    
    // Field ID 1, type STRING (9): [001][01001] = 0x29
    // Length: 5 (varint)
    // Chars: h e l l o
    TEST_ASSERT_EQUAL(7, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x29, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x05, buffer.data[1]);  // length
    TEST_ASSERT_EQUAL_HEX8('h', buffer.data[2]);
    TEST_ASSERT_EQUAL_HEX8('e', buffer.data[3]);
    TEST_ASSERT_EQUAL_HEX8('l', buffer.data[4]);
    TEST_ASSERT_EQUAL_HEX8('l', buffer.data[5]);
    TEST_ASSERT_EQUAL_HEX8('o', buffer.data[6]);
    
    CLEANUP();
}

// ============================================================================
// Container Writer Tests
// ============================================================================

void test_write_list_begin(void)
{
    INIT_WRITER(32);
    
    // List of 3 uint32 values at field 1
    bond_writer_write_list_begin(&writer, 1, BOND_TYPE_UINT32, 3);
    
    // Field ID 1, type LIST (11): [001][01011] = 0x2B
    // Element type: UINT32 (5) = 0x05
    // Count: 3 (varint) = 0x03
    TEST_ASSERT_EQUAL(3, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x2B, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x05, buffer.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x03, buffer.data[2]);
    
    CLEANUP();
}

void test_write_set_begin(void)
{
    INIT_WRITER(32);
    
    // Set of 10 strings at field 2
    bond_writer_write_set_begin(&writer, 2, BOND_TYPE_STRING, 10);
    
    // Field ID 2, type SET (12): [010][01100] = 0x4C
    // Element type: STRING (9) = 0x09
    // Count: 10 (varint) = 0x0A
    TEST_ASSERT_EQUAL(3, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x4C, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x09, buffer.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x0A, buffer.data[2]);
    
    CLEANUP();
}

void test_write_map_begin(void)
{
    INIT_WRITER(32);
    
    // Map of 5 string->int32 pairs at field 0
    bond_writer_write_map_begin(&writer, 0, BOND_TYPE_STRING, BOND_TYPE_INT32, 5);
    
    // Field ID 0, type MAP (13): [000][01101] = 0x0D
    // Key type: STRING (9) = 0x09
    // Value type: INT32 (16) = 0x10
    // Count: 5 (varint) = 0x05
    TEST_ASSERT_EQUAL(4, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x0D, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x09, buffer.data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x10, buffer.data[2]);
    TEST_ASSERT_EQUAL_HEX8(0x05, buffer.data[3]);
    
    CLEANUP();
}

// ============================================================================
// Raw Value Writer Tests (no field header)
// ============================================================================

void test_write_bool_value(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_bool_value(&writer, true);
    bond_writer_write_bool_value(&writer, false);
    
    TEST_ASSERT_EQUAL(2, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[1]);
    
    CLEANUP();
}

void test_write_uint32_value(void)
{
    INIT_WRITER(16);
    
    bond_writer_write_uint32_value(&writer, 300);
    
    // 300 = varint: 0xAC, 0x02
    TEST_ASSERT_EQUAL(2, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0xAC, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x02, buffer.data[1]);
    
    CLEANUP();
}

void test_write_string_value(void)
{
    INIT_WRITER(32);
    
    bond_writer_write_string_value(&writer, "test");
    
    // Length: 4 (varint)
    // Chars: t e s t
    TEST_ASSERT_EQUAL(5, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x04, buffer.data[0]);
    TEST_ASSERT_EQUAL_HEX8('t', buffer.data[1]);
    TEST_ASSERT_EQUAL_HEX8('e', buffer.data[2]);
    TEST_ASSERT_EQUAL_HEX8('s', buffer.data[3]);
    TEST_ASSERT_EQUAL_HEX8('t', buffer.data[4]);
    
    CLEANUP();
}

// ============================================================================
// Integration Tests - Complete Struct
// ============================================================================

void test_simple_struct(void)
{
    INIT_WRITER(64);
    
    // Serialize a struct with:
    // field 1: bool = true
    // field 2: uint32 = 42
    // field 3: string = "hi"
    
    bond_writer_struct_begin(&writer);
    bond_writer_write_bool(&writer, 1, true);
    bond_writer_write_uint32(&writer, 2, 42);
    bond_writer_write_string(&writer, 3, "hi");
    bond_writer_struct_end(&writer);
    
    // Expected bytes:
    // bool field 1:   0x22, 0x01  (BOOL=2: [001][00010])
    // uint32 field 2: 0x45, 0x2A  (UINT32=5: [010][00101])
    // string field 3: 0x69, 0x02, 'h', 'i' (STRING=9: [011][01001])
    // STOP:           0x00
    
    TEST_ASSERT_EQUAL(9, buffer.size);
    
    // bool field 1
    TEST_ASSERT_EQUAL_HEX8(0x22, buffer.data[0]);  // [001][00010]
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer.data[1]);
    
    // uint32 field 2
    TEST_ASSERT_EQUAL_HEX8(0x45, buffer.data[2]);  // [010][00101]
    TEST_ASSERT_EQUAL_HEX8(0x2A, buffer.data[3]);  // 42
    
    // string field 3
    TEST_ASSERT_EQUAL_HEX8(0x69, buffer.data[4]);  // [011][01001]
    TEST_ASSERT_EQUAL_HEX8(0x02, buffer.data[5]);  // length 2
    TEST_ASSERT_EQUAL_HEX8('h', buffer.data[6]);
    TEST_ASSERT_EQUAL_HEX8('i', buffer.data[7]);
    
    // STOP
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[8]);
    
    CLEANUP();
}

void test_struct_with_list(void)
{
    INIT_WRITER(64);
    
    // Struct with a list of 3 uint8 values at field 1
    bond_writer_struct_begin(&writer);
    bond_writer_write_list_begin(&writer, 1, BOND_TYPE_UINT8, 3);
    bond_writer_write_uint8_value(&writer, 10);
    bond_writer_write_uint8_value(&writer, 20);
    bond_writer_write_uint8_value(&writer, 30);
    bond_writer_struct_end(&writer);
    
    // Expected:
    // List header: 0x2B (field 1, LIST), 0x03 (UINT8), 0x03 (count)
    // Elements: 0x0A, 0x14, 0x1E
    // STOP: 0x00
    
    TEST_ASSERT_EQUAL(7, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x2B, buffer.data[0]);  // list field 1
    TEST_ASSERT_EQUAL_HEX8(0x03, buffer.data[1]);  // element type UINT8
    TEST_ASSERT_EQUAL_HEX8(0x03, buffer.data[2]);  // count 3
    TEST_ASSERT_EQUAL_HEX8(0x0A, buffer.data[3]);  // 10
    TEST_ASSERT_EQUAL_HEX8(0x14, buffer.data[4]);  // 20
    TEST_ASSERT_EQUAL_HEX8(0x1E, buffer.data[5]);  // 30
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[6]);  // STOP
    
    CLEANUP();
}

void test_struct_with_map(void)
{
    INIT_WRITER(64);
    
    // Struct with a map of 2 string->uint32 pairs at field 0
    bond_writer_struct_begin(&writer);
    bond_writer_write_map_begin(&writer, 0, BOND_TYPE_STRING, BOND_TYPE_UINT32, 2);
    bond_writer_write_string_value(&writer, "a");
    bond_writer_write_uint32_value(&writer, 1);
    bond_writer_write_string_value(&writer, "b");
    bond_writer_write_uint32_value(&writer, 2);
    bond_writer_struct_end(&writer);
    
    // Expected:
    // Map header: 0x0D (field 0, MAP), 0x09 (STRING), 0x05 (UINT32), 0x02 (count)
    // Pair 1: 0x01, 'a', 0x01
    // Pair 2: 0x01, 'b', 0x02
    // STOP: 0x00
    
    TEST_ASSERT_EQUAL(11, buffer.size);
    TEST_ASSERT_EQUAL_HEX8(0x0D, buffer.data[0]);   // map field 0
    TEST_ASSERT_EQUAL_HEX8(0x09, buffer.data[1]);   // key type STRING
    TEST_ASSERT_EQUAL_HEX8(0x05, buffer.data[2]);   // value type UINT32
    TEST_ASSERT_EQUAL_HEX8(0x02, buffer.data[3]);   // count 2
    // Pair 1
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer.data[4]);   // string len 1
    TEST_ASSERT_EQUAL_HEX8('a', buffer.data[5]);    // "a"
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer.data[6]);   // value 1
    // Pair 2
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer.data[7]);   // string len 1
    TEST_ASSERT_EQUAL_HEX8('b', buffer.data[8]);    // "b"
    TEST_ASSERT_EQUAL_HEX8(0x02, buffer.data[9]);   // value 2
    // STOP
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer.data[10]);
    
    CLEANUP();
}

// ============================================================================
// Test Runner
// ============================================================================

int main(void)
{
    UNITY_BEGIN();
    
    // Struct control
    RUN_TEST(test_struct_end_writes_stop);
    
    // Field header encoding
    RUN_TEST(test_field_header_id_0);
    RUN_TEST(test_field_header_id_5);
    RUN_TEST(test_field_header_id_6_escape);
    RUN_TEST(test_field_header_id_255_escape);
    RUN_TEST(test_field_header_id_256_escape);
    RUN_TEST(test_field_header_id_max);
    
    // Primitive writers
    RUN_TEST(test_write_bool_true);
    RUN_TEST(test_write_bool_false);
    RUN_TEST(test_write_uint8);
    RUN_TEST(test_write_uint16_small);
    RUN_TEST(test_write_uint16_large);
    RUN_TEST(test_write_uint32);
    RUN_TEST(test_write_uint64);
    RUN_TEST(test_write_int8);
    RUN_TEST(test_write_int16);
    RUN_TEST(test_write_int32);
    RUN_TEST(test_write_int64);
    RUN_TEST(test_write_float);
    RUN_TEST(test_write_double);
    RUN_TEST(test_write_string_empty);
    RUN_TEST(test_write_string);
    
    // Container writers
    RUN_TEST(test_write_list_begin);
    RUN_TEST(test_write_set_begin);
    RUN_TEST(test_write_map_begin);
    
    // Raw value writers
    RUN_TEST(test_write_bool_value);
    RUN_TEST(test_write_uint32_value);
    RUN_TEST(test_write_string_value);
    
    // Integration tests
    RUN_TEST(test_simple_struct);
    RUN_TEST(test_struct_with_list);
    RUN_TEST(test_struct_with_map);
    
    return UNITY_END();
}
