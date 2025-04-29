#pragma once

typedef enum {
    MODE_UNI,
    MODE_BI
} mix_mode_t;

extern mix_mode_t current_mode;

/**
 * @brief Handler for mod_mix module testing
 * 
 * This function implements the test sequence for the mod_mix module.
 * It is called when a module with ID MODULE_ID_MIX is detected.
 * @return true if the test was successful, false otherwise
 */
bool mod_mix_handler(void); 