/**
 * @file bond_reader.c
 * @brief CompactBinary v1 reader implementation
 */

#include "bond_reader.h"
#include "bond_encoding.h"
#include <string.h>

// ============================================================================
// Initialization
// ============================================================================

void bond_reader_init(BondReader *reader, bond_buffer *buffer)
{
    reader->buffer = buffer;
}

// ============================================================================
// Struct Control (no-ops for v1)
// ============================================================================

void bond_reader_struct_begin(BondReader *reader)
{
    (void)reader;
}

void bond_reader_struct_end(BondReader *reader)
{
    (void)reader;
}

// ============================================================================
// Field Header Reading
// ============================================================================

bool bond_reader_read_field_header(BondReader *reader, uint16_t *field_id, uint8_t *type)
{
    int first_byte = bond_buffer_read_byte(reader->buffer);
    if (first_byte == -1) {
        return false;
    }
    
    uint8_t id_hint = (uint8_t)first_byte >> 5;
    *type = (uint8_t)first_byte & 0x1F;
    
    if (id_hint < 6) {
        *field_id = id_hint;
        return true;
    }
    else if (id_hint == 6) {
        int next_byte = bond_buffer_read_byte(reader->buffer);
        if (next_byte == -1) {
            return false;
        }
        *field_id = (uint16_t)next_byte;
        return true;
    }
    else {  // id_hint == 7
        int byte1 = bond_buffer_read_byte(reader->buffer);
        if (byte1 == -1) {
            return false;
        }
        int byte2 = bond_buffer_read_byte(reader->buffer);
        if (byte2 == -1) {
            return false;
        }
        *field_id = (uint16_t)(byte1 | (byte2 << 8));
        return true;
    }
}

// ============================================================================
// Internal Helpers
// ============================================================================

// Read varint bytes into buffer. Returns bytes read, or 0 on error.
static size_t read_varint_bytes(BondReader *reader, uint8_t *buf, size_t max_bytes)
{
    int byte = bond_buffer_read_byte(reader->buffer);
    if (byte == -1)
    {
        return 0;
    }
    buf[0] = (uint8_t)byte;
    size_t len = 1;

    while ((buf[len - 1] & 0x80) != 0 && len < max_bytes)
    {
        int next_byte = bond_buffer_read_byte(reader->buffer);
        if (next_byte == -1)
        {
            return 0;
        }
        buf[len++] = (uint8_t)next_byte;
    }
    return len;
}

// ============================================================================
// Primitive Value Readers
// ============================================================================

bool bond_reader_read_bool_value(BondReader *reader, bool *value)
{
    int byte = bond_buffer_read_byte(reader->buffer);
    if (byte == -1) {
        return false;
    }
    *value = (byte != 0);
    return true;
}

bool bond_reader_read_uint8_value(BondReader *reader, uint8_t *value)
{
    int byte = bond_buffer_read_byte(reader->buffer);
    if (byte == -1) {
        return false;
    }
    *value = (uint8_t)byte;
    return true;
}

bool bond_reader_read_int8_value(BondReader *reader, int8_t *value)
{
    int byte = bond_buffer_read_byte(reader->buffer);
    if (byte == -1) {
        return false;
    }
    *value = (int8_t)byte;
    return true;
}

bool bond_reader_read_uint16_value(BondReader *reader, uint16_t *value)
{
    uint8_t buf[3];
    size_t len = read_varint_bytes(reader, buf, 3);
    if (len == 0)
    {
        return false;
    }
    size_t consumed = bond_decode_varint16(buf, value);
    return (consumed != 0 && consumed == len);
}

bool bond_reader_read_uint32_value(BondReader *reader, uint32_t *value)
{
    uint8_t buf[5];
    size_t len = read_varint_bytes(reader, buf, 5);
    if (len == 0)
    {
        return false;
    }
    size_t consumed = bond_decode_varint32(buf, value);
    return (consumed != 0 && consumed == len);
}

bool bond_reader_read_uint64_value(BondReader *reader, uint64_t *value)
{
    uint8_t buf[10];
    size_t len = read_varint_bytes(reader, buf, 10);
    if (len == 0)
    {
        return false;
    }
    size_t consumed = bond_decode_varint64(buf, value);
    return (consumed != 0 && consumed == len);
}

bool bond_reader_read_int16_value(BondReader *reader, int16_t *value)
{
    uint16_t uvalue;
    if (!bond_reader_read_uint16_value(reader, &uvalue))
    {
        return false;
    }
    *value = bond_zigzag_decode16(uvalue);
    return true;
}

bool bond_reader_read_int32_value(BondReader *reader, int32_t *value)
{
    uint32_t uvalue;
    if (!bond_reader_read_uint32_value(reader, &uvalue))
    {
        return false;
    }
    *value = bond_zigzag_decode32(uvalue);
    return true;
}

bool bond_reader_read_int64_value(BondReader *reader, int64_t *value)
{
    uint64_t uvalue;
    if (!bond_reader_read_uint64_value(reader, &uvalue))
    {
        return false;
    }
    *value = bond_zigzag_decode64(uvalue);
    return true;
}

bool bond_reader_read_float_value(BondReader *reader, float *value)
{
    uint8_t buf[4];
    size_t bytes_read = bond_buffer_read(reader->buffer, buf, 4);
    if (bytes_read != 4)
    {
        return false;
    }
    *value = bond_decode_float(buf);
    return true;
}

bool bond_reader_read_double_value(BondReader *reader, double *value)
{
    uint8_t buf[8];
    size_t bytes_read = bond_buffer_read(reader->buffer, buf, 8);
    if (bytes_read != 8)
    {
        return false;
    }
    *value = bond_decode_double(buf);
    return true;
}

bool bond_reader_read_string_value(BondReader *reader, const char **str, uint32_t *len)
{
    uint32_t str_len;
    if (!bond_reader_read_uint32_value(reader, &str_len))
    {
        return false;
    }
    if (bond_buffer_remaining(reader->buffer) < str_len)
    {
        return false;
    }
    *str = (const char *)(reader->buffer->data + reader->buffer->read_pos);
    reader->buffer->read_pos += str_len;
    *len = str_len;
    return true;
}

// ============================================================================
// Container Header Readers
// ============================================================================

bool bond_reader_read_list_begin(BondReader *reader, uint8_t *element_type, uint32_t *count)
{
    int byte = bond_buffer_read_byte(reader->buffer);
    if (byte == -1)
    {   
        return false;
    }
    *element_type = (uint8_t)byte;
    if (!bond_reader_read_uint32_value(reader, count))
    {
        return false;
    }
    return true;
}

bool bond_reader_read_set_begin(BondReader *reader, uint8_t *element_type, uint32_t *count)
{
    // Same wire format as list
    return bond_reader_read_list_begin(reader, element_type, count);
}

bool bond_reader_read_map_begin(BondReader *reader, uint8_t *key_type, uint8_t *value_type, uint32_t *count)
{
    int byte = bond_buffer_read_byte(reader->buffer);
    if (byte == -1)
    {
        return false;
    }
    *key_type = (uint8_t)byte;

    byte = bond_buffer_read_byte(reader->buffer);
    if (byte == -1)
    {
        return false;
    }
    *value_type = (uint8_t)byte;

    if (!bond_reader_read_uint32_value(reader, count))
    {
        return false;
    }
    return true;
}

// ============================================================================
// Skip Functions
// ============================================================================

// Helper to skip a varint (just read and discard)
static bool skip_varint(BondReader *reader)
{
    int byte;
    do
    {
        byte = bond_buffer_read_byte(reader->buffer);
        if (byte == -1)
        {
            return false;
        }
    } while ((byte & 0x80) != 0);
    return true;
}

bool bond_reader_skip(BondReader *reader, uint8_t type)
{
    switch (type)
    {
        case BOND_TYPE_BOOL:
        case BOND_TYPE_UINT8:
        case BOND_TYPE_INT8:
            // Skip 1 byte
            return bond_buffer_read_byte(reader->buffer) != -1;

        case BOND_TYPE_UINT16:
        case BOND_TYPE_UINT32:
        case BOND_TYPE_UINT64:
        case BOND_TYPE_INT16:
        case BOND_TYPE_INT32:
        case BOND_TYPE_INT64:
            // Skip varint
            return skip_varint(reader);

        case BOND_TYPE_FLOAT:
            // Skip 4 bytes
            return bond_buffer_remaining(reader->buffer) >= 4 &&
                   (reader->buffer->read_pos += 4, true);

        case BOND_TYPE_DOUBLE:
            // Skip 8 bytes
            return bond_buffer_remaining(reader->buffer) >= 8 &&
                   (reader->buffer->read_pos += 8, true);

        case BOND_TYPE_STRING:
        case BOND_TYPE_WSTRING:
        {
            // Read length, skip that many bytes
            uint32_t len;
            if (!bond_reader_read_uint32_value(reader, &len))
            {
                return false;
            }
            if (bond_buffer_remaining(reader->buffer) < len)
            {
                return false;
            }
            reader->buffer->read_pos += len;
            return true;
        }

        case BOND_TYPE_STRUCT:
        {
            // Read fields until STOP, skip each
            uint16_t field_id;
            uint8_t field_type;
            while (true)
            {
                if (!bond_reader_read_field_header(reader, &field_id, &field_type))
                {
                    return false;
                }
                if (field_type == BOND_TYPE_STOP || field_type == BOND_TYPE_STOP_BASE)
                {
                    return true;
                }
                if (!bond_reader_skip(reader, field_type))
                {
                    return false;
                }
            }
        }

        case BOND_TYPE_LIST:
        case BOND_TYPE_SET:
        {
            // Read header, skip count elements
            uint8_t element_type;
            uint32_t count;
            if (!bond_reader_read_list_begin(reader, &element_type, &count))
            {
                return false;
            }
            for (uint32_t i = 0; i < count; i++)
            {
                if (!bond_reader_skip(reader, element_type))
                {
                    return false;
                }
            }
            return true;
        }

        case BOND_TYPE_MAP:
        {
            // Read header, skip count key-value pairs
            uint8_t key_type, value_type;
            uint32_t count;
            if (!bond_reader_read_map_begin(reader, &key_type, &value_type, &count))
            {
                return false;
            }
            for (uint32_t i = 0; i < count; i++)
            {
                if (!bond_reader_skip(reader, key_type))
                {
                    return false;
                }
                if (!bond_reader_skip(reader, value_type))
                {
                    return false;
                }
            }
            return true;
        }

        default:
            // Unknown type
            return false;
    }
}
