// Aliases from data/modules/05_mod_env:
// alias eoc_1=in E
// alias eoc_2=in F
// alias fall_1=out C
// alias fall_2=2
// alias fall_pot=in pdB
// alias gate_1=out A
// alias gate_2=0
// alias mode=3
// alias output_ab_1=in A
// alias output_ab_2=in B
// alias output_cd_1=in C
// alias output_cd_2=in D
// alias rise_1=out B
// alias rise_2=1
// alias rise_pot=in pdA

void mod_env_init() {
    
}

void mod_env_handler() {
    
}

#define MOD_ENV_ID 5

module_descriptor mod_env = {
    .name = "mod-env",
    .init = mod_env_init,
    .handler = mod_env_handler,
};

