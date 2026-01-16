/**
 * @file bond_reader.h
 * @brief CompactBinary v1 reader for Bond serialization
 *
 * Reads Bond CompactBinary v1 format with absolute field IDs.
 * See docs/READER_DESIGN.md for design details.
 */

#ifndef BOND_READER_H
#define BOND_READER_H

#include "bond_buffer.h"
#include "bond_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Reader State
// ============================================================================

typedef struct {
    bond_buffer *buffer;   // Uses buffer's read_pos, data, size
} BondReader;

// ============================================================================
// Initialization
// ============================================================================

/**
 * Initialize a reader with a buffer
 * 
 * For reading received data, first call bond_buffer_init_from().
 * For round-trip testing, call bond_buffer_rewind() after writing.
 */
void bond_reader_init(BondReader *reader, bond_buffer *buffer);

// ============================================================================
// Struct Control
// ============================================================================

/**
 * Begin reading a struct (no-op for v1, included for symmetry)
 */
void bond_reader_struct_begin(BondReader *reader);

/**
 * End reading a struct (no-op for v1, included for symmetry)
 */
void bond_reader_struct_end(BondReader *reader);

// ============================================================================
// Field Header Reading
// ============================================================================

/**
 * Read the next field header
 * 
 * @param reader    The reader
 * @param field_id  Output: field ID (0 means STOP)
 * @param type      Output: field type (BondDataType)
 * @return true on success, false on error (truncated data)
 * 
 * When type == BOND_TYPE_STOP, the struct has ended.
 */
bool bond_reader_read_field_header(BondReader *reader, uint16_t *field_id, uint8_t *type);

// ============================================================================
// Primitive Value Readers (read value only, no field header)
// ============================================================================

bool bond_reader_read_bool_value(BondReader *reader, bool *value);
bool bond_reader_read_uint8_value(BondReader *reader, uint8_t *value);
bool bond_reader_read_uint16_value(BondReader *reader, uint16_t *value);
bool bond_reader_read_uint32_value(BondReader *reader, uint32_t *value);
bool bond_reader_read_uint64_value(BondReader *reader, uint64_t *value);
bool bond_reader_read_int8_value(BondReader *reader, int8_t *value);
bool bond_reader_read_int16_value(BondReader *reader, int16_t *value);
bool bond_reader_read_int32_value(BondReader *reader, int32_t *value);
bool bond_reader_read_int64_value(BondReader *reader, int64_t *value);
bool bond_reader_read_float_value(BondReader *reader, float *value);
bool bond_reader_read_double_value(BondReader *reader, double *value);

/**
 * Read a string value (zero-copy)
 * 
 * @param reader  The reader
 * @param str     Output: pointer to string data (NOT null-terminated, points into buffer)
 * @param len     Output: string length
 * @return true on success, false on error
 * 
 * Note: The returned string points directly into the buffer.
 * It is NOT null-terminated. Copy if you need a C string.
 */
bool bond_reader_read_string_value(BondReader *reader, const char **str, uint32_t *len);

// ============================================================================
// Container Header Readers
// ============================================================================

/**
 * Read list header (element type and count)
 */
bool bond_reader_read_list_begin(BondReader *reader, uint8_t *element_type, uint32_t *count);

/**
 * Read set header (element type and count) - same wire format as list
 */
bool bond_reader_read_set_begin(BondReader *reader, uint8_t *element_type, uint32_t *count);

/**
 * Read map header (key type, value type, and count)
 */
bool bond_reader_read_map_begin(BondReader *reader, uint8_t *key_type, uint8_t *value_type, uint32_t *count);

// ============================================================================
// Skip Functions (for unknown fields)
// ============================================================================

/**
 * Skip a value of the given type
 * 
 * Use this to skip unknown fields for forward compatibility.
 * Handles nested structs and containers recursively.
 * 
 * @param reader  The reader
 * @param type    The BondDataType to skip
 * @return true on success, false on error
 */
bool bond_reader_skip(BondReader *reader, uint8_t type);

#ifdef __cplusplus
}
#endif

#endif // BOND_READER_H
