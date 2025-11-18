// Aliases from data/modules/04_mod_sat:
// alias bias_1=out C
// alias bias_2=out D
// alias bias_pot=in pdB
// alias drive_pot=in pdA
// alias input_1=out A
// alias input_2=out B
// alias output_ab_1=in A
// alias output_ab_2=in B
// alias output_cd_1=in C
// alias output_cd_2=in D
// alias vol_pot=in pdC

void mod_sat_init() {
    
}

void mod_sat_handler() {
    
}

#define MOD_SAT_ID 4

module_descriptor mod_sat = {
    .name = "mod-sat",
    .init = mod_sat_init,
    .handler = mod_sat_handler,
};

