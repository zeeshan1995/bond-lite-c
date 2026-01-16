#include "bond_buffer.h"
#include <stdlib.h>
#include <string.h>

// ============ Lifecycle ============

// Create buffer with initial capacity (allocates memory)
// Returns: 0 on success, -1 on allocation failure
int bond_buffer_init(bond_buffer *buf, size_t initial_capacity)
{
    buf->data = (uint8_t *)malloc(initial_capacity);
    if (buf->data == NULL) {
        return -1;
    }
    buf->size = 0;
    buf->capacity = initial_capacity;
    buf->read_pos = 0;
    buf->owns_memory = true;
    return 0;
}

// Wrap existing memory (for decoding received data)
// Does NOT take ownership - caller must keep data alive
void bond_buffer_init_from(bond_buffer *buf, const uint8_t *data, size_t size)
{
    buf->data = (uint8_t *)data; // cast away const for internal use
    buf->size = size;
    buf->capacity = size;
    buf->read_pos = 0;
    buf->owns_memory = false;
}

// Free memory if we own it, reset all fields
void bond_buffer_destroy(bond_buffer *buf)
{
    if(buf-> owns_memory && buf->data != NULL)
    {
        free(buf->data);
    }
    buf->data = NULL;
    buf->size = 0;
    buf->capacity = 0;
    buf->read_pos = 0;
    buf->owns_memory = false;
}

// ============ Writing ============

// Ensure space for at least `additional` more bytes
// Returns: 0 on success, -1 on allocation failure
int bond_buffer_reserve(bond_buffer *buf, size_t additional)
{
    if (buf->size + additional > buf->capacity) 
    {
        size_t new_capacity = buf->capacity * BOND_BUFFER_GROWTH_FACTOR;
        if (new_capacity < buf->size + additional)
        {
            new_capacity = buf->size + additional;
        }
        uint8_t *new_data = (uint8_t *)realloc(buf->data, new_capacity);
        if (new_data == NULL)
        {
            return -1;
        }
        buf->data = new_data;
        buf->capacity = new_capacity;
    }

    return 0;
}

// Append raw bytes to buffer (grows if needed)
// Returns: 0 on success, -1 on allocation failure
int bond_buffer_write(bond_buffer *buf, const void *data, size_t len)
{
    if (bond_buffer_reserve(buf, len) != 0)
    {
        return -1;
    }
    memcpy(buf->data + buf->size, data, len);
    buf->size += len;
    return 0;
}

// Append single byte
int bond_buffer_write_byte(bond_buffer *buf, uint8_t byte)
{
    if (bond_buffer_reserve(buf, 1) != 0)
    {
        return -1;
    }
    buf->data[buf->size++] = byte; 
    return 0;
}

// ============ Reading ============

// Read bytes from current read position, advance read_pos
// Returns: number of bytes read (may be less than len if EOF)
size_t bond_buffer_read(bond_buffer *buf, void *dest, size_t len)
{
    size_t bytes_available = buf->size - buf->read_pos;
    size_t bytes_to_read = (len < bytes_available) ? len : bytes_available;
    memcpy(dest, buf->data + buf->read_pos, bytes_to_read);
    buf->read_pos += bytes_to_read;
    return bytes_to_read;
}

// Read single byte, returns -1 if no more data
int bond_buffer_read_byte(bond_buffer *buf)
{
    if (buf->read_pos < buf->size)
    {
        return buf->data[buf->read_pos++];
    }
    return -1;
}

// Peek at bytes without advancing read_pos
// TODO: Add peek() later if needed for look-ahead parsing
size_t bond_buffer_peek(const bond_buffer *buf, void *dest, size_t len)
{
    size_t bytes_available = buf->size - buf->read_pos;
    size_t bytes_to_peek = (len < bytes_available) ? len : bytes_available;
    memcpy(dest, buf->data + buf->read_pos, bytes_to_peek);
    return bytes_to_peek;
}

// Bytes remaining to read
size_t bond_buffer_remaining(const bond_buffer *buf)
{
    return buf->size - buf->read_pos;
}

// ============ Utility ============

// Reset for reuse (keeps allocated memory)
void bond_buffer_clear(bond_buffer *buf)
{
    buf->size = 0;
    buf->read_pos = 0;
}

// Reset read position to beginning
void bond_buffer_rewind(bond_buffer *buf)
{
    buf->read_pos = 0;
}
