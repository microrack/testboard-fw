// Aliases from data/modules/16_mod_vcf:
// alias cutoff_1=out D
// alias cutoff_2=2
// alias cutoff_pot=in zE
// alias depth_pot=in zD
// alias hp_mode=1
// alias input_1=in pdA
// alias input_2=in pdB
// alias lp_mode=0
// alias output_ab_1=in A
// alias output_ab_2=in B
// alias output_cd_1=in C
// alias output_cd_2=in D
// alias reso_pot=in pdC
// alias signal_hp=out B
// alias signal_lp=out A

void mod_vcf_init() {
    
}

void mod_vcf_handler() {
    
}

#define MOD_VCF_ID 16

module_descriptor mod_vcf = {
    .name = "mod-vcf",
    .init = mod_vcf_init,
    .handler = mod_vcf_handler,
};

