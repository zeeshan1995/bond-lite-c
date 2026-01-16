/**
 * @file enum_example.c
 * @brief Example: Serialize and deserialize structs with enums
 *
 * Demonstrates the equivalent of this Bond schema:
 *
 *   enum Status {
 *       Pending = 0,
 *       Active = 1,
 *       Completed = 2,
 *       Cancelled = 3
 *   }
 *
 *   enum Priority {
 *       Low = 0,
 *       Medium = 1,
 *       High = 2,
 *       Critical = 3
 *   }
 *
 *   struct Task {
 *       1: string title;
 *       2: Status status;
 *       3: Priority priority;
 *       4: uint32 assigned_to;
 *   }
 *
 * Note: Bond enums are serialized as int32 with zigzag encoding.
 */

#include <stdio.h>
#include <string.h>
#include "bond_lite.h"

// ============================================================================
// Enum Definitions
// ============================================================================

typedef enum {
    STATUS_PENDING = 0,
    STATUS_ACTIVE = 1,
    STATUS_COMPLETED = 2,
    STATUS_CANCELLED = 3
} Status;

typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_MEDIUM = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_CRITICAL = 3
} Priority;

// Helper for printing
const char* status_to_string(Status s)
{
    switch (s) {
        case STATUS_PENDING:   return "Pending";
        case STATUS_ACTIVE:    return "Active";
        case STATUS_COMPLETED: return "Completed";
        case STATUS_CANCELLED: return "Cancelled";
        default:               return "Unknown";
    }
}

const char* priority_to_string(Priority p)
{
    switch (p) {
        case PRIORITY_LOW:      return "Low";
        case PRIORITY_MEDIUM:   return "Medium";
        case PRIORITY_HIGH:     return "High";
        case PRIORITY_CRITICAL: return "Critical";
        default:                return "Unknown";
    }
}

// ============================================================================
// Struct Definition
// ============================================================================

typedef struct {
    const char *title;
    Status status;
    Priority priority;
    uint32_t assigned_to;
} Task;

// ============================================================================
// Serialization
// ============================================================================

bool task_serialize(const Task *task, bond_buffer *buffer)
{
    bond_writer writer;
    bond_writer_init(&writer, buffer);
    
    bond_writer_struct_begin(&writer);
    bond_writer_write_string(&writer, 1, task->title);
    // Enums are written as int32 in Bond
    bond_writer_write_int32(&writer, 2, (int32_t)task->status);
    bond_writer_write_int32(&writer, 3, (int32_t)task->priority);
    bond_writer_write_uint32(&writer, 4, task->assigned_to);
    bond_writer_struct_end(&writer);
    
    return true;
}

// ============================================================================
// Deserialization
// ============================================================================

bool task_deserialize(Task *task, bond_buffer *buffer)
{
    BondReader reader;
    bond_reader_init(&reader, buffer);
    
    bond_reader_struct_begin(&reader);
    
    uint16_t field_id;
    uint8_t type;
    
    // Initialize to defaults
    task->title = NULL;
    task->status = STATUS_PENDING;
    task->priority = PRIORITY_LOW;
    task->assigned_to = 0;
    
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
            case 1:  // title
            {
                if (type != BOND_TYPE_STRING)
                    return false;
                uint32_t len;
                if (!bond_reader_read_string_value(&reader, &task->title, &len))
                    return false;
                break;
            }
            
            case 2:  // status (enum as int32)
            {
                if (type != BOND_TYPE_INT32)
                    return false;
                int32_t val;
                if (!bond_reader_read_int32_value(&reader, &val))
                    return false;
                task->status = (Status)val;
                break;
            }
            
            case 3:  // priority (enum as int32)
            {
                if (type != BOND_TYPE_INT32)
                    return false;
                int32_t val;
                if (!bond_reader_read_int32_value(&reader, &val))
                    return false;
                task->priority = (Priority)val;
                break;
            }
            
            case 4:  // assigned_to
            {
                if (type != BOND_TYPE_UINT32)
                    return false;
                if (!bond_reader_read_uint32_value(&reader, &task->assigned_to))
                    return false;
                break;
            }
            
            default:
                // Unknown field - skip for forward compatibility
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
    // Create tasks with different enum values
    Task tasks[] = {
        { .title = "Write documentation", .status = STATUS_ACTIVE, .priority = PRIORITY_HIGH, .assigned_to = 42 },
        { .title = "Fix bug #123", .status = STATUS_PENDING, .priority = PRIORITY_CRITICAL, .assigned_to = 7 },
        { .title = "Code review", .status = STATUS_COMPLETED, .priority = PRIORITY_MEDIUM, .assigned_to = 15 },
    };
    
    size_t num_tasks = sizeof(tasks) / sizeof(tasks[0]);
    
    for (size_t i = 0; i < num_tasks; i++)
    {
        Task *t = &tasks[i];
        printf("Task %zu: '%s' [%s, %s] -> user %u\n",
               i + 1, t->title, 
               status_to_string(t->status),
               priority_to_string(t->priority),
               t->assigned_to);
        
        // Serialize
        bond_buffer buffer;
        bond_buffer_init(&buffer, 256);
        
        if (!task_serialize(t, &buffer))
        {
            fprintf(stderr, "Serialization failed\n");
            return 1;
        }
        
        printf("  Serialized: %zu bytes -> ", buffer.size);
        for (size_t j = 0; j < buffer.size; j++)
        {
            printf("%02X ", buffer.data[j]);
        }
        printf("\n");
        
        // Deserialize
        buffer.read_pos = 0;
        Task loaded;
        if (!task_deserialize(&loaded, &buffer))
        {
            fprintf(stderr, "Deserialization failed\n");
            return 1;
        }
        
        printf("  Roundtrip:  '%.*s' [%s, %s] -> user %u\n\n",
               (int)strlen(t->title), loaded.title,
               status_to_string(loaded.status),
               priority_to_string(loaded.priority),
               loaded.assigned_to);
        
        bond_buffer_destroy(&buffer);
    }
    
    printf("Success!\n");
    return 0;
}
