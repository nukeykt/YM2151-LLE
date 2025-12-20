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

        chip->fsm_cycle31 = (chip->fsm_sync1[0] >> 2) & 1;
        chip->fsm_cycle0 = (chip->fsm_sync1[0] >> 3) & 1;
        chip->fsm_cycle1 = (chip->fsm_sync1[0] >> 4) & 1;
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
            }
            else
            {
                chip->reg_timer_a_load[0] = chip->reg_timer_a_load[1];
                chip->reg_timer_b_load[0] = chip->reg_timer_b_load[1];
                chip->reg_timer_a_irq[0] = chip->reg_timer_a_irq[1];
                chip->reg_timer_b_irq[0] = chip->reg_timer_b_irq[1];
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
            if (sel_mask & (2 << i))
                chip->reg_ch_cell[i] = chip->reg_ch_in[1];
            if (sel_mask & (1 << i))
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

            chip->reg_tl_latch[0] = chip->reg_op_in[1];
            chip->reg_tl_latch[2] = chip->reg_tl_latch[1];
            memcpy(chip->reg_tl_value[1][0], chip->reg_tl_value[01][0], 32 * sizeof(uint16_t));

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

    int lfo_pm_out = chip->reg_lfo_pmd[0] == 0 ? 255 : chip->lfo_pm[0];
    lfo_pm_out ^= 128;

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
