// Aliases from data/modules/06_mod_eq:
// alias high_pot=in pdA
// alias input_1=out A
// alias input_2=out B
// alias low_pot=in pdC
// alias mid_pot=in pdB
// alias output_ab_1=in A
// alias output_ab_2=in B
// alias output_cd_1=in C
// alias output_cd_2=in D

void mod_eq_init() {
    hal_start_signal(SOURCE_A, 140.0f, WAVEFORM_SAWTOOTH);
    hal_start_signal(SOURCE_B, 140.0f * 3/4, WAVEFORM_SAWTOOTH);
}

void mod_eq_handler() {
    
}

#define MOD_EQ_ID 6

module_descriptor mod_eq = {
    .name = "mod-eq",
    .init = mod_eq_init,
    .handler = mod_eq_handler,
};

