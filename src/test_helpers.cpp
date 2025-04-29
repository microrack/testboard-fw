#include "test_helpers.h"
#include "board.h"
#include "hal.h"

power_rails_state_t get_power_rails_state(bool& p12v_state, bool& p5v_state, bool& m12v_ok) {
    // Read power rail states from MCP
    p12v_state = mcp0.digitalRead(PIN_P12V_PASS);
    p5v_state = mcp0.digitalRead(PIN_P5V_PASS);
    m12v_ok = !mcp0.digitalRead(PIN_M12V_PASS); // Inverted signal

    // Count how many rails are connected
    int connected_rails = 0;
    if (p12v_state) connected_rails++;
    if (p5v_state) connected_rails++;
    if (m12v_ok) connected_rails++;

    // Return appropriate state based on number of connected rails
    if (connected_rails == 0) {
        return POWER_RAILS_NONE;
    } else if (connected_rails == 3) {
        return POWER_RAILS_ALL;
    } else {
        return POWER_RAILS_PARTIAL;
    }
} 