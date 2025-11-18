// Aliases from data/modules/14_mod_out_x:
// alias a_input_1=out A
// alias a_input_2=4
// alias a_input_3=3
// alias a_input_4=2
// alias a_input_5=1
// alias a_input_6=0
// alias a_level=in zD
// alias a_output=in A
// alias b_input_1=out B
// alias b_input_2=9
// alias b_input_3=8
// alias b_input_4=7
// alias b_input_5=6
// alias b_input_6=5
// alias b_level=in zE
// alias b_output=in B
// alias mode=10

void mod_out_x_init() {
    
}

void mod_out_x_handler() {
    
}

#define MOD_OUT_X_ID 14

module_descriptor mod_out_x = {
    .name = "mod-out-x",
    .init = mod_out_x_init,
    .handler = mod_out_x_handler,
};

