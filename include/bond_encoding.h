/**
 * @file bond_encoding.h
 * @brief Low-level encoding primitives for Bond serialization
 *
 * Provides varint, zigzag, and float/double encoding/decoding.
 */

#ifndef BOND_ENCODING_H
#define BOND_ENCODING_H

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// Varint Encoding (LEB128)
// ============================================================================

/**
 * Encode unsigned integers as variable-length bytes (LEB128)
 * @param out Output buffer (must have space: 3/5/10 bytes max)
 * @param value Value to encode
 * @return Number of bytes written
 */
size_t bond_encode_varint16(uint8_t *out, uint16_t value);
size_t bond_encode_varint32(uint8_t *out, uint32_t value);
size_t bond_encode_varint64(uint8_t *out, uint64_t value);

/**
 * Decode varint back to unsigned integers
 * @param data Input buffer
 * @param out_value Decoded value
 * @return Number of bytes consumed
 */
size_t bond_decode_varint16(const uint8_t *data, uint16_t *out_value);
size_t bond_decode_varint32(const uint8_t *data, uint32_t *out_value);
size_t bond_decode_varint64(const uint8_t *data, uint64_t *out_value);

// ============================================================================
// ZigZag Encoding (signed to unsigned mapping)
// ============================================================================

/**
 * ZigZag encode signed integers to unsigned
 * Maps: 0→0, -1→1, 1→2, -2→3, 2→4, ...
 */
uint16_t bond_zigzag_encode16(int16_t value);
uint32_t bond_zigzag_encode32(int32_t value);
uint64_t bond_zigzag_encode64(int64_t value);

/**
 * ZigZag decode unsigned back to signed
 */
int16_t bond_zigzag_decode16(uint16_t value);
int32_t bond_zigzag_decode32(uint32_t value);
int64_t bond_zigzag_decode64(uint64_t value);

// ============================================================================
// Float/Double Encoding (IEEE 754 little-endian)
// ============================================================================

/**
 * Encode float/double as little-endian bytes
 * @param out Output buffer (4 bytes for float, 8 for double)
 * @param value Value to encode
 */
void bond_encode_float(uint8_t *out, float value);
void bond_encode_double(uint8_t *out, double value);

/**
 * Decode little-endian bytes back to float/double
 * @param data Input buffer
 * @return Decoded value
 */
float bond_decode_float(const uint8_t *data);
double bond_decode_double(const uint8_t *data);

#endif // BOND_ENCODING_H
