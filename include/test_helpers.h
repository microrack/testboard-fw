#pragma once

#include <Arduino.h>

/**
 * @brief Macro to execute a function and propagate its return value
 * 
 * This macro executes the provided function and if it returns false,
 * immediately returns false from the calling function.
 * 
 * @param func The function to execute
 */
#define TEST_RUN(func) do { \
    if (!(func)) { \
        return false; \
    } \
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