#pragma once

#include "hal.h"
#include "test_helpers.h"

/**
 * @brief Handler for mod_mix module testing
 * 
 * This function implements the test sequence for the mod_mix module.
 * It is called when a module with ID MODULE_ID_MIX is detected.
 * @return bool indicating the test result: true for success, false for failure
 */
bool mod_mix_handler(void); 