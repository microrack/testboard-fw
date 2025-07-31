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

bool check_current(ina_pin_t pin, const range_t& range, const char* rail_name) {
    ESP_LOGI(TAG, "Checking current on %s rail", rail_name);

    // Measure current
    int32_t current_ua = measure_current(pin);
    
    // Convert to milliamps for comparison
    int32_t current_ma = current_ua / 1000;
    
    ESP_LOGI(TAG, "%s current: %d mA (acceptable range: %d-%d uA)",
             rail_name, current_ma, range.min, range.max);
    
    // Check if current is within acceptable range
    bool current_ok = (current_ua >= range.min && current_ua <= range.max);
    
    if (!current_ok) {
        ESP_LOGE(TAG, "%s current out of range: %d uA (expected %d-%d uA)",
                 rail_name, current_ua, range.min, range.max);
        display_printf("%s current out of range\n%d uA (%d-%d uA)",
                      rail_name, current_ua, range.min, range.max);
    }
    
    return current_ok;
}

bool check_initial_current_consumption(const power_rails_current_ranges_t& ranges) {
    ESP_LOGI(TAG, "Checking initial current consumption on all rails");
    
    bool p12v_ok = check_current(INA_PIN_12V, ranges.p12v, "+12V");
    bool m12v_ok = check_current(INA_PIN_M12V, ranges.m12v, "-12V");
    bool p5v_ok = check_current(INA_PIN_5V, ranges.p5v, "+5V");
    
    return p12v_ok && m12v_ok && p5v_ok;
}

bool test_pin_range(ADC_sink_t pin, const range_t& range, const char* pin_name) {
    ESP_LOGI(TAG, "Testing %s", pin_name);

    // Measure voltage for the pin
    int32_t voltage_mv = hal_adc_read(pin);

    ESP_LOGI(TAG, "%s voltage: %d mV (acceptable range: %d-%d mV)",
             pin_name, voltage_mv, range.min, range.max);

    // Check voltage range
    if (voltage_mv < range.min || voltage_mv > range.max) {
        ESP_LOGE(TAG, "%s voltage out of range: %d mV (expected %d-%d mV)",
                 pin_name, voltage_mv, range.min, range.max);
        display_printf("%s voltage out of range\n%d mV (%d-%d mV)", 
                      pin_name, voltage_mv, range.min, range.max);
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

    ESP_LOGI(TAG, "%s source:\nHigh-Z: %.2f V (acceptable range: %.2f-%.2f V)\nPull-down: %.2f V (acceptable range: %.2f-%.2f V)",
             source_name, v_hiz, hiz_range.min, hiz_range.max, v_pd, pd_range.min, pd_range.max);

    // Check if voltages are within expected ranges
    if (v_hiz < hiz_range.min || v_hiz > hiz_range.max) {
        ESP_LOGE(TAG, "%s source high-Z voltage out of range: %.2f V (expected %.2f-%.2f V)",
                 source_name, v_hiz, hiz_range.min, hiz_range.max);
        display_printf("%s high-Z out of range\n%.2f V (%.2f-%.2f V)", 
                      source_name, v_hiz, hiz_range.min, hiz_range.max);
        return false;
    }
    if (v_pd < pd_range.min || v_pd > pd_range.max) {
        ESP_LOGE(TAG, "%s source pull-down voltage out of range: %.2f V (expected %.2f-%.2f V)",
                 source_name, v_pd, pd_range.min, pd_range.max);
        display_printf("%s pull-down out of range\n%.2f V (%.2f-%.2f V)", 
                      source_name, v_pd, pd_range.min, pd_range.max);
        return false;
    }

    return true;
}

bool test_mode(const int led_pin1, const int led_pin2, const mode_current_ranges_t& ranges, int* output_mode) {
    ESP_LOGI(TAG, "Testing mode");

    mcp0.pinMode(led_pin1, OUTPUT);
    mcp0.digitalWrite(led_pin1, LOW);
    mcp0.pinMode(led_pin2, OUTPUT);
    mcp0.digitalWrite(led_pin2, LOW);
    delay(1);
    mcp0.pinMode(led_pin1, INPUT_PULLUP);
    mcp0.pinMode(led_pin2, INPUT_PULLUP);

    // Check initial levels - both should be 0
    bool pin1 = mcp0.digitalRead(led_pin1);
    bool pin2 = mcp0.digitalRead(led_pin2);

    if (pin1 != 0 || pin2 != 0) {
        ESP_LOGE(TAG, "Error: Initial LED levels incorrect. Pin1: %d, Pin2: %d", pin1, pin2);
        display_printf("LED levels incorrect\nPin1: %d Pin2: %d", pin1, pin2);
        return false;
    }

    // Test first pin
    mcp0.pinMode(led_pin1, OUTPUT);
    mcp0.digitalWrite(led_pin1, LOW);
    delay(10);  // Wait for current to stabilize
    int32_t current_pin1 = measure_current(PIN_INA_5V);
    mcp0.pinMode(led_pin1, INPUT_PULLUP);

    // Test second pin
    mcp0.pinMode(led_pin2, OUTPUT);
    mcp0.digitalWrite(led_pin2, LOW);
    delay(10);  // Wait for current to stabilize
    int32_t current_pin2 = measure_current(PIN_INA_5V);
    mcp0.pinMode(led_pin2, INPUT_PULLUP);

    // Convert to mA
    float current_pin1_ma = current_pin1 / 1000.0f;
    float current_pin2_ma = current_pin2 / 1000.0f;

    // Print current consumption
    ESP_LOGI(TAG, "Current consumption on 5V rail:");
    ESP_LOGI(TAG, "Pin1: %.2f mA (active range: %.2f-%.2f mA, inactive range: %.2f-%.2f mA)",
             current_pin1_ma, ranges.active.min, ranges.active.max, ranges.inactive.min, ranges.inactive.max);
    ESP_LOGI(TAG, "Pin2: %.2f mA (active range: %.2f-%.2f mA, inactive range: %.2f-%.2f mA)",
             current_pin2_ma, ranges.active.min, ranges.active.max, ranges.inactive.min, ranges.inactive.max);

    // Determine mode based on current measurements
    if (current_pin1_ma >= ranges.active.min && current_pin1_ma <= ranges.active.max && 
        current_pin2_ma >= ranges.inactive.min && current_pin2_ma <= ranges.inactive.max) {
        ESP_LOGI(TAG, "Mode set to Pin1 active");
        if (output_mode) *output_mode = 0;
        return true;
    } else if (current_pin1_ma >= ranges.inactive.min && current_pin1_ma <= ranges.inactive.max && 
               current_pin2_ma >= ranges.active.min && current_pin2_ma <= ranges.active.max) {
        ESP_LOGI(TAG, "Mode set to Pin2 active");
        if (output_mode) *output_mode = 1;
        return true;
    }

    ESP_LOGE(TAG, "Invalid current combination:\nPin1: %.2f mA\nPin2: %.2f mA", 
             current_pin1_ma, current_pin2_ma);
    display_printf("Invalid currents\nPin1: %.2f mA\nPin2: %.2f mA", 
                  current_pin1_ma, current_pin2_ma);
    return false;
}

bool execute_test_sequence(const test_operation_t* operations, size_t count) {
    ESP_LOGI(TAG, "Executing test sequence with %zu operations", count);
    
    for (size_t i = 0; i < count; i++) {
        const test_operation_t& op = operations[i];
        bool result = false;
        
        // Handle repeatable operations
        if (op.repeat) {
            do {
                result = execute_single_operation(op);
                if (!result) {
                    delay(50); // Short delay before retry
                }
            } while (!result);
        } else {
            result = execute_single_operation(op);
        }
        
        if (!result) {
            ESP_LOGE(TAG, "Test operation %zu failed", i);
            return false;
        }
        
        // Small delay between operations
        delay(1);
    }
    
    ESP_LOGI(TAG, "Test sequence completed successfully");
    return true;
}

// Helper function to execute a single test operation
bool execute_single_operation(const test_operation_t& op) {
    switch (op.op) {
        case TEST_OP_SOURCE: {
            hal_set_source((source_net_t)op.pin, op.arg1);
            return true;
        }
        
        case TEST_OP_IO: {
            hal_set_io((mcp_io_t)op.pin, (io_state_t)op.arg1);
            return true;
        }
        
        case TEST_OP_SINK_PD: {
            // Assuming PIN_SINK_PD_A is the pin for sink pulldown
            mcp1.pinMode(PIN_SINK_PD_A, OUTPUT);
            mcp1.digitalWrite(PIN_SINK_PD_A, op.arg1 ? HIGH : LOW);
            return true;
        }
        
        case TEST_OP_CHECK_CURRENT: {
            range_t range = {op.arg1, op.arg2};
            const char* rail_name = (op.pin == INA_PIN_12V) ? "+12V" : 
                                   (op.pin == INA_PIN_5V) ? "+5V" : "-12V";
            return check_current((ina_pin_t)op.pin, range, rail_name);
        }
        
        case TEST_OP_CHECK_PIN: {
            range_t range = {op.arg1, op.arg2};
            const char* pin_name = (op.pin == ADC_sink_1k_A) ? "A" :
                                  (op.pin == ADC_sink_1k_B) ? "B" :
                                  (op.pin == ADC_sink_1k_C) ? "C" :
                                  (op.pin == ADC_sink_1k_D) ? "D" :
                                  (op.pin == ADC_sink_1k_E) ? "E" :
                                  (op.pin == ADC_sink_1k_F) ? "F" : "Unknown";
            return test_pin_range((ADC_sink_t)op.pin, range, pin_name);
        }
        
        default:
            ESP_LOGE(TAG, "Unknown test operation type: %d", op.op);
            return false;
    }
} 