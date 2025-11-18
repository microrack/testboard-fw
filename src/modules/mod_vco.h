// Aliases from data/modules/17_mod_vco:
// alias bi_mode=10
// alias fm_1=out B
// alias fm_2=1
// alias fnie_fm_pot=in pdB
// alias lfo_mode=6
// alias pitch_pot=in pdA
// alias pulse_out_1=in C
// alias pulse_out_2=in D
// alias saw_out_1=in A
// alias saw_out_2=in B
// alias sync_1=8
// alias sync_2=9
// alias tri_out_1=in E
// alias tri_out_2=in F
// alias uni_mode=11
// alias v_oct_1=out A
// alias v_oct_2=0
// alias vco_mode=7

void mod_vco_init() {
    
}

void mod_vco_handler() {
    
}

#define MOD_VCO_ID 17

module_descriptor mod_vco = {
    .name = "mod-vco",
    .init = mod_vco_init,
    .handler = mod_vco_handler,
};

