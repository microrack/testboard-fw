// Aliases from data/modules/15_mod_seq:
// alias a_pad=4
// alias a_pot=in pdC
// alias a_step=12
// alias b_pad=5
// alias b_pot=in zD
// alias b_step=13
// alias c_pad=6
// alias c_pot=in zE
// alias c_step=14
// alias clk_in_rst_out_1=0
// alias clk_in_rst_out_2=1
// alias clk_out_rst_in_1=2
// alias clk_out_rst_in_2=3
// alias clock_cv_1=out A
// alias clock_cv_2=out B
// alias clock_pot=in pdB
// alias cv_1=in A
// alias cv_2=in B
// alias d_pad=7
// alias d_pot=in zF
// alias d_step=15
// alias gate_1=in C
// alias gate_2=in D
// alias rst_1=9
// alias rst_2=10
// alias rst_3=out C
// alias run=8
// alias steps_1=in E
// alias steps_2=in F
// alias steps_3=in pdA
// alias steps_4=11

void mod_seq_init() {
    hal_set_io(IO0, IO_INPUT);
    hal_set_io(IO1, IO_INPUT);
    hal_set_io(IO2, IO_INPUT);
    hal_set_io(IO3, IO_INPUT);

    hal_set_io(IO9, IO_INPUT);
    hal_set_io(IO10, IO_INPUT);

    hal_set_source(SOURCE_A, 0);
    hal_set_source(SOURCE_B, 0);
}

const int SEQ_TIME = 40;

void mod_seq_handler() {
    hal_set_io(IO0, IO_INPUT);
    hal_set_source(SOURCE_A, -2000); delay(100); hal_set_source(SOURCE_A, 5000); delay(400);
    hal_set_source(SOURCE_B, -2000); delay(100); hal_set_source(SOURCE_B, 5000); delay(400);

    for(size_t i = 0; i < 8; i++) {
        hal_set_io(IO0, IO_HIGH); delay(40); hal_set_io(IO0, IO_LOW); delay(40);
    }

    hal_set_io(IO0, IO_HIGH); delayMicroseconds(SEQ_TIME); hal_set_io(IO0, IO_LOW); delayMicroseconds(SEQ_TIME);
    hal_set_io(IO1, IO_HIGH); delayMicroseconds(SEQ_TIME); hal_set_io(IO1, IO_LOW); delayMicroseconds(SEQ_TIME);

    hal_set_io(IO0, IO_HIGH); delayMicroseconds(SEQ_TIME); hal_set_io(IO0, IO_LOW); delayMicroseconds(SEQ_TIME);
    hal_set_io(IO3, IO_HIGH); delayMicroseconds(SEQ_TIME); hal_set_io(IO3, IO_LOW); delayMicroseconds(SEQ_TIME);

    hal_set_io(IO0, IO_HIGH); delayMicroseconds(SEQ_TIME); hal_set_io(IO0, IO_LOW); delayMicroseconds(SEQ_TIME);
    hal_set_io(IO9, IO_HIGH); delayMicroseconds(SEQ_TIME); hal_set_io(IO9, IO_LOW); delayMicroseconds(SEQ_TIME);
}

#define MOD_SEQ_ID 15

module_descriptor mod_seq = {
    .name = "mod-seq",
    .init = mod_seq_init,
    .handler = mod_seq_handler,
};

