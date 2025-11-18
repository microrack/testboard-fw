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

// Module descriptor structure
struct module_descriptor {
    const char* name;
    void (*init)();
    void (*handler)();
};

// Module descriptors array (max index is 28)
#define MODULES_COUNT 29
module_descriptor* modules[MODULES_COUNT] = {nullptr};

#include "modules/mod_empty_test.h"
#include "modules/mod_clk.h"
#include "modules/mod_comp.h"
#include "modules/mod_sat.h"
#include "modules/mod_env.h"
#include "modules/mod_eq.h"
#include "modules/mod_esp32.h"
#include "modules/mod_in_63.h"
#include "modules/mod_jacket.h"
#include "modules/mod_mix.h"
#include "modules/mod_lpg.h"
#include "modules/mod_key.h"
#include "modules/mod_noise.h"
#include "modules/mod_out_x.h"
#include "modules/mod_seq.h"
#include "modules/mod_vcf.h"
#include "modules/mod_vco.h"
#include "modules/mod_delay.h"

/*
    {.name = "empty-test"},  // 0
    {.name = "mod-clk"},     // 1
    {.name = "mod-comp"},    // 2
    {.name = nullptr},       // 3 (not used)
    {.name = "mod-sat"},     // 4
    {.name = "mod-env"},     // 5
    {.name = "mod-eq"},      // 6
    {.name = "mod-esp32"},   // 7
    {.name = "mod-in-63"},   // 8
    {.name = "mod-jacket"},  // 9
    {.name = "mod-mix"},     // 10
    {.name = "mod-lpg"},     // 11
    {.name = "mod-key"},     // 12
    {.name = "mod-noise"},   // 13
    {.name = "mod-out-x"},   // 14
    {.name = "mod-seq"},     // 15
    {.name = "mod-vcf"},     // 16
    {.name = "mod-vco"},     // 17
    {.name = nullptr},       // 18 (not used)
    {.name = nullptr},       // 19 (not used)
    {.name = nullptr},       // 20 (not used)
    {.name = nullptr},       // 21 (not used)
    {.name = nullptr},       // 22 (not used)
    {.name = nullptr},       // 23 (not used)
    {.name = nullptr},       // 24 (not used)
    {.name = nullptr},       // 25 (not used)
    {.name = nullptr},       // 26 (not used)
    {.name = nullptr},       // 27 (not used)
    {.name = "mod-delay"},   // 28
};
*/

// External objects from hal.cpp
extern DAC8552 dac1;
extern DAC8552 dac2;

const module_descriptor* module = nullptr;

void setup() {
    // Assign module descriptors
    modules[MOD_EMPTY_TEST_ID] = &mod_empty_test;
    modules[MOD_CLK_ID] = &mod_clk;
    modules[MOD_COMP_ID] = &mod_comp;
    modules[MOD_SAT_ID] = &mod_sat;
    modules[MOD_ENV_ID] = &mod_env;
    modules[MOD_EQ_ID] = &mod_eq;
    modules[MOD_ESP32_ID] = &mod_esp32;
    modules[MOD_IN_63_ID] = &mod_in_63;
    modules[MOD_JACKET_ID] = &mod_jacket;
    modules[MOD_MIX_ID] = &mod_mix;
    modules[MOD_LPG_ID] = &mod_lpg;
    modules[MOD_KEY_ID] = &mod_key;
    modules[MOD_NOISE_ID] = &mod_noise;
    modules[MOD_OUT_X_ID] = &mod_out_x;
    modules[MOD_SEQ_ID] = &mod_seq;
    modules[MOD_VCF_ID] = &mod_vcf;
    modules[MOD_VCO_ID] = &mod_vco;
    modules[MOD_DELAY_ID] = &mod_delay;

    // Initialize hardware abstraction layer
    hal_init();

    // Initialize display
    if (!display_init()) {
        ESP_LOGE(TAG, "Failed to initialize display");
        for(;;); // Don't proceed, loop forever
    }

    ESP_LOGI(TAG, "Hello, Microrack!");

    uint8_t adapter_id = hal_adapter_id();

    if (adapter_id >= MODULES_COUNT || modules[adapter_id] == nullptr || modules[adapter_id]->name == nullptr) {
        ESP_LOGE(TAG, "Invalid adapter ID: %d", adapter_id);
        display_printf("Invalid adapter ID: %d", adapter_id);
        for(;;); // Don't proceed, loop forever
    }

    module = modules[adapter_id];

    display_printf("Module: %s", module->name);
    ESP_LOGI(TAG, "Module: %s", module->name);

    hal_current_calibrate();
    hal_adc_calibrate();

    if(module && module->init) {
        module->init();
    }
}

void loop() {
    if(module && module->handler) {
        module->handler();
    }
}
