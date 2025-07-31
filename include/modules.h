#pragma once

#include <stdint.h>
#include "test_helpers.h"
#include "mod_jacket.h"

// Module IDs
#define MODULE_ID_JACKET 0x09

// Module information structure
typedef struct {
    uint8_t id;
    const char* name;
    const test_operation_t* test_operations;
    size_t test_operations_count;
} module_info_t;

// Get module info by ID
const module_info_t* get_module_info(uint8_t id);

// Execute module tests using declarative approach
bool execute_module_tests(const module_info_t* module); 