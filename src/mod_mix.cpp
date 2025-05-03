#include "mod_mix.h"
#include "display.h"
#include "esp_log.h"
#include "board.h"
#include "hal.h"
#include "display.h"
#include "test_helpers.h"

static const char* TAG = "mod_mix";

const int LED_BI = IO3;
const int LED_UNI = IO4;

// Voltage ranges
const range_t UNI_VOLTAGE_RANGE = {-0.1f, 0.2f};
const float VOLTAGE_ACCURACY = 0.1f;  // Maximum allowed voltage difference between sinks

// Signal ranges
const range_t mix_active = {2.8f, 5.1f};    // Mix signal active range
const range_t mix_inactive = {-0.2f, 0.6f};  // Mix signal inactive range
const range_t channel_active = {2.8f, 5.1f};    // Channel signal active range
const range_t channel_inactive = {-0.2f, 0.6f};  // Channel signal inactive range

// Pot mappings
const ADC_sink_t POT_A = ADC_sink_Z_D;
const ADC_sink_t POT_B = ADC_sink_Z_E;
const ADC_sink_t POT_C = ADC_sink_Z_F;

// Sink arrays
const ADC_sink_t mix_sinks[] = {
    ADC_sink_1k_A,
    ADC_sink_1k_B,
    ADC_sink_1k_C,
    ADC_sink_1k_D
};
const int mix_sink_count = sizeof(mix_sinks) / sizeof(mix_sinks[0]);

const ADC_sink_t channel_sinks[] = {
    ADC_sink_1k_E,
    ADC_sink_1k_F,
    ADC_sink_PD_A
};
const int channel_sink_count = sizeof(channel_sinks) / sizeof(channel_sinks[0]);

// Sink labels for better error reporting
const char* mix_sink_labels[] = {"Mix A", "Mix B", "Mix C", "Mix D"};
const char* channel_sink_labels[] = {"Channel E", "Channel F", "Channel PD_A"};

static bool test_mix_outputs(const float* voltages);
static bool test_inputs_outputs(void);
static bool test_inactive(void);
static bool test_active_channel(int idx);
static bool check_mix_outputs(range_t range);
static void prepare_inputs_outputs(int input);
static void setup_test(void);

static bool check_mode_and_prompt() {
    mode_current_ranges_t mode_ranges = {
        .active = {21.0f, 27.0f},   // Active mode: 20-27 mA
        .inactive = {13.0f, 20.0f}  // Inactive mode: 13-18 mA
    };
    int mode;
    if(!test_mode(LED_BI, LED_UNI, mode_ranges, &mode)) {
        return false;
    }

    if (mode == 0) {
        display_printf("push the button");
        return false;
    }
    
    return true;
}

static void setup_test(void) {
    ESP_LOGI(TAG, "Setting up test environment");
    
    // Set all DAC outputs to 0V
    hal_set_source(SOURCE_A, 0.0f);
    hal_set_source(SOURCE_B, 0.0f);
    hal_set_source(SOURCE_C, 0.0f);
    hal_set_source(SOURCE_D, 0.0f);
    
    // Set both LED pins to input
    mcp0.pinMode(LED_BI, INPUT);
    mcp0.pinMode(LED_UNI, INPUT);
    
    // Set IO0..IO2 to output with logical 0
    for (int i = IO0; i <= IO2; i++) {
        mcp0.pinMode(i, OUTPUT);
        mcp0.digitalWrite(i, LOW);
    }
    
    delay(1); // Small delay to allow settings to take effect
}

static bool test_inactive(void) {
    ESP_LOGI(TAG, "Test 1: All sources low");
    
    // Check all mix sinks are inactive
    for (int i = 0; i < mix_sink_count; i++) {
        // Get the actual voltage for detailed error reporting
        int32_t voltage_raw = hal_adc_read(mix_sinks[i]);
        float voltage = voltage_raw / 1000.0f;
        
        ESP_LOGI(TAG, "%s voltage: %.2f V (inactive range: %.2f-%.2f V)", 
                mix_sink_labels[i], voltage, mix_inactive.min, mix_inactive.max);
        
        if (voltage < mix_inactive.min || voltage > mix_inactive.max) {
            ESP_LOGE(TAG, "%s voltage out of inactive range: %.2f V (expected %.2f-%.2f V)",
                     mix_sink_labels[i], voltage, mix_inactive.min, mix_inactive.max);
            display_printf("Test 1: All inputs low\n%s voltage: %.2f V\nExpected: %.2f-%.2f V", 
                          mix_sink_labels[i], voltage, mix_inactive.min, mix_inactive.max);
            return false;
        }
    }
    
    // Check all channel sinks are inactive
    for (int i = 0; i < channel_sink_count; i++) {
        // Get the actual voltage for detailed error reporting
        int32_t voltage_raw = hal_adc_read(channel_sinks[i]);
        float voltage = voltage_raw / 1000.0f;
        
        ESP_LOGI(TAG, "%s voltage: %.2f V (inactive range: %.2f-%.2f V)", 
                channel_sink_labels[i], voltage, channel_inactive.min, channel_inactive.max);
        
        if (voltage < channel_inactive.min || voltage > channel_inactive.max) {
            ESP_LOGE(TAG, "%s voltage out of inactive range: %.2f V (expected %.2f-%.2f V)",
                     channel_sink_labels[i], voltage, channel_inactive.min, channel_inactive.max);
            display_printf("Test 1: All inputs low\n%s voltage: %.2f V\nExpected: %.2f-%.2f V", 
                          channel_sink_labels[i], voltage, channel_inactive.min, channel_inactive.max);
            return false;
        }
    }
    
    return true;
}

static bool test_active_channel(int idx) {
    int source = idx + IO0;
    ESP_LOGI(TAG, "Testing source IO%d", source);
    
    // Set the current source HIGH
    mcp0.digitalWrite(source, HIGH);
    delay(1);  // Small delay to allow signals to stabilize
    
    // Check all mix sinks are active
    for (int i = 0; i < mix_sink_count; i++) {
        // Get the actual voltage for detailed error reporting
        int32_t voltage_raw = hal_adc_read(mix_sinks[i]);
        float voltage = voltage_raw / 1000.0f;
        
        ESP_LOGI(TAG, "IO%d high: %s voltage: %.2f V (active range: %.2f-%.2f V)", 
                source, mix_sink_labels[i], voltage, mix_active.min, mix_active.max);
        
        if (voltage < mix_active.min || voltage > mix_active.max) {
            ESP_LOGE(TAG, "%s voltage out of active range with IO%d high: %.2f V (expected %.2f-%.2f V)",
                     mix_sink_labels[i], source, voltage, mix_active.min, mix_active.max);
            display_printf("Test: Input IO%d high\n%s voltage: %.2f V\nExpected: %.2f-%.2f V", 
                          source, mix_sink_labels[i], voltage, mix_active.min, mix_active.max);
            mcp0.digitalWrite(source, LOW);  // Reset source before returning
            return false;
        }
    }
    
    // Check the corresponding channel is active, others are inactive
    for (int i = 0; i < channel_sink_count; i++) {
        // Get the actual voltage for detailed error reporting
        int32_t voltage_raw = hal_adc_read(channel_sinks[i]);
        float voltage = voltage_raw / 1000.0f;
        
        if (i == idx) {  // Corresponding channel
            ESP_LOGI(TAG, "IO%d high: %s voltage: %.2f V (active range: %.2f-%.2f V)", 
                    source, channel_sink_labels[i], voltage, channel_active.min, channel_active.max);
            
            if (voltage < channel_active.min || voltage > channel_active.max) {
                ESP_LOGE(TAG, "%s voltage out of active range with IO%d high: %.2f V (expected %.2f-%.2f V)",
                         channel_sink_labels[i], source, voltage, channel_active.min, channel_active.max);
                display_printf("Test: Input IO%d high\n%s should be active\nVoltage: %.2f V\nExpected: %.2f-%.2f V", 
                              source, channel_sink_labels[i], voltage, channel_active.min, channel_active.max);
                mcp0.digitalWrite(source, LOW);  // Reset source before returning
                return false;
            }
        } else {  // Other channels
            ESP_LOGI(TAG, "IO%d high: %s voltage: %.2f V (inactive range: %.2f-%.2f V)", 
                    source, channel_sink_labels[i], voltage, channel_inactive.min, channel_inactive.max);
            
            if (voltage < channel_inactive.min || voltage > channel_inactive.max) {
                ESP_LOGE(TAG, "%s voltage out of inactive range with IO%d high: %.2f V (expected %.2f-%.2f V)",
                         channel_sink_labels[i], source, voltage, channel_inactive.min, channel_inactive.max);
                display_printf("Test: Input IO%d high\n%s should be inactive\nVoltage: %.2f V\nExpected: %.2f-%.2f V", 
                              source, channel_sink_labels[i], voltage, channel_inactive.min, channel_inactive.max);
                mcp0.digitalWrite(source, LOW);  // Reset source before returning
                return false;
            }
        }
    }
    
    // Set the source back to LOW
    mcp0.digitalWrite(source, LOW);
    delay(1);
    
    return true;
}

bool test_inputs_outputs(void) {
    ESP_LOGI(TAG, "Testing inputs and outputs");
    
    // Set all sources to 5V to ensure signals can pass
    hal_set_source(SOURCE_A, 5.0f);
    hal_set_source(SOURCE_B, 5.0f);
    hal_set_source(SOURCE_C, 5.0f);
    hal_set_source(SOURCE_D, 0.0f);  // Keep D at 0V
    
    // Set all IO0..IO2 to output with logical 0
    for (int i = IO0; i <= IO2; i++) {
        mcp0.pinMode(i, OUTPUT);
        mcp0.digitalWrite(i, LOW);
    }
    
    // Enable pulldown for sinkPD_A
    mcp1.pinMode(PIN_SINK_PD_A, OUTPUT);
    mcp1.digitalWrite(PIN_SINK_PD_A, HIGH);
    
    delay(1);  // Small delay to allow signals to stabilize
    
    // Test 1: All sources are low - test everything is inactive
    TEST_RUN_REPEAT(test_inactive());
    
    // Test 2-4: Test each source individually
    for (int i = 0; i < 3; i++) {
        TEST_RUN_REPEAT(test_active_channel(i));
    }
    
    ESP_LOGI(TAG, "All input/output tests passed");
    return true;
}

bool mod_mix_handler(void) {
    hal_clear_console();
    ESP_LOGI(TAG, "Starting mod_mix test sequence");
    
    // Setup test environment
    setup_test();

    // Check current on each power rail
    TEST_RUN(check_current(INA_PIN_12V, {22.0f, 40.0f}, "+12V"));
    TEST_RUN(check_current(INA_PIN_M12V, {22.0f, 80.0f}, "-12V"));
    TEST_RUN(check_current(INA_PIN_5V, {13.0f, 25.0f}, "+5V"));

    TEST_RUN(check_mode_and_prompt());
    TEST_RUN(test_pin_range(POT_A, UNI_VOLTAGE_RANGE, "Pot 1"));
    TEST_RUN(test_pin_range(POT_B, UNI_VOLTAGE_RANGE, "Pot 2"));
    TEST_RUN(test_pin_range(POT_C, UNI_VOLTAGE_RANGE, "Pot 3"));
    
    // Test +5V source
    TEST_RUN(test_pin_pd(PD_B, {4.9f, 5.1f}, {4.9f, 5.1f}, "+5V"));
    // Test -5V source
    TEST_RUN(test_pin_pd(PD_C, {-5.1f, -4.1f}, {-5.1f, -4.1f}, "-5V"));

    // Test inputs and outputs functionality
    TEST_RUN(test_inputs_outputs());

    return true;
}

static void prepare_inputs_outputs(int input) {
    ESP_LOGI(TAG, "Testing inputs and outputs");

    // Set source D to 0V
    hal_set_source(SOURCE_D, 0.0f);

    // Set all gain
    hal_set_source(SOURCE_A, 5.0f);
    hal_set_source(SOURCE_B, 5.0f);
    hal_set_source(SOURCE_C, 5.0f);

    // Set all IO0 to IO2 to output with logical 0
    for (int i = IO0; i <= IO2; i++) {
        mcp0.pinMode(i, OUTPUT);
        mcp0.digitalWrite(i, LOW);
    }
    if(input >=0) {
        mcp0.digitalWrite(input, HIGH);
    }

    // Enable pulldown for sinkPD_A
    mcp1.pinMode(PIN_SINK_PD_A, OUTPUT);
    mcp1.digitalWrite(PIN_SINK_PD_A, HIGH);

    delay(1);
}

static bool check_mix_outputs(range_t range) {
    ESP_LOGI(TAG, "Checking mix outputs with range: %.2f-%.2f V", range.min, range.max);
    
    TEST_RUN(test_pin_range(ADC_sink_1k_A, range, "A"));
    TEST_RUN(test_pin_range(ADC_sink_1k_B, range, "B"));
    TEST_RUN(test_pin_range(ADC_sink_1k_C, range, "C"));
    TEST_RUN(test_pin_range(ADC_sink_1k_D, range, "D"));
    
    return true;
}
