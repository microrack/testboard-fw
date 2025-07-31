#include "modules.h"
#include "display.h"
#include "esp_log.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

static const char* TAG = "modules";

// Global module storage
static module_info_t* modules = nullptr;
static size_t modules_count = 0;
static test_operation_t* operations_buffer = nullptr;
static size_t operations_buffer_size = 0;

// Helper function to convert string operation type to enum
static test_op_type_t string_to_op_type(const char* op_str) {
    if (strcmp(op_str, "SOURCE") == 0) return TEST_OP_SOURCE;
    if (strcmp(op_str, "IO") == 0) return TEST_OP_IO;
    if (strcmp(op_str, "SINK_PD") == 0) return TEST_OP_SINK_PD;
    if (strcmp(op_str, "CHECK_CURRENT") == 0) return TEST_OP_CHECK_CURRENT;
    if (strcmp(op_str, "CHECK_PIN") == 0) return TEST_OP_CHECK_PIN;
    return TEST_OP_SOURCE; // Default fallback
}

bool init_modules_from_fs() {
    ESP_LOGI(TAG, "Initializing modules from filesystem");
    
    if (!LittleFS.begin(true)) {
        ESP_LOGE(TAG, "Failed to mount LittleFS");
        return false;
    }
    
    File file = LittleFS.open("/modules.json", "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open modules.json");
        return false;
    }
    
    // Parse JSON
    DynamicJsonDocument doc(16384); // 16KB should be enough
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", error.c_str());
        return false;
    }
    
    JsonArray modules_array = doc["modules"];
    if (!modules_array) {
        ESP_LOGE(TAG, "No modules array found in JSON");
        return false;
    }
    
    // Count total operations needed
    size_t total_operations = 0;
    for (JsonObject module : modules_array) {
        JsonArray operations = module["operations"];
        if (operations) {
            total_operations += operations.size();
        }
    }
    
    // Allocate memory
    if (operations_buffer) {
        free(operations_buffer);
    }
    if (modules) {
        free(modules);
    }
    
    operations_buffer = (test_operation_t*)malloc(total_operations * sizeof(test_operation_t));
    modules = (module_info_t*)malloc(modules_array.size() * sizeof(module_info_t));
    
    if (!operations_buffer || !modules) {
        ESP_LOGE(TAG, "Failed to allocate memory for modules");
        return false;
    }
    
    modules_count = modules_array.size();
    operations_buffer_size = total_operations;
    
    // Parse modules
    size_t operation_index = 0;
    size_t module_index = 0;
    
    for (JsonObject module : modules_array) {
        module_info_t& mod = modules[module_index];
        mod.id = module["id"] | 0;
        mod.name = strdup(module["name"] | "unknown");
        mod.test_operations = &operations_buffer[operation_index];
        mod.test_operations_count = 0;
        
        JsonArray operations = module["operations"];
        if (operations) {
            for (JsonObject op : operations) {
                test_operation_t& operation = operations_buffer[operation_index];
                operation.repeat = op["repeat"] | false;
                operation.op = string_to_op_type(op["op"] | "SOURCE");
                operation.pin = op["pin"] | 0;
                operation.arg1 = op["arg1"] | 0;
                operation.arg2 = op["arg2"] | 0;
                
                operation_index++;
                mod.test_operations_count++;
            }
        }
        
        module_index++;
        ESP_LOGI(TAG, "Loaded module %s (ID: %d) with %zu operations", 
                 mod.name, mod.id, mod.test_operations_count);
    }
    
    ESP_LOGI(TAG, "Successfully loaded %zu modules with %zu total operations", 
             modules_count, total_operations);
    return true;
}

const module_info_t* get_module_info(uint8_t id) {
    if (!modules) {
        ESP_LOGE(TAG, "Modules not initialized");
        return nullptr;
    }
    
    for (size_t i = 0; i < modules_count; i++) {
        if (modules[i].id == id) {
            return &modules[i];
        }
    }
    
    ESP_LOGW(TAG, "Module with ID %d not found", id);
    return nullptr;
}

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