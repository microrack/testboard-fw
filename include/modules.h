#pragma once

#include <stdint.h>
#include "test_helpers.h"
#include "mod_mix.h"
#include "mod_jacket.h"

// Module IDs
#define MODULE_ID_MIX 10
#define MODULE_ID_JACKET 11

// Module handler function type
typedef bool (*module_handler_t)(void);

// Module information structure
typedef struct {
    uint8_t id;
    const char* name;
    module_handler_t handler;
} module_info_t;

// Get module info by ID
const module_info_t* get_module_info(uint8_t id);

// Module handlers
bool mod_unknown_handler(void); 