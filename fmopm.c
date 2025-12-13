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

    int ic = chip->ic;


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

        chip->fsm_sh1_l[0] = (chip->fsm_sh1_l[1] << 1) | chip->fsm_out[1];
        chip->fsm_sh2_l[0] = (chip->fsm_sh2_l[1] << 1) | chip->fsm_out[0];

        if (chip->fsm_op_rst)
            chip->fsm_op_cnt[0] = 0;
        else
            chip->fsm_op_cnt[0] = (chip->fsm_op_cnt[1] + chip->fsm_op_inc) & 3;

        chip->fsm_sync1[1] = chip->fsm_sync1[0];
        chip->fsm_cycle0_l = chip->fsm_cycle0;
        chip->fsm_cycle1_l = chip->fsm_cycle1;
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

        chip->fsm_cycle0 = (chip->fsm_sync1[0] >> 3) & 1;
        chip->fsm_cycle1 = (chip->fsm_sync1[0] >> 4) & 1;
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
        }
        
        if (ic)
        {
            chip->reg_test[0] = 0;
            chip->reg_timer_a[0] = 0;
            chip->reg_timer_b[0] = 0;
            chip->reg_timer_a_load[0] = 0;
            chip->reg_timer_b_load[0] = 0;
            chip->reg_timer_a_irq[0] = 0;
            chip->reg_timer_b_irq[0] = 0;
        }
        else
        {
            if (write1_en && chip->reg_write_01[1])
                chip->reg_test[0] = data1;
            else
                chip->reg_test[0] = chip->reg_test[1];
            chip->reg_timer_a[0] = chip->reg_timer_a[1];
            if (write1_en && chip->reg_write_10[1])
            {
                chip->reg_timer_a[0] & ~= 0x3fc;
                chip->reg_timer_a[0] |= chip->data1 << 2;
            }
            if (write1_en && chip->reg_write_11[1])
            {
                chip->reg_timer_a[0] & ~= 0x3;
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

        chip->reg_test[1] = chip->reg_test[0];
        chip->reg_timer_a[1] = chip->reg_timer_a[0];
        chip->reg_timer_b[1] = chip->reg_timer_b[0];
        chip->reg_timer_a_load[1] = chip->reg_timer_a_load[0];
        chip->reg_timer_b_load[1] = chip->reg_timer_b_load[0];
        chip->reg_timer_a_irq[1] = chip->reg_timer_a_irq[0];
        chip->reg_timer_b_irq[1] = chip->reg_timer_b_irq[0];
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
}
