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

// Pot mappings
const ADC_sink_t POT_A = ADC_sink_Z_D;
const ADC_sink_t POT_B = ADC_sink_Z_E;
const ADC_sink_t POT_C = ADC_sink_Z_F;

static bool test_mix_outputs(const float* voltages);
static bool test_inputs_outputs(void);

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

test_result_t mod_mix_handler(void) {
    hal_clear_console();
    ESP_LOGI(TAG, "Starting mod_mix test sequence");
    display_printf("Testing mod_mix...");

    power_rails_current_ranges_t power_ranges = {
        .p12v = {22.0f, 40.0f},  // +12V: 22-30 mA
        .m12v = {22.0f, 80.0f},  // -12V: 22-60 mA
        .p5v = {13.0f, 25.0f}    // +5V: 13-25 mA
    };
    // Check initial current consumption
    TEST_RUN(check_initial_current_consumption(power_ranges));
    TEST_RUN(check_mode_and_prompt());
    TEST_RUN(test_pin_range(POT_A, UNI_VOLTAGE_RANGE, "Pot 1"));
    TEST_RUN(test_pin_range(POT_B, UNI_VOLTAGE_RANGE, "Pot 2"));
    TEST_RUN(test_pin_range(POT_C, UNI_VOLTAGE_RANGE, "Pot 3"));
    
    // Test +5V source
    TEST_RUN(test_pin_pd(PD_B, {4.9f, 5.1f}, {4.9f, 5.1f}, "+5V"));
    
    // Test -5V source
    TEST_RUN(test_pin_pd(PD_C, {-5.1f, -4.1f}, {-5.1f, -4.1f}, "-5V"));
    
    TEST_RUN(test_inputs_outputs());

    return TEST_OK;
}

static bool test_mix_outputs(const float* voltages) {
    // Check if voltages 0-3 are within accuracy
    float max_diff = 0.0f;
    int max_i = 0, max_j = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            float diff = fabs(voltages[i] - voltages[j]);
            if (diff > max_diff) {
                max_diff = diff;
                max_i = i;
                max_j = j;
            }
        }
    }

    if (max_diff > VOLTAGE_ACCURACY) {
        ESP_LOGE(TAG, "Voltage difference between mix outputs exceeds %.2f V: %.2f V", 
                 VOLTAGE_ACCURACY, max_diff);
        ESP_LOGE(TAG, "A: %.2f V, B: %.2f V, C: %.2f V, D: %.2f V",
                 voltages[0], voltages[1], voltages[2], voltages[3]);
        display_printf("Mix outputs differ too much\n%c: %.2f V\n%c: %.2f V\nDiff: %.2f V",
                      'A' + max_i, voltages[max_i],
                      'A' + max_j, voltages[max_j],
                      max_diff);
        return false;
    }

    return true;
}

static bool test_inputs_outputs(void) {
    ESP_LOGI(TAG, "Testing inputs and outputs");

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
            display_printf("Mix outputs failed\nwith input %d HIGH", input);
            return false;
        }

        // Set input back to LOW
        mcp0.digitalWrite(input, LOW);
    }

    return true;
}