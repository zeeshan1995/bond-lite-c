#include <unity.h>
#include <stdint.h>
#include <string.h>

// Declare functions
int bond_encode_varint32(uint8_t *output, uint32_t value);
int bond_decode_varint32(const uint8_t *input, uint32_t *value);

void setUp(void) {}
void tearDown(void) {}

// ============ Encode Tests ============

void test_encode_zero(void)
{
    uint8_t output[8];
    int len = bond_encode_varint32(output, 0);
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL_HEX8(0x00, output[0]);
}

void test_encode_one(void)
{
    uint8_t output[8];
    int len = bond_encode_varint32(output, 1);
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL_HEX8(0x01, output[0]);
}

void test_encode_127(void)
{
    uint8_t output[8];
    int len = bond_encode_varint32(output, 127);
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL_HEX8(0x7F, output[0]);
}

void test_encode_128(void)
{
    uint8_t output[8];
    uint8_t expected[] = {0x80, 0x01};
    int len = bond_encode_varint32(output, 128);
    TEST_ASSERT_EQUAL(2, len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, output, 2);
}

void test_encode_300(void)
{
    uint8_t output[8];
    uint8_t expected[] = {0xAC, 0x02};
    int len = bond_encode_varint32(output, 300);
    TEST_ASSERT_EQUAL(2, len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, output, 2);
}

void test_encode_16383(void)
{
    uint8_t output[8];
    uint8_t expected[] = {0xFF, 0x7F};
    int len = bond_encode_varint32(output, 16383);
    TEST_ASSERT_EQUAL(2, len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, output, 2);
}

void test_encode_16384(void)
{
    uint8_t output[8];
    uint8_t expected[] = {0x80, 0x80, 0x01};
    int len = bond_encode_varint32(output, 16384);
    TEST_ASSERT_EQUAL(3, len);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, output, 3);
}

// ============ Decode Roundtrip Tests ============

void test_decode_roundtrip(void)
{
    uint32_t test_values[] = {0, 1, 127, 128, 300, 16383, 16384, 65535, 1000000};
    
    for (int i = 0; i < 9; i++) {
        uint8_t buf[8];
        uint32_t decoded;
        int enc_len = bond_encode_varint32(buf, test_values[i]);
        int dec_len = bond_decode_varint32(buf, &decoded);
        
        TEST_ASSERT_EQUAL(test_values[i], decoded);
        TEST_ASSERT_EQUAL(enc_len, dec_len);
    }
}

// ============ Test Runner ============

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_encode_zero);
    RUN_TEST(test_encode_one);
    RUN_TEST(test_encode_127);
    RUN_TEST(test_encode_128);
    RUN_TEST(test_encode_300);
    RUN_TEST(test_encode_16383);
    RUN_TEST(test_encode_16384);
    RUN_TEST(test_decode_roundtrip);
    
    return UNITY_END();
}
