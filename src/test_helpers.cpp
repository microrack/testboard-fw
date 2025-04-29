#include "test_helpers.h"
#include "board.h"
#include "hal.h"
#include "display.h"

static const char* TAG = "test_helpers";

power_rails_state_t get_power_rails_state(bool* p12v_state, bool* p5v_state, bool* m12v_state) {
    // Create local variables to store the states
    bool p12v = mcp1.digitalRead(PIN_P12V_PASS);
    bool p5v = mcp1.digitalRead(PIN_P5V_PASS);
    bool m12v = !mcp1.digitalRead(PIN_M12V_PASS); // Inverted signal

    // Only write to output parameters if they are not NULL
    if (p12v_state) *p12v_state = p12v;
    if (p5v_state) *p5v_state = p5v;
    if (m12v_state) *m12v_state = m12v;

    // Determine overall state
    if (p12v && p5v && m12v) {
        return POWER_RAILS_ALL;
    } else if (!p12v && !p5v && !m12v) {
        return POWER_RAILS_NONE;
    } else {
        return POWER_RAILS_PARTIAL;
    }
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
    power_rails_state_t rails_state = get_power_rails_state(&p12v_ok, &p5v_ok, &m12v_ok);

    if (rails_state != POWER_RAILS_NONE) {
        display_printf("Remove module for calibration");

        do {
            rails_state = get_power_rails_state(&p12v_ok, &p5v_ok, &m12v_ok);
            delay(100);
        } while (rails_state != POWER_RAILS_NONE);
    }

    // Step 5: Perform calibration
    display_printf("Calibrating...");
    
    hal_current_calibrate();
    hal_adc_calibrate();
    
    ESP_LOGI(TAG, "Calibration complete");
    return true;
}

power_rails_state_t wait_for_module_insertion(bool& p12v_ok, bool& p5v_ok, bool& m12v_ok) {
    power_rails_state_t rails_state;
    do {
        rails_state = get_power_rails_state(&p12v_ok, &p5v_ok, &m12v_ok);
        delay(100);
    } while (rails_state != POWER_RAILS_ALL);
    return rails_state;
}

power_rails_state_t wait_for_module_removal(bool& p12v_ok, bool& p5v_ok, bool& m12v_ok) {
    power_rails_state_t rails_state;
    do {
        rails_state = get_power_rails_state(&p12v_ok, &p5v_ok, &m12v_ok);
        delay(100);
    } while (rails_state != POWER_RAILS_NONE);
    return rails_state;
}

bool check_initial_current_consumption(const power_rails_current_ranges_t& ranges) {
    ESP_LOGI(TAG, "Checking initial current consumption");

    // Measure currents on all rails
    int32_t current_p12v = measure_current(PIN_INA_12V);
    int32_t current_m12v = measure_current(PIN_INA_M12V);
    int32_t current_p5v = measure_current(PIN_INA_5V);

    // Convert to mA for logging and comparison
    float p12v_ma = current_p12v / 1000.0f;
    float m12v_ma = current_m12v / 1000.0f;
    float p5v_ma = current_p5v / 1000.0f;

    ESP_LOGI(TAG, "Current measurements:\n+12V: %.2f mA\n-12V: %.2f mA\n+5V: %.2f mA",
             p12v_ma, m12v_ma, p5v_ma);

    // Check if all currents are within acceptable ranges
    bool p12v_ok = (p12v_ma >= ranges.p12v.min && p12v_ma <= ranges.p12v.max);
    bool m12v_ok = (m12v_ma >= ranges.m12v.min && m12v_ma <= ranges.m12v.max);
    bool p5v_ok = (p5v_ma >= ranges.p5v.min && p5v_ma <= ranges.p5v.max);

    if (!p12v_ok) {
        ESP_LOGE(TAG, "+12V current out of range: %.2f mA (expected %.2f-%.2f mA)",
                 p12v_ma, ranges.p12v.min, ranges.p12v.max);
    }
    if (!m12v_ok) {
        ESP_LOGE(TAG, "-12V current out of range: %.2f mA (expected %.2f-%.2f mA)",
                 m12v_ma, ranges.m12v.min, ranges.m12v.max);
    }
    if (!p5v_ok) {
        ESP_LOGE(TAG, "+5V current out of range: %.2f mA (expected %.2f-%.2f mA)",
                 p5v_ma, ranges.p5v.min, ranges.p5v.max);
    }

    return p12v_ok && m12v_ok && p5v_ok;
}

bool test_pin_range(ADC_sink_t pin, const range_t& range, const char* pin_name) {
    ESP_LOGI(TAG, "Testing %s", pin_name);

    // Measure voltage for the pin
    int32_t voltage_raw = hal_adc_read(pin);
    float voltage = voltage_raw / 1000.0f;  // Convert to volts

    ESP_LOGI(TAG, "%s voltage: %.2f V", pin_name, voltage);

    // Check voltage range
    if (voltage < range.min || voltage > range.max) {
        ESP_LOGE(TAG, "%s voltage out of range: %.2f V", pin_name, voltage);
        display_printf("%s voltage out of range\n%.2f V", pin_name, voltage);
        return false;
    }

    return true;
}

bool test_pin_pd(const voltage_source_t& source, 
                const range_t& hiz_range, const range_t& pd_range,
                const char* source_name) {
    ESP_LOGI(TAG, "Testing %s source", source_name);

    // Configure PD sink pin as output
    mcp1.pinMode(source.pd_pin, OUTPUT);

    // Test high impedance state
    mcp1.digitalWrite(source.pd_pin, LOW);  // Pull-down inactive
    delay(10);  // Wait for voltage to stabilize
    int32_t voltage_hiz = hal_adc_read(source.adc_pin);
    
    // Test pull-down state
    mcp1.digitalWrite(source.pd_pin, HIGH);  // Pull-down active
    delay(10);  // Wait for voltage to stabilize
    int32_t voltage_pd = hal_adc_read(source.adc_pin);

    // Convert to volts
    float v_hiz = voltage_hiz / 1000.0f;
    float v_pd = voltage_pd / 1000.0f;

    ESP_LOGI(TAG, "%s source:\nHigh-Z: %.2f V\nPull-down: %.2f V",
             source_name, v_hiz, v_pd);

    // Check if voltages are within expected ranges
    if (v_hiz < hiz_range.min || v_hiz > hiz_range.max) {
        ESP_LOGE(TAG, "%s source high-Z voltage out of range", source_name);
        display_printf("%s high-Z out of range\n%.2f V", source_name, v_hiz);
        return false;
    }
    if (v_pd < pd_range.min || v_pd > pd_range.max) {
        ESP_LOGE(TAG, "%s source pull-down voltage out of range", source_name);
        display_printf("%s pull-down out of range\n%.2f V", source_name, v_pd);
        return false;
    }

    return true;
} 