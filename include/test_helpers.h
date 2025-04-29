#pragma once

#include <Arduino.h>

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