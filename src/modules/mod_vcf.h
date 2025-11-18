// Aliases from data/modules/16_mod_vcf:
// alias cutoff_1=out D
// alias cutoff_2=2
// alias cutoff_pot=in zE
// alias depth_pot=in zD
// alias hp_mode=1
// alias input_1=out A
// alias input_2=out B
// alias lp_mode=0
// alias output_ab_1=in A
// alias output_ab_2=in B
// alias output_cd_1=in C
// alias output_cd_2=in D
// alias reso_pot=in pdC
// alias signal_hp=out B
// alias signal_lp=out A

void mod_vcf_init() {
    hal_set_source(SOURCE_D, 0);
    hal_set_io(IO2, IO_LOW);
    hal_start_signal(SOURCE_A, 140.0f, WAVEFORM_SAWTOOTH);
    hal_start_signal(SOURCE_B, 140.0f * 3/4, WAVEFORM_SAWTOOTH);
}

void mod_vcf_handler() {
    hal_set_source(SOURCE_D, 5000); delay(100); hal_set_source(SOURCE_D, 0); delay(100);
    hal_set_io(IO2, IO_HIGH); delay(100); hal_set_io(IO2, IO_LOW); delay(100);
}

#define MOD_VCF_ID 16

module_descriptor mod_vcf = {
    .name = "mod-vcf",
    .init = mod_vcf_init,
    .handler = mod_vcf_handler,
};

