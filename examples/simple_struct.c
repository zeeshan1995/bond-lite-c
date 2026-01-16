/**
 * @file simple_struct.c
 * @brief Example: Serialize and deserialize a simple struct
 *
 * Demonstrates the equivalent of this Bond schema:
 *
 *   struct Person {
 *       1: string name;
 *       2: uint32 age;
 *       3: string email;
 *   }
 */

#include <stdio.h>
#include <string.h>
#include "bond_lite.h"

// ============================================================================
// Struct Definition (in C)
// ============================================================================

typedef struct {
    const char *name;
    uint32_t age;
    const char *email;
} Person;

// ============================================================================
// Serialization
// ============================================================================

bool person_serialize(const Person *person, bond_buffer *buffer)
{
    bond_writer writer;
    bond_writer_init(&writer, buffer);
    
    bond_writer_struct_begin(&writer);
    bond_writer_write_string(&writer, 1, person->name);
    bond_writer_write_uint32(&writer, 2, person->age);
    bond_writer_write_string(&writer, 3, person->email);
    bond_writer_struct_end(&writer);
    
    return true;
}

// ============================================================================
// Deserialization
// ============================================================================

bool person_deserialize(Person *person, bond_buffer *buffer)
{
    BondReader reader;
    bond_reader_init(&reader, buffer);
    
    bond_reader_struct_begin(&reader);
    
    uint16_t field_id;
    uint8_t type;
    
    // Initialize to defaults
    person->name = NULL;
    person->age = 0;
    person->email = NULL;
    
    while (true)
    {
        if (!bond_reader_read_field_header(&reader, &field_id, &type))
        {
            return false;
        }
        
        if (type == BOND_TYPE_STOP)
        {
            break;
        }
        
        switch (field_id)
        {
            case 1:  // name
                if (type != BOND_TYPE_STRING)
                {
                    return false;
                }
                uint32_t name_len;
                if (!bond_reader_read_string_value(&reader, &person->name, &name_len))
                {
                    return false;
                }
                break;
                
            case 2:  // age
                if (type != BOND_TYPE_UINT32)
                {
                    return false;
                }
                if (!bond_reader_read_uint32_value(&reader, &person->age))
                {
                    return false;
                }
                break;
                
            case 3:  // email
                if (type != BOND_TYPE_STRING)
                {
                    return false;
                }
                uint32_t email_len;
                if (!bond_reader_read_string_value(&reader, &person->email, &email_len))
                {
                    return false;
                }
                break;
                
            default:
                // Unknown field - skip it (forward compatibility)
                if (!bond_reader_skip(&reader, type))
                {
                    return false;
                }
                break;
        }
    }
    
    bond_reader_struct_end(&reader);
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main(void)
{
    // Create a person
    Person alice = {
        .name = "Alice",
        .age = 30,
        .email = "alice@example.com"
    };
    
    printf("Original: name=%s, age=%u, email=%s\n",
           alice.name, alice.age, alice.email);
    
    // Serialize
    bond_buffer buffer;
    bond_buffer_init(&buffer, 256);
    
    if (!person_serialize(&alice, &buffer))
    {
        fprintf(stderr, "Serialization failed\n");
        return 1;
    }
    
    printf("Serialized to %zu bytes\n", buffer.size);
    
    // Print hex dump
    printf("Hex: ");
    for (size_t i = 0; i < buffer.size; i++)
    {
        printf("%02X ", buffer.data[i]);
    }
    printf("\n");
    
    // Reset read position for deserialization
    buffer.read_pos = 0;
    
    // Deserialize
    Person bob;
    if (!person_deserialize(&bob, &buffer))
    {
        fprintf(stderr, "Deserialization failed\n");
        return 1;
    }
    
    // Note: strings point into buffer - NOT null-terminated!
    // For this example, we know they're null-terminated in the original
    printf("Deserialized: name=%.*s, age=%u, email=%.*s\n",
           (int)strlen(alice.name), bob.name,
           bob.age,
           (int)strlen(alice.email), bob.email);
    
    // Cleanup
    bond_buffer_destroy(&buffer);
    
    printf("Success!\n");
    return 0;
}
