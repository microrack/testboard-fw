#pragma once

#include <Arduino.h>

/**
 * @brief Macro to execute a function and retry on failure
 * 
 * This macro executes the provided function and if it returns false,
 * turns on the FAIL LED and retries the function until either:
 * 1. The function succeeds, or
 * 2. The power rails state is no longer POWER_RAILS_ALL
 * 
 * @param func The function to execute
 */
#define TEST_RUN(func) do { \
    bool p12v_ok, p5v_ok, m12v_ok; \
    while (!(func)) { \
        mcp1.digitalWrite(PIN_LED_FAIL, HIGH); \
        mcp1.digitalWrite(PIN_LED_OK, LOW); \
        if (get_power_rails_state(p12v_ok, p5v_ok, m12v_ok) != POWER_RAILS_ALL) { \
            return false; \
        } \
        delay(100); \
    } \
    mcp1.digitalWrite(PIN_LED_FAIL, LOW); \
} while(0)

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
power_rails_state_t get_power_rails_state(bool& p12v_state, bool& p5v_state, bool& m12v_state);

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
    float min;  // Minimum acceptable value
    float max;  // Maximum acceptable value
} range_t;

/**
 * @brief Structure defining current ranges for all power rails
 */
typedef struct {
    range_t p12v;  // +12V rail range in mA
    range_t m12v;  // -12V rail range in mA
    range_t p5v;   // +5V rail range in mA
} power_rails_current_ranges_t;

/**
 * @brief Checks if the initial current consumption is within acceptable ranges
 * 
 * @param ranges Structure containing acceptable current ranges for all power rails
 * @return true if all current measurements are within acceptable ranges, false otherwise
 */
bool check_initial_current_consumption(const power_rails_current_ranges_t& ranges); 