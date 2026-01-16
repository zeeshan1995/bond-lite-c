#ifndef BOND_BUFFER_H
#define BOND_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ============ Buffer Structure ============

#define BOND_BUFFER_GROWTH_FACTOR 2

typedef struct {
    uint8_t *data;      // The byte array
    size_t size;        // Bytes currently written
    size_t capacity;    // Total allocated space
    size_t read_pos;    // Current read position (for decoding)
    bool owns_memory;   // True if we malloc'd data (need to free)
} bond_buffer;

// ============ Lifecycle ============

// Create buffer with initial capacity (allocates memory)
// Returns: 0 on success, -1 on allocation failure
int bond_buffer_init(bond_buffer *buf, size_t initial_capacity);

// Wrap existing memory (for decoding received data)
// Does NOT take ownership - caller must keep data alive
void bond_buffer_init_from(bond_buffer *buf, const uint8_t *data, size_t size);

// Free memory if we own it, reset all fields
void bond_buffer_destroy(bond_buffer *buf);

// ============ Writing ============

// Ensure space for at least `additional` more bytes
// Returns: 0 on success, -1 on allocation failure
int bond_buffer_reserve(bond_buffer *buf, size_t additional);

// Append raw bytes to buffer (grows if needed)
// Returns: 0 on success, -1 on allocation failure
int bond_buffer_write(bond_buffer *buf, const void *data, size_t len);

// Append single byte
int bond_buffer_write_byte(bond_buffer *buf, uint8_t byte);

// ============ Reading ============

// Read bytes from current read position, advance read_pos
// Returns: number of bytes read (may be less than len if EOF)
size_t bond_buffer_read(bond_buffer *buf, void *dest, size_t len);

// Read single byte, returns -1 if no more data
int bond_buffer_read_byte(bond_buffer *buf);

// Peek at bytes without advancing read_pos
size_t bond_buffer_peek(const bond_buffer *buf, void *dest, size_t len);

// Bytes remaining to read
size_t bond_buffer_remaining(const bond_buffer *buf);

// ============ Utility ============

// Reset for reuse (keeps allocated memory)
void bond_buffer_clear(bond_buffer *buf);

// Reset read position to beginning
void bond_buffer_rewind(bond_buffer *buf);

#endif // BOND_BUFFER_H
