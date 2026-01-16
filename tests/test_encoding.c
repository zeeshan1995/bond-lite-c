#include <unity.h>
#include <stdint.h>
#include <string.h>

// Declare functions
int bond_encode_varint16(uint8_t *output, uint16_t value);
int bond_decode_varint16(const uint8_t *input, uint16_t *value);
int bond_encode_varint32(uint8_t *output, uint32_t value);
int bond_decode_varint32(const uint8_t *input, uint32_t *value);
int bond_encode_varint64(uint8_t *output, uint64_t value);
int bond_decode_varint64(const uint8_t *input, uint64_t *value);
uint16_t bond_zigzag_encode16(int16_t value);
int16_t bond_zigzag_decode16(uint16_t value);
uint32_t bond_zigzag_encode32(int32_t value);
int32_t bond_zigzag_decode32(uint32_t value);
uint64_t bond_zigzag_encode64(int64_t value);
int64_t bond_zigzag_decode64(uint64_t value);

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

// ============ ZigZag Tests ============

void test_zigzag_encode_values(void)
{
    TEST_ASSERT_EQUAL_UINT32(0, bond_zigzag_encode32(0));
    TEST_ASSERT_EQUAL_UINT32(1, bond_zigzag_encode32(-1));
    TEST_ASSERT_EQUAL_UINT32(2, bond_zigzag_encode32(1));
    TEST_ASSERT_EQUAL_UINT32(3, bond_zigzag_encode32(-2));
    TEST_ASSERT_EQUAL_UINT32(4, bond_zigzag_encode32(2));
    TEST_ASSERT_EQUAL_UINT32(254, bond_zigzag_encode32(127));
    TEST_ASSERT_EQUAL_UINT32(255, bond_zigzag_encode32(-128));
}

void test_zigzag_decode_values(void)
{
    TEST_ASSERT_EQUAL_INT32(0, bond_zigzag_decode32(0));
    TEST_ASSERT_EQUAL_INT32(-1, bond_zigzag_decode32(1));
    TEST_ASSERT_EQUAL_INT32(1, bond_zigzag_decode32(2));
    TEST_ASSERT_EQUAL_INT32(-2, bond_zigzag_decode32(3));
    TEST_ASSERT_EQUAL_INT32(2, bond_zigzag_decode32(4));
    TEST_ASSERT_EQUAL_INT32(127, bond_zigzag_decode32(254));
    TEST_ASSERT_EQUAL_INT32(-128, bond_zigzag_decode32(255));
}

void test_zigzag_roundtrip(void)
{
    int32_t test_values[] = {0, 1, -1, 127, -128, 32767, -32768, 2147483647, -2147483648};
    
    for (int i = 0; i < 9; i++) {
        uint32_t encoded = bond_zigzag_encode32(test_values[i]);
        int32_t decoded = bond_zigzag_decode32(encoded);
        TEST_ASSERT_EQUAL_INT32(test_values[i], decoded);
    }
}

// ============ Varint16 Tests ============

void test_varint16_roundtrip(void)
{
    uint16_t test_values[] = {0, 1, 127, 128, 255, 256, 16383, 16384, 65535};
    
    for (int i = 0; i < 9; i++) {
        uint8_t buf[4];
        uint16_t decoded;
        int enc_len = bond_encode_varint16(buf, test_values[i]);
        int dec_len = bond_decode_varint16(buf, &decoded);
        
        TEST_ASSERT_EQUAL(test_values[i], decoded);
        TEST_ASSERT_EQUAL(enc_len, dec_len);
    }
}

// ============ Varint64 Tests ============

void test_varint64_roundtrip(void)
{
    uint64_t test_values[] = {0, 1, 127, 128, 0xFFFFFFFF, 0x100000000ULL, 0xFFFFFFFFFFFFFFFFULL};
    
    for (int i = 0; i < 7; i++) {
        uint8_t buf[16];
        uint64_t decoded;
        int enc_len = bond_encode_varint64(buf, test_values[i]);
        int dec_len = bond_decode_varint64(buf, &decoded);
        
        TEST_ASSERT_EQUAL_UINT64(test_values[i], decoded);
        TEST_ASSERT_EQUAL(enc_len, dec_len);
    }
}

// ============ ZigZag16 Tests ============

void test_zigzag16_roundtrip(void)
{
    int16_t test_values[] = {0, 1, -1, 127, -128, 32767, -32768};
    
    for (int i = 0; i < 7; i++) {
        uint16_t encoded = bond_zigzag_encode16(test_values[i]);
        int16_t decoded = bond_zigzag_decode16(encoded);
        TEST_ASSERT_EQUAL_INT16(test_values[i], decoded);
    }
}

// ============ ZigZag64 Tests ============

void test_zigzag64_roundtrip(void)
{
    int64_t test_values[] = {0, 1, -1, 127, -128, 2147483647LL, -2147483648LL, 9223372036854775807LL};
    
    for (int i = 0; i < 8; i++) {
        uint64_t encoded = bond_zigzag_encode64(test_values[i]);
        int64_t decoded = bond_zigzag_decode64(encoded);
        TEST_ASSERT_EQUAL_INT64(test_values[i], decoded);
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
    RUN_TEST(test_zigzag_encode_values);
    RUN_TEST(test_zigzag_decode_values);
    RUN_TEST(test_zigzag_roundtrip);
    RUN_TEST(test_varint16_roundtrip);
    RUN_TEST(test_varint64_roundtrip);
    RUN_TEST(test_zigzag16_roundtrip);
    RUN_TEST(test_zigzag64_roundtrip);
    
    return UNITY_END();
}
