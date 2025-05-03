#pragma once

#include <stdint.h>
#include "test_helpers.h"
#include "mod_mix.h"

// Module IDs
#define MODULE_ID_MIX 10

// Module handler function type
typedef test_result_t (*module_handler_t)(void);

// Module information structure
typedef struct {
    uint8_t id;
    const char* name;
    module_handler_t handler;
} module_info_t;

// Get module info by ID
const module_info_t* get_module_info(uint8_t id);

// Module handlers
test_result_t mod_unknown_handler(void); 