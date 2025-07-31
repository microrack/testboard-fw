#include "mod_jacket.h"
#include "display.h"
#include "esp_log.h"
#include "board.h"
#include "hal.h"
#include "test_helpers.h"

static const char* TAG = "mod_jacket";

// Test operations for jacket module
const test_operation_t jacket_test_operations[] = {
    // Set SOURCE_A, B, C to 0
    {false, TEST_OP_SOURCE, SOURCE_A, 0, 0},
    {false, TEST_OP_SOURCE, SOURCE_B, 0, 0},
    {false, TEST_OP_SOURCE, SOURCE_C, 0, 0},
    
    // Set IO 0, 1, 2 to HiZ (input mode)
    {false, TEST_OP_IO, IO0, IO_INPUT, 0},
    {false, TEST_OP_IO, IO1, IO_INPUT, 0},
    {false, TEST_OP_IO, IO2, IO_INPUT, 0},
    
    // Check current (in microamps)
    {false, TEST_OP_CHECK_CURRENT, INA_PIN_12V, 0, 1000},
    {false, TEST_OP_CHECK_CURRENT, INA_PIN_M12V, 0, 2000},
    {false, TEST_OP_CHECK_CURRENT, INA_PIN_5V, 2000, 6000},
    
    // Test A channel: IO0 in HiZ, SOURCE_A 2V
    {false, TEST_OP_SOURCE, SOURCE_A, 2000, 0},
    {false, TEST_OP_CHECK_PIN, ADC_sink_1k_A, 120, 190},
    
    // Test A channel: SOURCE_A 0V, IO0 in 1
    {false, TEST_OP_SOURCE, SOURCE_A, 0, 0},
    {false, TEST_OP_IO, IO0, IO_HIGH, 0},
    {true, TEST_OP_CHECK_PIN, ADC_sink_1k_A, 4100, 5100},
    {false, TEST_OP_IO, IO0, IO_INPUT, 0},
    
    // Test B channel: IO1 in HiZ, SOURCE_B 2V
    {false, TEST_OP_SOURCE, SOURCE_B, 2000, 0},
    {false, TEST_OP_CHECK_PIN, ADC_sink_1k_B, 120, 190},
    
    // Test B channel: SOURCE_B 0V, IO1 in 1
    {false, TEST_OP_SOURCE, SOURCE_B, 0, 0},
    {false, TEST_OP_IO, IO1, IO_HIGH, 0},
    {false, TEST_OP_CHECK_PIN, ADC_sink_1k_B, 4100, 5100},
    {false, TEST_OP_IO, IO1, IO_INPUT, 0},
    
    // Test C channel: IO2 in HiZ, SOURCE_C 2V
    {false, TEST_OP_SOURCE, SOURCE_C, 2000, 0},
    {false, TEST_OP_CHECK_PIN, ADC_sink_1k_C, 120, 190},
    
    // Test C channel: SOURCE_C 0V, IO2 in 1
    {false, TEST_OP_SOURCE, SOURCE_C, 0, 0},
    {false, TEST_OP_IO, IO2, IO_HIGH, 0},
    {false, TEST_OP_CHECK_PIN, ADC_sink_1k_C, 4100, 5100},
    {false, TEST_OP_IO, IO2, IO_INPUT, 0},
};

// Size of the test operations array
const size_t jacket_test_operations_count = sizeof(jacket_test_operations) / sizeof(jacket_test_operations[0]); 