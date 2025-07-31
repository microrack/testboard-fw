#include "mod_jacket.h"
#include "display.h"
#include "esp_log.h"
#include "board.h"
#include "hal.h"
#include "test_helpers.h"

static const char* TAG = "mod_jacket";

bool mod_jacket_handler(void) {
    hal_clear_console();
    ESP_LOGI(TAG, "Starting mod_jacket test sequence");
    
    display_printf("Jacket Module Test\nStarting...");
    
    // Set SOURCE_A, B, C to 0
    hal_set_source(SOURCE_A, 0.0f);
    hal_set_source(SOURCE_B, 0.0f);
    hal_set_source(SOURCE_C, 0.0f);
    
    // Set IO 0, 1, 2 to HiZ (input mode)
    mcp0.pinMode(IO0, INPUT);
    mcp0.pinMode(IO1, INPUT);
    mcp0.pinMode(IO2, INPUT);
    
    delay(1); // Small delay to allow settings to take effect
    
    // TEST_RUN: check current
    // on 12V must be < 1 mA
    TEST_RUN(check_current(INA_PIN_12V, {0.0f, 1.0f}, "+12V"));
    
    // on -12V must be < 1 mA
    TEST_RUN(check_current(INA_PIN_M12V, {0.0f, 1.0f}, "-12V"));
    
    // on +5V must be between 5 and 10 mA
    TEST_RUN(check_current(INA_PIN_5V, {5.0f, 10.0f}, "+5V"));
    
    display_printf("Jacket Module Test\nCompleted Successfully");
    ESP_LOGI(TAG, "mod_jacket test sequence completed successfully");
    
    return true;
} 