/**
 * @file test_roundtrip.c
 * @brief Roundtrip tests: write with BondWriter, read with BondReader
 */

#include "unity.h"
#include "bond_writer.h"
#include "bond_reader.h"
#include "bond_buffer.h"
#include "bond_types.h"
#include <string.h>
#include <math.h>

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// Primitive Roundtrip Tests
// ============================================================================

void test_roundtrip_bool(void)
{
    
    bond_buffer buffer;
    bond_buffer_init(&buffer, 256);
    
    // Write
    bond_writer writer;
    bond_writer_init(&writer, &buffer);
    bond_writer_struct_begin(&writer);
    bond_writer_write_bool(&writer, 1, true);
    bond_writer_write_bool(&writer, 2, false);
    bond_writer_struct_end(&writer);
    
    // Reset for reading
    buffer.read_pos = 0;
    
    // Read
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    bond_reader_struct_begin(&reader);
    
    uint16_t field_id;
    uint8_t type;
    bool value;
    
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(1, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_BOOL, type);
    TEST_ASSERT_TRUE(bond_reader_read_bool_value(&reader, &value));
    TEST_ASSERT_TRUE(value);
    
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(2, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_BOOL, type);
    TEST_ASSERT_TRUE(bond_reader_read_bool_value(&reader, &value));
    TEST_ASSERT_FALSE(value);
    
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(BOND_TYPE_STOP, type);
    
    bond_reader_struct_end(&reader);
}

void test_roundtrip_integers(void)
{
    
    bond_buffer buffer;
    bond_buffer_init(&buffer, 256);
    
    // Write
    bond_writer writer;
    bond_writer_init(&writer, &buffer);
    bond_writer_struct_begin(&writer);
    bond_writer_write_uint8(&writer, 1, 255);
    bond_writer_write_int8(&writer, 2, -128);
    bond_writer_write_uint16(&writer, 3, 65535);
    bond_writer_write_int16(&writer, 4, -1000);
    bond_writer_write_uint32(&writer, 5, 0xDEADBEEF);
    bond_writer_write_int32(&writer, 100, -123456);  // escape6 field id
    bond_writer_write_uint64(&writer, 300, 0x123456789ABCDEF0ULL);  // escape7 field id
    bond_writer_write_int64(&writer, 301, -9999999999LL);
    bond_writer_struct_end(&writer);
    
    // Reset for reading
    buffer.read_pos = 0;
    
    // Read
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    bond_reader_struct_begin(&reader);
    
    uint16_t field_id;
    uint8_t type;
    
    // uint8
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(1, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_UINT8, type);
    uint8_t u8;
    TEST_ASSERT_TRUE(bond_reader_read_uint8_value(&reader, &u8));
    TEST_ASSERT_EQUAL(255, u8);
    
    // int8
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(2, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_INT8, type);
    int8_t i8;
    TEST_ASSERT_TRUE(bond_reader_read_int8_value(&reader, &i8));
    TEST_ASSERT_EQUAL(-128, i8);
    
    // uint16
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(3, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_UINT16, type);
    uint16_t u16;
    TEST_ASSERT_TRUE(bond_reader_read_uint16_value(&reader, &u16));
    TEST_ASSERT_EQUAL(65535, u16);
    
    // int16
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(4, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_INT16, type);
    int16_t i16;
    TEST_ASSERT_TRUE(bond_reader_read_int16_value(&reader, &i16));
    TEST_ASSERT_EQUAL(-1000, i16);
    
    // uint32
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(5, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_UINT32, type);
    uint32_t u32;
    TEST_ASSERT_TRUE(bond_reader_read_uint32_value(&reader, &u32));
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, u32);
    
    // int32 (field id 100 - escape6)
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(100, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_INT32, type);
    int32_t i32;
    TEST_ASSERT_TRUE(bond_reader_read_int32_value(&reader, &i32));
    TEST_ASSERT_EQUAL(-123456, i32);
    
    // uint64 (field id 300 - escape7)
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(300, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_UINT64, type);
    uint64_t u64;
    TEST_ASSERT_TRUE(bond_reader_read_uint64_value(&reader, &u64));
    TEST_ASSERT_EQUAL_HEX64(0x123456789ABCDEF0ULL, u64);
    
    // int64
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(301, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_INT64, type);
    int64_t i64;
    TEST_ASSERT_TRUE(bond_reader_read_int64_value(&reader, &i64));
    TEST_ASSERT_EQUAL(-9999999999LL, i64);
    
    // STOP
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(BOND_TYPE_STOP, type);
    
    bond_reader_struct_end(&reader);
}

void test_roundtrip_float_double(void)
{
    
    bond_buffer buffer;
    bond_buffer_init(&buffer, 256);
    
    // Write
    bond_writer writer;
    bond_writer_init(&writer, &buffer);
    bond_writer_struct_begin(&writer);
    bond_writer_write_float(&writer, 1, 3.14159f);
    bond_writer_write_double(&writer, 2, 2.718281828459045);
    bond_writer_struct_end(&writer);
    
    // Reset for reading
    buffer.read_pos = 0;
    
    // Read
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    bond_reader_struct_begin(&reader);
    
    uint16_t field_id;
    uint8_t type;
    
    // float
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(1, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_FLOAT, type);
    float f;
    TEST_ASSERT_TRUE(bond_reader_read_float_value(&reader, &f));
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 3.14159f, f);
    
    // double
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(2, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_DOUBLE, type);
    double d;
    TEST_ASSERT_TRUE(bond_reader_read_double_value(&reader, &d));
    TEST_ASSERT_FLOAT_WITHIN(0.0000001f, 2.718281828459045f, (float)d);
    
    // STOP
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(BOND_TYPE_STOP, type);
    
    bond_reader_struct_end(&reader);
}

void test_roundtrip_string(void)
{
    
    bond_buffer buffer;
    bond_buffer_init(&buffer, 256);
    
    // Write
    bond_writer writer;
    bond_writer_init(&writer, &buffer);
    bond_writer_struct_begin(&writer);
    bond_writer_write_string(&writer, 1, "Hello, Bond!");
    bond_writer_write_string(&writer, 2, "");  // empty string
    bond_writer_struct_end(&writer);
    
    // Reset for reading
    buffer.read_pos = 0;
    
    // Read
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    bond_reader_struct_begin(&reader);
    
    uint16_t field_id;
    uint8_t type;
    const char *str;
    uint32_t len;
    
    // First string
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(1, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_STRING, type);
    TEST_ASSERT_TRUE(bond_reader_read_string_value(&reader, &str, &len));
    TEST_ASSERT_EQUAL(12, len);
    TEST_ASSERT_EQUAL_MEMORY("Hello, Bond!", str, len);
    
    // Empty string
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(2, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_STRING, type);
    TEST_ASSERT_TRUE(bond_reader_read_string_value(&reader, &str, &len));
    TEST_ASSERT_EQUAL(0, len);
    
    // STOP
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(BOND_TYPE_STOP, type);
    
    bond_reader_struct_end(&reader);
}

// ============================================================================
// Container Roundtrip Tests
// ============================================================================

void test_roundtrip_list(void)
{
    
    bond_buffer buffer;
    bond_buffer_init(&buffer, 256);
    
    // Write list of uint32
    bond_writer writer;
    bond_writer_init(&writer, &buffer);
    bond_writer_struct_begin(&writer);
    bond_writer_write_list_begin(&writer, 1, BOND_TYPE_UINT32, 3);
    bond_writer_write_uint32_value(&writer, 100);
    bond_writer_write_uint32_value(&writer, 200);
    bond_writer_write_uint32_value(&writer, 300);
    bond_writer_struct_end(&writer);
    
    // Reset for reading
    buffer.read_pos = 0;
    
    // Read
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    bond_reader_struct_begin(&reader);
    
    uint16_t field_id;
    uint8_t type;
    
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(1, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_LIST, type);
    
    uint8_t element_type;
    uint32_t count;
    TEST_ASSERT_TRUE(bond_reader_read_list_begin(&reader, &element_type, &count));
    TEST_ASSERT_EQUAL(BOND_TYPE_UINT32, element_type);
    TEST_ASSERT_EQUAL(3, count);
    
    uint32_t values[3];
    for (int i = 0; i < 3; i++)
    {
        TEST_ASSERT_TRUE(bond_reader_read_uint32_value(&reader, &values[i]));
    }
    TEST_ASSERT_EQUAL(100, values[0]);
    TEST_ASSERT_EQUAL(200, values[1]);
    TEST_ASSERT_EQUAL(300, values[2]);
    
    // STOP
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(BOND_TYPE_STOP, type);
    
    bond_reader_struct_end(&reader);
}

void test_roundtrip_map(void)
{
    
    bond_buffer buffer;
    bond_buffer_init(&buffer, 256);
    
    // Write map<uint8, string>
    bond_writer writer;
    bond_writer_init(&writer, &buffer);
    bond_writer_struct_begin(&writer);
    bond_writer_write_map_begin(&writer, 1, BOND_TYPE_UINT8, BOND_TYPE_STRING, 2);
    // Entry 1: 1 -> "one"
    bond_writer_write_uint8_value(&writer, 1);
    bond_writer_write_string_value(&writer, "one");
    // Entry 2: 2 -> "two"
    bond_writer_write_uint8_value(&writer, 2);
    bond_writer_write_string_value(&writer, "two");
    bond_writer_struct_end(&writer);
    
    // Reset for reading
    buffer.read_pos = 0;
    
    // Read
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    bond_reader_struct_begin(&reader);
    
    uint16_t field_id;
    uint8_t field_type;
    
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &field_type));
    TEST_ASSERT_EQUAL(1, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_MAP, field_type);
    
    uint8_t key_type, value_type;
    uint32_t count;
    TEST_ASSERT_TRUE(bond_reader_read_map_begin(&reader, &key_type, &value_type, &count));
    TEST_ASSERT_EQUAL(BOND_TYPE_UINT8, key_type);
    TEST_ASSERT_EQUAL(BOND_TYPE_STRING, value_type);
    TEST_ASSERT_EQUAL(2, count);
    
    // Entry 1
    uint8_t key;
    const char *val;
    uint32_t val_len;
    TEST_ASSERT_TRUE(bond_reader_read_uint8_value(&reader, &key));
    TEST_ASSERT_EQUAL(1, key);
    TEST_ASSERT_TRUE(bond_reader_read_string_value(&reader, &val, &val_len));
    TEST_ASSERT_EQUAL(3, val_len);
    TEST_ASSERT_EQUAL_MEMORY("one", val, 3);
    
    // Entry 2
    TEST_ASSERT_TRUE(bond_reader_read_uint8_value(&reader, &key));
    TEST_ASSERT_EQUAL(2, key);
    TEST_ASSERT_TRUE(bond_reader_read_string_value(&reader, &val, &val_len));
    TEST_ASSERT_EQUAL(3, val_len);
    TEST_ASSERT_EQUAL_MEMORY("two", val, 3);
    
    // STOP
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &field_type));
    TEST_ASSERT_EQUAL(BOND_TYPE_STOP, field_type);
    
    bond_reader_struct_end(&reader);
}

// ============================================================================
// Skip Roundtrip Tests
// ============================================================================

void test_roundtrip_skip_unknown_field(void)
{
    
    bond_buffer buffer;
    bond_buffer_init(&buffer, 256);
    
    // Write struct with 3 fields
    bond_writer writer;
    bond_writer_init(&writer, &buffer);
    bond_writer_struct_begin(&writer);
    bond_writer_write_uint32(&writer, 1, 42);
    bond_writer_write_string(&writer, 2, "skip me");  // We'll skip this
    bond_writer_write_uint32(&writer, 3, 99);
    bond_writer_struct_end(&writer);
    
    // Reset for reading
    buffer.read_pos = 0;
    
    // Read, but skip field 2
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    bond_reader_struct_begin(&reader);
    
    uint16_t field_id;
    uint8_t type;
    
    // Field 1
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(1, field_id);
    uint32_t val1;
    TEST_ASSERT_TRUE(bond_reader_read_uint32_value(&reader, &val1));
    TEST_ASSERT_EQUAL(42, val1);
    
    // Field 2 - skip it
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(2, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_STRING, type);
    TEST_ASSERT_TRUE(bond_reader_skip(&reader, type));
    
    // Field 3 - should still work
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(3, field_id);
    uint32_t val3;
    TEST_ASSERT_TRUE(bond_reader_read_uint32_value(&reader, &val3));
    TEST_ASSERT_EQUAL(99, val3);
    
    // STOP
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(BOND_TYPE_STOP, type);
    
    bond_reader_struct_end(&reader);
}

void test_roundtrip_skip_nested_struct(void)
{
    
    bond_buffer buffer;
    bond_buffer_init(&buffer, 256);
    
    // Write outer struct with nested struct
    bond_writer writer;
    bond_writer_init(&writer, &buffer);
    bond_writer_struct_begin(&writer);
    
    bond_writer_write_uint32(&writer, 1, 111);
    
    // Nested struct at field 2
    bond_writer_write_field_header(&writer, 2, BOND_TYPE_STRUCT);
    bond_writer_struct_begin(&writer);
    bond_writer_write_string(&writer, 1, "nested");
    bond_writer_write_uint64(&writer, 2, 999999999ULL);
    bond_writer_struct_end(&writer);
    
    bond_writer_write_uint32(&writer, 3, 222);
    bond_writer_struct_end(&writer);
    
    // Reset for reading
    buffer.read_pos = 0;
    
    // Read, skip nested struct
    BondReader reader;
    bond_reader_init(&reader, &buffer);
    bond_reader_struct_begin(&reader);
    
    uint16_t field_id;
    uint8_t type;
    
    // Field 1
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(1, field_id);
    uint32_t val1;
    TEST_ASSERT_TRUE(bond_reader_read_uint32_value(&reader, &val1));
    TEST_ASSERT_EQUAL(111, val1);
    
    // Field 2 (nested struct) - skip it entirely
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(2, field_id);
    TEST_ASSERT_EQUAL(BOND_TYPE_STRUCT, type);
    TEST_ASSERT_TRUE(bond_reader_skip(&reader, type));
    
    // Field 3 - should still work
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(3, field_id);
    uint32_t val3;
    TEST_ASSERT_TRUE(bond_reader_read_uint32_value(&reader, &val3));
    TEST_ASSERT_EQUAL(222, val3);
    
    // STOP
    TEST_ASSERT_TRUE(bond_reader_read_field_header(&reader, &field_id, &type));
    TEST_ASSERT_EQUAL(BOND_TYPE_STOP, type);
    
    bond_reader_struct_end(&reader);
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    UNITY_BEGIN();
    
    // Primitive roundtrips
    RUN_TEST(test_roundtrip_bool);
    RUN_TEST(test_roundtrip_integers);
    RUN_TEST(test_roundtrip_float_double);
    RUN_TEST(test_roundtrip_string);
    
    // Container roundtrips
    RUN_TEST(test_roundtrip_list);
    RUN_TEST(test_roundtrip_map);
    
    // Skip roundtrips
    RUN_TEST(test_roundtrip_skip_unknown_field);
    RUN_TEST(test_roundtrip_skip_nested_struct);
    
    return UNITY_END();
}
