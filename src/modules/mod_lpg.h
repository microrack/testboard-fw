// Aliases from data/modules/11_mod_lpg:
// alias comb_mode=1
// alias cv_1=out D
// alias cv_2=4
// alias depth_pot=in pdB
// alias input_1=out A
// alias input_2=3
// alias level=in pdA
// alias lpg_mode=0
// alias output_ab_1=in A
// alias output_ab_2=in B
// alias output_cd_1=in C
// alias output_cd_2=in D
// alias trig_1=out B
// alias trig_2=2

void mod_lpg_init() {
    hal_set_source(SOURCE_D, -5000);
    hal_set_io(IO4, IO_LOW);
    hal_set_source(SOURCE_B, -5000);
    hal_set_io(IO2, IO_LOW);
}

void mod_lpg_handler() {
    hal_start_signal(SOURCE_A, 140.0f, WAVEFORM_SAWTOOTH);
    hal_set_source(SOURCE_D, 5000); delay(5); hal_set_source(SOURCE_D, -5000); delay(200);
    hal_set_source(SOURCE_B, 5000); delay(20); hal_set_source(SOURCE_B, -5000); delay(200);
    hal_start_signal(SOURCE_A, 70.0f, WAVEFORM_SAWTOOTH);
    hal_set_io(IO4, IO_HIGH); delay(5); hal_set_io(IO4, IO_LOW); delay(200);
    hal_set_io(IO2, IO_HIGH); delay(20); hal_set_io(IO2, IO_LOW); delay(200);
}

#define MOD_LPG_ID 11

module_descriptor mod_lpg = {
    .name = "mod-lpg",
    .init = mod_lpg_init,
    .handler = mod_lpg_handler,
};
