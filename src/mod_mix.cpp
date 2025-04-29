#include "mod_mix.h"
#include "display.h"
#include "esp_log.h"
#include "board.h"
#include "hal.h"
#include "display.h"

static const char* TAG = "mod_mix";

const int LED_BI = IO3;
const int LED_UNI = IO4;

static void test_mode(void);
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

static void test_mode(void) {
    ESP_LOGI(TAG, "Testing mode");

    mcp0.pinMode(LED_BI, INPUT);
    mcp0.pinMode(LED_UNI, INPUT);

    bool bi = mcp0.digitalRead(LED_BI);
    bool uni = mcp0.digitalRead(LED_UNI);

    ESP_LOGI(TAG, "LED_BI: %d, LED_UNI: %d", bi, uni);
}