#include "modules.h"
#include "display.h"
#include "esp_log.h"

static const char* TAG = "modules";

// Module definitions
static const module_info_t modules[] = {
    {MODULE_ID_MIX, "mod_mix", mod_mix_handler},
    {MODULE_ID_JACKET, "mod_jacket", mod_jacket_handler},
    // Add more modules here as they are implemented
    {0, "unknown", mod_unknown_handler}  // Default handler for unknown modules
};

const module_info_t* get_module_info(uint8_t id) {
    for (size_t i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
        if (modules[i].id == id) {
            return &modules[i];
        }
    }
    return &modules[sizeof(modules) / sizeof(modules[0]) - 1]; // Return unknown module
}

// Module handlers implementation
bool mod_unknown_handler(void) {
    ESP_LOGW(TAG, "Unknown module detected");
    display_printf("Unknown module type");
    return false;
} 