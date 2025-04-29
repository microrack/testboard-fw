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

static const char* TAG = "main";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        ESP_LOGE(TAG, "SSD1306 allocation failed");
        for(;;); // Don't proceed, loop forever
    }

    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.cp437(true);         // Use full 256 char 'Code Page 437' font
    display.setRotation(0);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("Hello, Microrack!\n");
    display.display();

    // Initialize MCP IO expanders
    mcp_init();

    // Initialize DAC
    dac_init();

    // Configure INA196A pins as inputs
    pinMode(PIN_INA_12V, INPUT);
    pinMode(PIN_INA_5V, INPUT);
    pinMode(PIN_INA_M12V, INPUT);

    // Console output
    ESP_LOGI(TAG, "Hello, Microrack!");

    mcp1.digitalWrite(PIN_LED_OK, HIGH);
    mcp1.digitalWrite(PIN_LED_FAIL, HIGH);

    // Read and display adapter ID
    uint8_t id = hal_adapter_id();
    display.printf("Adapter ID: 0x%02X\n", id);
    display.display();
}

void loop() {
    // Step 1: Switch off both LEDs
    mcp1.digitalWrite(PIN_LED_OK, LOW);
    mcp1.digitalWrite(PIN_LED_FAIL, LOW);

    // Step 2: Wait for adapter
    uint8_t adapter_id = hal_adapter_id();
    if (adapter_id == 0b11111) {
        ESP_LOGI(TAG, "Waiting for adapter...");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.printf("Waiting for adapter...");
        display.display();

        while (hal_adapter_id() == 0b11111) {
            delay(100);
        }
        adapter_id = hal_adapter_id();
    }

    // Adapter detected
    ESP_LOGI(TAG, "Adapter detected: 0x%02X", adapter_id);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("Adapter ID: 0x%02X", adapter_id);
    display.display();

    // Step 4: Wait for module removal for calibration
    bool p12v_ok, p5v_ok, m12v_ok;
    bool rails_ok = get_power_rails_state(p12v_ok, p5v_ok, m12v_ok);

    if (rails_ok) {
        ESP_LOGI(TAG, "Waiting for calibration, remove module");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.printf("Remove module for calibration");
        display.display();

        do {
            rails_ok = get_power_rails_state(p12v_ok, p5v_ok, m12v_ok);
            delay(100);
        } while (rails_ok);
    }

    // Step 5: Perform calibration
    ESP_LOGI(TAG, "Performing calibration...");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("Calibrating...");
    display.display();
    
    hal_current_calibrate();
    hal_adc_calibrate();
    
    ESP_LOGI(TAG, "Calibration complete");

    // Main measurement loop
    while (1) {
        // Step 6.1: Turn on fail LED and wait for module
        mcp1.digitalWrite(PIN_LED_FAIL, HIGH);
        ESP_LOGI(TAG, "Waiting for module...");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.printf("Waiting for module...");
        display.display();

        while (!rails_ok) {
            rails_ok = get_power_rails_state(p12v_ok, p5v_ok, m12v_ok);
            delay(100);
        }

        // Step 6.2: Module inserted, wait 100ms and measure
        delay(100);
        int32_t current_12v = measure_current(PIN_INA_12V);
        int32_t current_5v = measure_current(PIN_INA_5V);
        int32_t current_m12v = measure_current(PIN_INA_M12V);

        ESP_LOGI(TAG, "Current measurements:");
        ESP_LOGI(TAG, "+12V: %d uA", current_12v);
        ESP_LOGI(TAG, "+5V: %d uA", current_5v);
        ESP_LOGI(TAG, "-12V: %d uA", current_m12v);

        display.clearDisplay();
        display.setCursor(0, 0);
        display.printf("+12V: %d uA\n", current_12v);
        display.printf("+5V: %d uA\n", current_5v);
        display.printf("-12V: %d uA", current_m12v);
        display.display();

        mcp1.digitalWrite(PIN_LED_FAIL, LOW);
        mcp1.digitalWrite(PIN_LED_OK, HIGH);

        // Step 6.3: Wait for module removal
        while (rails_ok) {
            rails_ok = get_power_rails_state(p12v_ok, p5v_ok, m12v_ok);
            delay(100);
        }

        mcp1.digitalWrite(PIN_LED_OK, LOW);
    }
}
