/**
 * @file bond_writer.c
 * @brief Bond CompactBinary v1 Writer Implementation
 */

#include "bond_writer.h"
#include "bond_encoding.h"
#include <string.h>

// ============================================================================
// Lifecycle
// ============================================================================

void bond_writer_init(bond_writer *writer, bond_buffer *buffer)
{
    writer->buffer = buffer;
}

// ============================================================================
// Struct Control
// ============================================================================

void bond_writer_struct_begin(bond_writer *writer) 
{
    // TODO: Implement (no-op in v1)
}

void bond_writer_struct_end(bond_writer *writer) 
{
    bond_buffer_write_byte(writer->buffer, BOND_TYPE_STOP);
}

// ============================================================================
// Field Header
// ============================================================================

/**
 * Write field header using v1 absolute field ID encoding.
 * 
 * The top 3 bits encode the field ID, but values 6 and 7 are ESCAPE CODES:
 *   0-5 = Field ID fits directly (1 byte total)
 *   6   = Escape: actual ID (6-255) follows in next 1 byte (2 bytes total)
 *   7   = Escape: actual ID (256-65535) follows in next 2 bytes LE (3 bytes total)
 * 
 * Format:
 *   id 0-5:   [id:3][type:5]
 *   id 6-255: [6:3][type:5][id:8]         (0xC0 | type, then id)
 *   id 256+:  [7:3][type:5][id_lo][id_hi] (0xE0 | type, then id LE)
 * 
 * NOTE: Headers use RAW BYTES, not varint. This allows fast parsing:
 * read 1 byte, check top 3 bits, instantly know how many more to read.
 * Varint is only used for VALUES (uint16/32/64, int16/32/64, lengths, counts).
 */
void bond_writer_write_field_header(bond_writer *writer, uint16_t field_id, BondDataType type) {
    if(field_id <= 5)
    {
        // Field ID fits in top 3 bits
        uint8_t header = (uint8_t)(type | (field_id << 5));
        bond_buffer_write_byte(writer->buffer, header);
    }
    else if(field_id <= 0xFF)
    {
        // Escape code 6: 1-byte ID follows
        uint8_t header = (uint8_t)(type | (6 << 5));  // 0xC0 | type
        bond_buffer_write_byte(writer->buffer, header);
        bond_buffer_write_byte(writer->buffer, (uint8_t)field_id);
    }
    else
    {
        // Escape code 7: 2-byte ID follows (little-endian)
        uint8_t header = (uint8_t)(type | (7 << 5));  // 0xE0 | type
        bond_buffer_write_byte(writer->buffer, header);
        bond_buffer_write_byte(writer->buffer, (uint8_t)(field_id & 0xFF));
        bond_buffer_write_byte(writer->buffer, (uint8_t)(field_id >> 8));
    }
}

// ============================================================================
// Primitive Writers (with field header)
// ============================================================================

void bond_writer_write_bool(bond_writer *writer, uint16_t field_id, bool value)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_BOOL);
    bond_writer_write_bool_value(writer, value);
}

void bond_writer_write_uint8(bond_writer *writer, uint16_t field_id, uint8_t value)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_UINT8);
    bond_writer_write_uint8_value(writer, value);
}

void bond_writer_write_uint16(bond_writer *writer, uint16_t field_id, uint16_t value)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_UINT16);
    bond_writer_write_uint16_value(writer, value);
}

void bond_writer_write_uint32(bond_writer *writer, uint16_t field_id, uint32_t value)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_UINT32);
    bond_writer_write_uint32_value(writer, value);
}

void bond_writer_write_uint64(bond_writer *writer, uint16_t field_id, uint64_t value)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_UINT64);
    bond_writer_write_uint64_value(writer, value);
}

void bond_writer_write_int8(bond_writer *writer, uint16_t field_id, int8_t value)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_INT8);
    bond_writer_write_int8_value(writer, value);
}

void bond_writer_write_int16(bond_writer *writer, uint16_t field_id, int16_t value)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_INT16);
    bond_writer_write_int16_value(writer, value);
}

void bond_writer_write_int32(bond_writer *writer, uint16_t field_id, int32_t value)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_INT32);
    bond_writer_write_int32_value(writer, value);
}

void bond_writer_write_int64(bond_writer *writer, uint16_t field_id, int64_t value)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_INT64);
    bond_writer_write_int64_value(writer, value);
}

void bond_writer_write_float(bond_writer *writer, uint16_t field_id, float value)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_FLOAT);
    bond_writer_write_float_value(writer, value);
}

void bond_writer_write_double(bond_writer *writer, uint16_t field_id, double value)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_DOUBLE);
    bond_writer_write_double_value(writer, value);
}

void bond_writer_write_string(bond_writer *writer, uint16_t field_id, const char *value) 
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_STRING);
    bond_writer_write_string_value(writer, value);
}

// ============================================================================
// Container Writers
// ============================================================================

/**
 * Write list container header.
 * 
 * Wire format (v1):
 *   [field_header]     - field ID + BOND_TYPE_LIST (see write_field_header)
 *   [element_type: 1]  - raw byte, BondDataType of elements
 *   [count: varint32]  - number of elements to follow
 * 
 * After calling this, write 'count' elements using the appropriate _value writer.
 * No end marker needed - reader knows count upfront.
 */
void bond_writer_write_list_begin(bond_writer *writer, uint16_t field_id,
                                  BondDataType element_type, uint32_t count)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_LIST);
    bond_buffer_write_byte(writer->buffer, (uint8_t)element_type);
    bond_writer_write_uint32_value(writer, count);
}

/**
 * Write set container header.
 * 
 * Wire format identical to list, just different type in field header:
 *   [field_header]     - field ID + BOND_TYPE_SET
 *   [element_type: 1]  - raw byte, BondDataType of elements
 *   [count: varint32]  - number of elements to follow
 */
void bond_writer_write_set_begin(bond_writer *writer, uint16_t field_id,
                                 BondDataType element_type, uint32_t count)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_SET);
    bond_buffer_write_byte(writer->buffer, (uint8_t)element_type);
    bond_writer_write_uint32_value(writer, count);
}

/**
 * Write map container header.
 * 
 * Wire format (v1):
 *   [field_header]      - field ID + BOND_TYPE_MAP
 *   [key_type: 1]       - raw byte, BondDataType of keys
 *   [value_type: 1]     - raw byte, BondDataType of values
 *   [count: varint32]   - number of key-value pairs to follow
 * 
 * After calling this, write 'count' pairs: key, value, key, value, ...
 * Each key/value written using the appropriate _value writer.
 */
void bond_writer_write_map_begin(bond_writer *writer, uint16_t field_id,
                                 BondDataType key_type, BondDataType value_type,
                                 uint32_t count)
{
    bond_writer_write_field_header(writer, field_id, BOND_TYPE_MAP);
    bond_buffer_write_byte(writer->buffer, (uint8_t)key_type);
    bond_buffer_write_byte(writer->buffer, (uint8_t)value_type);
    bond_writer_write_uint32_value(writer, count);
}

// ============================================================================
// Raw Value Writers (no field header)
// ============================================================================

void bond_writer_write_bool_value(bond_writer *writer, bool value)
{
    bond_buffer_write_byte(writer->buffer, value ? 1 : 0);
}

void bond_writer_write_uint8_value(bond_writer *writer, uint8_t value)
{
    bond_buffer_write_byte(writer->buffer, value);
}

void bond_writer_write_uint16_value(bond_writer *writer, uint16_t value)
{
    uint8_t buf[3];  // max 3 bytes for varint16
    size_t len = bond_encode_varint16(buf, value);
    bond_buffer_write(writer->buffer, buf, len);
}

void bond_writer_write_uint32_value(bond_writer *writer, uint32_t value)
{
    uint8_t buf[5];  // max 5 bytes for varint32
    size_t len = bond_encode_varint32(buf, value);
    bond_buffer_write(writer->buffer, buf, len);
}

void bond_writer_write_uint64_value(bond_writer *writer, uint64_t value)
{
    uint8_t buf[10];  // max 10 bytes for varint64
    size_t len = bond_encode_varint64(buf, value);
    bond_buffer_write(writer->buffer, buf, len);
}

void bond_writer_write_int8_value(bond_writer *writer, int8_t value)
{
    bond_buffer_write_byte(writer->buffer, (uint8_t)value);
}

void bond_writer_write_int16_value(bond_writer *writer, int16_t value)
{
    uint8_t buf[3];  // max 3 bytes for varint16
    size_t len = bond_encode_varint16(buf, bond_zigzag_encode16(value));
    bond_buffer_write(writer->buffer, buf, len);
}

void bond_writer_write_int32_value(bond_writer *writer, int32_t value)
{
    uint8_t buf[5];  // max 5 bytes for varint32
    size_t len = bond_encode_varint32(buf, bond_zigzag_encode32(value));
    bond_buffer_write(writer->buffer, buf, len);
}

void bond_writer_write_int64_value(bond_writer *writer, int64_t value)
{
    uint8_t buf[10];  // max 10 bytes for varint64
    size_t len = bond_encode_varint64(buf, bond_zigzag_encode64(value));
    bond_buffer_write(writer->buffer, buf, len);
}

void bond_writer_write_float_value(bond_writer *writer, float value)
{
    uint8_t buf[4];
    bond_encode_float(buf, value);
    bond_buffer_write(writer->buffer, buf, 4);
}

void bond_writer_write_double_value(bond_writer *writer, double value)
{
    uint8_t buf[8];
    bond_encode_double(buf, value);
    bond_buffer_write(writer->buffer, buf, 8);
}

void bond_writer_write_string_value(bond_writer *writer, const char *value)
{
    size_t len = strlen(value);
    uint8_t len_buf[5];  // max 5 bytes for varint32
    size_t len_len = bond_encode_varint32(len_buf, (uint32_t)len);
    bond_buffer_write(writer->buffer, len_buf, len_len);
    bond_buffer_write(writer->buffer, (const uint8_t *)value, len);
}
