// Aliases from data/modules/02_mod_comp:
// alias comp_pot=in pdC
// alias env_out_1=in pdA
// alias env_out_2=in pdB
// alias fast_mode=1
// alias input_1=out C
// alias input_2=out D
// alias output_1=in A
// alias output_2=in B
// alias sidechain_1=out A
// alias sidechain_2=out B
// alias sidechain_mode=0
// alias slow_mode=2
// alias speed_mode=3

void mod_comp_init() {
    hal_start_signal(SOURCE_C, 140.0f, WAVEFORM_SAWTOOTH);
}

void mod_comp_handler() {
    hal_start_signal(SOURCE_A, 140.0f, WAVEFORM_SAWTOOTH); delay(100); hal_stop_signal(SOURCE_A); delay(300);
    hal_start_signal(SOURCE_B, 140.0f, WAVEFORM_SAWTOOTH); delay(100); hal_stop_signal(SOURCE_B); delay(300);
}

#define MOD_COMP_ID 2

module_descriptor mod_comp = {
    .name = "mod-comp",
    .init = mod_comp_init,
    .handler = mod_comp_handler,
};

