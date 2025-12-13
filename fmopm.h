#pragma once


typedef struct {
    int ym2164;
    int clk;
    int ic; // neg
    int cs; // neg
    int wr; // neg
    int rd; // neg
    int a0;
    int data;
} fmopm_input_t;


typedef struct {
    fmopm_input_t input;

    int clk1;
    int clk2;

    int ic_latch[2];
    int ic_latch2[2];
    int ic;

    int write0_trig;
    int write0_l[3];
    int write1_trig;
    int write1_l[3];

    int data1;
    int data2;

    int fsm_cnt[2];
    int fsm_out[17];
    int fsm_sh1_l[2];
    int fsm_sh2_l[2];
    int fsm_op_inc;
    int fsm_op_rst;
    int fsm_op_cnt[2];
    int fsm_acc_sync;
    int fsm_sync1[2];
    int fsm_cycle0;
    int fsm_cycle0_l;
    int fsm_cycle1;
    int fsm_cycle1_l;

    int reg_write_01[2];
    int reg_write_08[2];
    int reg_write_0f[2];
    int reg_write_10[2];
    int reg_write_11[2];
    int reg_write_12[2];
    int reg_test[2];
    int reg_timer_a[2];
    int reg_timer_b[2];
    int reg_timer_a_load[2];
    int reg_timer_b_load[2];
    int reg_timer_a_load_l;
    int reg_timer_b_load_l;
    int reg_timer_a_irq[2];
    int reg_timer_b_irq[2];

    int timer_a_cnt[2];
    int timer_a_inc;
    int timer_a_of;
    int timer_a_en[2];
    int timer_a_load;
    int timer_a_reset[2];
    int timer_a_set;
    int timer_a_status[2];
    int timer_b_cnt[2];
    int timer_b_inc;
    int timer_b_of;
    int timer_b_en[2];
    int timer_b_load;
    int timer_b_reset[2];
    int timer_b_set;
    int timer_b_status[2];
    int timer_b_subcnt[2];
    int timer_b_subcnt_of;

    int busy_cnt[2];
    int busy_cnt_en[2];

    int o_sy;
    int o_sh1;
    int o_sh2;

    int tm_w1;
    int tm_w2;
    int tm_w3;
    int tm_w4;
} fmopm_t;
