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

    prepare_inputs_outputs(-1);
    TEST_RUN(check_mix_outputs(range_t{0.0f, 1.0f}));

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
    TEST_RUN(test_pin_range(ADC_sink_1k_A, range, "A"));
    TEST_RUN(test_pin_range(ADC_sink_1k_B, range, "B"));
    TEST_RUN(test_pin_range(ADC_sink_1k_C, range, "C"));
    TEST_RUN(test_pin_range(ADC_sink_1k_D, range, "D"));

    return true;
}
