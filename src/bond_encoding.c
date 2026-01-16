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
