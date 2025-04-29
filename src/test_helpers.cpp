#include "test_helpers.h"
#include "board.h"
#include "hal.h"

bool get_power_rails_state(bool& p12v_state, bool& p5v_state, bool& m12v_state) {
    // Read power rail states from MCP
    p12v_state = mcp0.digitalRead(PIN_P12V_PASS);
    p5v_state = mcp0.digitalRead(PIN_P5V_PASS);
    m12v_state = !mcp0.digitalRead(PIN_M12V_PASS); // Inverted signal

    // Return true only if all rails are OK
    return p12v_state && p5v_state && m12v_state;
} 