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
    // Initialize serial port
    Serial.begin(115200);
    delay(1000);  // Small delay for startup

    // Initialize ESP logging
    esp_log_level_set("*", ESP_LOG_DEBUG);
    esp_log_level_set("hal", ESP_LOG_INFO);  // Set log level for hal tag

    // Initialize hardware abstraction layer
    hal_init();

    // Initialize display
    if (!display_init()) {
        ESP_LOGE(TAG, "Failed to initialize display");
        for(;;); // Don't proceed, loop forever
    }

    // Initialize MCP IO expanders
    mcp_init();

    // Initialize DAC
    dac_init();

    // Configure INA196A pins as inputs
    pinMode(PIN_INA_12V, INPUT);
    pinMode(PIN_INA_5V, INPUT);
    pinMode(PIN_INA_M12V, INPUT);

    ESP_LOGI(TAG, "Hello, Microrack!");

    mcp1.digitalWrite(PIN_LED_OK, HIGH);
    mcp1.digitalWrite(PIN_LED_FAIL, HIGH);

    // Read and display adapter ID
    uint8_t id = hal_adapter_id();
    display_printf("Adapter ID: 0x%02X", id);

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
    power_rails_state_t rails_state;
    
    do {
        rails_state = get_power_rails_state(p12v_ok, p5v_ok, m12v_ok);
        delay(100);
    } while (rails_state != POWER_RAILS_ALL);

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
    do {
        rails_state = get_power_rails_state(p12v_ok, p5v_ok, m12v_ok);
        delay(100);
    } while (rails_state != POWER_RAILS_NONE);

    mcp1.digitalWrite(PIN_LED_OK, LOW);
}
