/**
 * @file bond_writer.h
 * @brief Bond CompactBinary v1 Writer
 *
 * Serializes data into Bond CompactBinary v1 format.
 */

#ifndef BOND_WRITER_H
#define BOND_WRITER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bond_buffer.h"
#include "bond_types.h"

/**
 * Writer state for serialization
 */
typedef struct {
    bond_buffer *buffer;    // Output buffer
} bond_writer;

// ============================================================================
// Lifecycle
// ============================================================================

/**
 * Initialize writer with output buffer
 */
void bond_writer_init(bond_writer *writer, bond_buffer *buffer);

// ============================================================================
// Struct Control
// ============================================================================

/**
 * Begin writing a struct (no-op in v1, resets state if needed)
 */
void bond_writer_struct_begin(bond_writer *writer);

/**
 * End struct - writes BT_STOP marker
 */
void bond_writer_struct_end(bond_writer *writer);

// ============================================================================
// Field Header
// ============================================================================

/**
 * Write field header (type + field ID)
 * - id 0-5: 1 byte [id:3][type:5]
 * - id 6-255: 2 bytes [6:3][type:5][id:8]
 * - id 256+: 3 bytes [7:3][type:5][id_lo:8][id_hi:8]
 */
void bond_writer_write_field_header(bond_writer *writer, uint16_t field_id, BondDataType type);

// ============================================================================
// Primitive Writers (with field header)
// ============================================================================

void bond_writer_write_bool(bond_writer *writer, uint16_t field_id, bool value);

void bond_writer_write_uint8(bond_writer *writer, uint16_t field_id, uint8_t value);
void bond_writer_write_uint16(bond_writer *writer, uint16_t field_id, uint16_t value);
void bond_writer_write_uint32(bond_writer *writer, uint16_t field_id, uint32_t value);
void bond_writer_write_uint64(bond_writer *writer, uint16_t field_id, uint64_t value);

void bond_writer_write_int8(bond_writer *writer, uint16_t field_id, int8_t value);
void bond_writer_write_int16(bond_writer *writer, uint16_t field_id, int16_t value);
void bond_writer_write_int32(bond_writer *writer, uint16_t field_id, int32_t value);
void bond_writer_write_int64(bond_writer *writer, uint16_t field_id, int64_t value);

void bond_writer_write_float(bond_writer *writer, uint16_t field_id, float value);
void bond_writer_write_double(bond_writer *writer, uint16_t field_id, double value);

void bond_writer_write_string(bond_writer *writer, uint16_t field_id, const char *value);

// ============================================================================
// Container Writers
// ============================================================================

/**
 * Write list container header
 * Format: [field_header][element_type:8][count:varint]
 */
void bond_writer_write_list_begin(bond_writer *writer, uint16_t field_id, 
                                  BondDataType element_type, uint32_t count);

/**
 * Write set container header
 * Format: [field_header][element_type:8][count:varint]
 */
void bond_writer_write_set_begin(bond_writer *writer, uint16_t field_id, 
                                 BondDataType element_type, uint32_t count);

/**
 * Write map container header
 * Format: [key_type:8][value_type:8][count:varint]
 */
void bond_writer_write_map_begin(bond_writer *writer, uint16_t field_id,
                                 BondDataType key_type, BondDataType value_type, 
                                 uint32_t count);

// ============================================================================
// Raw Value Writers (no field header - for container elements)
// ============================================================================

void bond_writer_write_bool_value(bond_writer *writer, bool value);
void bond_writer_write_uint8_value(bond_writer *writer, uint8_t value);
void bond_writer_write_uint16_value(bond_writer *writer, uint16_t value);
void bond_writer_write_uint32_value(bond_writer *writer, uint32_t value);
void bond_writer_write_uint64_value(bond_writer *writer, uint64_t value);
void bond_writer_write_int8_value(bond_writer *writer, int8_t value);
void bond_writer_write_int16_value(bond_writer *writer, int16_t value);
void bond_writer_write_int32_value(bond_writer *writer, int32_t value);
void bond_writer_write_int64_value(bond_writer *writer, int64_t value);
void bond_writer_write_float_value(bond_writer *writer, float value);
void bond_writer_write_double_value(bond_writer *writer, double value);
void bond_writer_write_string_value(bond_writer *writer, const char *value);

#endif // BOND_WRITER_H
