// Aliases from data/modules/09_mod_jacket:
// alias a_io_1=out A
// alias a_io_2=0
// alias a_jack=in A
// alias b_io_1=out B
// alias b_io_2=1
// alias b_jack=in B
// alias c_io_1=out C
// alias c_io_2=2
// alias c_jack=in C

void mod_jacket_init() {
    hal_set_io(IO0, IO_INPUT);
    hal_set_io(IO1, IO_INPUT);
    hal_set_io(IO2, IO_INPUT);
}

void mod_jacket_handler() {
    hal_start_signal(SOURCE_A, 100.0f, WAVEFORM_SQUARE);
    delay(200);
    hal_stop_signal(SOURCE_A);
    hal_start_signal(SOURCE_B, 150.0f, WAVEFORM_SQUARE);
    delay(200);
    hal_stop_signal(SOURCE_B);
    hal_start_signal(SOURCE_C, 200.0f, WAVEFORM_SQUARE);
    delay(200);
    hal_stop_signal(SOURCE_C);
}

#define MOD_JACKET_ID 9

module_descriptor mod_jacket = {
    .name = "mod-jacket",
    .init = mod_jacket_init,
    .handler = mod_jacket_handler,
};

