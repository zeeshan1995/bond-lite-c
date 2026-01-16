#include <unity.h>
#include "bond_buffer.h"

void setUp(void) {}
void tearDown(void) {}

// ============ Lifecycle Tests ============

void test_init_creates_buffer(void)
{
    bond_buffer buf;
    int result = bond_buffer_init(&buf, 64);
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_NOT_NULL(buf.data);
    TEST_ASSERT_EQUAL(0, buf.size);
    TEST_ASSERT_EQUAL(64, buf.capacity);
    TEST_ASSERT_EQUAL(0, buf.read_pos);
    TEST_ASSERT_TRUE(buf.owns_memory);
    
    bond_buffer_destroy(&buf);
}

void test_init_from_wraps_external(void)
{
    uint8_t external[] = {1, 2, 3, 4, 5};
    bond_buffer buf;
    
    bond_buffer_init_from(&buf, external, 5);
    
    TEST_ASSERT_EQUAL_PTR(external, buf.data);
    TEST_ASSERT_EQUAL(5, buf.size);
    TEST_ASSERT_EQUAL(5, buf.capacity);
    TEST_ASSERT_EQUAL(0, buf.read_pos);
    TEST_ASSERT_FALSE(buf.owns_memory);
    
    bond_buffer_destroy(&buf);  // Should NOT free external
}

void test_destroy_clears_fields(void)
{
    bond_buffer buf;
    bond_buffer_init(&buf, 32);
    bond_buffer_destroy(&buf);
    
    TEST_ASSERT_NULL(buf.data);
    TEST_ASSERT_EQUAL(0, buf.size);
    TEST_ASSERT_EQUAL(0, buf.capacity);
    TEST_ASSERT_FALSE(buf.owns_memory);
}

// ============ Writing Tests ============

void test_write_byte(void)
{
    bond_buffer buf;
    bond_buffer_init(&buf, 8);
    
    TEST_ASSERT_EQUAL(0, bond_buffer_write_byte(&buf, 0xAB));
    TEST_ASSERT_EQUAL(1, buf.size);
    TEST_ASSERT_EQUAL_HEX8(0xAB, buf.data[0]);
    
    bond_buffer_destroy(&buf);
}

void test_write_multiple_bytes(void)
{
    bond_buffer buf;
    bond_buffer_init(&buf, 8);
    
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    TEST_ASSERT_EQUAL(0, bond_buffer_write(&buf, data, 4));
    TEST_ASSERT_EQUAL(4, buf.size);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(data, buf.data, 4);
    
    bond_buffer_destroy(&buf);
}

void test_write_grows_buffer(void)
{
    bond_buffer buf;
    bond_buffer_init(&buf, 4);  // Start small
    
    // Write more than capacity
    uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    TEST_ASSERT_EQUAL(0, bond_buffer_write(&buf, data, 8));
    
    TEST_ASSERT_EQUAL(8, buf.size);
    TEST_ASSERT_TRUE(buf.capacity >= 8);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(data, buf.data, 8);
    
    bond_buffer_destroy(&buf);
}

void test_reserve_grows_capacity(void)
{
    bond_buffer buf;
    bond_buffer_init(&buf, 4);
    
    TEST_ASSERT_EQUAL(0, bond_buffer_reserve(&buf, 100));
    TEST_ASSERT_TRUE(buf.capacity >= 100);
    TEST_ASSERT_EQUAL(0, buf.size);  // Size unchanged
    
    bond_buffer_destroy(&buf);
}

// ============ Reading Tests ============

void test_read_byte(void)
{
    uint8_t data[] = {0xAA, 0xBB, 0xCC};
    bond_buffer buf;
    bond_buffer_init_from(&buf, data, 3);
    
    TEST_ASSERT_EQUAL(0xAA, bond_buffer_read_byte(&buf));
    TEST_ASSERT_EQUAL(0xBB, bond_buffer_read_byte(&buf));
    TEST_ASSERT_EQUAL(0xCC, bond_buffer_read_byte(&buf));
    TEST_ASSERT_EQUAL(-1, bond_buffer_read_byte(&buf));  // EOF
    
    bond_buffer_destroy(&buf);
}

void test_read_multiple(void)
{
    uint8_t data[] = {1, 2, 3, 4, 5};
    bond_buffer buf;
    bond_buffer_init_from(&buf, data, 5);
    
    uint8_t dest[3];
    size_t read = bond_buffer_read(&buf, dest, 3);
    
    TEST_ASSERT_EQUAL(3, read);
    TEST_ASSERT_EQUAL(3, buf.read_pos);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(data, dest, 3);
    
    bond_buffer_destroy(&buf);
}

void test_read_partial_at_eof(void)
{
    uint8_t data[] = {1, 2, 3};
    bond_buffer buf;
    bond_buffer_init_from(&buf, data, 3);
    
    uint8_t dest[10];
    size_t read = bond_buffer_read(&buf, dest, 10);  // Ask for more than available
    
    TEST_ASSERT_EQUAL(3, read);  // Only got 3
    TEST_ASSERT_EQUAL(3, buf.read_pos);
    
    bond_buffer_destroy(&buf);
}

void test_remaining(void)
{
    uint8_t data[] = {1, 2, 3, 4, 5};
    bond_buffer buf;
    bond_buffer_init_from(&buf, data, 5);
    
    TEST_ASSERT_EQUAL(5, bond_buffer_remaining(&buf));
    bond_buffer_read_byte(&buf);
    TEST_ASSERT_EQUAL(4, bond_buffer_remaining(&buf));
    bond_buffer_read_byte(&buf);
    bond_buffer_read_byte(&buf);
    TEST_ASSERT_EQUAL(2, bond_buffer_remaining(&buf));
    
    bond_buffer_destroy(&buf);
}

void test_peek_does_not_advance(void)
{
    uint8_t data[] = {0xAA, 0xBB, 0xCC};
    bond_buffer buf;
    bond_buffer_init_from(&buf, data, 3);
    
    uint8_t dest[2];
    size_t peeked = bond_buffer_peek(&buf, dest, 2);
    
    TEST_ASSERT_EQUAL(2, peeked);
    TEST_ASSERT_EQUAL(0, buf.read_pos);  // Not advanced!
    TEST_ASSERT_EQUAL_HEX8(0xAA, dest[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, dest[1]);
    
    bond_buffer_destroy(&buf);
}

// ============ Utility Tests ============

void test_clear_resets_size(void)
{
    bond_buffer buf;
    bond_buffer_init(&buf, 32);
    bond_buffer_write_byte(&buf, 1);
    bond_buffer_write_byte(&buf, 2);
    
    size_t old_capacity = buf.capacity;
    bond_buffer_clear(&buf);
    
    TEST_ASSERT_EQUAL(0, buf.size);
    TEST_ASSERT_EQUAL(0, buf.read_pos);
    TEST_ASSERT_EQUAL(old_capacity, buf.capacity);  // Capacity preserved
    
    bond_buffer_destroy(&buf);
}

void test_rewind_resets_read_pos(void)
{
    uint8_t data[] = {1, 2, 3, 4, 5};
    bond_buffer buf;
    bond_buffer_init_from(&buf, data, 5);
    
    bond_buffer_read_byte(&buf);
    bond_buffer_read_byte(&buf);
    TEST_ASSERT_EQUAL(2, buf.read_pos);
    
    bond_buffer_rewind(&buf);
    TEST_ASSERT_EQUAL(0, buf.read_pos);
    TEST_ASSERT_EQUAL(1, bond_buffer_read_byte(&buf));  // Can read from start again
    
    bond_buffer_destroy(&buf);
}

// ============ Roundtrip Test ============

void test_write_then_read(void)
{
    bond_buffer buf;
    bond_buffer_init(&buf, 16);
    
    // Write some bytes
    bond_buffer_write_byte(&buf, 0x11);
    bond_buffer_write_byte(&buf, 0x22);
    uint8_t chunk[] = {0x33, 0x44, 0x55};
    bond_buffer_write(&buf, chunk, 3);
    
    TEST_ASSERT_EQUAL(5, buf.size);
    
    // Rewind and read back
    bond_buffer_rewind(&buf);
    TEST_ASSERT_EQUAL(0x11, bond_buffer_read_byte(&buf));
    TEST_ASSERT_EQUAL(0x22, bond_buffer_read_byte(&buf));
    TEST_ASSERT_EQUAL(0x33, bond_buffer_read_byte(&buf));
    TEST_ASSERT_EQUAL(0x44, bond_buffer_read_byte(&buf));
    TEST_ASSERT_EQUAL(0x55, bond_buffer_read_byte(&buf));
    TEST_ASSERT_EQUAL(-1, bond_buffer_read_byte(&buf));  // EOF
    
    bond_buffer_destroy(&buf);
}

// ============ Test Runner ============

int main(void)
{
    UNITY_BEGIN();
    
    // Lifecycle
    RUN_TEST(test_init_creates_buffer);
    RUN_TEST(test_init_from_wraps_external);
    RUN_TEST(test_destroy_clears_fields);
    
    // Writing
    RUN_TEST(test_write_byte);
    RUN_TEST(test_write_multiple_bytes);
    RUN_TEST(test_write_grows_buffer);
    RUN_TEST(test_reserve_grows_capacity);
    
    // Reading
    RUN_TEST(test_read_byte);
    RUN_TEST(test_read_multiple);
    RUN_TEST(test_read_partial_at_eof);
    RUN_TEST(test_remaining);
    RUN_TEST(test_peek_does_not_advance);
    
    // Utility
    RUN_TEST(test_clear_resets_size);
    RUN_TEST(test_rewind_resets_read_pos);
    
    // Roundtrip
    RUN_TEST(test_write_then_read);
    
    return UNITY_END();
}
