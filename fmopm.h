#pragma once
#include <stdint.h>

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
    int fsm_cycle31;
    int fsm_cycle0;
    int fsm_cycle0_l;
    int fsm_cycle1;
    int fsm_cycle1_l;
    int fsm_reg_sync[2];
    int fsm_lfo_mul[2];

    int reg_write_01[2];
    int reg_write_08[2];
    int reg_write_0f[2];
    int reg_write_10[2];
    int reg_write_11[2];
    int reg_write_12[2];
    int reg_write_14[2];
    int reg_write_18[2];
    int reg_write_19[2];
    int reg_write_1b[2];
    int reg_test[2];
    int reg_timer_a[2];
    int reg_timer_b[2];
    int reg_timer_a_load[2];
    int reg_timer_b_load[2];
    int reg_timer_a_load_l;
    int reg_timer_b_load_l;
    int reg_timer_a_irq[2];
    int reg_timer_b_irq[2];
    int reg_noise_en[2];
    int reg_noise_freq[2];
    int reg_kon_channel[2];
    int reg_kon_operator[2];
    int reg_lfo_freq[2];
    int reg_lfo_amd[2];
    int reg_lfo_pmd[2];
    int reg_lfo_freq_write;
    int reg_lfo_wave[2];
    int reg_ct[2];
    int reg_address[2];
    int reg_address_valid[2];
    int reg_data[2];
    int reg_data_valid[2];
    int reg_counter[2];
    int reg_match00;
    int reg_match20;
    int reg_match20_l[2];
    int reg_match28;
    int reg_match28_l[2];
    int reg_match30;
    int reg_match30_l[2];
    int reg_match38;
    int reg_match40;
    int reg_match60;
    int reg_match80;
    int reg_matcha0;
    int reg_matchc0;
    int reg_matche0;
    uint8_t reg_con_fb_rl[2][8];
    uint8_t reg_kc[2][8];
    uint8_t reg_kf[2][8];
    uint8_t reg_ams_pms[2][8];
    uint8_t reg_mul_dt1[2][32];
    uint8_t reg_tl[2][32];
    uint8_t reg_ar_ks[2][32];
    uint8_t reg_d1r_am[2][32];
    uint8_t reg_d2r_dt2[2][32];
    uint8_t reg_rr_d1l[2][32];
    int reg_div4[2];
    int reg_ch_sel[2];
    uint64_t reg_op_sel[2];
    int reg_ch_sync;
    uint64_t reg_ch_cell[8];
    uint64_t reg_ch_bus;
    uint64_t reg_ch_latch;
    uint64_t reg_ch_in[2];
    uint64_t reg_op_cell[8];
    uint64_t reg_op_bus;
    uint64_t reg_op_latch;
    uint64_t reg_op_in[2];
    int reg_ramp_sync;
    int reg_ramp_step;
    uint8_t reg_ramp_cnt[2][8];
    int reg_tl_latch[3];
    uint16_t reg_tl_value[2][31];
    uint16_t reg_tl_value_l;
    uint16_t reg_tl_value_sum;
    int reg_tl_add1;
    int reg_tl_add2;

    int reg_kon_cnt[2];
    int reg_kon_match;
    int reg_kon[4][2];

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

    int lfo_sync[2];
    int lfo_sync2[2];
    int lfo_cnt1[2];
    int lfo_cnt1_h[2];
    int lfo_cnt1_of_l;
    int lfo_cnt1_of_h;
    int lfo_cnt1_load_val_hi;
    int lfo_cnt1_load[4];
    int lfo_subcnt[2];
    int lfo_subcnt_of[4];
    int lfo_cnt2_inc;
    int lfo_cnt2[2];
    int lfo_cnt2_of[2];
    int lfo_test;
    int lfo_inc[2];
    int lfo_inc_lock;
    int lfo_depth;
    int lfo_bcnt[2];
    int lfo_bcnt_rst;
    int lfo_sum_c_out;
    int lfo_sum_c_in;
    int lfo_out_shifter[2];
    int lfo_shifter[2];
    int lfo_wave1; // square
    int lfo_wave2; // triangle
    int lfo_wave3; // noise
    int lfo_sel;
    int lfo_sum2_c_out[2];
    int lfo_sign_saw;
    int lfo_sign_trig;
    int lfo_cnt1_of_h_lock;
    int lfo_cnt1_of_h_latch;
    int lfo_premul[2];
    int lfo_am[2];
    int lfo_pm[2];

    int freq_pms;
    int freq_lfo_pm;
    int freq_lfo_sign[5];
    int freq_kc_lfo[5];
    int freq_kc_lfo_l_of[2];
    int freq_kc_lfo_of[4];
    int freq_kc_lfo_of2[2];
    int freq_kc_corr_sub[2];
    int freq_kc_cliplow;
    int freq_kc_cliphigh;
    int freq_dt2[3];
    int freq_kc_dt_lo;
    int freq_kc_dt_hi;
    int freq_kc_clipped_hi[2];
    int freq_kc_dt[5];
    int freq_kc_dt_c[2];
    int freq_kc_dt_of[3];
    int freq_block;
    int freq_freq_frac[3];
    int freq_basefreq[3];
    int freq_slope;
    int freq_slopetype;
    int freq_freq_lerp_a;
    int freq_freq_lerp_b;
    int freq_freq_lerp_d_e;
    int freq_freq_lerp;
    int freq_fnum[3];

    int noise_cnt[2];
    int noise_cnt_inc;
    int noise_cnt_match[3];
    int noise_lfsr[2];
    int noise_bit[2];

    int o_sy;
    int o_sh1;
    int o_sh2;
} fmopm_t;
