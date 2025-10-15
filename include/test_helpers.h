#pragma once

#include <Arduino.h>
#include "hal.h"
#include "sigscoper.h"

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
bool check_current(ina_pin_t pin, const range_t& range, const char* rail_name, int32_t* result = nullptr);

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
bool test_pin_range(ADC_sink_t pin, const range_t& range, const char* pin_name, int32_t* result = nullptr);

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

bool test_mode(const int led_pin1, const int led_pin2, const mode_current_ranges_t& ranges, int* output_mode); 

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

/**
 * @brief Check if IO pin level matches expected value
 * 
 * @param pin The IO pin to check
 * @param expected_level Expected level (1 for HIGH, 0 for LOW)
 * @param level_name Name of the expected level for display purposes
 * @param result Output parameter for actual level read
 * @return true if the level matches expected, false otherwise
 */
bool check_io_level(mcp_io_t pin, int expected_level, const char* level_name, int32_t* result = nullptr);

 