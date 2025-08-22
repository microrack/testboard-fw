#ifndef TEST_RESULTS_H
#define TEST_RESULTS_H

#include "test_helpers.h"
#include "modules.h"

bool allocate_test_results_arrays(module_info_t* module);
void reset_all_test_results();
void print_all_test_results();
void display_all_test_results();
test_operation_result_t* get_global_test_results();
module_info_t* get_current_module();
void save_all_test_results();

#endif // TEST_RESULTS_H 