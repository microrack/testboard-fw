#include "test_results.h"
#include "modules.h"
#include "esp_log.h"
#include <LittleFS.h>

static const char* TAG = "test_results";

// Global variables for test results management
static test_operation_result_t** global_test_results = nullptr;
static size_t global_modules_count = 0;

bool allocate_test_results_arrays() {
    ESP_LOGI(TAG, "Allocating test results arrays");
    
    // Get modules count
    global_modules_count = get_modules_count();
    if (global_modules_count == 0) {
        ESP_LOGE(TAG, "No modules found");
        return false;
    }
    
    // Allocate array of pointers to test results
    global_test_results = (test_operation_result_t**)malloc(global_modules_count * sizeof(test_operation_result_t*));
    if (!global_test_results) {
        ESP_LOGE(TAG, "Failed to allocate global test results array");
        return false;
    }
    
    // Initialize all pointers to nullptr and allocate memory for each module
    for (size_t i = 0; i < global_modules_count; i++) {
        global_test_results[i] = nullptr;
        
        // Get module info to know how many operations to allocate
        module_info_t* modules = get_modules_array();
        if (modules && i < global_modules_count) {
            size_t operations_count = modules[i].test_operations_count;
            if (operations_count > 0) {
                global_test_results[i] = (test_operation_result_t*)malloc(operations_count * sizeof(test_operation_result_t));
                if (!global_test_results[i]) {
                    ESP_LOGE(TAG, "Failed to allocate test results for module %zu", i);
                    return false;
                }
                ESP_LOGI(TAG, "Allocated %zu test results for module %zu", operations_count, i);
            }
        }
    }
    
    ESP_LOGI(TAG, "Test results arrays allocated for %zu modules", global_modules_count);
    return true;
}

void reset_all_test_results() {
    ESP_LOGI(TAG, "Resetting all test results");
    
    if (!global_test_results) {
        ESP_LOGE(TAG, "Test results arrays not allocated");
        return;
    }
    
    for (size_t i = 0; i < global_modules_count; i++) {
        if (global_test_results[i]) {
            // Get module info to know how many operations to reset
            module_info_t* modules = get_modules_array();
            if (modules && i < global_modules_count) {
                size_t operations_count = modules[i].test_operations_count;
                for (size_t j = 0; j < operations_count; j++) {
                    global_test_results[i][j].passed = false;
                    global_test_results[i][j].result = 0;
                }
                ESP_LOGI(TAG, "Reset %zu test results for module %zu", operations_count, i);
            }
        }
    }
}

void print_all_test_results() {
    ESP_LOGI(TAG, "=== PRINTING ALL TEST RESULTS ===");
    
    if (!global_test_results) {
        ESP_LOGE(TAG, "Test results arrays not allocated");
        return;
    }
    
    module_info_t* modules = get_modules_array();
    
    for (size_t i = 0; i < global_modules_count; i++) {
        if (global_test_results[i] && modules && i < global_modules_count) {
            const module_info_t& module = modules[i];
            ESP_LOGI(TAG, "Module %zu: %s (ID: %d)", i, module.name, module.id);
            
            for (size_t j = 0; j < module.test_operations_count; j++) {
                const test_operation_t& op = module.test_operations[j];
                const test_operation_result_t& res = global_test_results[i][j];
                
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
        }
    }
    ESP_LOGI(TAG, "=== END TEST RESULTS ===");
}

test_operation_result_t** get_global_test_results() {
    return global_test_results;
}

void save_all_test_results() {
    ESP_LOGI(TAG, "Saving test results to file");
    
    if (!global_test_results) {
        ESP_LOGE(TAG, "Test results arrays not allocated");
        return;
    }
    
    module_info_t* modules = get_modules_array();
    if (!modules) {
        ESP_LOGE(TAG, "Modules array not available");
        return;
    }
    
    // Find the current module (assuming we're testing one module at a time)
    module_info_t* current_module = nullptr;
    size_t current_module_index = 0;
    
    for (size_t i = 0; i < global_modules_count; i++) {
        if (global_test_results[i] && modules[i].test_operations_count > 0) {
            current_module = &modules[i];
            current_module_index = i;
            break;
        }
    }
    
    if (!current_module) {
        ESP_LOGE(TAG, "No current module found for saving results");
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
        const test_operation_result_t& res = global_test_results[current_module_index][j];
        file.printf("%s %ld\n", res.passed ? "true" : "false", res.result);
    }
    
    file.close();
    ESP_LOGI(TAG, "Test results saved to /results for module: %s", current_module->name);
} 