// Aliases from data/modules/10_mod_mix:
// alias a_input=0
// alias a_pot=in zD
// alias b_input=1
// alias b_pot=in zE
// alias bi_mode=3
// alias c_input_1=out D
// alias c_input_2=2
// alias c_pot=in zF
// alias gains_1=out A
// alias gains_2=out B
// alias gains_3=out C
// alias output_ab_1=in A
// alias output_ab_2=in B
// alias output_cd_1=in C
// alias output_cd_2=in D
// alias outputs_1=in E
// alias outputs_2=in F
// alias outputs_3=in pdA
// alias p5_m5_1=in pdB
// alias p5_m5_2=in pdC
// alias uni_mode=4

void mod_mix_init() {
    hal_start_signal(SOURCE_D, 200, WAVEFORM_SAWTOOTH);
    hal_set_io(IO0, IO_INPUT);
    hal_set_io(IO1, IO_INPUT);
    hal_set_io(IO2, IO_INPUT);

    hal_set_source(SOURCE_A, 0);
    hal_set_source(SOURCE_B, 0);
    hal_set_source(SOURCE_C, 0);
}

void mod_mix_handler() {
    hal_set_source(SOURCE_A, 5000); delay(50);
    hal_set_source(SOURCE_A, -5000); delay(50);
    hal_set_source(SOURCE_A, 0); delay(50);

    hal_set_source(SOURCE_B, 5000); delay(50);
    hal_set_source(SOURCE_B, -5000); delay(50);
    hal_set_source(SOURCE_B, 0); delay(50);


    hal_set_source(SOURCE_C, 5000); delay(50);
    hal_set_source(SOURCE_C, -5000); delay(50);
    hal_set_source(SOURCE_C, 0); delay(50);
}

#define MOD_MIX_ID 10

module_descriptor mod_mix = {
    .name = "mod-mix",
    .init = mod_mix_init,
    .handler = mod_mix_handler,
};

