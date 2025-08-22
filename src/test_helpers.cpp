#include "test_helpers.h"
#include "board.h"
#include "hal.h"
#include "display.h"
#include "test_results.h"
#include <LittleFS.h>
#include <esp_log.h>

static const char* TAG = "test_helpers";

// Global Sigscoper instance and state
Sigscoper global_sigscoper;
int last_scope_pin = -1;
bool sigscoper_initialized = false;

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
    ESP_LOGI(TAG, "Starting startup sequence");
    
    // Step 1: Initialize HAL
    hal_init();

    // Step 2: Initialize display
    if (!display_init()) {
        ESP_LOGE(TAG, "Failed to initialize display");
        return false;
    }
    
    // Step 3: Initialize modules
    if (!init_modules_from_fs()) {
        ESP_LOGE(TAG, "Failed to initialize modules from filesystem");
        return false;
    }
    
    // Step 4: Initialize filesystem
    if (!LittleFS.begin(true)) {
        ESP_LOGE(TAG, "Failed to initialize filesystem");
        return false;
    }

    // Step 5: Calibrate sensors
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
        return false;
    }
    if (v_pd < pd_range.min || v_pd > pd_range.max) {
        ESP_LOGE(TAG, "%s source pull-down voltage out of range: %.2f V (expected %.2f-%.2f V)",
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
    return false;
}

bool execute_test_sequence(const test_operation_t* operations, size_t count, test_operation_result_t* results) {
    // ESP_LOGI(TAG, "Executing test sequence with %zu operations", count);
    
    for (size_t i = 0; i < count; i++) {
        // ESP_LOGI(TAG, "Start of operation %d", i);
        const test_operation_t& op = operations[i];
        bool result = false;
        int32_t actual_result = 0;
        
        // Handle repeatable operations using TEST_RUN_REPEAT logic
        if (op.repeat) {
            do {
                result = execute_single_operation(op, &actual_result);
                if (!result) {
                    mcp1.digitalWrite(PIN_LED_FAIL, HIGH);
                    mcp1.digitalWrite(PIN_LED_OK, LOW);
                    if (get_power_rails_state(NULL, NULL, NULL) != POWER_RAILS_ALL) {
                        ESP_LOGE(TAG, "Power rails disconnected during repeatable operation");
                        results[i].passed = false;
                        results[i].result = actual_result;
                        return false;
                    }
                    delay(10);
                }
            } while (!result);
            mcp1.digitalWrite(PIN_LED_FAIL, LOW);
        } else {
            result = execute_single_operation(op, &actual_result);
            if(!results[i].passed) {
                results[i].result = actual_result;
            }
            results[i].passed = result;
            if (!result) {
                mcp1.digitalWrite(PIN_LED_FAIL, HIGH);
                mcp1.digitalWrite(PIN_LED_OK, LOW);
                return false;
            }
        }
        
        // Small delay between operations
        delay(1);
    }

    return true;
}

// Helper function to map current measurement pin numbers from JSON to actual pins
int map_current_pin(int pin) {
    switch (pin) {
        case 0: return PIN_INA_12V;   // +12V rail
        case 1: return PIN_INA_5V;    // +5V rail
        case 2: return PIN_INA_M12V;  // -12V rail
        default: return pin;           // Return as-is for other pins
    }
}

// Helper function to execute a single test operation
bool execute_single_operation(const test_operation_t& op, int32_t* result) {
    // ESP_LOGI(TAG, "Start of execute_single_operation: %d", op.op);
    switch (op.op) {
        case TEST_OP_SOURCE: {
            hal_set_source((source_net_t)op.pin, op.arg1);
            return true;
        }
        
        case TEST_OP_SOURCE_SIG: {
            hal_start_signal((source_net_t)op.pin, (float)op.arg1);
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
            int mapped_pin = map_current_pin(op.pin);
            const char* rail_name = (mapped_pin == PIN_INA_12V) ? "+12V" : 
                                   (mapped_pin == PIN_INA_5V) ? "+5V" : "-12V";
            if (result) {
                *result = measure_current(mapped_pin);
                ESP_LOGI(TAG, "Current measurement: %d uA", *result);
            }
            return check_current((ina_pin_t)mapped_pin, range, rail_name);
        }
        
        case TEST_OP_CHECK_PIN: {
            range_t range = {op.arg1, op.arg2};
            const char* pin_name = (op.pin == ADC_sink_1k_A) ? "A" :
                                  (op.pin == ADC_sink_1k_B) ? "B" :
                                  (op.pin == ADC_sink_1k_C) ? "C" :
                                  (op.pin == ADC_sink_1k_D) ? "D" :
                                  (op.pin == ADC_sink_1k_E) ? "E" :
                                  (op.pin == ADC_sink_1k_F) ? "F" :
                                  (op.pin == ADC_sink_PD_A) ? "pdA" :
                                  (op.pin == ADC_sink_PD_B) ? "pdB" :
                                  (op.pin == ADC_sink_PD_C) ? "pdC" :
                                  (op.pin == ADC_sink_Z_D) ? "zD" :
                                  (op.pin == ADC_sink_Z_E) ? "zE" :
                                  (op.pin == ADC_sink_Z_F) ? "zF" : "Unknown";
            if (result) {
                // Get actual voltage value for failed test
                *result = hal_adc_read((ADC_sink_t)op.pin); // Returns value in millivolts
                // ESP_LOGI(TAG, "Voltage measurement: %d mV", *result);
            }
            return test_pin_range((ADC_sink_t)op.pin, range, pin_name);
        }
        
        case TEST_OP_RESET: {
            return execute_reset_operation();
        }
        
        case TEST_OP_SCOPE: {
            return start_sigscoper((ADC_sink_t)op.pin, op.arg1, op.arg2);
        }
        
        case TEST_OP_CHECK_MIN: {
            range_t range = {op.arg1, op.arg2};
            if (result) {
                *result = 0; // Initialize result
            }
            return check_signal_min((ADC_sink_t)op.pin, range, result);
        }
        
        case TEST_OP_CHECK_MAX: {
            range_t range = {op.arg1, op.arg2};
            if (result) {
                *result = 0; // Initialize result
            }
            return check_signal_max((ADC_sink_t)op.pin, range, result);
        }
        
        case TEST_OP_CHECK_AVG: {
            range_t range = {op.arg1, op.arg2};
            if (result) {
                *result = 0; // Initialize result
            }
            return check_signal_avg((ADC_sink_t)op.pin, range, result);
        }
        
        case TEST_OP_CHECK_FREQ: {
            range_t range = {op.arg1, op.arg2};
            if (result) {
                *result = 0; // Initialize result
            }
            return check_signal_freq((ADC_sink_t)op.pin, range, result);
        }
        
        case TEST_OP_CHECK_AMPLITUDE: {
            range_t range = {op.arg1, op.arg2};
            if (result) {
                *result = 0; // Initialize result
            }
            return check_signal_amplitude((ADC_sink_t)op.pin, range, result);
        }
        
        case TEST_OP_DELAY: {
            ESP_LOGI(TAG, "Executing delay operation: %d ms", op.arg1);
            delay(op.arg1);
            return true;
        }
        
        default:
            ESP_LOGE(TAG, "Unknown test operation type: %d", op.op);
            return false;
    }
}

bool execute_reset_operation() {
    ESP_LOGI(TAG, "Executing reset operation - setting all pins to safe state");
    
    // 1. Set all IO pins to HiZ (input mode)
    for (int pin = 0; pin <= 15; pin++) {
        hal_set_io((mcp_io_t)pin, IO_INPUT);
    }
    
    // 2. Set all voltage sources to 0V
    hal_set_source(SOURCE_A, 0);
    hal_set_source(SOURCE_B, 0);
    hal_set_source(SOURCE_C, 0);
    hal_set_source(SOURCE_D, 0);
    
    // 3. Disable all pulldowns
    // Set pulldown pins to high-Z (input mode)
    mcp1.pinMode(PIN_SINK_PD_A, OUTPUT);
    mcp1.pinMode(PIN_SINK_PD_B, OUTPUT);
    mcp1.pinMode(PIN_SINK_PD_C, OUTPUT);

    mcp1.pinMode(PIN_SINK_PD_A, LOW);
    mcp1.pinMode(PIN_SINK_PD_B, LOW);
    mcp1.pinMode(PIN_SINK_PD_C, LOW);
    
    // ESP_LOGI(TAG, "Reset operation completed successfully");
    
    // Stop Sigscoper if running
    if (global_sigscoper.is_running()) {
        global_sigscoper.stop();
    }
    
    return true;
}

// Helper function to convert ADC_sink_t to adc_unit_t
adc_unit_t adc_sink_to_unit(ADC_sink_t pin) {
    switch (pin) {
        case ADC_sink_1k_A: return ADC_UNIT_1;  // GPIO36 -> ADC1
        case ADC_sink_1k_B: return ADC_UNIT_1;  // GPIO39 -> ADC1
        case ADC_sink_1k_C: return ADC_UNIT_1;  // GPIO34 -> ADC1
        case ADC_sink_1k_D: return ADC_UNIT_1;  // GPIO35 -> ADC1
        case ADC_sink_1k_E: return ADC_UNIT_1;  // GPIO32 -> ADC1
        case ADC_sink_1k_F: return ADC_UNIT_1;  // GPIO33 -> ADC1
        case ADC_sink_PD_A: return ADC_UNIT_2;  // GPIO25 -> ADC2
        case ADC_sink_PD_B: return ADC_UNIT_2;  // GPIO26 -> ADC2
        case ADC_sink_PD_C: return ADC_UNIT_2;  // GPIO27 -> ADC2
        case ADC_sink_Z_D: return ADC_UNIT_2;   // GPIO14 -> ADC2
        case ADC_sink_Z_E: return ADC_UNIT_2;   // GPIO12 -> ADC2
        case ADC_sink_Z_F: return ADC_UNIT_2;   // GPIO13 -> ADC2
        default: return ADC_UNIT_1;
    }
}

// Helper function to convert ADC_sink_t to adc_channel_t
static adc_channel_t adc_sink_to_channel(ADC_sink_t pin) {
    switch (pin) {
        case ADC_sink_1k_A: return ADC_CHANNEL_0;  // GPIO36
        case ADC_sink_1k_B: return ADC_CHANNEL_3;  // GPIO39
        case ADC_sink_1k_C: return ADC_CHANNEL_6;  // GPIO34
        case ADC_sink_1k_D: return ADC_CHANNEL_7;  // GPIO35
        case ADC_sink_1k_E: return ADC_CHANNEL_4;  // GPIO32
        case ADC_sink_1k_F: return ADC_CHANNEL_5;  // GPIO33
        case ADC_sink_PD_A: return ADC_CHANNEL_8;  // GPIO25
        case ADC_sink_PD_B: return ADC_CHANNEL_9;  // GPIO26
        case ADC_sink_PD_C: return ADC_CHANNEL_7;  // GPIO27 (Note: conflicts with 1k_D)
        case ADC_sink_Z_D: return ADC_CHANNEL_5;   // GPIO14
        case ADC_sink_Z_E: return ADC_CHANNEL_4;   // GPIO12
        case ADC_sink_Z_F: return ADC_CHANNEL_6;   // GPIO13
        default: return ADC_CHANNEL_0;
    }
}

// Helper function to get pin name for display
static const char* get_pin_name(ADC_sink_t pin) {
    switch (pin) {
        case ADC_sink_1k_A: return "A";
        case ADC_sink_1k_B: return "B";
        case ADC_sink_1k_C: return "C";
        case ADC_sink_1k_D: return "D";
        case ADC_sink_1k_E: return "E";
        case ADC_sink_1k_F: return "F";
        case ADC_sink_PD_A: return "pdA";
        case ADC_sink_PD_B: return "pdB";
        case ADC_sink_PD_C: return "pdC";
        case ADC_sink_Z_D: return "zD";
        case ADC_sink_Z_E: return "zE";
        case ADC_sink_Z_F: return "zF";
        default: return "Unknown";
    }
}

// Helper function for common signal checking logic
static bool check_signal_common(ADC_sink_t pin, SigscoperStats* stats) {
    // Check if scope was started with the same pin
    if (last_scope_pin != pin) {
        ESP_LOGE(TAG, "Scope was not started with pin %s (last pin: %d)", 
                 get_pin_name(pin), last_scope_pin);
        return false;
    }

    // ESP_LOGI(TAG, "Checking signal on pin %s", get_pin_name(pin));
    
    // Wait for acquisition to complete
    while (!global_sigscoper.is_ready()) {
        delay(10);
    }

    // ESP_LOGI(TAG, "Acquisition completed for pin %s", get_pin_name(pin));
    
    // Get statistics
    if (!global_sigscoper.get_stats(0, stats)) {
        ESP_LOGE(TAG, "Failed to get statistics from Sigscoper");
        return false;
    }
    
    return true;
}

// Function to start Sigscoper in FREE mode
bool start_sigscoper(ADC_sink_t pin, uint32_t sample_freq, size_t buffer_size) {
    ESP_LOGI(TAG, "Starting Sigscoper on pin %s, freq: %d Hz, buffer: %d", 
             get_pin_name(pin), sample_freq, buffer_size);
    
    // Initialize Sigscoper if not already done
    if (!sigscoper_initialized) {
        if (!global_sigscoper.begin()) {
            ESP_LOGE(TAG, "Failed to initialize Sigscoper");
            return false;
        }
        sigscoper_initialized = true;
    }
    
    // Wait for previous acquisition
    if (global_sigscoper.is_running()) {
        global_sigscoper.stop();
    }
    
    // Configure Sigscoper
    SigscoperConfig config;
    config.channel_count = 1;
    config.channels[0] = adc_sink_to_channel(pin);
    config.adc_unit = adc_sink_to_unit(pin);  // Set ADC unit based on pin
    config.trigger_mode = TriggerMode::FREE;
    config.trigger_level = 2048;  // Default trigger level
    config.sampling_rate = sample_freq;
    config.auto_speed = 0.002f;   // Default auto speed
    config.buffer_size = buffer_size;
    
    // Start acquisition
    if (!global_sigscoper.start(config)) {
        ESP_LOGE(TAG, "Failed to start Sigscoper");
        return false;
    }
    
    last_scope_pin = pin;
    // ESP_LOGI(TAG, "Sigscoper started successfully");
    return true;
}



// Function to check signal minimum value
bool check_signal_min(ADC_sink_t pin, const range_t& range, int32_t* result) {
    SigscoperStats stats;
    if (!check_signal_common(pin, &stats)) {
        return false;
    }
    
    int32_t value = hal_adc_raw2mv(stats.min_value, pin);
    
    // Store result value
    if (result) {
        *result = value;
    }
    
    ESP_LOGI(TAG, "min on pin %s: %d (acceptable range: %d-%d)",
             get_pin_name(pin), value, range.min, range.max);
    
    // Check if value is within range
    if (value < range.min || value > range.max) {
        ESP_LOGE(TAG, "min on pin %s out of range: %d (expected %d-%d)",
                 get_pin_name(pin), value, range.min, range.max);
        return false;
    }
    
    return true;
}

// Function to check signal maximum value
bool check_signal_max(ADC_sink_t pin, const range_t& range, int32_t* result) {    
    SigscoperStats stats;
    if (!check_signal_common(pin, &stats)) {
        return false;
    }
    
    int32_t value = hal_adc_raw2mv(stats.max_value, pin);
    
    // Store result value
    if (result) {
        *result = value;
    }
    
    ESP_LOGI(TAG, "max on pin %s: %d (acceptable range: %d-%d)",
             get_pin_name(pin), value, range.min, range.max);
    
    // Check if value is within range
    if (value < range.min || value > range.max) {
        ESP_LOGE(TAG, "max on pin %s out of range: %d (expected %d-%d)",
                 get_pin_name(pin), value, range.min, range.max);
        return false;
    }
    
    return true;
}

// Function to check signal average value
bool check_signal_avg(ADC_sink_t pin, const range_t& range, int32_t* result) {
    ESP_LOGI(TAG, "Checking avg on pin %s", get_pin_name(pin));
    
    SigscoperStats stats;
    if (!check_signal_common(pin, &stats)) {
        return false;
    }
    
    int32_t value = hal_adc_raw2mv(stats.avg_value, pin);
    
    // Store result value
    if (result) {
        *result = value;
    }
    
    ESP_LOGI(TAG, "avg on pin %s: %d (acceptable range: %d-%d)",
             get_pin_name(pin), value, range.min, range.max);
    
    // Check if value is within range
    if (value < range.min || value > range.max) {
        ESP_LOGE(TAG, "avg on pin %s out of range: %d (expected %d-%d)",
                 get_pin_name(pin), value, range.min, range.max);
        return false;
    }
    
    return true;
}

// Function to check signal frequency
bool check_signal_freq(ADC_sink_t pin, const range_t& range, int32_t* result) {
    SigscoperStats stats;
    if (!check_signal_common(pin, &stats)) {
        return false;
    }
    
    float value = stats.frequency;
    // for some reason, fs at 20k looks like 16384
    value = value * 16384 / 20000;
    
    // Store result value (convert float to int32_t)
    if (result) {
        *result = (int32_t)value;
    }
    
    ESP_LOGI(TAG, "freq on pin %s: %.2f (acceptable range: %d-%d)",
             get_pin_name(pin), value, range.min, range.max);
    
    // Check if value is within range
    if (value < range.min || value > range.max) {
        ESP_LOGE(TAG, "freq on pin %s out of range: %.2f (expected %d-%d)",
                 get_pin_name(pin), value, range.min, range.max);
        return false;
    }
    
    return true;
}

// Function to check signal amplitude (max - min)
bool check_signal_amplitude(ADC_sink_t pin, const range_t& range, int32_t* result) {
    SigscoperStats stats;
    if (!check_signal_common(pin, &stats)) {
        return false;
    }
    
    // Calculate amplitude as max - min
    int32_t amplitude = hal_adc_raw2mv(stats.max_value, pin) - hal_adc_raw2mv(stats.min_value, pin);
    
    // Store result value
    if (result) {
        *result = amplitude;
    }
    
    ESP_LOGI(TAG, "amplitude on pin %s: %d (acceptable range: %d-%d)",
             get_pin_name(pin), amplitude, range.min, range.max);
    
    // Check if value is within range
    if (amplitude < range.min || amplitude > range.max) {
        ESP_LOGE(TAG, "amplitude on pin %s out of range: %d (expected %d-%d)",
                 get_pin_name(pin), amplitude, range.min, range.max);
        return false;
    }
    
    return true;
} 