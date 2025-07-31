#include "modules.h"
#include "display.h"
#include "esp_log.h"

static const char* TAG = "modules";

// Module definitions
static const module_info_t modules[] = {
    {MODULE_ID_JACKET, "mod_jacket", jacket_test_operations, jacket_test_operations_count}, // Автоматический подсчет
    // Add more modules here as they are implemented
    {0, "unknown", nullptr, 0}  // Default handler for unknown modules
};

const module_info_t* get_module_info(uint8_t id) {
    for (size_t i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
        if (modules[i].id == id) {
            return &modules[i];
        }
    }
    return &modules[sizeof(modules) / sizeof(modules[0]) - 1]; // Return unknown module
}

// Function to execute module tests using declarative approach
bool execute_module_tests(const module_info_t* module) {
    if (!module) {
        ESP_LOGE(TAG, "Module info is null");
        return false;
    }
    
    ESP_LOGI(TAG, "Executing tests for module: %s", module->name);
    
    // If module has test operations, use declarative approach
    if (module->test_operations && module->test_operations_count > 0) {
        return execute_test_sequence(module->test_operations, module->test_operations_count);
    }
    
    ESP_LOGE(TAG, "No test operations for module: %s", module->name);
    return false;
} 