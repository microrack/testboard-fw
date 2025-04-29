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

// Voltage range constants
const float UNI_VOLTAGE_MIN = -0.1f;
const float UNI_VOLTAGE_MAX = 5.1f;
const float BI_VOLTAGE_MIN = -5.1f;
const float BI_VOLTAGE_MAX = 5.1f;

// Voltage source range constants
const float P5V_SOURCE_MIN = 4.9f;
const float P5V_SOURCE_MAX = 5.1f;
const float M5V_SOURCE_MIN = -5.1f;
const float M5V_SOURCE_MAX = -4.1f;

mix_mode_t current_mode = MODE_UNI;  // Default mode
float gain[3] = {0.0f, 0.0f, 0.0f};  // Global gain array for D, E, F pots

static bool test_mode(void);
static bool test_pot(void);
static bool test_voltage_sources(void);

bool mod_mix_handler(void) {
    ESP_LOGI(TAG, "Starting mod_mix test sequence");
    display_printf("Testing mod_mix...");

    power_rails_current_ranges_t ranges = {
        .p12v = {22000, 30000},  // +12V: 22-30 mA
        .m12v = {22000, 60000},  // -12V: 22-30 mA
        .p5v = {13000, 25000}    // +5V: 13-25 mA
    };
    // Check initial current consumption
    TEST_RUN(check_initial_current_consumption(ranges));

    TEST_RUN(test_mode());
    TEST_RUN(test_pot());
    TEST_RUN(test_voltage_sources());

    /*
    while (true) {
        hal_clear_console();
        hal_print_current();
        test_voltage_sources();
        delay(100);
    }
    */

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
        current_mode = MODE_BI;
        ESP_LOGI(TAG, "Mode set to BI");
        return true;
    } else if (current_bi >= 13000 && current_bi <= 18000 && 
               current_uni >= 20000 && current_uni <= 25000) {
        current_mode = MODE_UNI;
        ESP_LOGI(TAG, "Mode set to UNI");
        return true;
    }

    ESP_LOGE(TAG, "Invalid current combination:\nBI: %d uA\nUNI: %d uA", current_bi, current_uni);
    return false;
}

static bool test_pot(void) {
    ESP_LOGI(TAG, "Checking pot voltages");

    // Measure voltages for sinks D, E, F
    int32_t voltage_d = hal_adc_read(ADC_sink_Z_D);
    int32_t voltage_e = hal_adc_read(ADC_sink_Z_E);
    int32_t voltage_f = hal_adc_read(ADC_sink_Z_F);

    // Convert to volts
    float v_d = voltage_d / 1000.0f;
    float v_e = voltage_e / 1000.0f;
    float v_f = voltage_f / 1000.0f;

    ESP_LOGI(TAG, "Pot voltages:\nD: %.2f V\nE: %.2f V\nF: %.2f V", v_d, v_e, v_f);

    // Check voltage ranges based on mode
    if (current_mode == MODE_UNI) {
        // In UNI mode, voltages should be between 0 and 5V
        if (v_d < UNI_VOLTAGE_MIN || v_d > UNI_VOLTAGE_MAX || 
            v_e < UNI_VOLTAGE_MIN || v_e > UNI_VOLTAGE_MAX || 
            v_f < UNI_VOLTAGE_MIN || v_f > UNI_VOLTAGE_MAX) {
            ESP_LOGE(TAG, "Voltage out of range in UNI mode");
            return false;
        }
    } else {
        // In BI mode, voltages should be between -5 and 5V
        if (v_d < BI_VOLTAGE_MIN || v_d > BI_VOLTAGE_MAX || 
            v_e < BI_VOLTAGE_MIN || v_e > BI_VOLTAGE_MAX || 
            v_f < BI_VOLTAGE_MIN || v_f > BI_VOLTAGE_MAX) {
            ESP_LOGE(TAG, "Voltage out of range in BI mode");
            return false;
        }
    }

    // Calculate gains by mapping voltage range to -1..+1
    gain[0] = v_d / 5.0f;
    gain[1] = v_e / 5.0f;
    gain[2] = v_f / 5.0f;

    ESP_LOGI(TAG, "Calculated gains:\nD: %.2f\nE: %.2f\nF: %.2f", gain[0], gain[1], gain[2]);
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
    bool p5v_ok = (p5v_inactive >= P5V_SOURCE_MIN && p5v_inactive <= P5V_SOURCE_MAX) && 
                  (p5v_active >= P5V_SOURCE_MIN && p5v_active <= P5V_SOURCE_MAX);
    
    bool m5v_ok = (m5v_inactive >= M5V_SOURCE_MIN && m5v_inactive <= M5V_SOURCE_MAX) && 
                  (m5v_active >= M5V_SOURCE_MIN && m5v_active <= M5V_SOURCE_MAX);

    if (!p5v_ok || !m5v_ok) {
        ESP_LOGE(TAG, "Voltage source test failed");
        return false;
    }

    return true;
}