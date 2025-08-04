#pragma once

#include <stdint.h>
#include <stddef.h>
#include "test_helpers.h"
#include "modules.h"

// Test results management functions
bool allocate_test_results_arrays();
void reset_all_test_results();
void print_all_test_results();
void save_all_test_results();
test_operation_result_t** get_global_test_results(); 