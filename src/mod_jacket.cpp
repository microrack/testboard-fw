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
    hal_set_source(SOURCE_A, 0);
    hal_set_source(SOURCE_B, 0);
    hal_set_source(SOURCE_C, 0);
    
    // Set IO 0, 1, 2 to HiZ (input mode)
    hal_set_io(IO0, IO_INPUT);
    hal_set_io(IO1, IO_INPUT);
    hal_set_io(IO2, IO_INPUT);
    
    delay(1); // Small delay to allow settings to take effect
    
    // TEST_RUN: check current (in microamps)
    TEST_RUN(check_current(INA_PIN_12V, range_t{0, 1000}, "+12V"));
    TEST_RUN(check_current(INA_PIN_M12V, range_t{0, 2000}, "-12V"));
    TEST_RUN(check_current(INA_PIN_5V, range_t{2000, 6000}, "+5V"));
    
    // Test IO channels
    // Test A channel: IO0 in HiZ, SOURCE_A 2V
    hal_set_source(SOURCE_A, 2000);
    TEST_RUN(test_pin_range(ADC_sink_1k_A, range_t{120, 190}, "A"));
    
    // Test A channel: SOURCE_A 0V, IO0 in 1
    hal_set_source(SOURCE_A, 0);
    hal_set_io(IO0, IO_HIGH);
    TEST_RUN_REPEAT(test_pin_range(ADC_sink_1k_A, range_t{4100, 5100}, "A"));
    hal_set_io(IO0, IO_INPUT);
    
    // Test B channel: IO1 in HiZ, SOURCE_B 2V
    hal_set_source(SOURCE_B, 2000);
    TEST_RUN(test_pin_range(ADC_sink_1k_B, range_t{120, 190}, "B"));
    
    // Test B channel: SOURCE_B 0V, IO1 in 1
    hal_set_source(SOURCE_B, 0);
    hal_set_io(IO1, IO_HIGH);
    TEST_RUN(test_pin_range(ADC_sink_1k_B, range_t{4100, 5100}, "B"));
    hal_set_io(IO1, IO_INPUT);
    
    // Test C channel: IO2 in HiZ, SOURCE_C 2V
    hal_set_source(SOURCE_C, 2000);
    TEST_RUN(test_pin_range(ADC_sink_1k_C, range_t{120, 190}, "C"));
    
    // Test C channel: SOURCE_C 0V, IO2 in 1
    hal_set_source(SOURCE_C, 0);
    hal_set_io(IO2, IO_HIGH);
    TEST_RUN(test_pin_range(ADC_sink_1k_C, range_t{4100, 5100}, "C"));
    hal_set_io(IO2, IO_INPUT);
    
    display_printf("Jacket Module Test\nCompleted Successfully");
    ESP_LOGI(TAG, "mod_jacket test sequence completed successfully");
    
    return true;
} 