#pragma once

#include <stdint.h>
#include <stddef.h>
#include "test_helpers.h"

// Test operation types
typedef enum {
    TEST_OP_SOURCE,      // Set voltage source
    TEST_OP_SOURCE_SIG,  // Start signal generator
    TEST_OP_IO,          // Set IO pin state
    TEST_OP_SINK_PD,     // Set sink pulldown
    TEST_OP_CHECK_CURRENT, // Check current consumption
    TEST_OP_CHECK_PIN,   // Check pin voltage
    TEST_OP_RESET,       // Reset all pins to safe state
    TEST_OP_SCOPE,       // Start Sigscoper in FREE mode
    TEST_OP_CHECK_MIN,   // Check minimum signal value
    TEST_OP_CHECK_MAX,   // Check maximum signal value
    TEST_OP_CHECK_AVG,   // Check average signal value
    TEST_OP_CHECK_FREQ,  // Check signal frequency
    TEST_OP_CHECK_AMPLITUDE, // Check signal amplitude (max - min)
    TEST_OP_DELAY        // Delay for specified time in milliseconds
} test_op_type_t;

// Test operation structure
typedef struct {
    bool repeat;           // Use TEST_RUN_REPEAT if true, TEST_RUN if false
    test_op_type_t op;    // Operation type
    int pin;              // Pin number
    int32_t arg1;         // Voltage for SOURCE, state for IO, 0/1 for SINK_PD, low value for checks
    int32_t arg2;         // High value for checks (only used for CHECK_CURRENT and CHECK_PIN)
} test_operation_t;

// Test result structure
typedef struct {
    bool passed;          // true if test passed, false if failed
    int32_t result;       // actual value obtained from the operation (if test failed)
    uint32_t execution_time_ms; // execution time in milliseconds
} test_operation_result_t;

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

// Set current module index
void set_current_module_index(size_t index);

// Get current module index
size_t get_current_module_index();

// Get module info by ID
module_info_t* get_current_module_info();

// Get modules array (for internal use)
module_info_t* get_modules_array();

// Get modules count (for test results)
size_t get_modules_count();

// Execute module tests using declarative approach
bool execute_module_tests(module_info_t* module); 