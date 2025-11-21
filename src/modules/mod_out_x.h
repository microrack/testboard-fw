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
    hal_start_signal(SOURCE_A, 140.0f, WAVEFORM_SAWTOOTH);
    hal_start_signal(SOURCE_B, 140.0f * 3/4, WAVEFORM_SAWTOOTH);
}

void mod_out_x_handler() {
    hal_set_io(IO0, IO_HIGH); delay(1); hal_set_io(IO0, IO_LOW); delay(50);
    hal_set_io(IO2, IO_HIGH); delay(1); hal_set_io(IO2, IO_LOW); delay(50);
    hal_set_io(IO5, IO_HIGH); delay(1); hal_set_io(IO5, IO_LOW); delay(50);
    hal_set_io(IO7, IO_HIGH); delay(1); hal_set_io(IO7, IO_LOW); delay(50);
}

#define MOD_OUT_X_ID 14

module_descriptor mod_out_x = {
    .name = "mod-out-x",
    .init = mod_out_x_init,
    .handler = mod_out_x_handler,
};

