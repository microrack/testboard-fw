#include "test_helpers.h"
#include "board.h"
#include "hal.h"
#include "display.h"

power_rails_state_t get_power_rails_state(bool& p12v_state, bool& p5v_state, bool& m12v_ok) {
    // Read power rail states from MCP
    p12v_state = mcp1.digitalRead(PIN_P12V_PASS);
    p5v_state = mcp1.digitalRead(PIN_P5V_PASS);
    m12v_ok = !mcp1.digitalRead(PIN_M12V_PASS); // Inverted signal

    ESP_LOGD("test_helpers", "Power rail states - +12V: %d, +5V: %d, -12V: %d", 
             p12v_state, p5v_state, m12v_ok);

    // Count how many rails are connected
    int connected_rails = 0;
    if (p12v_state) connected_rails++;
    if (p5v_state) connected_rails++;
    if (m12v_ok) connected_rails++;

    ESP_LOGD("test_helpers", "Connected rails count: %d", connected_rails);

    // Return appropriate state based on number of connected rails
    power_rails_state_t state;
    if (connected_rails == 0) {
        state = POWER_RAILS_NONE;
    } else if (connected_rails == 3) {
        state = POWER_RAILS_ALL;
    } else {
        state = POWER_RAILS_PARTIAL;
    }

    ESP_LOGD("test_helpers", "Power rail state: %s", 
             state == POWER_RAILS_NONE ? "NONE" : 
             state == POWER_RAILS_ALL ? "ALL" : "PARTIAL");

    return state;
}

bool perform_startup_sequence() {
    // Step 1: Switch off both LEDs
    mcp1.digitalWrite(PIN_LED_OK, LOW);
    mcp1.digitalWrite(PIN_LED_FAIL, LOW);

    // Step 2: Wait for adapter
    uint8_t adapter_id = hal_adapter_id();
    if (adapter_id == 0b11111) {
        display_printf("Waiting for adapter...");

        while (hal_adapter_id() == 0b11111) {
            delay(100);
        }
        adapter_id = hal_adapter_id();
    }

    // Adapter detected
    display_printf("Adapter detected: 0x%02X", adapter_id);

    // Step 4: Wait for module removal for calibration
    bool p12v_ok, p5v_ok, m12v_ok;
    power_rails_state_t rails_state = get_power_rails_state(p12v_ok, p5v_ok, m12v_ok);

    if (rails_state != POWER_RAILS_NONE) {
        display_printf("Remove module for calibration");

        do {
            rails_state = get_power_rails_state(p12v_ok, p5v_ok, m12v_ok);
            delay(100);
        } while (rails_state != POWER_RAILS_NONE);
    }

    // Step 5: Perform calibration
    display_printf("Calibrating...");
    
    hal_current_calibrate();
    hal_adc_calibrate();
    
    ESP_LOGI("test_helpers", "Calibration complete");
    return true;
}

power_rails_state_t wait_for_module_insertion(bool& p12v_ok, bool& p5v_ok, bool& m12v_ok) {
    power_rails_state_t rails_state;
    do {
        rails_state = get_power_rails_state(p12v_ok, p5v_ok, m12v_ok);
        delay(100);
    } while (rails_state != POWER_RAILS_ALL);
    return rails_state;
}

power_rails_state_t wait_for_module_removal(bool& p12v_ok, bool& p5v_ok, bool& m12v_ok) {
    power_rails_state_t rails_state;
    do {
        rails_state = get_power_rails_state(p12v_ok, p5v_ok, m12v_ok);
        delay(100);
    } while (rails_state != POWER_RAILS_NONE);
    return rails_state;
}

bool check_initial_current_consumption(const power_rails_current_ranges_t& ranges) {
    ESP_LOGI("test_helpers", "Checking initial current consumption");

    // Measure currents on all rails
    int32_t current_p12v = measure_current(PIN_INA_12V);
    int32_t current_m12v = measure_current(PIN_INA_M12V);
    int32_t current_p5v = measure_current(PIN_INA_5V);

    // Convert to mA for logging and comparison
    float p12v_ma = current_p12v / 1000.0f;
    float m12v_ma = current_m12v / 1000.0f;
    float p5v_ma = current_p5v / 1000.0f;

    ESP_LOGI("test_helpers", "Current measurements:\n+12V: %.2f mA\n-12V: %.2f mA\n+5V: %.2f mA",
             p12v_ma, m12v_ma, p5v_ma);

    // Check if all currents are within acceptable ranges
    bool p12v_ok = (p12v_ma >= ranges.p12v.min && p12v_ma <= ranges.p12v.max);
    bool m12v_ok = (m12v_ma >= ranges.m12v.min && m12v_ma <= ranges.m12v.max);
    bool p5v_ok = (p5v_ma >= ranges.p5v.min && p5v_ma <= ranges.p5v.max);

    if (!p12v_ok) {
        ESP_LOGE("test_helpers", "+12V current out of range: %.2f mA (expected %.2f-%.2f mA)",
                 p12v_ma, ranges.p12v.min, ranges.p12v.max);
    }
    if (!m12v_ok) {
        ESP_LOGE("test_helpers", "-12V current out of range: %.2f mA (expected %.2f-%.2f mA)",
                 m12v_ma, ranges.m12v.min, ranges.m12v.max);
    }
    if (!p5v_ok) {
        ESP_LOGE("test_helpers", "+5V current out of range: %.2f mA (expected %.2f-%.2f mA)",
                 p5v_ma, ranges.p5v.min, ranges.p5v.max);
    }

    return p12v_ok && m12v_ok && p5v_ok;
} 