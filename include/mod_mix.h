#pragma once

#include "hal.h"
#include "test_helpers.h"

typedef enum {
    MODE_UNI,
    MODE_BI
} mix_mode_t;

/**
 * @brief Structure defining current ranges for mode detection
 */
typedef struct {
    range_t active;    // Current range for active mode in mA
    range_t inactive;  // Current range for inactive mode in mA
} mode_current_ranges_t;

/**
 * @brief Handler for mod_mix module testing
 * 
 * This function implements the test sequence for the mod_mix module.
 * It is called when a module with ID MODULE_ID_MIX is detected.
 * @return true if the test was successful, false otherwise
 */
bool mod_mix_handler(void);

void set_gain(float input_gains[3]); 