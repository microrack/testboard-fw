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
    hal_set_io(IO1, IO_INPUT);
    hal_set_io(IO2, IO_INPUT);
    hal_set_io(IO3, IO_INPUT);

}

const int TIME = 10;

void mod_clk_handler() {
    for(size_t i = 0; i < 8; i++) {
        hal_set_io(IO0, IO_HIGH); delay(60); hal_set_io(IO0, IO_LOW);
    }
    for(size_t i = 0; i < 8; i++) {
        hal_set_io(IO0, IO_HIGH); delayMicroseconds(TIME); hal_set_io(IO0, IO_LOW);
        hal_set_io(IO1, IO_HIGH); delayMicroseconds(TIME); hal_set_io(IO1, IO_INPUT);
    }
    
    for(size_t i = 0; i < 8; i++) {
        hal_set_io(IO0, IO_HIGH); delayMicroseconds(TIME); hal_set_io(IO0, IO_LOW);
        hal_set_io(IO3, IO_HIGH); delayMicroseconds(TIME); hal_set_io(IO3, IO_INPUT);
    }
}

#define MOD_CLK_ID 1

module_descriptor mod_clk = {
    .name = "mod-clk",
    .init = mod_clk_init,
    .handler = mod_clk_handler,
};

