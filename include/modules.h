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
    test_operation_result_t* test_results;         // Array of test results (same size as test_operations)
} module_info_t;

// Initialize modules from filesystem
bool init_modules_from_fs();

// Get module info by ID
module_info_t* get_module_info(uint8_t id);

// Get modules array (for internal use)
module_info_t* get_modules_array();

// Get modules count (for test results)
size_t get_modules_count();

// Execute module tests using declarative approach
bool execute_module_tests(module_info_t* module); 