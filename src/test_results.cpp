#include "test_results.h"
#include "modules.h"
#include "esp_log.h"
#include <LittleFS.h>

static const char* TAG = "test_results";

// Global variables for test results management
static test_operation_result_t* global_test_results = nullptr;
static module_info_t* current_module = nullptr;

bool allocate_test_results_arrays(module_info_t* module) {
    ESP_LOGI(TAG, "Allocating test results array for module: %s", module ? module->name : "NULL");
    
    // Set current module
    current_module = module;
    
    // Allocate memory for test results
    if (module->test_operations_count > 0) {
        global_test_results = (test_operation_result_t*)malloc(module->test_operations_count * sizeof(test_operation_result_t));
        if (!global_test_results) {
            ESP_LOGE(TAG, "Failed to allocate test results for module %s", module->name);
            return false;
        }
        ESP_LOGI(TAG, "Allocated %zu test results for module %s", module->test_operations_count, module->name);
    } else {
        ESP_LOGW(TAG, "Module %s has no test operations", module->name);
        global_test_results = nullptr;
    }
    
    ESP_LOGI(TAG, "Test results array allocated for module: %s", module->name);
    return true;
}

void reset_all_test_results() {    
    if (!global_test_results || !current_module) {
        ESP_LOGE(TAG, "Test results array not allocated or no current module");
        return;
    }
    
    for (size_t j = 0; j < current_module->test_operations_count; j++) {
        global_test_results[j].passed = false;
        global_test_results[j].result = 0;
    }
}

void print_all_test_results() {
    ESP_LOGI(TAG, "=== PRINTING TEST RESULTS ===");
    
    if (!global_test_results || !current_module) {
        ESP_LOGE(TAG, "Test results array not allocated or no current module");
        return;
    }
    
    ESP_LOGI(TAG, "Module: %s (ID: %d)", current_module->name, current_module->id);
    
    for (size_t j = 0; j < current_module->test_operations_count; j++) {
        const test_operation_t& op = current_module->test_operations[j];
        const test_operation_result_t& res = global_test_results[j];
        
        ESP_LOGI(TAG, "  Operation %zu: %s (pin: %d, arg1: %ld, arg2: %ld)", 
                 j, 
                 (op.op == TEST_OP_SOURCE) ? "SOURCE" :
                 (op.op == TEST_OP_IO) ? "IO" :
                 (op.op == TEST_OP_SINK_PD) ? "SINK_PD" :
                 (op.op == TEST_OP_CHECK_CURRENT) ? "CHECK_CURRENT" :
                 (op.op == TEST_OP_CHECK_PIN) ? "CHECK_PIN" :
                 (op.op == TEST_OP_RESET) ? "RESET" : "UNKNOWN",
                 op.pin, op.arg1, op.arg2);
        
        ESP_LOGI(TAG, "    Flag: %s, Result: %ld", 
                 res.passed ? "TRUE" : "FALSE", 
                 res.result);
    }
    ESP_LOGI(TAG, "=== END TEST RESULTS ===");
}

test_operation_result_t* get_global_test_results() {
    return global_test_results;
}

module_info_t* get_current_module() {
    return current_module;
}

void save_all_test_results() {
    ESP_LOGI(TAG, "Saving test results to file");
    
    if (!global_test_results || !current_module) {
        ESP_LOGE(TAG, "Test results array not allocated or no current module");
        return;
    }
    
    // Open file for writing
    File file = LittleFS.open("/results", "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open results file for writing");
        return;
    }
    
    // Write module name in first line
    file.println(current_module->name);
    
    // Write results for each operation
    for (size_t j = 0; j < current_module->test_operations_count; j++) {
        const test_operation_result_t& res = global_test_results[j];
        file.printf("%s %ld\n", res.passed ? "true" : "false", res.result);
    }
    
    file.close();
    ESP_LOGI(TAG, "Test results saved to /results for module: %s", current_module->name);
} 