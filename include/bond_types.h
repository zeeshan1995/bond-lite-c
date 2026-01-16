/**
 * @file bond_types.h
 * @brief Bond data type definitions
 *
 * Type IDs from official Microsoft Bond:
 * https://github.com/microsoft/bond/blob/master/idl/bond/core/bond_const.bond
 */

#ifndef BOND_TYPES_H
#define BOND_TYPES_H

#include <stdint.h>

/**
 * Bond data type identifiers
 * These match the official BondDataType enum from bond_const.bond
 */
typedef enum {
    BOND_TYPE_STOP       = 0,   // End of struct marker
    BOND_TYPE_STOP_BASE  = 1,   // End of base struct marker
    BOND_TYPE_BOOL       = 2,
    BOND_TYPE_UINT8      = 3,
    BOND_TYPE_UINT16     = 4,
    BOND_TYPE_UINT32     = 5,
    BOND_TYPE_UINT64     = 6,
    BOND_TYPE_FLOAT      = 7,
    BOND_TYPE_DOUBLE     = 8,
    BOND_TYPE_STRING     = 9,
    BOND_TYPE_STRUCT     = 10,
    BOND_TYPE_LIST       = 11,
    BOND_TYPE_SET        = 12,
    BOND_TYPE_MAP        = 13,
    BOND_TYPE_INT8       = 14,
    BOND_TYPE_INT16      = 15,
    BOND_TYPE_INT32      = 16,
    BOND_TYPE_INT64      = 17,
    BOND_TYPE_WSTRING    = 18,
    BOND_TYPE_UNAVAILABLE = 127
} BondDataType;

/**
 * Bond protocol type identifiers
 */
typedef enum {
    BOND_PROTOCOL_MARSHALED    = 0,
    BOND_PROTOCOL_FAST         = 0x464D,  // "FM"
    BOND_PROTOCOL_COMPACT      = 0x4243,  // "CB"
    BOND_PROTOCOL_SIMPLE_JSON  = 0x4A53,  // "JS"
    BOND_PROTOCOL_SIMPLE       = 0x5053   // "PS"
} BondProtocolType;

/**
 * List subtype identifiers
 */
typedef enum {
    BOND_LIST_NO_SUBTYPE       = 0,
    BOND_LIST_NULLABLE_SUBTYPE = 1,
    BOND_LIST_BLOB_SUBTYPE     = 2
} BondListSubType;

#endif // BOND_TYPES_H
