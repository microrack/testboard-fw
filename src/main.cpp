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
#include "modules.h"
#include "webserver.h"
#include "test_results.h"

static const char* TAG = "main";

// External objects from hal.cpp
extern DAC8552 dac1;
extern DAC8552 dac2;

// PD state variables
static bool pd_state_a = false;
static bool pd_state_b = false;
static bool pd_state_c = false;

// Global module info
static module_info_t* module = nullptr;

void setup() {
    // Initialize hardware abstraction layer
    hal_init();

    // Initialize display
    if (!display_init()) {
        ESP_LOGE(TAG, "Failed to initialize display");
        for(;;); // Don't proceed, loop forever
    }

    ESP_LOGI(TAG, "Hello, Microrack!");

    // Initialize modules from filesystem
    if (!init_modules_from_fs()) {
        ESP_LOGE(TAG, "Failed to initialize modules from filesystem");
        for(;;); // Don't proceed, loop forever
    }

    // Perform startup sequence
    if (!perform_startup_sequence()) {
        ESP_LOGE(TAG, "Startup sequence failed");
        for(;;); // Don't proceed, loop forever
    }

    // Get adapter ID and corresponding module info
    uint8_t adapter_id = hal_adapter_id();
    module = get_module_info(adapter_id);

    if (!module) {
        ESP_LOGE(TAG, "Unknown module detected");
        mcp1.digitalWrite(PIN_LED_FAIL, HIGH);
        mcp1.digitalWrite(PIN_LED_OK, LOW);
        for(;;);
    }

    allocate_test_results_arrays(module);

    // Initialize web server
    if (!init_webserver()) {
        ESP_LOGE(TAG, "Failed to initialize web server");
        for(;;); // Don't proceed, loop forever
    }
}

void loop() {
    if (load_wifi_credentials()) {
        if (!connect_to_wifi()) {
            ESP_LOGW(TAG, "Failed to reconnect to WiFi, starting AP mode");
            enable_wifi();
        }
    } else {
        ESP_LOGW(TAG, "No WiFi credentials found, starting AP mode");
        enable_wifi();
    }

    display_printf("Waiting for module...");

    bool p12v_ok, p5v_ok, m12v_ok;
    
    wait_for_module_insertion(p12v_ok, p5v_ok, m12v_ok);
    
    // Reset all test results after module insertion
    reset_all_test_results();

    // Disable WiFi before testing
    disable_wifi();
    
    ESP_LOGI(TAG, "Module detected: %s", module->name);
    display_printf("Module: %s", module->name);

    mcp1.digitalWrite(PIN_LED_FAIL, LOW);

    bool test_result;
    do {
        test_result = execute_module_tests(module);
        if (test_result) {
            // Test passed
            mcp1.digitalWrite(PIN_LED_OK, HIGH);
            mcp1.digitalWrite(PIN_LED_FAIL, LOW);
            display_printf("Module OK");
        } else {
            // Test failed
            ESP_LOGE(TAG, "Test failed, retrying...");
            mcp1.digitalWrite(PIN_LED_OK, LOW);
            mcp1.digitalWrite(PIN_LED_FAIL, HIGH);
            
            // Check if module is still present
            power_rails_state_t state = get_power_rails_state(NULL, NULL, NULL);
            if (state == POWER_RAILS_NONE) {
                break; // Module was removed, exit the loop
            }
            
            // Short delay before retrying
            delay(50);
        }

        display_all_test_results();
    } while (!test_result);

    // Step 6.3: Wait for module removal
    wait_for_module_removal(p12v_ok, p5v_ok, m12v_ok);

    execute_reset_operation();

    mcp1.digitalWrite(PIN_LED_FAIL, HIGH);
    mcp1.digitalWrite(PIN_LED_OK, LOW);
    display_printf("Module ejected");

    delay(100);

    bool keep_removed = true;

    // check every 10 ms
    // if module stay removed during 100 ms, print test results
    for (int i = 0; i < 10; i++) {
        if (get_power_rails_state(NULL, NULL, NULL) == POWER_RAILS_NONE) {
            delay(10);
        } else {
            keep_removed = false;
            break;
        }
    }

    if (keep_removed) {
        print_all_test_results();
        save_all_test_results();
    }
}
