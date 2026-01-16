/**
 * @file test_reader.c
 * @brief Unit tests for bond_reader - field header only for now
 */

#include "unity.h"
#include "bond_reader.h"
#include "bond_types.h"

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// Field Header Tests
// ============================================================================

void test_read_field_header_id_0(void)
{
    // Field ID 0, type UINT32 (5): [000][00101] = 0x05
    uint8_t data[] = {0x05};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint16_t field_id;
    uint8_t type;
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(0, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_UINT32, type);
}

void test_read_field_header_id_5(void)
{
    // Field ID 5, type STRING (9): [101][01001] = 0xA9
    uint8_t data[] = {0xA9};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint16_t field_id;
    uint8_t type;
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(5, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_STRING, type);
}

void test_read_field_header_escape6(void)
{
    // Field ID 100, type BOOL (2): escape 6 = [110][00010] = 0xC2, then 0x64
    uint8_t data[] = {0xC2, 0x64};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint16_t field_id;
    uint8_t type;
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(100, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_BOOL, type);
}

void test_read_field_header_escape7(void)
{
    // Field ID 300 (0x012C), type UINT64 (6): escape 7 = [111][00110] = 0xE6, then 0x2C, 0x01
    uint8_t data[] = {0xE6, 0x2C, 0x01};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint16_t field_id;
    uint8_t type;
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(300, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_UINT64, type);
}

void test_read_field_header_stop(void)
{
    // STOP marker: 0x00
    uint8_t data[] = {0x00};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint16_t field_id;
    uint8_t type;
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(0, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_STOP, type);
}

void test_read_field_header_truncated_escape6(void)
{
    // Escape 6 but missing the field ID byte
    uint8_t data[] = {0xC2};  // No second byte
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint16_t field_id;
    uint8_t type;
    TEST_ASSERT_FALSE(bond_reader_read_field_header(&reader, &field_id, &type));
}

void test_read_field_header_empty_buffer(void)
{
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, NULL, 0);
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint16_t field_id;
    uint8_t type;
    TEST_ASSERT_FALSE(bond_reader_read_field_header(&reader, &field_id, &type));
}

// ============================================================================
// Bool Value Tests
// ============================================================================

void test_read_bool_value_true(void)
{
    uint8_t data[] = {0x01};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    bool value;
    TEST_ASSERT_TRUE(bond_reader_read_bool_value(&reader, &value));
    TEST_ASSERT_TRUE(value);
}

void test_read_bool_value_false(void)
{
    uint8_t data[] = {0x00};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    bool value;
    TEST_ASSERT_TRUE(bond_reader_read_bool_value(&reader, &value));
    TEST_ASSERT_FALSE(value);
}

void test_read_bool_value_nonzero(void)
{
    // Any non-zero value should be true
    uint8_t data[] = {0xFF};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    bool value;
    TEST_ASSERT_TRUE(bond_reader_read_bool_value(&reader, &value));
    TEST_ASSERT_TRUE(value);
}

void test_read_bool_value_empty_buffer(void)
{
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, NULL, 0);
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    bool value;
    TEST_ASSERT_FALSE(bond_reader_read_bool_value(&reader, &value));
}

void test_read_multiple_bools(void)
{
    uint8_t data[] = {0x01, 0x00, 0x01};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    bool v1, v2, v3;
    TEST_ASSERT_TRUE(bond_reader_read_bool_value(&reader, &v1));
    TEST_ASSERT_TRUE(bond_reader_read_bool_value(&reader, &v2));
    TEST_ASSERT_TRUE(bond_reader_read_bool_value(&reader, &v3));
    TEST_ASSERT_TRUE(v1);
    TEST_ASSERT_FALSE(v2);
    TEST_ASSERT_TRUE(v3);
    
    // No more data
    bool v4;
    TEST_ASSERT_FALSE(bond_reader_read_bool_value(&reader, &v4));
}

// ============================================================================
// Uint8/Int8 Value Tests
// ============================================================================

void test_read_uint8_value(void)
{
    uint8_t data[] = {0xAB};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint8_t value;
    TEST_ASSERT_TRUE(bond_reader_read_uint8_value(&reader, &value));
    TEST_ASSERT_EQUAL_HEX8(0xAB, value);
}

void test_read_int8_value_positive(void)
{
    uint8_t data[] = {0x7F};  // 127
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    int8_t value;
    TEST_ASSERT_TRUE(bond_reader_read_int8_value(&reader, &value));
    TEST_ASSERT_EQUAL(127, value);
}

void test_read_int8_value_negative(void)
{
    uint8_t data[] = {0xFF};  // -1 in two's complement
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    int8_t value;
    TEST_ASSERT_TRUE(bond_reader_read_int8_value(&reader, &value));
    TEST_ASSERT_EQUAL(-1, value);
}

// ============================================================================
// Uint16 Value Tests
// ============================================================================

void test_read_uint16_value_small(void)
{
    // 127 = 0x7F (1-byte varint)
    uint8_t data[] = {0x7F};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint16_t value;
    TEST_ASSERT_TRUE(bond_reader_read_uint16_value(&reader, &value));
    TEST_ASSERT_EQUAL(127, value);
}

void test_read_uint16_value_large(void)
{
    // 300 = 0x012C as varint: 0xAC, 0x02
    uint8_t data[] = {0xAC, 0x02};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint16_t value;
    TEST_ASSERT_TRUE(bond_reader_read_uint16_value(&reader, &value));
    TEST_ASSERT_EQUAL(300, value);
}

void test_read_uint16_value_max(void)
{
    // 65535 = 0xFFFF as varint: 0xFF, 0xFF, 0x03
    uint8_t data[] = {0xFF, 0xFF, 0x03};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint16_t value;
    TEST_ASSERT_TRUE(bond_reader_read_uint16_value(&reader, &value));
    TEST_ASSERT_EQUAL(65535, value);
}

void test_read_uint16_value_truncated(void)
{
    // Incomplete varint (continuation bit set but no more bytes)
    uint8_t data[] = {0x80};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint16_t value;
    TEST_ASSERT_FALSE(bond_reader_read_uint16_value(&reader, &value));
}

// ============================================================================
// Uint32 Value Tests
// ============================================================================

void test_read_uint32_value_small(void)
{
    // 127 fits in 1 byte
    uint8_t data[] = {0x7F};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint32_t value;
    TEST_ASSERT_TRUE(bond_reader_read_uint32_value(&reader, &value));
    TEST_ASSERT_EQUAL(127, value);
}

void test_read_uint32_value_2bytes(void)
{
    // 300 = 0x012C as varint: 0xAC, 0x02
    uint8_t data[] = {0xAC, 0x02};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint32_t value;
    TEST_ASSERT_TRUE(bond_reader_read_uint32_value(&reader, &value));
    TEST_ASSERT_EQUAL(300, value);
}

void test_read_uint32_value_max(void)
{
    // 0xFFFFFFFF as varint: 0xFF, 0xFF, 0xFF, 0xFF, 0x0F
    uint8_t data[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x0F};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint32_t value;
    TEST_ASSERT_TRUE(bond_reader_read_uint32_value(&reader, &value));
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, value);
}

void test_read_uint32_value_truncated(void)
{
    // Incomplete varint (continuation bit set but no more bytes)
    uint8_t data[] = {0x80, 0x80};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint32_t value;
    TEST_ASSERT_FALSE(bond_reader_read_uint32_value(&reader, &value));
}

// ============================================================================
// Uint64 Value Tests
// ============================================================================

void test_read_uint64_value_small(void)
{
    uint8_t data[] = {0x7F};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint64_t value;
    TEST_ASSERT_TRUE(bond_reader_read_uint64_value(&reader, &value));
    TEST_ASSERT_EQUAL_UINT64(127, value);
}

void test_read_uint64_value_large(void)
{
    // 0xFFFFFFFF (4294967295) as varint: 0xFF, 0xFF, 0xFF, 0xFF, 0x0F
    uint8_t data[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x0F};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    uint64_t value;
    TEST_ASSERT_TRUE(bond_reader_read_uint64_value(&reader, &value));
    TEST_ASSERT_EQUAL_UINT64(4294967295ULL, value);
}

// ============================================================================
// Signed Integer Value Tests
// ============================================================================

void test_read_int16_value_positive(void)
{
    // 100 zigzag encoded = 200 = 0xC8, 0x01
    uint8_t data[] = {0xC8, 0x01};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    int16_t value;
    TEST_ASSERT_TRUE(bond_reader_read_int16_value(&reader, &value));
    TEST_ASSERT_EQUAL_INT16(100, value);
}

void test_read_int16_value_negative(void)
{
    // -100 zigzag encoded = 199 = 0xC7, 0x01
    uint8_t data[] = {0xC7, 0x01};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    int16_t value;
    TEST_ASSERT_TRUE(bond_reader_read_int16_value(&reader, &value));
    TEST_ASSERT_EQUAL_INT16(-100, value);
}

void test_read_int32_value_positive(void)
{
    // 1000 zigzag encoded = 2000 = 0xD0, 0x0F
    uint8_t data[] = {0xD0, 0x0F};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    int32_t value;
    TEST_ASSERT_TRUE(bond_reader_read_int32_value(&reader, &value));
    TEST_ASSERT_EQUAL_INT32(1000, value);
}

void test_read_int32_value_negative(void)
{
    // -1000 zigzag encoded = 1999 = 0xCF, 0x0F
    uint8_t data[] = {0xCF, 0x0F};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    int32_t value;
    TEST_ASSERT_TRUE(bond_reader_read_int32_value(&reader, &value));
    TEST_ASSERT_EQUAL_INT32(-1000, value);
}

void test_read_int64_value_negative(void)
{
    // -1 zigzag encoded = 1 = 0x01
    uint8_t data[] = {0x01};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    int64_t value;
    TEST_ASSERT_TRUE(bond_reader_read_int64_value(&reader, &value));
    TEST_ASSERT_EQUAL_INT64(-1, value);
}

// ============================================================================
// Float/Double Value Tests
// ============================================================================

void test_read_float_value(void)
{
    // 3.14f in little-endian IEEE 754
    uint8_t data[] = {0xC3, 0xF5, 0x48, 0x40};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    float value;
    TEST_ASSERT_TRUE(bond_reader_read_float_value(&reader, &value));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.14f, value);
}

void test_read_double_value(void)
{
    // 3.14159265359 in little-endian IEEE 754
    uint8_t data[] = {0xEA, 0x2E, 0x44, 0x54, 0xFB, 0x21, 0x09, 0x40};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    double value;
    TEST_ASSERT_TRUE(bond_reader_read_double_value(&reader, &value));
    // Cast to float for comparison (Unity double disabled)
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.1416f, (float)value);
}

void test_read_float_truncated(void)
{
    // Only 3 bytes instead of 4
    uint8_t data[] = {0xC3, 0xF5, 0x48};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    float value;
    TEST_ASSERT_FALSE(bond_reader_read_float_value(&reader, &value));
}

// ============================================================================
// String Value Tests
// ============================================================================

void test_read_string_value_simple(void)
{
    // Length 5, then "hello"
    uint8_t data[] = {0x05, 'h', 'e', 'l', 'l', 'o'};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    const char *str;
    uint32_t len;
    TEST_ASSERT_TRUE(bond_reader_read_string_value(&reader, &str, &len));
    TEST_ASSERT_EQUAL_UINT32(5, len);
    TEST_ASSERT_EQUAL_MEMORY("hello", str, 5);
}

void test_read_string_value_empty(void)
{
    // Length 0
    uint8_t data[] = {0x00};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    const char *str;
    uint32_t len;
    TEST_ASSERT_TRUE(bond_reader_read_string_value(&reader, &str, &len));
    TEST_ASSERT_EQUAL_UINT32(0, len);
}

void test_read_string_value_truncated(void)
{
    // Length says 10 but only 3 bytes available
    uint8_t data[] = {0x0A, 'a', 'b', 'c'};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    const char *str;
    uint32_t len;
    TEST_ASSERT_FALSE(bond_reader_read_string_value(&reader, &str, &len));
}

// ============================================================================
// Skip Tests
// ============================================================================

void test_skip_bool(void)
{
    uint8_t data[] = {0x01, 0x42};  // bool=true, then 0x42
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    TEST_ASSERT_TRUE(bond_reader_skip(&reader, BOND_TYPE_BOOL));
    // Next byte should be 0x42
    TEST_ASSERT_EQUAL(0x42, bond_buffer_read_byte(reader.buffer));
}

void test_skip_uint32(void)
{
    // 300 as varint (0xAC, 0x02), then 0x42
    uint8_t data[] = {0xAC, 0x02, 0x42};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    TEST_ASSERT_TRUE(bond_reader_skip(&reader, BOND_TYPE_UINT32));
    TEST_ASSERT_EQUAL(0x42, bond_buffer_read_byte(reader.buffer));
}

void test_skip_float(void)
{
    // 4 bytes of float, then 0x42
    uint8_t data[] = {0xC3, 0xF5, 0x48, 0x40, 0x42};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    TEST_ASSERT_TRUE(bond_reader_skip(&reader, BOND_TYPE_FLOAT));
    TEST_ASSERT_EQUAL(0x42, bond_buffer_read_byte(reader.buffer));
}

void test_skip_double(void)
{
    // 8 bytes of double, then 0x42
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x42};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    TEST_ASSERT_TRUE(bond_reader_skip(&reader, BOND_TYPE_DOUBLE));
    TEST_ASSERT_EQUAL(0x42, bond_buffer_read_byte(reader.buffer));
}

void test_skip_string(void)
{
    // Length 5, "hello", then 0x42
    uint8_t data[] = {0x05, 'h', 'e', 'l', 'l', 'o', 0x42};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    TEST_ASSERT_TRUE(bond_reader_skip(&reader, BOND_TYPE_STRING));
    TEST_ASSERT_EQUAL(0x42, bond_buffer_read_byte(reader.buffer));
}

void test_skip_list(void)
{
    // List of 3 uint8: element_type=UINT8(3), count=3, values 0x01,0x02,0x03, then 0x42
    uint8_t data[] = {BOND_TYPE_UINT8, 0x03, 0x01, 0x02, 0x03, 0x42};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    TEST_ASSERT_TRUE(bond_reader_skip(&reader, BOND_TYPE_LIST));
    TEST_ASSERT_EQUAL(0x42, bond_buffer_read_byte(reader.buffer));
}

void test_skip_map(void)
{
    // Map<uint8, uint8> with 2 entries: key_type=3, value_type=3, count=2,
    // entries: (0x0A, 0x14), (0x0B, 0x15), then 0x42
    uint8_t data[] = {BOND_TYPE_UINT8, BOND_TYPE_UINT8, 0x02, 0x0A, 0x14, 0x0B, 0x15, 0x42};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    TEST_ASSERT_TRUE(bond_reader_skip(&reader, BOND_TYPE_MAP));
    TEST_ASSERT_EQUAL(0x42, bond_buffer_read_byte(reader.buffer));
}

void test_skip_struct(void)
{
    // Simple struct: field 1 (bool=true), field 2 (uint8=42), STOP, then 0x99
    // Field 1: id=1, type=BOOL(2) -> (1<<5)|2 = 0x22, value=0x01
    // Field 2: id=2, type=UINT8(3) -> (2<<5)|3 = 0x43, value=0x2A
    // STOP: type=0 -> 0x00
    uint8_t data[] = {0x22, 0x01, 0x43, 0x2A, 0x00, 0x99};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    TEST_ASSERT_TRUE(bond_reader_skip(&reader, BOND_TYPE_STRUCT));
    TEST_ASSERT_EQUAL(0x99, bond_buffer_read_byte(reader.buffer));
}

void test_skip_nested_struct(void)
{
    // Outer struct with field 1 = inner struct, then STOP
    // Inner struct: field 1 (uint8=0x11), STOP
    // Field 1 outer: id=1, type=STRUCT(10) -> (1<<5)|10 = 0x2A
    // Field 1 inner: id=1, type=UINT8(3) -> 0x23, value=0x11
    // STOP inner: 0x00
    // STOP outer: 0x00
    // Then 0x88
    uint8_t data[] = {0x2A, 0x23, 0x11, 0x00, 0x00, 0x88};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    TEST_ASSERT_TRUE(bond_reader_skip(&reader, BOND_TYPE_STRUCT));
    TEST_ASSERT_EQUAL(0x88, bond_buffer_read_byte(reader.buffer));
}

void test_skip_unknown_type(void)
{
    uint8_t data[] = {0x01};
    bond_buffer buffer;
    bond_buffer_init_from(&buffer, data, sizeof(data));
    
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    
    // Type 127 is UNAVAILABLE/unknown
    TEST_ASSERT_FALSE(bond_reader_skip(&reader, 127));
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();
    
    // Field header tests
    RUN_TEST(test_read_field_header_id_0);
    RUN_TEST(test_read_field_header_id_5);
    RUN_TEST(test_read_field_header_escape6);
    RUN_TEST(test_read_field_header_escape7);
    RUN_TEST(test_read_field_header_stop);
    RUN_TEST(test_read_field_header_truncated_escape6);
    RUN_TEST(test_read_field_header_empty_buffer);
    
    // Bool value tests
    RUN_TEST(test_read_bool_value_true);
    RUN_TEST(test_read_bool_value_false);
    RUN_TEST(test_read_bool_value_nonzero);
    RUN_TEST(test_read_bool_value_empty_buffer);
    RUN_TEST(test_read_multiple_bools);
    
    // Uint8/Int8 value tests
    RUN_TEST(test_read_uint8_value);
    RUN_TEST(test_read_int8_value_positive);
    RUN_TEST(test_read_int8_value_negative);
    
    // Uint16 value tests
    RUN_TEST(test_read_uint16_value_small);
    RUN_TEST(test_read_uint16_value_large);
    RUN_TEST(test_read_uint16_value_max);
    RUN_TEST(test_read_uint16_value_truncated);
    
    // Uint32 value tests
    RUN_TEST(test_read_uint32_value_small);
    RUN_TEST(test_read_uint32_value_2bytes);
    RUN_TEST(test_read_uint32_value_max);
    RUN_TEST(test_read_uint32_value_truncated);
    
    // Uint64 value tests
    RUN_TEST(test_read_uint64_value_small);
    RUN_TEST(test_read_uint64_value_large);
    
    // Signed integer tests
    RUN_TEST(test_read_int16_value_positive);
    RUN_TEST(test_read_int16_value_negative);
    RUN_TEST(test_read_int32_value_positive);
    RUN_TEST(test_read_int32_value_negative);
    RUN_TEST(test_read_int64_value_negative);
    
    // Float/Double tests
    RUN_TEST(test_read_float_value);
    RUN_TEST(test_read_double_value);
    RUN_TEST(test_read_float_truncated);
    
    // String tests
    RUN_TEST(test_read_string_value_simple);
    RUN_TEST(test_read_string_value_empty);
    RUN_TEST(test_read_string_value_truncated);
    
    // Skip tests
    RUN_TEST(test_skip_bool);
    RUN_TEST(test_skip_uint32);
    RUN_TEST(test_skip_float);
    RUN_TEST(test_skip_double);
    RUN_TEST(test_skip_string);
    RUN_TEST(test_skip_list);
    RUN_TEST(test_skip_map);
    RUN_TEST(test_skip_struct);
    RUN_TEST(test_skip_nested_struct);
    RUN_TEST(test_skip_unknown_type);
    
    return UNITY_END();
}
