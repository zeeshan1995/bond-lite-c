#include <stdint.h>

// Write a varint-encoded uint32 to a byte array
// Returns: number of bytes written
int bond_encode_varint32(uint8_t *output, uint32_t value)
{
    int count = 0;
    while (value > 0x7F)
    {
        // get the lower 7 bits and set MSB and casting to uint8_t discards higher bits
        output[count++] = (uint8_t)(value | 0x80);
        value >>= 7;
    }
    output[count++] = (uint8_t)value;
    return count;
}

// Read a varint-encoded uint32 from a byte array
// Returns: number of bytes read
int bond_decode_varint32(const uint8_t *input, uint32_t *value)
{
    int i_byte = 0;
    uint32_t result = 0;
    int shift = 0;
    while(shift <= 28)
    {
        uint8_t bytes = input[i_byte++];
        // get the lower 7 bits and shift them into place
        result |= (bytes & 0x7F) << shift;
        if ((bytes & 0x80) == 0)
        {
            *value = result;
            return i_byte;  
        }
        shift += 7;
    }
    return 0; // Error: varint32 too long
}

// ZigZag encode: signed → unsigned
// Maps small negative numbers to small positive numbers
uint32_t bond_zigzag_encode32(int32_t value)
{
    /*Algo :
        ZigZag encoding maps signed integers to unsigned integers
        so that numbers with a small absolute value (for instance, -1)
        have a small varint encoded value too.
       
        Intuitively, it works by mapping negative values to odd numbers
        and positive values to even numbers.
        Multiply the positive values by 2. make every positive value to even. 
        and hence the odd are reserved for negative values. multiply the negative values by 2 and subtract 1 to make them odd.

        for bit manipulation:
        (value << 1) : multiply by 2
        (value >> 31) : if value is negative, this results in all bits being 1 (i.e., -1 in two's complement), otherwise all bits are 0.
    */

    return (value << 1) ^ (value >> 31);
}

// ZigZag decode: unsigned → signed
int32_t bond_zigzag_decode32(uint32_t value)
{
    return value >> 1 ^ -(int32_t)(value & 1);
}

// ============ Varint16 ============

int bond_encode_varint16(uint8_t *output, uint16_t value)
{
    int count = 0;
    while (value > 0x7F)
    {
        output[count++] = (uint8_t)(value | 0x80);
        value >>= 7;
    }
    output[count++] = (uint8_t)value;
    return count;
}

int bond_decode_varint16(const uint8_t *input, uint16_t *value)
{
    int i_byte = 0;
    uint16_t result = 0;
    int shift = 0;
    while(shift <= 14)
    {
        uint8_t bytes = input[i_byte++];
        result |= (bytes & 0x7F) << shift;
        if ((bytes & 0x80) == 0)
        {
            *value = result;
            return i_byte;
        }
        shift += 7;
    }
    return 0; // Error: varint16 too long
}

// ============ Varint64 ============

int bond_encode_varint64(uint8_t *output, uint64_t value)
{
    int count = 0;
    while (value > 0x7F)
    {
        output[count++] = (uint8_t)(value | 0x80);
        value >>= 7;
    }
    output[count++] = (uint8_t)value;
    return count;
}

int bond_decode_varint64(const uint8_t *input, uint64_t *value)
{
    int i_byte = 0;
    uint64_t result = 0;
    int shift = 0;
    while(shift <= 63)
    {
        uint8_t bytes = input[i_byte++];
        result |= (uint64_t)(bytes & 0x7F) << shift;
        if ((bytes & 0x80) == 0)
        {
            *value = result;
            return i_byte;
        }
        shift += 7;
    }
    return 0; // Error: varint64 too long
}
