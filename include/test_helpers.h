#pragma once

#include <Arduino.h>
#include "hal.h"
#include "sigscoper.h"

/**
 * @brief Macro to execute a function and return false on failure
 * 
 * This macro executes the provided function and if it returns false,
 * returns false from the caller function.
 * 
 * @param func The function to execute
 */
#define TEST_RUN(func) do { \
    if (!func) { \
        return false; \
    } \
} while(0)

/**
 * @brief Macro to execute a function and retry on failure
 * 
 * This macro executes the provided function and if it returns false,
 * turns on the FAIL LED and retries the function until either:
 * 1. The function returns true (returns TEST_NEED_REPEAT if this was a retry)
 * 2. The power rails state is no longer POWER_RAILS_ALL (returns TEST_INTERRUPTED)
 * 
 * @param func The function to execute
 */
#define TEST_RUN_REPEAT(func) do { \
    if (!func) { \
        while (!func) { \
            mcp1.digitalWrite(PIN_LED_FAIL, HIGH); \
            mcp1.digitalWrite(PIN_LED_OK, LOW); \
            if (get_power_rails_state(NULL, NULL, NULL) != POWER_RAILS_ALL) { \
                return false; \
            } \
            delay(10); \
        } \
        mcp1.digitalWrite(PIN_LED_FAIL, LOW); \
        return false; \
    } \
} while(0)

/**
 * @brief Test result states
 */
typedef enum {
    TEST_OK,           // Test passed successfully
    TEST_NEED_REPEAT,  // Test needs to be repeated
    TEST_INTERRUPTED   // Test was interrupted (e.g. power rails disconnected)
} test_result_t;

/**
 * @brief Power rail connection states
 */
typedef enum {
    POWER_RAILS_NONE,      // No power rails are connected
    POWER_RAILS_PARTIAL,   // Some but not all power rails are connected
    POWER_RAILS_ALL        // All power rails are connected
} power_rails_state_t;

/**
 * @brief Checks the state of all power rails
 * 
 * @param p12v_state Output parameter for +12V rail state (true if OK)
 * @param p5v_state Output parameter for +5V rail state (true if OK)
 * @param m12v_state Output parameter for -12V rail state (true if OK)
 * @return power_rails_state_t indicating the overall state of power rails
 */
power_rails_state_t get_power_rails_state(bool* p12v_state, bool* p5v_state, bool* m12v_state);

/**
 * @brief Performs the startup sequence including adapter detection and calibration
 * 
 * @return true if startup was successful, false otherwise
 */
bool perform_startup_sequence();

/**
 * @brief Waits for a module to be inserted (all power rails connected)
 * 
 * @param p12v_ok Output parameter for +12V rail state
 * @param p5v_ok Output parameter for +5V rail state
 * @param m12v_ok Output parameter for -12V rail state
 * @return power_rails_state_t The final state of the power rails
 */
power_rails_state_t wait_for_module_insertion(bool& p12v_ok, bool& p5v_ok, bool& m12v_ok);

/**
 * @brief Waits for a module to be removed (no power rails connected)
 * 
 * @param p12v_ok Output parameter for +12V rail state
 * @param p5v_ok Output parameter for +5V rail state
 * @param m12v_ok Output parameter for -12V rail state
 * @return power_rails_state_t The final state of the power rails
 */
power_rails_state_t wait_for_module_removal(bool& p12v_ok, bool& p5v_ok, bool& m12v_ok);

/**
 * @brief Structure defining range for a value
 */
typedef struct {
    int32_t min;  // Minimum acceptable value in millivolts/milliamps
    int32_t max;  // Maximum acceptable value in millivolts/milliamps
} range_t;

/**
 * @brief Structure defining current ranges for all power rails
 */
typedef struct {
    range_t p12v;  // +12V rail range in microamps
    range_t m12v;  // -12V rail range in microamps
    range_t p5v;   // +5V rail range in microamps
} power_rails_current_ranges_t;

/**
 * @brief Current measurement pins
 */
typedef enum {
    INA_PIN_12V = PIN_INA_12V,    // +12V rail current monitor
    INA_PIN_5V = PIN_INA_5V,      // +5V rail current monitor
    INA_PIN_M12V = PIN_INA_M12V   // -12V rail current monitor
} ina_pin_t;

/**
 * @brief Checks if the current consumption on a specific rail is within acceptable range
 * 
 * @param pin The INA pin to measure current from
 * @param range The acceptable current range in microamps
 * @param rail_name Name of the rail for display purposes
 * @return true if current measurement is within acceptable range, false otherwise
 */
bool check_current(ina_pin_t pin, const range_t& range, const char* rail_name);

/**
 * @brief Checks if the initial current consumption is within acceptable ranges
 * 
 * @param ranges Structure containing acceptable current ranges for all power rails
 * @return true if all current measurements are within acceptable ranges, false otherwise
 */
bool check_initial_current_consumption(const power_rails_current_ranges_t& ranges);

/**
 * @brief Test if a pin's voltage is within specified range
 * 
 * @param pin The ADC sink pin to test
 * @param range The expected voltage range in millivolts
 * @param pin_name The name of the pin for display purposes
 * @return true if the voltage is within range, false otherwise
 */
bool test_pin_range(ADC_sink_t pin, const range_t& range, const char* pin_name);

/**
 * @brief Test a pin with pull-down functionality
 * 
 * @param source The voltage source configuration (ADC pin and PD control pin)
 * @param hiz_range Expected voltage range in high-impedance state
 * @param pd_range Expected voltage range in pull-down state
 * @param source_name Name of the source for display purposes
 * @return true if both high-Z and pull-down voltages are within expected ranges
 */
bool test_pin_pd(const voltage_source_t& source, 
                const range_t& hiz_range, const range_t& pd_range,
                const char* source_name);

typedef struct {
    range_t active;
    range_t inactive;
} mode_current_ranges_t;

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

bool test_mode(const int led_pin1, const int led_pin2, const mode_current_ranges_t& ranges, int* output_mode); 

/**
 * @brief Execute a sequence of test operations
 * 
 * @param operations Array of test operations to execute
 * @param count Number of operations in the array
 * @param results Array to store test results (must be pre-allocated with size 'count')
 * @return true if all tests passed, false otherwise
 */
bool execute_test_sequence(const test_operation_t* operations, size_t count, test_operation_result_t* results);

/**
 * @brief Execute a single test operation
 * 
 * @param op The test operation to execute
 * @param result Pointer to store the actual result value (for failed tests)
 * @return true if the operation succeeded, false otherwise
 */
bool execute_single_operation(const test_operation_t& op, int32_t* result = nullptr);

/**
 * @brief Execute reset operation - set all pins to safe state
 * 
 * This function:
 * 1. Sets all IO pins to HiZ (input mode)
 * 2. Sets all voltage sources to 0V
 * 3. Disables all pulldowns
 * 
 * @return true if reset was successful, false otherwise
 */
bool execute_reset_operation();

// Helper function to map current measurement pin numbers from JSON to actual pins
int map_current_pin(int pin);

// Sigscoper functions
bool start_sigscoper(ADC_sink_t pin, uint32_t sample_freq, size_t buffer_size);
bool check_signal_min(ADC_sink_t pin, const range_t& range, int32_t* result = nullptr);
bool check_signal_max(ADC_sink_t pin, const range_t& range, int32_t* result = nullptr);
bool check_signal_avg(ADC_sink_t pin, const range_t& range, int32_t* result = nullptr);
bool check_signal_freq(ADC_sink_t pin, const range_t& range, int32_t* result = nullptr);
bool check_signal_amplitude(ADC_sink_t pin, const range_t& range, int32_t* result = nullptr);

// Helper functions for ADC mapping
adc_unit_t adc_sink_to_unit(ADC_sink_t pin);

 