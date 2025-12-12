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
}
