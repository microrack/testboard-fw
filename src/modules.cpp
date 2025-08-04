#include "modules.h"
#include "display.h"
#include "esp_log.h"
#include <LittleFS.h>
#include "board.h"
#include <cstring>
#include <cstdlib>
#include "test_results.h"

static const char* TAG = "modules";

// Global variables for module storage
static module_info_t* modules = nullptr;
static size_t modules_count = 0;
static test_operation_t* operations_buffer = nullptr;
static size_t operations_buffer_size = 0;

// Helper function to convert string to source_net_t
static source_net_t string_to_source(const char* str) {
    if (strcmp(str, "A") == 0) return SOURCE_A;
    if (strcmp(str, "B") == 0) return SOURCE_B;
    if (strcmp(str, "C") == 0) return SOURCE_C;
    if (strcmp(str, "D") == 0) return SOURCE_D;
    return SOURCE_A; // Default
}

// Helper function to convert string to io_state_t
static io_state_t string_to_io_state(const char* str) {
    if (strcmp(str, "h") == 0) return IO_HIGH;
    if (strcmp(str, "l") == 0) return IO_LOW;
    if (strcmp(str, "z") == 0) return IO_INPUT;
    return IO_INPUT; // Default
}

// Helper function to convert string to current rail
static int string_to_current_rail(const char* str) {
    if (strcmp(str, "+12") == 0) return 0; // Maps to PIN_INA_12V
    if (strcmp(str, "+5") == 0) return 1;  // Maps to PIN_INA_5V
    if (strcmp(str, "-12") == 0) return 2; // Maps to PIN_INA_M12V
    return 0; // Default
}

// Helper function to convert string to voltage pin
static ADC_sink_t string_to_voltage_pin(const char* str) {
    if (strcmp(str, "A") == 0) return ADC_sink_1k_A;
    if (strcmp(str, "B") == 0) return ADC_sink_1k_B;
    if (strcmp(str, "C") == 0) return ADC_sink_1k_C;
    if (strcmp(str, "D") == 0) return ADC_sink_1k_D;
    if (strcmp(str, "E") == 0) return ADC_sink_1k_E;
    if (strcmp(str, "F") == 0) return ADC_sink_1k_F;
    if (strcmp(str, "pdA") == 0) return ADC_sink_PD_A;
    if (strcmp(str, "pdB") == 0) return ADC_sink_PD_B;
    if (strcmp(str, "pdC") == 0) return ADC_sink_PD_C;
    if (strcmp(str, "zD") == 0) return ADC_sink_Z_D;
    if (strcmp(str, "zE") == 0) return ADC_sink_Z_E;
    if (strcmp(str, "zF") == 0) return ADC_sink_Z_F;
    return ADC_sink_1k_A; // Default
}

// Helper function to skip whitespace
static const char* skip_whitespace(const char* str) {
    while (*str == ' ' || *str == '\t') str++;
    return str;
}

// Helper function to get next token
static const char* get_token(const char* str, char* token, size_t max_len) {
    str = skip_whitespace(str);
    size_t i = 0;
    while (*str && *str != ' ' && *str != '\t' && *str != '\n' && *str != '\r' && i < max_len - 1) {
        token[i++] = *str++;
    }
    token[i] = '\0';
    return str;
}

bool init_modules_from_fs() {
    ESP_LOGI(TAG, "Initializing modules from filesystem");
    
    if (!LittleFS.begin(true)) {
        ESP_LOGE(TAG, "Failed to mount LittleFS");
        return false;
    }
    
    File file = LittleFS.open("/config", "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open config");
        return false;
    }
    
    // First pass: count modules and operations
    size_t total_operations = 0;
    size_t module_count = 0;
    file.seek(0);
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        
        if (line.length() == 0 || line.startsWith("#")) {
            continue;
        }
        
        if (line.startsWith("module ")) {
            module_count++;
        } else if (line.length() > 0) {
            total_operations++;
        }
    }
    
    ESP_LOGI(TAG, "Found %zu modules with %zu total operations", module_count, total_operations);
    
    // Allocate memory
    if (operations_buffer) {
        free(operations_buffer);
    }
    if (modules) {
        free(modules);
    }
    
    operations_buffer = (test_operation_t*)malloc(total_operations * sizeof(test_operation_t));
    modules = (module_info_t*)malloc(module_count * sizeof(module_info_t));
    

    
    if (!operations_buffer || !modules) {
        ESP_LOGE(TAG, "Failed to allocate memory for modules");
        file.close();
        return false;
    }
    
    modules_count = module_count;
    operations_buffer_size = total_operations;
    
    // Second pass: parse modules and operations
    file.seek(0);
    size_t operation_index = 0;
    size_t module_index = 0;
    module_info_t* current_module = nullptr;
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        
        if (line.length() == 0 || line.startsWith("#")) {
            continue;
        }
        
        if (line.startsWith("module ")) {
            // Parse module line: "module <name> <id>"
            const char* line_str = line.c_str();
            char token[64];
            
            line_str = get_token(line_str, token, sizeof(token)); // Skip "module"
            line_str = get_token(line_str, token, sizeof(token)); // Get name
            
            char* name = strdup(token);
            line_str = get_token(line_str, token, sizeof(token)); // Get id
            
            current_module = &modules[module_index];
            current_module->id = atoi(token);
            current_module->name = name;
            current_module->test_operations = &operations_buffer[operation_index];
            current_module->test_operations_count = 0;
            current_module->test_results = nullptr; // Will be allocated when needed
            
            module_index++;
            ESP_LOGI(TAG, "Parsing module: %s (ID: %d)", name, current_module->id);
            
        } else if (current_module && line.length() > 0) {
            // Parse operation line
            const char* line_str = line.c_str();
            char token[64];
            test_operation_t& op = operations_buffer[operation_index];
            
            // Check for repeat flag (+ at end of line)
            bool repeat_flag = false;
            if (line.endsWith("+")) {
                repeat_flag = true;
                line = line.substring(0, line.length() - 1); // Remove the +
                line_str = line.c_str();
            }
            
            line_str = get_token(line_str, token, sizeof(token));
            
            if (strcmp(token, "src") == 0) {
                op.op = TEST_OP_SOURCE;
                op.repeat = repeat_flag;
                
                line_str = get_token(line_str, token, sizeof(token));
                op.pin = string_to_source(token);
                
                line_str = get_token(line_str, token, sizeof(token));
                op.arg1 = atoi(token);
                op.arg2 = 0;
                
            } else if (strcmp(token, "src_sig") == 0) {
                op.op = TEST_OP_SOURCE_SIG;
                op.repeat = repeat_flag;
                
                line_str = get_token(line_str, token, sizeof(token));
                op.pin = string_to_source(token);
                
                line_str = get_token(line_str, token, sizeof(token));
                op.arg1 = atoi(token); // Frequency in Hz
                op.arg2 = 0;
                
            } else if (strcmp(token, "io") == 0) {
                op.op = TEST_OP_IO;
                op.repeat = repeat_flag;
                
                line_str = get_token(line_str, token, sizeof(token));
                op.pin = atoi(token);
                
                line_str = get_token(line_str, token, sizeof(token));
                op.arg1 = string_to_io_state(token);
                op.arg2 = 0;
                
            } else if (strcmp(token, "pd") == 0) {
                op.op = TEST_OP_SINK_PD;
                op.repeat = repeat_flag;
                
                line_str = get_token(line_str, token, sizeof(token));
                op.pin = 0; // Assuming PIN_SINK_PD_A
                
                line_str = get_token(line_str, token, sizeof(token));
                op.arg1 = (strcmp(token, "p") == 0) ? 1 : 0;
                op.arg2 = 0;
                
            } else if (strcmp(token, "i") == 0) {
                op.op = TEST_OP_CHECK_CURRENT;
                op.repeat = repeat_flag;
                
                line_str = get_token(line_str, token, sizeof(token));
                op.pin = string_to_current_rail(token);
                
                line_str = get_token(line_str, token, sizeof(token));
                op.arg1 = atoi(token);
                
                line_str = get_token(line_str, token, sizeof(token));
                op.arg2 = atoi(token);
                
            } else if (strcmp(token, "v") == 0) {
                op.op = TEST_OP_CHECK_PIN;
                op.repeat = repeat_flag;
                
                line_str = get_token(line_str, token, sizeof(token));
                op.pin = string_to_voltage_pin(token);
                
                line_str = get_token(line_str, token, sizeof(token));
                op.arg1 = atoi(token);
                
                line_str = get_token(line_str, token, sizeof(token));
                op.arg2 = atoi(token);
                
            } else if (strcmp(token, "reset") == 0) {
                op.op = TEST_OP_RESET;
                op.repeat = repeat_flag;
                op.pin = 0;  // Not used for reset
                op.arg1 = 0; // Not used for reset
                op.arg2 = 0; // Not used for reset
                
            } else {
                ESP_LOGW(TAG, "Unknown operation: %s", token);
                continue;
            }
            
            operation_index++;
            current_module->test_operations_count++;
        }
    }
    
    file.close();
    
    ESP_LOGI(TAG, "Successfully loaded %zu modules with %zu total operations", 
             modules_count, total_operations);
    return true;
}

module_info_t* get_module_info(uint8_t id) {
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

module_info_t* get_modules_array() {
    return modules;
}

size_t get_modules_count() {
    return modules_count;
}

bool execute_module_tests(module_info_t* module) {
    if (!module) {
        ESP_LOGE(TAG, "Module info is null");
        return false;
    }
    
    ESP_LOGI(TAG, "Executing tests for module: %s", module->name);
    
    // If module has test operations, use declarative approach
    if (module->test_operations && module->test_operations_count > 0) {
        // Get test results array from global storage
        extern test_operation_result_t* get_global_test_results();
        test_operation_result_t* global_results = get_global_test_results();
        
        if (!global_results) {
            ESP_LOGE(TAG, "Test results array not available for module: %s", module->name);
            return false;
        }
        
        bool success = execute_test_sequence(module->test_operations, module->test_operations_count, global_results);
        
        return success;
    }
    
    ESP_LOGE(TAG, "No test operations for module: %s", module->name);
    return false;
}
