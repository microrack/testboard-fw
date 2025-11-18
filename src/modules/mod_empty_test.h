// Aliases from data/modules/00_empty_test:
// (no aliases defined)

void mod_empty_test_init() {
    
}

void mod_empty_test_handler() {
    
}

#define MOD_EMPTY_TEST_ID 0

module_descriptor mod_empty_test = {
    .name = "empty-test",
    .init = mod_empty_test_init,
    .handler = mod_empty_test_handler,
};

