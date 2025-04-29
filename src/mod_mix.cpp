#include "mod_mix.h"
#include "display.h"
#include "esp_log.h"
#include "board.h"
#include "hal.h"
#include "display.h"

static const char* TAG = "mod_mix";

const int LED_BI = IO3;
const int LED_UNI = IO4;

mix_mode_t current_mode = MODE_UNI;  // Default mode

static bool test_mode(void);
void mod_mix_handler(void) {
    ESP_LOGI(TAG, "Starting mod_mix test sequence");
    display_printf("Testing mod_mix...");

    while (true) { 
        hal_clear_console();
        hal_print_current();
        test_mode();
        delay(100);
    }
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