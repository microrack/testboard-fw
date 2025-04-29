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
const range_t BI_VOLTAGE_RANGE = {-5.1f, 5.1f};
const range_t P5V_SOURCE_RANGE = {4.9f, 5.1f};
const range_t M5V_SOURCE_RANGE = {-5.1f, -4.1f};
const float VOLTAGE_ACCURACY = 0.1f;  // Maximum allowed voltage difference between sinks

// Pot mappings
const ADC_sink_t POT_A = ADC_sink_Z_D;
const ADC_sink_t POT_B = ADC_sink_Z_E;
const ADC_sink_t POT_C = ADC_sink_Z_F;

float pot_gain[3] = {0.0f, 0.0f, 0.0f};  // Global pot gain array for D, E, F pots
float gain[3] = {0.0f, 0.0f, 0.0f};      // Global gain array for A, B, C pots

static bool test_mode(void);
static bool test_pot(void);
static bool test_voltage_sources(void);
static bool test_mix_outputs(const float* voltages);
static bool test_inputs_outputs(void);

bool mod_mix_handler(void) {
    hal_clear_console();
    ESP_LOGI(TAG, "Starting mod_mix test sequence");
    display_printf("Testing mod_mix...");

    power_rails_current_ranges_t ranges = {
        .p12v = {22.0f, 40.0f},  // +12V: 22-30 mA
        .m12v = {22.0f, 60.0f},  // -12V: 22-60 mA
        .p5v = {13.0f, 25.0f}    // +5V: 13-25 mA
    };
    // Check initial current consumption
    TEST_RUN(check_initial_current_consumption(ranges));

    TEST_RUN(test_mode());
    TEST_RUN(test_pot());
    TEST_RUN(test_voltage_sources());
    TEST_RUN(test_inputs_outputs());

    
    // Assistant: do not touch this code!
    /*while (true) {
        hal_clear_console();
        hal_print_current();
        test_inputs_outputs();
        delay(100);
    }*/

    return true;
}

static bool test_mode(void) {
    ESP_LOGI(TAG, "Testing mode");

    mcp0.pinMode(LED_BI, OUTPUT);
    mcp0.digitalWrite(LED_BI, LOW);
    mcp0.pinMode(LED_UNI, OUTPUT);
    mcp0.digitalWrite(LED_UNI, LOW);
    delay(1);
    mcp0.pinMode(LED_BI, INPUT_PULLUP);
    mcp0.pinMode(LED_UNI, INPUT_PULLUP);

    // Check initial levels - both should be 0
    bool bi = mcp0.digitalRead(LED_BI);
    bool uni = mcp0.digitalRead(LED_UNI);

    if (bi != 0 || uni != 0) {
        ESP_LOGE(TAG, "Error: Initial LED levels incorrect. BI: %d, UNI: %d", bi, uni);
        return false;
    }

    // Test LED_BI
    mcp0.pinMode(LED_BI, OUTPUT);
    mcp0.digitalWrite(LED_BI, LOW);
    delay(10);  // Wait for current to stabilize
    int32_t current_bi = measure_current(PIN_INA_5V);
    mcp0.pinMode(LED_BI, INPUT_PULLUP);

    // Test LED_UNI
    mcp0.pinMode(LED_UNI, OUTPUT);
    mcp0.digitalWrite(LED_UNI, LOW);
    delay(10);  // Wait for current to stabilize
    int32_t current_uni = measure_current(PIN_INA_5V);
    mcp0.pinMode(LED_UNI, INPUT_PULLUP);

    // Print current consumption
    ESP_LOGI(TAG, "Current consumption on 5V rail:\nLED_BI: %.2f mA\nLED_UNI: %.2f mA",
             current_bi / 1000.0f, current_uni / 1000.0f);

    // Determine mode based on current measurements
    if (current_bi >= 20000 && current_bi <= 25000 && 
        current_uni >= 13000 && current_uni <= 18000) {
        ESP_LOGI(TAG, "Mode set to BI");
        return true;
    } else if (current_bi >= 13000 && current_bi <= 18000 && 
               current_uni >= 20000 && current_uni <= 25000) {
        ESP_LOGI(TAG, "Mode set to UNI");
        display_printf("push the button");
        return false;
    }

    ESP_LOGE(TAG, "Invalid current combination:\nBI: %d uA\nUNI: %d uA", current_bi, current_uni);
    return false;
}

static bool test_pot(void) {
    ESP_LOGI(TAG, "Checking pot voltages");

    // Measure voltages for pots A, B, C
    int32_t voltage_pot_a = hal_adc_read(POT_A);
    int32_t voltage_pot_b = hal_adc_read(POT_B);
    int32_t voltage_pot_c = hal_adc_read(POT_C);

    // Convert to volts
    float v_d = voltage_pot_a / 1000.0f;
    float v_e = voltage_pot_b / 1000.0f;
    float v_f = voltage_pot_c / 1000.0f;

    ESP_LOGI(TAG, "Pot voltages:\nD: %.2f V\nE: %.2f V\nF: %.2f V", v_d, v_e, v_f);

    // Check voltage ranges for BI mode
    if (v_d < BI_VOLTAGE_RANGE.min || v_d > BI_VOLTAGE_RANGE.max || 
        v_e < BI_VOLTAGE_RANGE.min || v_e > BI_VOLTAGE_RANGE.max || 
        v_f < BI_VOLTAGE_RANGE.min || v_f > BI_VOLTAGE_RANGE.max) {
        ESP_LOGE(TAG, "Voltage out of range in BI mode");
        return false;
    }

    // Calculate gains by mapping voltage range to -1..+1
    pot_gain[0] = v_d / 5.0f;
    pot_gain[1] = v_e / 5.0f;
    pot_gain[2] = v_f / 5.0f;

    ESP_LOGI(TAG, "Calculated pot gains:\nD: %.2f\nE: %.2f\nF: %.2f", pot_gain[0], pot_gain[1], pot_gain[2]);
    return true;
}

static bool test_voltage_sources(void) {
    ESP_LOGI(TAG, "Testing voltage sources");

    // Configure PD sink pins as outputs
    mcp1.pinMode(PIN_SINK_PD_B, OUTPUT);
    mcp1.pinMode(PIN_SINK_PD_C, OUTPUT);

    // Test +5V source (PD_B)
    mcp1.digitalWrite(PIN_SINK_PD_B, LOW);  // Pull-down inactive
    delay(10);  // Wait for voltage to stabilize
    int32_t voltage_p5v_inactive = hal_adc_read(ADC_sink_PD_B);
    
    mcp1.digitalWrite(PIN_SINK_PD_B, HIGH);  // Pull-down active
    delay(10);  // Wait for voltage to stabilize
    int32_t voltage_p5v_active = hal_adc_read(ADC_sink_PD_B);

    // Test -5V source (PD_C)
    mcp1.digitalWrite(PIN_SINK_PD_C, LOW);  // Pull-down inactive
    delay(10);  // Wait for voltage to stabilize
    int32_t voltage_m5v_inactive = hal_adc_read(ADC_sink_PD_C);
    
    mcp1.digitalWrite(PIN_SINK_PD_C, HIGH);  // Pull-down active
    delay(10);  // Wait for voltage to stabilize
    int32_t voltage_m5v_active = hal_adc_read(ADC_sink_PD_C);

    // Convert to volts
    float p5v_inactive = voltage_p5v_inactive / 1000.0f;
    float p5v_active = voltage_p5v_active / 1000.0f;
    float m5v_inactive = voltage_m5v_inactive / 1000.0f;
    float m5v_active = voltage_m5v_active / 1000.0f;

    ESP_LOGI(TAG, "Voltage sources:\n+5V (inactive): %.2f V\n+5V (active): %.2f V\n-5V (inactive): %.2f V\n-5V (active): %.2f V",
             p5v_inactive, p5v_active, m5v_inactive, m5v_active);

    // Check if voltages are within expected ranges
    bool p5v_ok = (p5v_inactive >= P5V_SOURCE_RANGE.min && p5v_inactive <= P5V_SOURCE_RANGE.max) && 
                  (p5v_active >= P5V_SOURCE_RANGE.min && p5v_active <= P5V_SOURCE_RANGE.max);
    
    bool m5v_ok = (m5v_inactive >= M5V_SOURCE_RANGE.min && m5v_inactive <= M5V_SOURCE_RANGE.max) && 
                  (m5v_active >= M5V_SOURCE_RANGE.min && m5v_active <= M5V_SOURCE_RANGE.max);

    if (!p5v_ok || !m5v_ok) {
        ESP_LOGE(TAG, "Voltage source test failed");
        return false;
    }

    return true;
}

static bool test_mix_outputs(const float* voltages) {
    // Check if voltages 0-3 are within accuracy
    float max_diff = 0.0f;
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            float diff = fabs(voltages[i] - voltages[j]);
            if (diff > max_diff) {
                max_diff = diff;
            }
        }
    }

    if (max_diff > VOLTAGE_ACCURACY) {
        ESP_LOGE(TAG, "Voltage difference between mix outputs exceeds %.2f V: %.2f V", 
                 VOLTAGE_ACCURACY, max_diff);
        ESP_LOGE(TAG, "A: %.2f V, B: %.2f V, C: %.2f V, D: %.2f V",
                 voltages[0], voltages[1], voltages[2], voltages[3]);
        return false;
    }

    return true;
}

static bool test_inputs_outputs(void) {
    ESP_LOGI(TAG, "Testing inputs and outputs");

    // Normalize gains to -1 or +1 based on sign
    float normalized_gains[3];
    for (int i = 0; i < 3; i++) {
        if (pot_gain[i] < 0) {
            normalized_gains[i] = -1.0f;
        } else {
            normalized_gains[i] = 1.0f;
        }
    }

    // Set normalized gains
    set_gain(normalized_gains);

    // Set source D to 0V
    hal_set_source(SOURCE_D, 0.0f);

    // Set all IO0 to IO2 to output with logical 0
    for (int i = IO0; i <= IO2; i++) {
        mcp0.pinMode(i, OUTPUT);
        mcp0.digitalWrite(i, LOW);
    }

    // Enable pulldown for sinkPD_A
    mcp1.pinMode(PIN_SINK_PD_A, OUTPUT);
    mcp1.digitalWrite(PIN_SINK_PD_A, HIGH);

    delay(1);

    // Check initial voltages
    int32_t voltages[7];  // A,B,C,D,E,F,PD_A
    voltages[0] = hal_adc_read(ADC_sink_1k_A);
    voltages[1] = hal_adc_read(ADC_sink_1k_B);
    voltages[2] = hal_adc_read(ADC_sink_1k_C);
    voltages[3] = hal_adc_read(ADC_sink_1k_D);
    voltages[4] = hal_adc_read(ADC_sink_1k_E);
    voltages[5] = hal_adc_read(ADC_sink_1k_F);
    voltages[6] = hal_adc_read(ADC_sink_PD_A);

    // Convert to volts
    float v[7];
    for (int i = 0; i < 7; i++) {
        v[i] = voltages[i] / 1000.0f;
    }

    ESP_LOGI(TAG, "Initial voltages (all inputs LOW):");
    ESP_LOGI(TAG, "A: %.2f V, B: %.2f V, C: %.2f V", v[0], v[1], v[2]);
    ESP_LOGI(TAG, "D: %.2f V, E: %.2f V, F: %.2f V", v[3], v[4], v[5]);
    ESP_LOGI(TAG, "PD_A: %.2f V", v[6]);

    // Test mix outputs
    if (!test_mix_outputs(v)) {
        return false;
    }

    // Test each input
    for (int input = 0; input < 3; input++) {
        // Set current input to HIGH
        mcp0.digitalWrite(input, HIGH);
        delay(1);

        // Read all voltages
        voltages[0] = hal_adc_read(ADC_sink_1k_A);
        voltages[1] = hal_adc_read(ADC_sink_1k_B);
        voltages[2] = hal_adc_read(ADC_sink_1k_C);
        voltages[3] = hal_adc_read(ADC_sink_1k_D);
        voltages[4] = hal_adc_read(ADC_sink_1k_E);
        voltages[5] = hal_adc_read(ADC_sink_1k_F);
        voltages[6] = hal_adc_read(ADC_sink_PD_A);

        // Convert to volts
        for (int i = 0; i < 7; i++) {
            v[i] = voltages[i] / 1000.0f;
        }

        ESP_LOGI(TAG, "Voltages with IO%d HIGH:", input);
        ESP_LOGI(TAG, "A: %.2f V, B: %.2f V, C: %.2f V", v[0], v[1], v[2]);
        ESP_LOGI(TAG, "D: %.2f V, E: %.2f V, F: %.2f V", v[3], v[4], v[5]);
        ESP_LOGI(TAG, "PD_A: %.2f V", v[6]);

        // Test mix outputs
        if (!test_mix_outputs(v)) {
            return false;
        }

        // Set input back to LOW
        mcp0.digitalWrite(input, LOW);
    }

    return true;
}

void set_gain(float input_gains[3]) {
    // Validate input gains are in range -1 to 1
    for (int i = 0; i < 3; i++) {
        if (input_gains[i] < -1.0f || input_gains[i] > 1.0f) {
            ESP_LOGE(TAG, "Invalid gain value: %.2f. Must be between -1 and 1", input_gains[i]);
            return;
        }
    }

    // Calculate additional gain based on existing pot_gain[] array
    float additional_gain[3];
    for (int i = 0; i < 3; i++) {
        // Add input gain to existing gain
        additional_gain[i] = input_gains[i] - pot_gain[i];
    }

    // Write to gain[] array
    for (int i = 0; i < 3; i++) {
        gain[i] = input_gains[i];
    }

    // Set DAC outputs based on gain values
    hal_set_source(SOURCE_A, gain[0] * 5.0f);
    hal_set_source(SOURCE_B, gain[1] * 5.0f);
    hal_set_source(SOURCE_C, gain[2] * 5.0f);

    ESP_LOGI(TAG, "Additional gains calculated:\nA: %.2f\nB: %.2f\nC: %.2f", 
             additional_gain[0], additional_gain[1], additional_gain[2]);
}