// Aliases from data/modules/28_mod_delay:
// alias fb_pot=in pdB
// alias input_1=out A
// alias input_2=0
// alias mix_pot=in pdA
// alias out_ab_1=in A
// alias out_ab_2=in B
// alias out_cd_1=in C
// alias out_cd_2=in D
// alias return_1=out B
// alias return_2=1
// alias send_1=in E
// alias send_2=in F
// alias time_1=out C
// alias time_2=2
// alias time_pot=in pdC

void mod_delay_init() {
    
}

void mod_delay_handler() {
    
}

#define MOD_DELAY_ID 28

module_descriptor mod_delay = {
    .name = "mod-delay",
    .init = mod_delay_init,
    .handler = mod_delay_handler,
};

