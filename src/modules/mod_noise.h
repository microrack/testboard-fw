// Aliases from data/modules/13_mod_noise:
// alias input_1=out C
// alias input_2=out D
// alias sample_1=out B
// alias sample_2=in pdB
// alias step_1=in E
// alias step_2=in F
// alias track_1=out A
// alias track_2=in pdA
// alias track_pad=0
// alias blue_a=in A
// alias blue_b=in B
// alias pink_a=in C
// alias pink_b=in D

void mod_noise_init() {
    
}

void mod_noise_handler() {
    
}

#define MOD_NOISE_ID 13

module_descriptor mod_noise = {
    .name = "mod-noise",
    .init = mod_noise_init,
    .handler = mod_noise_handler,
};

