// Aliases from data/modules/01_mod_clk:
// alias clk_in_=0
// alias rst_out=1
// alias clk_out=2
// alias rst_in=3
// alias out_1=4
// alias out_2=5
// alias out_3=6
// alias out_4=7
// alias out_5=8
// alias out_6=9
// alias out_7=10
// alias out_8=11

void mod_clk_init() {
    
}

void mod_clk_handler() {
    
}

#define MOD_CLK_ID 1

module_descriptor mod_clk = {
    .name = "mod-clk",
    .init = mod_clk_init,
    .handler = mod_clk_handler,
};

