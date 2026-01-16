/**
 * @file nested_struct.c
 * @brief Example: Serialize and deserialize nested structs with containers
 *
 * Demonstrates the equivalent of this Bond schema:
 *
 *   struct Address {
 *       1: string street;
 *       2: string city;
 *       3: uint32 zip;
 *   }
 *
 *   struct Company {
 *       1: string name;
 *       2: Address headquarters;
 *       3: list<string> departments;
 *   }
 */

#include <stdio.h>
#include <string.h>
#include "bond_lite.h"

// ============================================================================
// Struct Definitions
// ============================================================================

typedef struct {
    const char *street;
    const char *city;
    uint32_t zip;
} Address;

typedef struct {
    const char *name;
    Address headquarters;
    const char **departments;
    uint32_t department_count;
} Company;

// ============================================================================
// Serialization
// ============================================================================

void address_serialize(const Address *addr, bond_writer *writer)
{
    bond_writer_struct_begin(writer);
    bond_writer_write_string(writer, 1, addr->street);
    bond_writer_write_string(writer, 2, addr->city);
    bond_writer_write_uint32(writer, 3, addr->zip);
    bond_writer_struct_end(writer);
}

void company_serialize(const Company *company, bond_buffer *buffer)
{
    bond_writer writer;
    bond_writer_init(&writer, buffer);
    
    bond_writer_struct_begin(&writer);
    
    // Field 1: name
    bond_writer_write_string(&writer, 1, company->name);
    
    // Field 2: headquarters (nested struct)
    bond_writer_write_field_header(&writer, 2, BOND_TYPE_STRUCT);
    address_serialize(&company->headquarters, &writer);
    
    // Field 3: departments (list<string>)
    bond_writer_write_list_begin(&writer, 3, BOND_TYPE_STRING, company->department_count);
    for (uint32_t i = 0; i < company->department_count; i++)
    {
        bond_writer_write_string_value(&writer, company->departments[i]);
    }
    
    bond_writer_struct_end(&writer);
}

// ============================================================================
// Deserialization
// ============================================================================

bool address_deserialize(Address *addr, BondReader *reader)
{
    bond_reader_struct_begin(reader);
    
    uint16_t field_id;
    uint8_t type;
    uint32_t len;
    
    addr->street = NULL;
    addr->city = NULL;
    addr->zip = 0;
    
    while (true)
    {
        if (!bond_reader_read_field_header(reader, &field_id, &type))
        {
            return false;
        }
        
        if (type == BOND_TYPE_STOP)
        {
            break;
        }
        
        switch (field_id)
        {
            case 1:
                if (!bond_reader_read_string_value(reader, &addr->street, &len))
                    return false;
                break;
            case 2:
                if (!bond_reader_read_string_value(reader, &addr->city, &len))
                    return false;
                break;
            case 3:
                if (!bond_reader_read_uint32_value(reader, &addr->zip))
                    return false;
                break;
            default:
                if (!bond_reader_skip(reader, type))
                    return false;
                break;
        }
    }
    
    bond_reader_struct_end(reader);
    return true;
}

// Note: For simplicity, this example uses static storage for department names
static const char *g_departments[16];

bool company_deserialize(Company *company, bond_buffer *buffer)
{
    BondReader reader;
    bond_reader_init(&reader, buffer);
    
    bond_reader_struct_begin(&reader);
    
    uint16_t field_id;
    uint8_t type;
    uint32_t len;
    
    company->name = NULL;
    company->departments = g_departments;
    company->department_count = 0;
    
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
                if (!bond_reader_read_string_value(&reader, &company->name, &len))
                    return false;
                break;
                
            case 2:  // headquarters (nested struct)
                if (type != BOND_TYPE_STRUCT)
                    return false;
                if (!address_deserialize(&company->headquarters, &reader))
                    return false;
                break;
                
            case 3:  // departments (list<string>)
            {
                if (type != BOND_TYPE_LIST)
                    return false;
                
                uint8_t element_type;
                uint32_t count;
                if (!bond_reader_read_list_begin(&reader, &element_type, &count))
                    return false;
                
                if (element_type != BOND_TYPE_STRING)
                    return false;
                
                company->department_count = count;
                for (uint32_t i = 0; i < count && i < 16; i++)
                {
                    if (!bond_reader_read_string_value(&reader, &g_departments[i], &len))
                        return false;
                }
                break;
            }
                
            default:
                if (!bond_reader_skip(&reader, type))
                    return false;
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
    // Create a company
    const char *depts[] = {"Engineering", "Sales", "Marketing"};
    
    Company acme = {
        .name = "Acme Corp",
        .headquarters = {
            .street = "123 Main St",
            .city = "Anytown",
            .zip = 12345
        },
        .departments = depts,
        .department_count = 3
    };
    
    printf("Original Company:\n");
    printf("  Name: %s\n", acme.name);
    printf("  HQ: %s, %s %u\n", 
           acme.headquarters.street, 
           acme.headquarters.city, 
           acme.headquarters.zip);
    printf("  Departments: ");
    for (uint32_t i = 0; i < acme.department_count; i++)
    {
        printf("%s%s", acme.departments[i], 
               i < acme.department_count - 1 ? ", " : "\n");
    }
    
    // Serialize
    bond_buffer buffer;
    bond_buffer_init(&buffer, 512);
    
    company_serialize(&acme, &buffer);
    printf("\nSerialized to %zu bytes\n", buffer.size);
    
    // Print hex dump
    printf("Hex: ");
    for (size_t i = 0; i < buffer.size; i++)
    {
        printf("%02X ", buffer.data[i]);
    }
    printf("\n\n");
    
    // Reset for reading
    buffer.read_pos = 0;
    
    // Deserialize
    Company loaded;
    if (!company_deserialize(&loaded, &buffer))
    {
        fprintf(stderr, "Deserialization failed\n");
        return 1;
    }
    
    printf("Deserialized Company:\n");
    printf("  Name: %.*s\n", (int)strlen(acme.name), loaded.name);
    printf("  HQ: %.*s, %.*s %u\n", 
           (int)strlen(acme.headquarters.street), loaded.headquarters.street,
           (int)strlen(acme.headquarters.city), loaded.headquarters.city,
           loaded.headquarters.zip);
    printf("  Departments: ");
    for (uint32_t i = 0; i < loaded.department_count; i++)
    {
        printf("%.*s%s", (int)strlen(depts[i]), loaded.departments[i],
               i < loaded.department_count - 1 ? ", " : "\n");
    }
    
    // Cleanup
    bond_buffer_destroy(&buffer);
    
    printf("\nSuccess!\n");
    return 0;
}
