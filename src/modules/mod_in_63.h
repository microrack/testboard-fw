// Aliases from data/modules/08_mod_in_63:
// alias dc_mode=1
// alias input_a=9 + A
// alias input_b=10 + B
// alias level_a_pot=in pdA
// alias level_b_pot=in pdB
// alias nc=out D
// alias output_a_1=in A
// alias output_a_2=in B
// alias output_a_3=in C
// alias output_b_1=in D
// alias output_b_2=in E
// alias output_b_3=in F
// alias pullup_mode=0
// alias signal_a=4
// alias signal_b=5

void mod_in_63_init() {
    hal_start_signal(SOURCE_A, 140.0f, WAVEFORM_SAWTOOTH);
    hal_start_signal(SOURCE_B, 140.0f, WAVEFORM_SAWTOOTH);

    hal_set_io(IO9, IO_INPUT);
    hal_set_io(IO10, IO_INPUT);
}

void mod_in_63_handler() {
    hal_set_io(IO9, IO_INPUT); delay(100); hal_set_io(IO9, IO_LOW); delay(100);
    hal_set_io(IO10, IO_INPUT); delay(100); hal_set_io(IO10, IO_LOW); delay(100);
}

#define MOD_IN_63_ID 8

module_descriptor mod_in_63 = {
    .name = "mod-in-63",
    .init = mod_in_63_init,
    .handler = mod_in_63_handler,
};

