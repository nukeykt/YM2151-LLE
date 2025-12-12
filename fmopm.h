#pragma once


typedef struct {
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

    int o_sy;

    int tm_w1;
    int tm_w2;
    int tm_w3;
    int tm_w4;
} fmopm_t;
