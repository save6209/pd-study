#include "sim.h"
volatile int sim_ea = 1;
int sim_inject_at = 0, sim_count = 0, sim_pending = 0, sim_isr_runs = 0;

void sim_byte_access(void)
{
    sim_count++;
    if (sim_inject_at && sim_count == sim_inject_at) {
        if (sim_ea) { sim_isr_runs++; tick_isr(); }
        else        { sim_pending = 1; }   /* EA=0：硬體會把中斷旗標掛住，EA=1 後才進入 */
    }
}
void sim_reset(void)
{
    sim_ea = 1; sim_inject_at = 0; sim_count = 0; sim_pending = 0; sim_isr_runs = 0;
}
