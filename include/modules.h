#pragma once

#include <stdint.h>
#include <stddef.h>
#include "test_helpers.h"

/**
 * @brief Module information structure
 */
typedef struct {
    uint8_t id;                          // Module ID
    char* name;                          // Module name
    const test_operation_t* test_operations;  // Array of test operations
    size_t test_operations_count;        // Number of test operations
} module_info_t;

// Initialize modules from filesystem
bool init_modules_from_fs();

// Get module info by ID
const module_info_t* get_module_info(uint8_t id);

// Execute module tests using declarative approach
bool execute_module_tests(const module_info_t* module); 