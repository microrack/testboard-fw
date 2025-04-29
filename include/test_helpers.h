#pragma once

#include <Arduino.h>

/**
 * @brief Checks the state of all power rails
 * 
 * @param p12v_state Output parameter for +12V rail state (true if OK)
 * @param p5v_state Output parameter for +5V rail state (true if OK)
 * @param m12v_state Output parameter for -12V rail state (true if OK)
 * @return true if all power rails are OK
 * @return false if any power rail is not OK
 */
bool get_power_rails_state(bool& p12v_state, bool& p5v_state, bool& m12v_state); 