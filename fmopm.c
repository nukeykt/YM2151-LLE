#include <string.h>
#include "fmopm.h"

enum {
    eg_state_attack = 0,
    eg_state_decay,
    eg_state_sustain,
    eg_state_release
};

static const int fm_algorithm[4][6][8] = {
    {
        { 1, 1, 1, 1, 1, 1, 1, 1 }, /* OP1_0         */
        { 1, 1, 1, 1, 1, 1, 1, 1 }, /* OP1_1         */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* OP2           */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 0, 0, 1 }  /* Out           */
    },
    {
        { 0, 1, 0, 0, 0, 1, 0, 0 }, /* OP1_0         */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* OP1_1         */
        { 1, 1, 1, 0, 0, 0, 0, 0 }, /* OP2           */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 1, 1, 1 }  /* Out           */
    },
    {
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* OP1_0         */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* OP1_1         */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* OP2           */
        { 1, 0, 0, 1, 1, 1, 1, 0 }, /* Last operator */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* Last operator */
        { 0, 0, 0, 0, 1, 1, 1, 1 }  /* Out           */
    },
    {
        { 0, 0, 1, 0, 0, 1, 0, 0 }, /* OP1_0         */
        { 0, 0, 0, 0, 0, 0, 0, 0 }, /* OP1_1         */
        { 0, 0, 0, 1, 0, 0, 0, 0 }, /* OP2           */
        { 1, 1, 0, 1, 1, 0, 0, 0 }, /* Last operator */
        { 0, 0, 1, 0, 0, 0, 0, 0 }, /* Last operator */
        { 1, 1, 1, 1, 1, 1, 1, 1 }  /* Out           */
    }
};

typedef struct {
    int basefreq;
    int approxtype;
    int slope;
} freqtable_t;

static const freqtable_t pg_freqtable[64] = {
    { 1299, 1,  3 }, { 1318, 1,  3 }, { 1337, 1,  3 }, { 1356, 1,  4 },
    { 1376, 1,  4 }, { 1396, 1,  4 }, { 1416, 1,  5 }, { 1437, 1,  4 },
    { 1458, 1,  5 }, { 1479, 1,  5 }, { 1501, 1,  6 }, { 1523, 1,  6 },
    { 0,    0,  0 }, { 0,    0,  0 }, { 0,    0,  0 }, { 0,    0,  0 },
    { 1545, 1,  6 }, { 1567, 1,  6 }, { 1590, 1,  7 }, { 1613, 1,  7 },
    { 1637, 1,  7 }, { 1660, 1,  8 }, { 1685, 1,  8 }, { 1709, 1,  8 },
    { 1734, 1,  9 }, { 1759, 1,  9 }, { 1785, 1, 10 }, { 1811, 1, 10 },
    { 0,    0,  0 }, { 0,    0,  0 }, { 0,    0,  0 }, { 0,    0,  0 },
    { 1837, 1, 10 }, { 1864, 1, 11 }, { 1891, 1, 11 }, { 1918, 1, 12 },
    { 1946, 1, 12 }, { 1975, 1, 12 }, { 2003, 1, 13 }, { 2032, 1, 14 },
    { 2062, 1, 14 }, { 2092, 1, 14 }, { 2122, 1, 15 }, { 2153, 1, 15 },
    { 0,    0,  0 }, { 0,    0,  0 }, { 0,    0,  0 }, { 0,    0,  0 },
    { 2185, 1, 15 }, { 2216, 0, 15 }, { 2249, 0, 15 }, { 2281, 0, 15 },
    { 2315, 0, 15 }, { 2348, 0, 15 }, { 2382, 0, 14 }, { 2417, 0, 14 },
    { 2452, 0, 14 }, { 2488, 0, 14 }, { 2524, 0, 14 }, { 2561, 0, 14 },
    { 0,    0,  0 }, { 0,    0,  0 }, { 0,    0,  0 }, { 0,    0,  0 }
};


void FMOPM_Clock(fmopm_t* chip, int clk)
{
    int i;

    chip->input.clk = clk;

    int ic0 = !chip->input.ic;

    int mclk1 = !clk;
    int mclk2 = clk;

    if (mclk1)
    {
        chip->ic_latch[0] = ic0;
        chip->ic_latch2[0] = !((ic0 && !chip->ic_latch[1]) || chip->ic_latch2[1]);
    }
    if (mclk2)
    {
        chip->ic_latch[1] = chip->ic_latch[0];
        chip->ic_latch2[1] = chip->ic_latch2[0];

        chip->o_sy = !chip->ic_latch2[0];

        chip->clk1 = !chip->ic_latch2[0];
        chip->clk2 = chip->ic_latch2[0];
    }


    int clk1 = chip->clk1;
    int clk2 = chip->clk2;

    if (clk2)
        chip->ic = !ic0;

    int ic = !chip->ic;


    int wr0 = (!chip->input.wr && !chip->input.a0 && !chip->input.cs) || chip->ic;
    int wr1 = !chip->input.wr && chip->input.a0 && !chip->input.cs && !chip->ic;
    int rd = !chip->input.rd && chip->input.a0 && !chip->input.cs && !chip->ic;

    if (clk1)
    {
        // chip->write0_l[0] = chip->write0_l[0];
        chip->write0_l[1] = chip->write0_l[0];
    }
    if (chip->write0_l[1])
        chip->write0_trig = 0;
    else if (wr0)
        chip->write0_trig = 1;
    if (clk2)
    {
        chip->write0_l[0] = chip->write0_trig;
        chip->write0_l[2] = chip->write0_l[1];
    }

    if (clk1)
    {
        // chip->write1_l[0] = chip->write1_l[0];
        chip->write1_l[1] = chip->write1_l[0];
    }
    if (chip->write1_l[1])
        chip->write1_trig = 0;
    else if (wr0)
        chip->write1_trig = 1;
    if (clk2)
    {
        chip->write1_l[0] = chip->write1_trig;
        chip->write1_l[2] = chip->write1_l[1];
    }

    int write0_en = chip->write0_l[2];
    int write1_en = chip->write1_l[2];

    if (!chip->input.cs)
        chip->data1 = chip->input.data;
    if (!chip->input.wr)
        chip->data2 = chip->input.data;

    if (clk1)
    {
        chip->fsm_cnt[0] = ic ? 0 : ((chip->fsm_cnt[1] + 1) & 31);

        chip->fsm_out[0] = (chip->fsm_cnt[1] & 24) == 24;
        chip->fsm_out[1] = (chip->fsm_cnt[1] & 24) == 8;
        chip->fsm_out[2] = (chip->fsm_cnt[1] & 7) == 3;
        chip->fsm_out[3] = chip->fsm_cnt[1] == 11;
        chip->fsm_out[4] = chip->fsm_cnt[1] == 11;
        chip->fsm_out[5] = chip->fsm_cnt[1] == 28;
        chip->fsm_out[6] = chip->fsm_cnt[1] == 11;

        chip->fsm_out[7] = (chip->fsm_cnt[1] & 15) == 15;
        chip->fsm_out[8] = (chip->fsm_cnt[1] & 15) == 15;
        chip->fsm_out[9] = (chip->fsm_cnt[1] & 15) == 4;
        chip->fsm_out[10] = chip->fsm_cnt[1] == 4;
        chip->fsm_out[11] = (chip->fsm_cnt[1] & 15) == 11;
        chip->fsm_out[12] = (chip->fsm_cnt[1] & 14) == 14;
        chip->fsm_out[13] = (chip->fsm_cnt[1] & 14) == 4;
        chip->fsm_out[14] = (chip->fsm_cnt[1] & 12) == 0;
        chip->fsm_out[15] = (chip->fsm_cnt[1] & 15) == 6;
        chip->fsm_out[16] = chip->fsm_cnt[1] == 29;
        chip->fsm_out[17] = chip->fsm_cnt[1] >> 4;
        chip->fsm_lfo_mul[0] = chip->fsm_out[12] || chip->fsm_out[13] || chip->fsm_out[14];

        chip->fsm_sh1_l[0] = (chip->fsm_sh1_l[1] << 1) | chip->fsm_out[1];
        chip->fsm_sh2_l[0] = (chip->fsm_sh2_l[1] << 1) | chip->fsm_out[0];

        if (chip->fsm_op_rst)
            chip->fsm_op_cnt[0] = 0;
        else
            chip->fsm_op_cnt[0] = (chip->fsm_op_cnt[1] + chip->fsm_op_inc) & 3;

        chip->fsm_sync1[1] = chip->fsm_sync1[0];
        chip->fsm_cycle0_l = chip->fsm_cycle0;
        chip->fsm_cycle1_l = chip->fsm_cycle1;
        chip->fsm_reg_sync[1] = chip->fsm_reg_sync[0];
    }
    if (clk2)
    {
        chip->fsm_cnt[1] = chip->fsm_cnt[0];

        chip->fsm_sh1_l[1] = chip->fsm_sh1_l[0];
        chip->fsm_sh2_l[1] = chip->fsm_sh2_l[0];

        chip->fsm_op_inc = chip->fsm_out[2];
        chip->fsm_op_rst = chip->fsm_out[3];
        chip->fsm_op_cnt[1] = chip->fsm_op_cnt[0];
        chip->fsm_acc_sync = chip->fsm_out[4];

        chip->o_sh1 = (chip->fsm_sh1_l[1] >> 5) & 1;
        chip->o_sh2 = (chip->fsm_sh2_l[1] >> 5) & 1;

        chip->fsm_sync1[0] = (chip->fsm_sync1[1] << 1) | chip->fsm_out[5];

        chip->fsm_cycle31 = (chip->fsm_sync1[1] >> 1) & 1;
        chip->fsm_cycle0 = (chip->fsm_sync1[1] >> 2) & 1;
        chip->fsm_cycle1 = (chip->fsm_sync1[1] >> 3) & 1;
        chip->fsm_reg_sync[0] = (chip->fsm_reg_sync[1] << 1) | chip->fsm_out[10];

        chip->fsm_lfo_mul[1] = chip->fsm_lfo_mul[0];
    }

    if (clk1)
    {
        int cnt = chip->busy_cnt[1] + chip->busy_cnt_en[1];
        int of = (cnt & 32) != 0 || ic;
        chip->busy_cnt[0] = ic ? 0 : (cnt & 31);
        chip->busy_cnt_en[0] = write1_en || (chip->busy_cnt_en[1] && !of);
    }
    if (clk2)
    {
        chip->busy_cnt[1] = chip->busy_cnt[0];
        chip->busy_cnt_en[1] = chip->busy_cnt_en[0];
    }


    if (clk1)
    {
        if (write0_en)
        {
            chip->reg_write_01[0] = chip->data1 == (chip->input.ym2164 ? 9 : 1);
            chip->reg_write_08[0] = chip->data1 == 8;
            chip->reg_write_0f[0] = chip->data1 == 0xf;
            chip->reg_write_10[0] = chip->data1 == 0x10;
            chip->reg_write_11[0] = chip->data1 == 0x11;
            chip->reg_write_12[0] = chip->data1 == 0x12;
            chip->reg_write_14[0] = chip->data1 == 0x14;
            chip->reg_write_18[0] = chip->data1 == 0x18;
            chip->reg_write_19[0] = chip->data1 == 0x19;
            chip->reg_write_1b[0] = chip->data1 == 0x1b;
        }
        else
        {
            chip->reg_write_01[0] = chip->reg_write_01[1];
            chip->reg_write_08[0] = chip->reg_write_08[1];
            chip->reg_write_0f[0] = chip->reg_write_0f[1];
            chip->reg_write_10[0] = chip->reg_write_10[1];
            chip->reg_write_11[0] = chip->reg_write_11[1];
            chip->reg_write_12[0] = chip->reg_write_12[1];
            chip->reg_write_14[0] = chip->reg_write_14[1];
            chip->reg_write_18[0] = chip->reg_write_18[1];
            chip->reg_write_19[0] = chip->reg_write_19[1];
            chip->reg_write_1b[0] = chip->reg_write_1b[1];
        }

        chip->reg_lfo_freq_write = write1_en && chip->reg_write_18[1];
        
        if (ic)
        {
            chip->reg_test[0] = 0;
            chip->reg_timer_a[0] = 0;
            chip->reg_timer_b[0] = 0;
            chip->reg_timer_a_load[0] = 0;
            chip->reg_timer_b_load[0] = 0;
            chip->reg_timer_a_irq[0] = 0;
            chip->reg_timer_b_irq[0] = 0;
            chip->reg_csm_en[0] = 0;
            chip->reg_noise_en[0] = 0;
            chip->reg_noise_freq[0] = 0;
            chip->reg_kon_channel[0] = 0;
            chip->reg_kon_operator[0] = 0;
            chip->reg_lfo_freq[0] = 0;
            chip->reg_lfo_amd[0] = 0;
            chip->reg_lfo_pmd[0] = 0;
            chip->reg_lfo_wave[0] = 0;
            chip->reg_ct[0] = 0;
        }
        else
        {
            if (write1_en && chip->reg_write_01[1])
                chip->reg_test[0] = chip->data1;
            else
                chip->reg_test[0] = chip->reg_test[1];
            chip->reg_timer_a[0] = chip->reg_timer_a[1];
            if (write1_en && chip->reg_write_10[1])
            {
                chip->reg_timer_a[0] &= ~0x3fc;
                chip->reg_timer_a[0] |= chip->data1 << 2;
            }
            if (write1_en && chip->reg_write_11[1])
            {
                chip->reg_timer_a[0] &= ~0x3;
                chip->reg_timer_a[0] |= chip->data1 & 3;
            }
            if (write1_en && chip->reg_write_12[1])
                chip->reg_timer_b[0] = chip->data1;
            else
                chip->reg_timer_b[0] = chip->reg_timer_b[1];
            if (write1_en && chip->reg_write_14[1])
            {
                chip->reg_timer_a_load[0] = chip->data1 & 1;
                chip->reg_timer_b_load[0] = (chip->data1 >> 1) & 1;
                chip->reg_timer_a_irq[0] = (chip->data1 >> 2) & 1;
                chip->reg_timer_b_irq[0] = (chip->data1 >> 3) & 1;
                chip->reg_csm_en[0] = (chip->data1 >> 7) & 1;
            }
            else
            {
                chip->reg_timer_a_load[0] = chip->reg_timer_a_load[1];
                chip->reg_timer_b_load[0] = chip->reg_timer_b_load[1];
                chip->reg_timer_a_irq[0] = chip->reg_timer_a_irq[1];
                chip->reg_timer_b_irq[0] = chip->reg_timer_b_irq[1];
                chip->reg_csm_en[0] = chip->reg_csm_en[1];
            }
            if (write1_en && chip->reg_write_0f[1])
            {
                chip->reg_noise_en[0] = (chip->data1 >> 7) & 1;
                chip->reg_noise_freq[0] = chip->data1 & 15;
            }
            else
            {
                chip->reg_noise_en[0] = chip->reg_noise_en[1];
                chip->reg_noise_freq[0] = chip->reg_noise_freq[1];
            }
            if (write1_en && chip->reg_write_08[1])
            {
                chip->reg_kon_channel[0] = chip->data1 & 7;
                chip->reg_kon_operator[0] = (chip->data1 >> 3) & 15;
            }
            else
            {
                chip->reg_kon_channel[0] = chip->reg_kon_channel[1];
                chip->reg_kon_operator[0] = chip->reg_kon_operator[1];
            }
            if (chip->reg_lfo_freq_write)
                chip->reg_lfo_freq[0] = chip->data1;
            else
                chip->reg_lfo_freq[0] = chip->reg_lfo_freq[1];
            if (write1_en && chip->reg_write_1b[1])
            {
                chip->reg_lfo_wave[0] = chip->data1 & 3;
                chip->reg_ct[0] = (chip->data1 >> 6) & 3;
            }
            else
            {
                chip->reg_lfo_wave[0] = chip->reg_lfo_wave[1];
                chip->reg_ct[0] = chip->reg_ct[1];
            }
            if (write1_en && chip->reg_write_1b[1] && (chip->data1 & 128) == 0)
                chip->reg_lfo_amd[0] = chip->data1 & 127;
            else
                chip->reg_lfo_amd[0] = chip->reg_lfo_amd[1];
            if (write1_en && chip->reg_write_1b[1] && (chip->data1 & 128) != 0)
                chip->reg_lfo_pmd[0] = chip->data1 & 127;
            else
                chip->reg_lfo_pmd[0] = chip->reg_lfo_pmd[1];
        }
    }
    if (clk2)
    {
        chip->reg_write_01[1] = chip->reg_write_01[0];
        chip->reg_write_08[1] = chip->reg_write_08[0];
        chip->reg_write_0f[1] = chip->reg_write_0f[0];
        chip->reg_write_10[1] = chip->reg_write_10[0];
        chip->reg_write_11[1] = chip->reg_write_11[0];
        chip->reg_write_12[1] = chip->reg_write_12[0];
        chip->reg_write_14[1] = chip->reg_write_14[0];
        chip->reg_write_18[1] = chip->reg_write_18[0];
        chip->reg_write_19[1] = chip->reg_write_19[0];
        chip->reg_write_1b[1] = chip->reg_write_1b[0];

        chip->reg_test[1] = chip->reg_test[0];
        chip->reg_noise_en[1] = chip->reg_noise_en[0];
        chip->reg_noise_freq[1] = chip->reg_noise_freq[0];
        chip->reg_kon_channel[1] = chip->reg_kon_channel[0];
        chip->reg_kon_operator[1] = chip->reg_kon_operator[0];
        chip->reg_timer_a[1] = chip->reg_timer_a[0];
        chip->reg_timer_b[1] = chip->reg_timer_b[0];
        chip->reg_timer_a_load[1] = chip->reg_timer_a_load[0];
        chip->reg_timer_b_load[1] = chip->reg_timer_b_load[0];
        chip->reg_timer_a_irq[1] = chip->reg_timer_a_irq[0];
        chip->reg_timer_b_irq[1] = chip->reg_timer_b_irq[0];
        chip->reg_csm_en[1] = chip->reg_csm_en[0];
        chip->reg_lfo_freq[1] = chip->reg_lfo_freq[0];
        chip->reg_lfo_wave[1] = chip->reg_lfo_wave[0];
        chip->reg_lfo_pmd[1] = chip->reg_lfo_pmd[0];
        chip->reg_lfo_amd[1] = chip->reg_lfo_amd[0];
        chip->reg_ct[1] = chip->reg_ct[0];
    }

    if (clk1)
    {
        int cnt_a = chip->timer_a_cnt[1] + chip->timer_a_inc;
        chip->timer_a_of = (cnt_a >> 10) & 1;
        if (chip->timer_a_load)
            chip->timer_a_cnt[0] = chip->reg_timer_a[1];
        else
            chip->timer_a_cnt[0] = chip->timer_a_en[0] ? cnt_a & 0x3ff : 0;
        chip->timer_a_en[1] = chip->timer_a_en[0];
        chip->timer_a_reset[0] = write1_en && chip->reg_write_14[1] && (chip->data1 & 16) != 0;
        chip->timer_a_status[0] = chip->timer_a_set || (chip->timer_a_status[1] && !chip->timer_a_reset[1]);

        int cnt_b = chip->timer_b_cnt[1] + chip->timer_b_inc;
        chip->timer_b_of = (cnt_b >> 8) & 1;
        if (chip->timer_b_load)
            chip->timer_b_cnt[0] = chip->reg_timer_b[1];
        else
            chip->timer_b_cnt[0] = chip->timer_b_en[0] ? cnt_b & 0xff : 0;
        chip->timer_b_en[1] = chip->timer_b_en[0];
        chip->timer_b_reset[0] = write1_en && chip->reg_write_14[1] && (chip->data1 & 32) != 0;
        chip->timer_b_status[0] = chip->timer_b_set || (chip->timer_b_status[1] && !chip->timer_b_reset[1]);

        int subcnt = chip->timer_b_subcnt[1] + chip->fsm_cycle0;
        int of;
        if (chip->input.ym2164)
        {
            of = (subcnt >> 5) & 1;
            subcnt &= 31;
        }
        else
        {
            of = (subcnt >> 4) & 1;
            subcnt &= 15;
        }
        if (ic)
            chip->timer_b_subcnt[0] = 0;
        else
            chip->timer_b_subcnt[0] = subcnt;
        chip->timer_b_subcnt_of = of;
    }
    if (clk2)
    {
        int test = (chip->reg_test[0] & 4) != 0;
        chip->timer_a_cnt[1] = chip->timer_a_cnt[0];
        chip->timer_a_load = chip->timer_a_of || (!chip->timer_a_en[1] && chip->reg_timer_a_load_l);
        chip->timer_a_en[0] = chip->reg_timer_a_load_l;
        chip->timer_a_inc = (chip->reg_timer_a_load_l && chip->fsm_cycle0_l) || test;
        int rst_a = chip->timer_a_reset[0] || ic;
        chip->timer_a_reset[1] = rst_a;
        chip->timer_a_set = chip->timer_a_of && !rst_a && chip->reg_timer_a_irq[1];
        chip->timer_a_status[1] = chip->timer_a_status[0];

        chip->timer_b_cnt[1] = chip->timer_b_cnt[0];
        chip->timer_b_load = chip->timer_b_of || (!chip->timer_b_en[1] && chip->reg_timer_b_load[0]);
        chip->timer_b_en[0] = chip->reg_timer_b_load[0];
        chip->timer_b_inc = (chip->reg_timer_b_load[0] && chip->timer_b_subcnt_of) || test;
        int rst_b = chip->timer_b_reset[0] || ic;
        chip->timer_b_reset[1] = rst_b;
        chip->timer_b_set = chip->timer_b_of && !rst_b && chip->reg_timer_b_irq[1];
        chip->timer_b_status[1] = chip->timer_b_status[0];

        chip->timer_b_subcnt[1] = chip->timer_b_subcnt[0];
    }

    if (chip->fsm_cycle1 && chip->fsm_cycle1_l)
        chip->reg_timer_a_load_l = chip->reg_timer_a_load[0];


    if (clk1)
    {
        int addr_write = (chip->data2 & 0xe0) != 0 && write0_en;
        if (ic)
            chip->reg_address[0] = 0;
        else if (addr_write)
            chip->reg_address[0] = chip->data2;
        else
            chip->reg_address[0] = chip->reg_address[1];
        chip->reg_address_valid[0] = addr_write || (chip->reg_address_valid[1] && !write0_en);

        int test = 0;
        int reg_data = chip->reg_data[1];
        if (chip->input.ym2164)
        {
            test = (chip->reg_test[0] & 16) != 0;
            if (test)
                reg_data = 0xff;
        }
        int data_write = chip->reg_address_valid[1] && write1_en;
        if (ic)
            chip->reg_data[0] = 0;
        else if (data_write)
            chip->reg_data[0] = chip->data2;
        else
            chip->reg_data[0] = reg_data;
        chip->reg_data_valid[0] = data_write && (chip->reg_data_valid[1] && !write0_en);

        int sync;
        if (chip->input.ym2164)
            sync = (chip->fsm_reg_sync[0] >> 25) & 1;
        else
            sync = chip->fsm_cycle31;
        if (sync)
            chip->reg_counter[0] = 0;
        else
            chip->reg_counter[0] = (chip->reg_counter[1] + 1) & 31;

        if (chip->input.ym2164)
        {
            int cnt = chip->reg_div4[1] + 1;
            int of = (cnt >> 2) & 1;
            chip->reg_div4[0] = cnt & 3;

            int sel = (ic && of) || chip->reg_ch_sync;
            chip->reg_ch_sel[0] = (chip->reg_ch_sel[1] << 1) | sel;

            sel = (ic && of) || sync;
            chip->reg_op_sel[0] = (chip->reg_op_sel[1] << 1) | sel;

            chip->reg_ch_bus = 0;
            chip->reg_op_bus = 0;

            uint64_t val = chip->reg_ch_latch;
            if (test || chip->reg_match00)
            {
                val &= ~0xffull;
                val |= reg_data;
            }
            if (test || (chip->reg_match20_l[1] & 8) != 0)
            {
                val &= ~0xff00ull;
                val |= reg_data << 8;
            }
            if (test || chip->reg_match28_l[1])
            {
                val &= ~0xff0000ull;
                val |= (reg_data & 0x7f) << 16;
            }
            if (test || chip->reg_match30_l[1])
            {
                val &= ~0xff000000ull;
                val |= (reg_data & 0xfc) << 24;
            }
            if (test || chip->reg_match30)
            {
                val &= ~0xff00000000ull;
                val |= (uint64_t)(reg_data & 0x73) << 32;
            }
            chip->reg_ch_in[0] = val;


            val = chip->reg_op_latch;
            if (test || chip->reg_match40)
            {
                val &= ~0xffull;
                val |= reg_data & 0x7f;
            }
            if (test || chip->reg_match60)
            {
                val &= ~0xff00ull;
                val |= reg_data << 8;
            }
            if (test || chip->reg_match80)
            {
                val &= ~0xff0000ull;
                val |= (reg_data & 0xdf) << 16;
            }
            if (test || chip->reg_matcha0)
            {
                val &= ~0xff000000ull;
                val |= (reg_data & 0x9f) << 24;
            }
            if (test || chip->reg_matchc0)
            {
                val &= ~0xff00000000ull;
                val |= (uint64_t)(reg_data & 0xdf) << 32;
            }
            if (test || chip->reg_matche0)
            {
                val &= ~0xff0000000000ull;
                val |= (uint64_t)reg_data << 40;
            }

            chip->reg_op_in[0] = val;

            chip->reg_match20_l[0] = (chip->reg_match20_l[1] << 1) | chip->reg_match20;
            chip->reg_match28_l[0] = chip->reg_match28;
            chip->reg_match30_l[0] = chip->reg_match30;
        }
        else
        {
            memcpy(&chip->reg_con_fb_rl[0][1], &chip->reg_con_fb_rl[1][0], 7 * sizeof(uint8_t));
            memcpy(&chip->reg_kc[0][1], &chip->reg_kc[1][0], 7 * sizeof(uint8_t));
            memcpy(&chip->reg_kf[0][1], &chip->reg_kf[1][0], 7 * sizeof(uint8_t));
            memcpy(&chip->reg_ams_pms[0][1], &chip->reg_ams_pms[1][0], 7 * sizeof(uint8_t));
            memcpy(&chip->reg_mul_dt1[0][1], &chip->reg_mul_dt1[1][0], 31 * sizeof(uint8_t));
            memcpy(&chip->reg_tl[0][1], &chip->reg_tl[1][0], 31 * sizeof(uint8_t));
            memcpy(&chip->reg_ar_ks[0][1], &chip->reg_ar_ks[1][0], 31 * sizeof(uint8_t));
            memcpy(&chip->reg_d1r_am[0][1], &chip->reg_d1r_am[1][0], 31 * sizeof(uint8_t));
            memcpy(&chip->reg_d2r_dt2[0][1], &chip->reg_d2r_dt2[1][0], 31 * sizeof(uint8_t));
            memcpy(&chip->reg_rr_d1l[0][1], &chip->reg_rr_d1l[10][0], 31 * sizeof(uint8_t));
            if (ic)
            {
                chip->reg_con_fb_rl[0][0] = 0;
                chip->reg_kc[0][0] = 0;
                chip->reg_kf[0][0] = 0;
                chip->reg_ams_pms[0][0] = 0;
                chip->reg_mul_dt1[0][0] = 0;
                chip->reg_tl[0][0] = 0;
                chip->reg_ar_ks[0][0] = 0;
                chip->reg_d1r_am[0][0] = 0;
                chip->reg_d2r_dt2[0][0] = 0;
                chip->reg_rr_d1l[0][0] = 0;
            }
            else
            {
                chip->reg_con_fb_rl[0][0] = chip->reg_match20 ? reg_data : chip->reg_con_fb_rl[1][7];
                chip->reg_kc[0][0] = chip->reg_match28 ? (reg_data & 0x7f) : chip->reg_kc[1][7];
                chip->reg_kf[0][0] = chip->reg_match30 ? (reg_data & 0xfc) : chip->reg_kf[1][7];
                chip->reg_ams_pms[0][0] = chip->reg_match38 ? (reg_data & 0x73) : chip->reg_ams_pms[1][7];
                chip->reg_mul_dt1[0][0] = chip->reg_match40 ? (reg_data & 0x7f) : chip->reg_mul_dt1[1][31];
                chip->reg_tl[0][0] = chip->reg_match60 ? (reg_data & 0x7f) : chip->reg_tl[1][31];
                chip->reg_ar_ks[0][0] = chip->reg_match80 ? (reg_data & 0xdf) : chip->reg_ar_ks[1][31];
                chip->reg_d1r_am[0][0] = chip->reg_matcha0 ? (reg_data & 0x9f) : chip->reg_d1r_am[1][31];
                chip->reg_d2r_dt2[0][0] = chip->reg_matchc0 ? (reg_data & 0xdf) : chip->reg_d2r_dt2[1][31];
                chip->reg_rr_d1l[0][0] = chip->reg_matche0 ? reg_data : chip->reg_rr_d1l[1][31];
            }
        }
    }
    if (clk2)
    {
        chip->reg_address[1] = chip->reg_address[0];
        chip->reg_address_valid[1] = chip->reg_address_valid[0];
        chip->reg_data[1] = chip->reg_data[0];
        chip->reg_data_valid[1] = chip->reg_data_valid[0];
        chip->reg_counter[1] = chip->reg_counter[0];

        int op_match = chip->reg_counter[0] == (chip->reg_address[0] & 31) && chip->reg_data_valid[0];
        chip->reg_match40 = op_match && (chip->reg_address[0] & 0xe0) == 0x40;
        chip->reg_match60 = op_match && (chip->reg_address[0] & 0xe0) == 0x60;
        chip->reg_match80 = op_match && (chip->reg_address[0] & 0xe0) == 0x80;
        chip->reg_matcha0 = op_match && (chip->reg_address[0] & 0xe0) == 0xa0;
        chip->reg_matchc0 = op_match && (chip->reg_address[0] & 0xe0) == 0xc0;
        chip->reg_matche0 = op_match && (chip->reg_address[0] & 0xe0) == 0xe0;
        if (chip->input.ym2164)
        {
            int ch_match = (chip->reg_counter[0] & 7) == (chip->reg_address[0] & 7) && chip->reg_data_valid[0] && (chip->reg_address[0] & 0xc0) == 0x00;
            chip->reg_match00 = ch_match && (chip->reg_address[0] & 0x38) == 0;
            chip->reg_match20 = ch_match && (chip->reg_address[0] & 0x38) == 0x20;
            chip->reg_match28 = ch_match && (chip->reg_address[0] & 0x38) == 0x28;
            chip->reg_match30 = ch_match && (chip->reg_address[0] & 0x38) == 0x30;
            chip->reg_match38 = ch_match && (chip->reg_address[0] & 0x38) == 0x38;

            chip->reg_ch_sync = (chip->reg_counter[0] & 7) == 7;
            chip->reg_ch_sel[1] = chip->reg_ch_sel[0];
            chip->reg_op_sel[1] = chip->reg_op_sel[0];

            chip->reg_div4[1] = chip->reg_div4[0];

            chip->reg_ch_in[1] = chip->reg_ch_in[0];
            chip->reg_op_in[1] = chip->reg_op_in[0];

            chip->reg_match20_l[1] = chip->reg_match20_l[0];
            chip->reg_match28_l[1] = chip->reg_match28_l[0];
            chip->reg_match30_l[1] = chip->reg_match30_l[0];
        }
        else
        {
            int ch_match = (chip->reg_counter[0] & 7) == (chip->reg_address[0] & 7) && chip->reg_data_valid[0] && (chip->reg_address[0] & 0xe0) == 0x20;
            chip->reg_match20 = ch_match && (chip->reg_address[0] & 0x18) == 0;
            chip->reg_match28 = ch_match && (chip->reg_address[0] & 0x18) == 8;
            chip->reg_match30 = ch_match && (chip->reg_address[0] & 0x18) == 0x10;
            chip->reg_match38 = ch_match && (chip->reg_address[0] & 0x18) == 0x18;

            memcpy(&chip->reg_con_fb_rl[1][0], &chip->reg_con_fb_rl[0][0], 8 * sizeof(uint8_t));
            memcpy(&chip->reg_kc[1][0], &chip->reg_kc[0][0], 8 * sizeof(uint8_t));
            memcpy(&chip->reg_kf[1][0], &chip->reg_kf[0][0], 8 * sizeof(uint8_t));
            memcpy(&chip->reg_ams_pms[1][0], &chip->reg_ams_pms[0][0], 8 * sizeof(uint8_t));
            memcpy(&chip->reg_mul_dt1[1][0], &chip->reg_mul_dt1[0][0], 32 * sizeof(uint8_t));
            memcpy(&chip->reg_tl[1][0], &chip->reg_tl[0][0], 32 * sizeof(uint8_t));
            memcpy(&chip->reg_ar_ks[1][0], &chip->reg_ar_ks[0][0], 32 * sizeof(uint8_t));
            memcpy(&chip->reg_d1r_am[1][0], &chip->reg_d1r_am[0][0], 32 * sizeof(uint8_t));
            memcpy(&chip->reg_d2r_dt2[1][0], &chip->reg_d2r_dt2[0][0], 32 * sizeof(uint8_t));
            memcpy(&chip->reg_rr_d1l[1][0], &chip->reg_rr_d1l[0][0], 32 * sizeof(uint8_t));
        }
    }
    if (chip->input.ym2164)
    {
        int i;
        uint64_t sel_mask = chip->reg_ch_sel[1] & chip->reg_ch_sel[0];

        for (i = 7; i >= 0; i--)
        {
            if (sel_mask & (2ull << i))
                chip->reg_ch_cell[i] = chip->reg_ch_in[1];
            if (sel_mask & (1ull << i))
                chip->reg_ch_bus |= chip->reg_ch_cell[i];
        }

        sel_mask = chip->reg_op_sel[1] & chip->reg_op_sel[0];

        for (i = 31; i >= 0; i--)
        {
            if (sel_mask & (2ull << i))
                chip->reg_op_cell[i] = chip->reg_op_in[1];
            if (sel_mask & (1ull << i))
                chip->reg_op_bus |= chip->reg_op_cell[i];
        }

        if (clk1)
        {
            int ramp = chip->reg_ch_in[1] & 255;
            int match = ramp == chip->reg_ramp_cnt[1][7];
            int rst = ic || (chip->reg_ramp_sync && match);
            if (rst)
                chip->reg_ramp_cnt[0][0] = 0;
            else
                chip->reg_ramp_cnt[0][0] = chip->reg_ramp_cnt[1][7] + chip->reg_ramp_sync;
            memcpy(chip->reg_ramp_cnt[0][1], chip->reg_ramp_cnt[1][0], 7 * sizeof(uint8_t));
            chip->reg_ramp_step = match;

            chip->reg_tl_latch[0] = chip->reg_op_in[1] & 255;
            chip->reg_tl_latch[2] = chip->reg_tl_latch[1];
            memcpy(chip->reg_tl_value[1][0], chip->reg_tl_value[0][0], 32 * sizeof(uint16_t));

            chip->reg_tl_value_sum = chip->reg_tl_value_l + chip->reg_tl_add1;
            if (chip->reg_tl_add2)
                chip->reg_tl_value_sum += 1023;
            chip->reg_tl_value_sum &= 1023;

        }
        if (clk2)
        {
            chip->reg_ch_latch = chip->reg_ch_bus;
            chip->reg_op_latch = chip->reg_op_bus;
            chip->reg_ramp_sync = (chip->reg_counter[0] & 24) == 0;
            memcpy(chip->reg_ramp_cnt[1][0], chip->reg_ramp_cnt[0][0], 8 * sizeof(uint8_t));

            chip->reg_tl_latch[1] = chip->reg_tl_latch[0];

            memcpy(chip->reg_tl_value[0][1], chip->reg_tl_value[1][0], 31 * sizeof(uint16_t));

            chip->reg_tl_value_l = chip->reg_tl_value[1][31];

            if (chip->reg_ramp_step)
            {
                int add = chip->reg_tl_latch[0] + ((chip->reg_tl_value[1][31] >> 3) ^ 127);

                chip->reg_tl_add1 = (add >> 7) & 1;
                chip->reg_tl_add2 = !chip->reg_tl_add1 && (add & 127) != 127;
            }
            else
            {
                chip->reg_tl_add1 = 0;
                chip->reg_tl_add2 = 0;
            }

            if (chip->reg_tl_latch[2] & 128)
                chip->reg_tl_value[0][0] = chip->reg_tl_value_sum;
            else
                chip->reg_tl_value[0][0] = (chip->reg_tl_latch[2] & 127) << 3;
        }


    }

    if (clk1)
    {
        if (chip->fsm_cycle31)
            chip->reg_kon_cnt[0] = 0;
        else
            chip->reg_kon_cnt[0] = (chip->reg_kon_cnt[1] + 1) & 31;
        chip->reg_kon[0][0] = chip->reg_kon[0][1] << 1;
        chip->reg_kon[1][0] = chip->reg_kon[1][1] << 1;
        chip->reg_kon[2][0] = chip->reg_kon[2][1] << 1;
        chip->reg_kon[3][0] = chip->reg_kon[3][1] << 1;
        if (chip->reg_kon_match)
        {
            chip->reg_kon[0][0] |= (chip->reg_kon_operator[1] >> 0) & 1;
            chip->reg_kon[1][0] |= (chip->reg_kon_operator[1] >> 3) & 1;
            chip->reg_kon[2][0] |= (chip->reg_kon_operator[1] >> 1) & 1;
            chip->reg_kon[3][0] |= (chip->reg_kon_operator[1] >> 2) & 1;
        }
        else
        {
            if (!ic)
                chip->reg_kon[0][0] |= (chip->reg_kon[3][1] >> 7) & 1;
            chip->reg_kon[1][0] |= (chip->reg_kon[0][1] >> 7) & 1;
            chip->reg_kon[2][0] |= (chip->reg_kon[1][1] >> 7) & 1;
            chip->reg_kon[3][0] |= (chip->reg_kon[2][1] >> 7) & 1;
        }
    }
    if (clk2)
    {
        chip->reg_kon_cnt[1] = chip->reg_kon_cnt[0];
        chip->reg_kon_match = chip->reg_kon_cnt[0] == chip->reg_kon_channel[0];

        chip->reg_kon[0][1] = chip->reg_kon[0][0];
        chip->reg_kon[1][1] = chip->reg_kon[1][0];
        chip->reg_kon[2][1] = chip->reg_kon[2][0];
        chip->reg_kon[3][1] = chip->reg_kon[3][0];
    }

    if (clk1)
    {
        int lfrq_h = chip->reg_lfo_freq[1] >> 4;

        int load_val = 0x8000 - (1 << (15 - lfrq_h));

        int cnt = chip->lfo_cnt1[1] + chip->lfo_subcnt_of[3];
        int of = cnt >> 8;
        chip->lfo_cnt1_of_l = of;

        chip->lfo_cnt1_load_val_hi = load_val >> 8;

        if (chip->lfo_cnt1_load[2])
            chip->lfo_cnt1[0] = (load_val & 255);
        else
            chip->lfo_cnt1[0] = ic ? 0 : (cnt & 255);
        chip->lfo_sync[1] = chip->lfo_sync[0];
        chip->lfo_sync2[1] = chip->lfo_sync2[0];

        chip->lfo_cnt1_h[1] = chip->lfo_cnt1_h[0];

        chip->lfo_cnt1_load[1] = chip->lfo_cnt1_load[0];
        chip->lfo_cnt1_load[3] = chip->lfo_cnt1_load[2];

        int subcnt = chip->lfo_subcnt[1] + (chip->lfo_sync[0] & 1);
        int sub_of = subcnt >> 4;
        if (ic)
            chip->lfo_subcnt[0] = 0;
        else
            chip->lfo_subcnt[0] = subcnt & 15;

        chip->lfo_subcnt_of[0] = sub_of;
        chip->lfo_subcnt_of[2] = chip->lfo_subcnt[1];

        int of2 = cnt >> 4;
        if (ic)
            chip->lfo_cnt2[0] = 0;
        else
            chip->lfo_cnt2[0] = (chip->lfo_cnt2[1] + chip->lfo_cnt2_inc) & 15;

        chip->lfo_cnt2_of[1] = chip->lfo_cnt2_of[0];

        chip->lfo_test = (chip->reg_test[1] >> 2) & 1;

        chip->lfo_inc[1] = chip->lfo_inc[0];

        if (chip->lfo_bcnt_rst)
            chip->lfo_bcnt[0] = 0;
        else
        {
            int inc = (chip->lfo_sync[0] >> 2) & 1;
            chip->lfo_bcnt[0] = (chip->lfo_bcnt[1] + inc) & 15;
        }


        int lfo_bit = 0;
        int bcnt = chip->lfo_bcnt[1];
        switch (bcnt)
        {
            case 0:
                lfo_bit |= (chip->lfo_depth & 64) != 0 && (chip->lfo_premul[1] & 64) != 0;
                break;
            case 1:
                lfo_bit |= (chip->lfo_depth & 32) != 0 && (chip->lfo_premul[1] & 64) != 0;
                break;
            case 2:
                lfo_bit |= (chip->lfo_depth & 16) != 0 && (chip->lfo_premul[1] & 16) != 0;
                break;
            case 3:
                lfo_bit |= (chip->lfo_depth & 8) != 0 && (chip->lfo_premul[1] & 8) != 0;
                break;
            case 4:
                lfo_bit |= (chip->lfo_depth & 4) != 0 && (chip->lfo_premul[1] & 4) != 0;
                break;
            case 5:
                lfo_bit |= (chip->lfo_depth & 2) != 0 && (chip->lfo_premul[1] & 2) != 0;
                break;
            case 6:
                lfo_bit |= (chip->lfo_depth & 1) != 0 && (chip->lfo_premul[1] & 1) != 0;
                break;
        }

        int b0 = bcnt != 0 && (chip->lfo_out_shifter[1] & 1) != 0;
        int sum = lfo_bit + b0 + chip->lfo_sum_c_in;

        chip->lfo_out_shifter[0] = chip->lfo_out_shifter[1] >> 1;
        chip->lfo_out_shifter[0] |= (sum & 1) << 15;
        chip->lfo_sum_c_out = sum >> 1;

        chip->lfo_shifter[0] = chip->lfo_shifter[1] << 1;

        int x = !chip->lfo_wave3 && chip->lfo_inc[0] && (chip->lfo_sync[0] & 8) != 0;
        int w3 = (!chip->lfo_wave3 || !chip->lfo_inc_lock) && (chip->lfo_shifter[1] & 0x8000) != 0 && !ic && (chip->reg_test[1] & 2) == 0;
        int w2 = x && chip->lfo_wave2;
        int t = (chip->lfo_sum2_c_out[1] && !chip->lfo_wave3 && (chip->lfo_sync[0] & 8) == 0) || x;
        
        sum = w2 + w3 + t;

        chip->lfo_sum2_c_out[0] = sum >> 1;

        int bit = sum & 1;
        if (chip->lfo_wave3 && chip->lfo_inc_lock)
            bit |= (chip->noise_lfsr[1] >> 15) & 1;

        int bb = chip->lfo_sel ? chip->lfo_sign_saw : (!chip->lfo_sign_trig || !chip->lfo_wave2);
        int sb = chip->lfo_sel ? (chip->lfo_sync2[0] & 2) != 0 : !chip->lfo_sign_saw;
        int mb = chip->fsm_lfo_mul[1] && (chip->lfo_wave1 ? sb : bb);

        chip->lfo_premul[0] = (chip->lfo_premul[1] << 1) | mb;

        int am_load = (chip->lfo_sync[0] & 8) != 0 && !chip->lfo_sel && bcnt == 7;
        chip->lfo_am[0] = am_load ? (chip->lfo_out_shifter[1] >> 7) & 255 : chip->lfo_am[1];

        int pm_load = (chip->lfo_sync[0] & 8) != 0 && chip->lfo_sel && bcnt == 7;
        if (pm_load)
        {
            chip->lfo_pm[0] = (chip->lfo_out_shifter[1] >> 7) & 255;
            chip->lfo_pm[0] ^= 127;
            int s = chip->lfo_wave2 ? chip->lfo_sign_trig : chip->lfo_sign_saw;
            if (!s)
                chip->lfo_pm[0] ^= 128;
        }
        else
            chip->lfo_pm[0] = chip->lfo_pm[1];
    }
    if (clk2)
    {
        chip->lfo_sync[0] = (chip->lfo_sync[1] << 1) | chip->fsm_out[11];
        chip->lfo_sync2[0] = (chip->lfo_sync2[1] << 1) | chip->fsm_out[9];
        chip->lfo_cnt1[1] = chip->lfo_cnt1[0];

        int cnt = chip->lfo_cnt1_h[1] + chip->lfo_cnt1_of_l;
        int of = cnt >> 7;
        chip->lfo_cnt1_of_h = of;
        if (chip->lfo_cnt1_load[3])
            chip->lfo_cnt1_h[0] = chip->lfo_cnt1_load_val_hi;
        else
            chip->lfo_cnt1_h[0] = ic ? 0 : (cnt & 127);

        chip->lfo_subcnt[1] = chip->lfo_subcnt[0];
        chip->lfo_subcnt_of[1] = chip->lfo_subcnt_of[0];
        chip->lfo_subcnt_of[3] = chip->lfo_subcnt_of[2] || (chip->reg_test[0] & 8) != 0;

        chip->lfo_cnt1_load[0] = chip->reg_lfo_freq_write || of;
        chip->lfo_cnt1_load[2] = chip->lfo_cnt1_load[1];

        if (chip->lfo_sync2[1] & 1)
            chip->lfo_cnt1_of_h_latch = chip->lfo_cnt1_of_h_lock;
        int cnt2_inc = (chip->lfo_sync[1] & 2) != 0 && chip->lfo_cnt1_of_h_latch;
        chip->lfo_cnt2_inc = cnt2_inc;
        chip->lfo_cnt2[1] = chip->lfo_cnt2[0];

        chip->lfo_cnt2_of[0] = 0;
        if (cnt2_inc)
        {
            int lfrq_l = chip->reg_lfo_freq[0] & 15;
            int cnt = chip->lfo_cnt2[0];
            if (lfrq_l & 1)
                chip->lfo_cnt2_of[0] |= (cnt & 15) == 7;
            if (lfrq_l & 2)
                chip->lfo_cnt2_of[0] |= (cnt & 7) == 3;
            if (lfrq_l & 4)
                chip->lfo_cnt2_of[0] |= (cnt & 3) == 1;
            if (lfrq_l & 8)
                chip->lfo_cnt2_of[0] |= (cnt & 1) == 0;
        }

        chip->lfo_inc[0] = chip->lfo_test || of || chip->lfo_cnt2_of[1];

        if ((chip->lfo_sync[1] & 4) && (chip->lfo_sync[0] & 8))
        {
            chip->lfo_inc_lock = chip->lfo_inc[0];
            chip->lfo_sign_saw = (chip->lfo_shifter[1] >> 8) & 1;
            chip->lfo_sign_trig = (chip->lfo_shifter[1] >> 7) & 1;
            chip->lfo_cnt1_of_h_lock = chip->lfo_cnt1_of_h;
        }

        chip->lfo_depth = (chip->lfo_bcnt[0] & 8) != 0 ? chip->reg_lfo_pmd[0] : chip->reg_lfo_amd[0];
        chip->lfo_bcnt[1] = chip->lfo_bcnt[0];
        chip->lfo_bcnt_rst = chip->fsm_out[11] && chip->lfo_subcnt[0] == 2;

        chip->lfo_sum_c_in = chip->lfo_sum_c_out && (chip->lfo_sync[1] & 4) == 0;

        chip->lfo_out_shifter[1] = chip->lfo_out_shifter[0];
        chip->lfo_shifter[1] = chip->lfo_shifter[0];

        chip->lfo_wave1 = chip->reg_lfo_wave[0] == 1;
        chip->lfo_wave2 = chip->reg_lfo_wave[0] == 2;
        chip->lfo_wave3 = chip->reg_lfo_wave[0] == 3;

        int lfo_sel = (chip->lfo_bcnt[0] >> 3) & 1;
        chip->lfo_sel = lfo_sel;

        chip->lfo_sum2_c_out[1] = chip->lfo_sum2_c_out[0];

        chip->lfo_premul[1] = chip->lfo_premul[0];
        chip->lfo_am[1] = chip->lfo_am[0];

        chip->lfo_pm[1] = chip->lfo_pm[0];
    }

    if (clk1)
    {
        int rst = ic || (chip->noise_cnt_inc && chip->noise_cnt_match[0]);
        if (rst)
            chip->noise_cnt[0] = 0;
        else
            chip->noise_cnt[0] = (chip->noise_cnt[1] + chip->noise_cnt_inc) & 31;
        chip->noise_cnt_match[1] = chip->noise_cnt_match[0];

        int noise_step = ic || chip->noise_cnt_match[2];

        if (noise_step)
            chip->noise_bit[0] = (chip->noise_lfsr[1] >> 15) & 1;
        else
            chip->noise_bit[0] = chip->noise_bit[1];

        chip->noise_lfsr[0] = chip->noise_lfsr[1] << 1;
        if (noise_step)
        {
            if (!ic)
            {
                int rst = (chip->noise_lfsr[1] & 0xffff) == 0 && !chip->noise_bit[1];
                int xr = (chip->noise_lfsr[1] >> 13) & 1;
                xr ^= chip->noise_bit[1];
                chip->noise_lfsr[0] |= rst | xr;
            }
        }
        else
        {
            chip->noise_lfsr[0] |= (chip->noise_lfsr[1] >> 15) & 1;
        }
    }
    if (clk2)
    {
        chip->noise_cnt[1] = chip->noise_cnt[0];
        chip->noise_cnt_inc = (chip->lfo_sync[1] >> 2) & 1;
        chip->noise_cnt_match[0] = chip->noise_cnt[0] == (chip->reg_noise_freq[0] ^ 31);
        chip->noise_cnt_match[2] = chip->noise_cnt_match[1];
        chip->noise_bit[1] = chip->noise_bit[0];
        chip->noise_lfsr[1] = chip->noise_lfsr[0];
    }

    if (clk1)
    {
        int kfc = chip->reg_kf[1][1] | (chip->reg_kc[1][1] << 6);
        chip->freq_lfo_sign[1] = chip->freq_lfo_sign[0];
        chip->freq_lfo_sign[3] = chip->freq_lfo_sign[2];

        int lfo_add = chip->freq_pms ? (chip->freq_lfo_pm << chip->freq_pms) >> 5 : 0;
        int lfo_neg = chip->freq_lfo_sign[0];
        if (lfo_neg)
            lfo_add ^= 0x1fff;
        int sum = (kfc & 255) + (lfo_add & 255) + lfo_neg;
        int sum_l_of = sum >> 8;
        sum += (kfc & 0x1f00) + (lfo_add & 0x1f00);
        int sum_of = sum >> 13;
        chip->freq_kc_lfo[0] = sum & 0x1fff;
        chip->freq_kc_lfo_of[0] = sum_of;
        chip->freq_kc_lfo_of[2] = chip->freq_kc_lfo_of[1];
        chip->freq_kc_lfo_l_of[0] = sum_l_of;

        int sum2 = chip->freq_kc_lfo[1];
        if (!chip->freq_lfo_sign[2] && (chip->freq_kc_lfo_l_of[1] || (chip->freq_kc_lfo[1] & 0xc0) == 0xc0))
            sum2 += 64;
        int corr_sub = chip->freq_lfo_sign[2] && !chip->freq_kc_lfo_l_of[1];
        if (corr_sub)
            sum2 += 0x1fc0;
        chip->freq_kc_corr_sub[0] = corr_sub;
        chip->freq_kc_lfo[2] = sum2 & 0x1fff;
        chip->freq_kc_lfo[4] = chip->freq_kc_lfo[3];
        chip->freq_kc_lfo_of2[0] = sum2 >> 13;

        chip->freq_dt2[0] = (chip->reg_d2r_dt2[1][26] >> 6) & 3;
        chip->freq_dt2[2] = chip->freq_dt2[0];

        chip->freq_kc_cliplow = (!chip->freq_kc_lfo_of[3] && chip->freq_lfo_sign[3]) || (!chip->freq_kc_lfo_of2[1] && chip->freq_kc_corr_sub[1]);
        chip->freq_kc_cliphigh = !chip->freq_lfo_sign[3] && (chip->freq_kc_lfo_of[3] || chip->freq_kc_lfo_of2[1]);

        chip->freq_kc_dt_c[1] = chip->freq_kc_dt_c[0];
        chip->freq_kc_dt[0] = chip->freq_kc_dt_lo;
        chip->freq_kc_dt[2] = chip->freq_kc_dt[1] | (chip->freq_kc_dt_hi << 6);
        chip->freq_kc_dt[4] = chip->freq_kc_dt_of[2] ? 0x1fbf : chip->freq_kc_dt[3];

        chip->freq_kc_clipped_hi[1] = chip->freq_kc_clipped_hi[0];

        chip->freq_kc_dt_of[1] = chip->freq_kc_dt_of[0];
        chip->freq_basefreq[1] = chip->freq_basefreq[0];

        chip->freq_freq_frac[1] = chip->freq_freq_frac[0] & 1;

        chip->freq_freq_lerp_a = (chip->freq_slope & 8) != 0 && (chip->freq_freq_frac[0] & 1) != 0;

        int lerp_b = 0;
        if (chip->freq_slopetype)
        {
            if ((chip->freq_slope & 4) != 0 && (chip->freq_freq_frac[0] & 2) != 0)
                lerp_b |= 1;
            if ((chip->freq_slope & 8) != 0 && (chip->freq_freq_frac[0] & 2) != 0)
                lerp_b |= 2;
            if ((chip->freq_freq_frac[0] & 2) != 0)
                lerp_b |= 4;
        }
        else
        {
            if ((chip->freq_freq_frac[0] & 8) != 0)
                lerp_b |= 1;
            if ((chip->freq_freq_frac[0] & 1) != 0)
                lerp_b |= 2;
            if ((chip->freq_freq_frac[0] & 12) == 12 && (chip->freq_slope & 1) == 0)
                lerp_b |= 4;
            if ((chip->freq_freq_frac[0] & 2) != 0)
                lerp_b |= 8;
        }

        chip->freq_freq_lerp_b = lerp_b;

        int lerp_d = 0;
        int lerp_e = 0;
        if ((chip->freq_freq_frac[0] & 4) != 0)
        {
            lerp_e |= 8 | (chip->freq_slope >> 1);
        }
        if ((chip->freq_freq_frac[0] & 8) != 0)
        {
            lerp_d |= chip->freq_slope;
            if (!chip->freq_slopetype)
                lerp_d |= 1;
            lerp_e |= 16;
        }

        chip->freq_freq_lerp_d_e = lerp_d + lerp_e;

        chip->freq_fnum[0] = chip->freq_basefreq[2] + (chip->freq_freq_lerp >> 1) + chip->freq_freq_frac[1];
        chip->freq_fnum[2] = chip->freq_fnum[1];

        chip->freq_kcode[1] = chip->freq_kcode[0];
        chip->freq_kcode[3] = chip->freq_kcode[2];
    }
    if (clk2)
    {
        int lfo_pm_out = chip->reg_lfo_pmd[0] == 0 ? 255 : chip->lfo_pm[0];
        lfo_pm_out ^= 128;

        int pms = (chip->reg_ams_pms[0][0] >> 4) & 7;
        chip->freq_lfo_sign[0] = pms != 0 && (lfo_pm_out & 128) != 0;
        chip->freq_lfo_sign[2] = chip->freq_lfo_sign[1];
        chip->freq_lfo_sign[4] = chip->freq_lfo_sign[3];
        chip->freq_pms = pms;
        int lfo_add = ~lfo_pm_out & 15;

        int ps7 = pms == 7;

        int hi = ps7 ? (~lfo_pm_out >> 4) & 7 : (~lfo_pm_out >> 5) & 3;
        int hi2 = (hi >> 2) & 1;
        int add = (hi & 6) == 6 || (pms >= 6 && (hi & 3) == 3);

        int pm_sum = hi + hi2 + add;
        int lfo_add2 = ps7 ? pm_sum : ((pm_sum & 7) << 1) | ((~lfo_pm_out >> 4) & 1);
        lfo_add |= lfo_add2 << 4;

        chip->freq_lfo_pm = lfo_add;
        chip->freq_kc_lfo_of[1] = chip->freq_kc_lfo_of[0];
        chip->freq_kc_lfo_of[3] = chip->freq_kc_lfo_of[2];


        chip->freq_kc_lfo[1] = chip->freq_kc_lfo[0];
        chip->freq_kc_lfo[3] = chip->freq_kc_lfo[2];
        chip->freq_kc_lfo_of2[1] = chip->freq_kc_lfo_of2[0];
        chip->freq_kc_lfo_l_of[1] = chip->freq_kc_lfo_l_of[0];
        chip->freq_kc_corr_sub[1] = chip->freq_kc_corr_sub[0];


        int clipped = chip->freq_kc_lfo[4];
        if (chip->freq_kc_cliplow)
            clipped = 0;
        if (chip->freq_kc_cliphigh)
            clipped = 0x1fbf;

        int dt_add = 0;
        switch (chip->freq_dt2[0])
        {
        case 2:
            dt_add = 52;
            break;
        case 3:
            dt_add = 32;
            break;
        }

        int sum_lo = (clipped & 63) + dt_add;
        chip->freq_kc_dt_lo = sum_lo & 63;
        chip->freq_kc_dt_c[0] = sum_lo >> 6;
        chip->freq_kc_clipped_hi[0] = clipped >> 6;

        chip->freq_kc_dt[1] = chip->freq_kc_dt[0];
        chip->freq_kc_dt[3] = chip->freq_kc_dt[2];

        int dt2 = chip->freq_dt2[2] == 2;
        int c6 = chip->freq_kc_clipped_hi[1] & 1;
        int c7 = (chip->freq_kc_clipped_hi[1] >> 1) & 1;
        int w4 = (c7 && chip->freq_kc_dt_c[1]) || (chip->freq_kc_dt_c[1] && dt2 && c6);
        int c_hi = chip->freq_kc_dt_c[1] ? !w4 : (c7 && dt2);
        int dt_add_hi = 0;
        if (dt2)
            dt_add_hi |= 1;
        if (w4)
            dt_add_hi |= 2;
        if (chip->freq_dt2[2] == 3)
            dt_add_hi |= 4;
        if (chip->freq_dt2[2] != 0)
            dt_add_hi |= 8;
        int sum_hi = chip->freq_kc_clipped_hi[1] + dt_add_hi + c_hi;
        chip->freq_kc_dt_hi = sum_hi & 127;
        chip->freq_kc_dt_of[0] = sum_hi >> 7;
        chip->freq_kc_dt_of[2] = chip->freq_kc_dt_of[1];
        chip->freq_kcode[0] = chip->freq_kc_dt[4] >> 8;
        chip->freq_kcode[2] = chip->freq_kcode[1];

        chip->freq_freq_frac[0] = chip->freq_kc_dt[4] & 15;
        chip->freq_freq_frac[2] = chip->freq_freq_frac[1];

        const freqtable_t *ft = &pg_freqtable[(chip->freq_kc_dt[4] >> 4) & 63];

        chip->freq_basefreq[0] = ft->basefreq;
        chip->freq_basefreq[2] = chip->freq_basefreq[1];

        chip->freq_slope = ft->slope;
        chip->freq_slopetype = ft->approxtype;

        chip->freq_freq_lerp = (chip->freq_freq_lerp_d_e + chip->freq_freq_lerp_a + chip->freq_freq_lerp_b) & 63;

        chip->freq_fnum[1] = chip->freq_fnum[0];
    }

    if (clk1)
    {
        chip->pg_block[1] = chip->pg_block[0];

        chip->pg_dt_multi = chip->reg_mul_dt1[1][31];

        chip->dt_note[1] = chip->dt_note[0];
        chip->dt_blockmax[1] = chip->dt_blockmax[0];

        chip->dt_enable[1] = chip->dt_enable[0];

        chip->dt_sign[1] = chip->dt_sign[0];

        chip->dt_sum = chip->dt_add1 + chip->dt_add2 + 1;

        chip->pg_freqdt[0] = (chip->pg_freq + chip->pg_dt_add) & 0x1ffff;
        chip->pg_freqdt[2] = chip->pg_freqdt[1];

        chip->pg_multi[1] = chip->pg_multi[0];
        chip->pg_multi[3] = chip->pg_multi[2];
        chip->pg_multi[5] = chip->pg_multi[4];

        chip->pg_add[0] = chip->pg_multi[6] ? chip->pg_freqdt[3] * chip->pg_multi[6] :
            (chip->pg_freqdt[3] >> 1);
        chip->pg_add[2] = chip->pg_add[1];
        chip->pg_add[4] = chip->pg_add[3];
        chip->pg_add[6] = chip->pg_add[5];
        chip->pg_add[8] = chip->pg_add[7];

        chip->pg_reset[0] = chip->tm_w1;
        chip->pg_reset[2] = chip->pg_reset[1];
        chip->pg_reset[4] = chip->pg_reset[3] || (chip->reg_test[1] & 8) != 0;

        memcpy(&chip->pg_phase[0][1], &chip->pg_phase[1][0], 30 * sizeof(int));

        chip->pg_phase2[0] = chip->pg_phase[1][30];

        chip->pg_phase[0][0] = (chip->pg_phase2[1] + chip->pg_add[9]) & 0xfffff;

        chip->pg_dbg[0] = chip->pg_dbg[1] >> 1;
        if (chip->pg_dbgsync)
            chip->pg_dbg[0] |= chip->pg_phase[1][30] & 1023;

        chip->pg_out = chip->pg_phase[1][23] >> 10;
    }
    if (clk2)
    {
        chip->pg_block[0] = chip->freq_kcode[3] >> 2;
        chip->pg_freq = (chip->freq_fnum[2] << chip->pg_block[1]) >> 1;

        chip->dt_note[0] = chip->freq_kcode[3] & 3;
        chip->dt_blockmax[0] = (chip->freq_kcode[3] & 28) == 28;
        chip->dt_add1 = (chip->freq_kcode[3] >> 2) & 7;
        if ((chip->pg_dt_multi & 0x30) != 0)
            chip->dt_add1 |= 8;
        chip->dt_add2 = 0;
        if ((chip->pg_dt_multi & 0x30) == 0x30)
            chip->dt_add2 |= 1;
        if (chip->pg_dt_multi & 0x20)
            chip->dt_add2 |= 2;

        chip->dt_enable[0] = (chip->pg_dt_multi & 0x30) != 0;

        chip->dt_sign[0] = (chip->pg_dt_multi & 0x40) != 0;

        int dt_l = (chip->dt_sum & 1) << 2;
        if (!chip->dt_blockmax[1])
            dt_l |= chip->dt_note[1];
        int dt_h = chip->dt_sum >> 1;

        static const int pg_detune[8] = { 16, 17, 19, 20, 22, 24, 27, 29 };

        int dt_freq = pg_detune[dt_l] >> (9 - dt_h);

        if (chip->dt_sign[1])
            dt_freq = -dt_freq;

        chip->pg_dt_add = dt_freq;

        chip->pg_multi[0] = chip->pg_dt_multi & 15;
        chip->pg_multi[2] = chip->pg_multi[1];
        chip->pg_multi[4] = chip->pg_multi[3];
        chip->pg_multi[6] = chip->pg_multi[5];

        chip->pg_freqdt[1] = chip->pg_freqdt[0];
        chip->pg_freqdt[3] = chip->pg_freqdt[2];

        chip->pg_add[1] = chip->pg_add[0];
        chip->pg_add[3] = chip->pg_add[2];
        chip->pg_add[5] = chip->pg_reset[0] ? 0 : chip->pg_add[4];
        chip->pg_add[7] = chip->pg_add[6];
        chip->pg_add[9] = chip->pg_add[8];

        chip->pg_reset[1] = chip->pg_reset[0];
        chip->pg_reset[3] = chip->pg_reset[2];
        memcpy(&chip->pg_phase[1][0], &chip->pg_phase[0][0], 31 * sizeof(int));

        chip->pg_phase2[1] = chip->pg_reset[4] ? 0 : chip->pg_phase2[0];

        chip->pg_dbgsync = chip->fsm_out[10];

        chip->pg_dbg[1] = chip->pg_dbg[0];
    }

    if (clk1)
    {
        chip->eg_sync[1] = chip->eg_sync[0];
        chip->eg_sync2[1] = chip->eg_sync2[0];
        chip->eg_ic[0] = ic;

        if (chip->eg_subcnt_reset)
            chip->eg_subcnt[0] = 0;
        else
        {
            int subcnt = chip->eg_subcnt[1];
            if (chip->eg_sync[0] & 2)
                subcnt++;
            chip->eg_subcnt[0] = subcnt;
        }

        chip->eg_rate_sel[1] = chip->eg_rate_sel[0];

        int rate = 0;
        switch (chip->eg_rate_sel[2])
        {
            case eg_state_attack:
                rate = chip->reg_ar_ks[1][31] & 31;
                break;
            case eg_state_decay:
                rate = chip->reg_d1r_am[1][31] & 31;
                break;
            case eg_state_sustain:
                rate = chip->reg_d2r_dt2[1][31] & 31;
                break;
            case eg_state_release:
                rate = (chip->reg_rr_d1l[1][31] & 15) * 2 + 1;
                break;
        }
        if (ic)
            rate = 31;
        chip->eg_rate = rate;
        chip->eg_zerorate = rate == 0;

        int inc = (chip->eg_timer_carry[1] || (chip->eg_sync2[0] & 2) != 0) && !chip->eg_half && chip->eg_clock;
        int timer_bit = (chip->eg_timer[1] & 1) + inc;

        int timer_lock = (chip->eg_sync2[0] & 2) != 0 && chip->eg_clock && chip->eg_half;

        chip->eg_timer_lo = chip->eg_timer[1] & 3;

        chip->eg_timer_carry[0] = timer_bit >> 1;

        int next_bit = (timer_bit & 1) != 0 && !ic;
        chip->eg_timer[0] = (chip->eg_timer[1] >> 1) | (next_bit << 15);
        int bit = (chip->eg_timer[1] & 32768) != 0;
        int masked_bit = bit && chip->eg_masking[1];
        chip->eg_timer_masked[0] = (chip->eg_timer_masked[1] >> 1) | (masked_bit << 15);

        chip->eg_masking[0] = chip->eg_ic[1] || (chip->eg_sync2[0] & 2) != 0 || (chip->eg_masking[1] && !bit);

        chip->eg_timerlock_l = timer_lock;
        if (chip->eg_timerlock_l && timer_lock)
        {
            chip->eg_timer_lo_lock = chip->eg_timer_lo;
            chip->eg_shift_lock = 0;
            if (chip->eg_timer_masked[0] & 0x1555)
                chip->eg_shift_lock |= 1;
            if (chip->eg_timer_masked[0] & 0x2666)
                chip->eg_shift_lock |= 2;
            if (chip->eg_timer_masked[0] & 0x3878)
                chip->eg_shift_lock |= 4;
            if (chip->eg_timer_masked[0] & 0x3f80)
                chip->eg_shift_lock |= 8;
        }

        int kon_csm = (chip->reg_kon[3][1] & 32) != 0 || chip->eg_csm_kon[2];

        chip->eg_csm_kon[1] = chip->eg_csm_kon[0];

        chip->eg_keyon[0] = (chip->eg_keyon[1] << 1) | kon_csm;

        chip->eg_kon1 = kon_csm && (chip->eg_keyon[1] & 0x80000000) == 0;
        chip->eg_kon2[0] = (chip->eg_keyon[1] & 8) != 0 && (chip->eg_keyon[1] & 0x800000000ull) == 0;

        chip->eg_state[1][0] = chip->eg_state[0][0];
        chip->eg_state[1][1] = chip->eg_state[0][1];

        chip->eg_off = x;
        chip->eg_zero = x;
        chip->eg_slreach = x;

        chip->eg_maxrate[1] = chip->eg_maxrate[0];

        chip->eg_ks = (chip->reg_ar_ks[1][31] >> 6) & 3;

        chip->eg_rateks[1] = chip->eg_rateks[0];
    }
    if (clk2)
    {
        chip->eg_sync[0] = (chip->eg_sync[1] << 1) | chip->fsm_out[16];
        chip->eg_sync2[0] = (chip->eg_sync2[1] << 1) | chip->fsm_out[7];
        chip->eg_half = chip->fsm_out[17];
        chip->eg_ic[1] = chip->eg_ic[0];

        chip->eg_subcnt_reset = ic || ((chip->eg_subcnt[0] & 2) != 0 && (chip->eg_sync[1] & 1) != 0);
        chip->eg_subcnt[1] = chip->eg_subcnt[0];

        chip->eg_timer_carry[1] = chip->eg_timer_carry[0];

        chip->eg_timer[1] = chip->eg_timer[0];
        chip->eg_timer_masked[1] = chip->eg_timer_masked[0];
        chip->eg_masking[1] = chip->eg_masking[0];

        chip->eg_clock = (chip->eg_subcnt[0] & 2) != 0 || (chip->reg_test[0] & 1) != 0;

        chip->eg_csm_kon[2] = chip->eg_csm_kon[1];

        if ((chip->fsm_sync1[0] & 16) == 0 && (chip->fsm_sync1[1] & 16) != 0)
        {
            chip->eg_csm_kon[0] = chip->reg_csm_en[1] && chip->timer_a_load;
        }

        chip->eg_keyon[1] = chip->eg_keyon[0];

        int s0 = chip->eg_state[1][0];
        int s1 = chip->eg_state[1][1];
        if (chip->eg_kon1)
        {
            s0 &= ~0x10000000;
            s1 &= ~0x10000000;
        }

        chip->eg_rate_sel[0] = ((s0 >> 27) & 1) | ((s1 >> 26) & 2);
        chip->eg_rate_sel[2] = chip->eg_rate_sel[1];

        int state = 0;
        if (chip->eg_state[1][0] & 0x80000000)
            state |= 1;
        if (chip->eg_state[1][1] & 0x80000000)
            state |= 2;

        int nextstate = 0;
        if (state == eg_state_release && !chip->eg_kon2[0])
            nextstate |= eg_state_release;
        if (state == eg_state_sustain && !chip->eg_kon2[0])
            nextstate |= eg_state_sustain;
        if (state == eg_state_attack && !chip->eg_kon2[0] && chip->eg_zero)
            nextstate |= eg_state_decay;
        if (state == eg_state_decay && !chip->eg_kon2[0] && !chip->eg_slreach)
            nextstate |= eg_state_decay;
        if (state == eg_state_decay && !chip->eg_kon2[0] && chip->eg_slreach)
            nextstate |= eg_state_sustain;
        if (ic)
            nextstate |= eg_state_release;
        int kon = (chip->eg_keyon[0] >> 4) & 1;
        if (!chip->eg_kon2[0] && !kon)
            nextstate |= eg_state_release;
        if (!chip->eg_kon2[0] && chip->eg_off && state != eg_state_attack)
            nextstate |= eg_state_release;

        chip->eg_linear = (state == eg_state_decay && !chip->eg_off && !chip->eg_kon2[0] && !chip->eg_slreach)
            || (state >= eg_state_sustain && !chip->eg_off && !chip->eg_kon2[0]);
        chip->eg_exponent = !chip->eg_zero && state == eg_state_attack && kon && !chip->eg_maxrate[1];

        chip->eg_state[0][0] = chip->eg_state[1][0] << 1;
        chip->eg_state[0][1] = chip->eg_state[1][1] << 1;
        chip->eg_state[0][0] |= nextstate & 1;
        chip->eg_state[0][1] |= (nextstate >> 1) & 1;

        chip->eg_kon2[1] = chip->eg_kon2[0];

        int rks = 0;
        switch (chip->eg_ks)
        {
            case 0:
                if (!chip->eg_zerorate)
                    rks = chip->freq_kcode[3] >> 3;
                break;
            case 1:
                rks = chip->freq_kcode[3] >> 2;
                break;
            case 2:
                rks = chip->freq_kcode[3] >> 1;
                break;
            case 3:
                rks = chip->freq_kcode[3];
                break;
        }

        int rateks = chip->eg_rate * 2 + rks;
        chip->eg_rateks[0] = rateks;

        int rateks2 = chip->eg_rateks[1];
        if (rateks2 & 64)
            rateks2 = 63;

        chip->eg_maxrate[0] = rateks2 == 63;

        chip->eg_rate12 = (rateks2 >> 2) == 12;
        chip->eg_rate13 = (rateks2 >> 2) == 13;
        chip->eg_rate14 = (rateks2 >> 2) == 14;
        chip->eg_rate15 = (rateks2 >> 2) == 15;
    }
}

fmopm_t opm;

int main()
{
    int i;
    opm.input.cs = 1;
    opm.input.wr = 1;
    opm.input.rd = 1;
    opm.input.ic = 1;
    opm.input.a0 = 0;
    opm.input.data = 0;
    for (i = 0; i < 64 * 4; i++)
    {
        FMOPM_Clock(&opm, 0);
        FMOPM_Clock(&opm, 1);
    }
    opm.input.ic = 0;
    for (i = 0; i < 64 * 4; i++)
    {
        FMOPM_Clock(&opm, 0);
        FMOPM_Clock(&opm, 1);
    }
    opm.input.ic = 1;
    for (i = 0; i < 64 * 4; i++)
    {
        FMOPM_Clock(&opm, 0);
        FMOPM_Clock(&opm, 1);
    }
    for (i = 0; i < 64 * 100; i++)
    {
        FMOPM_Clock(&opm, 0);
        FMOPM_Clock(&opm, 1);
    }
}
