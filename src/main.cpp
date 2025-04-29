#include <Arduino.h>
#include <esp_log.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_MCP23X17.h>
#include <DAC8552.h>

#include "board.h"
#include "hal.h"
#include "test_helpers.h"
#include "display.h"

static const char* TAG = "main";

// External objects from hal.cpp
extern DAC8552 dac1;
extern DAC8552 dac2;

// PD state variables
static bool pd_state_a = false;
static bool pd_state_b = false;
static bool pd_state_c = false;

void setup() {
    // Initialize hardware abstraction layer
    hal_init();

    // Initialize display
    if (!display_init()) {
        ESP_LOGE(TAG, "Failed to initialize display");
        for(;;); // Don't proceed, loop forever
    }

    ESP_LOGI(TAG, "Hello, Microrack!");

    // Perform startup sequence
    if (!perform_startup_sequence()) {
        ESP_LOGE(TAG, "Startup sequence failed");
        for(;;); // Don't proceed, loop forever
    }
}

void loop() {
    // Step 6.1: Turn on fail LED and wait for module
    mcp1.digitalWrite(PIN_LED_FAIL, HIGH);
    display_printf("Waiting for module...");

    bool p12v_ok, p5v_ok, m12v_ok;
    wait_for_module_insertion(p12v_ok, p5v_ok, m12v_ok);

    // Step 6.2: Module inserted, wait 100ms and measure
    delay(100);
    int32_t current_12v = measure_current(PIN_INA_12V);
    int32_t current_5v = measure_current(PIN_INA_5V);
    int32_t current_m12v = measure_current(PIN_INA_M12V);

    display_printf("+12V: %d uA\n+5V: %d uA\n-12V: %d uA", 
        current_12v, current_5v, current_m12v);

    mcp1.digitalWrite(PIN_LED_FAIL, LOW);
    mcp1.digitalWrite(PIN_LED_OK, HIGH);

    // Step 6.3: Wait for module removal
    wait_for_module_removal(p12v_ok, p5v_ok, m12v_ok);

    mcp1.digitalWrite(PIN_LED_OK, LOW);
}
