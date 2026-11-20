/* sim.h - 在 PC 上模擬 8051「ISR 可在任意兩個 byte 存取之間插入」的環境。
 * 不要修改本檔。 */
#ifndef SIM_H
#define SIM_H
#include <stdint.h>

void tick_isr(void);              /* 1ms Timer ISR（在 tick.c 提供） */

extern volatile int sim_ea;       /* 模擬 EA 位元：1=允許中斷 */
extern int sim_inject_at;         /* 在第 N 次 byte 存取後觸發 ISR（1 起算；0=不觸發） */
extern int sim_count;             /* 目前已進行的 byte 存取次數 */
extern int sim_pending;           /* EA=0 時被延後的 ISR */
extern int sim_isr_runs;          /* ISR 實際執行次數 */

/* 主程式每存取「一個 byte」的共享變數後，都要呼叫一次，
 * 代表 8051 此刻「可能」被中斷（8 位元 CPU 一次只能搬 1 byte）。 */
void sim_byte_access(void);

#define EA_DISABLE()  do { sim_ea = 0; } while (0)             /* C51: EA = 0; */
#define EA_ENABLE()   do { sim_ea = 1; \
                           if (sim_pending) { sim_pending = 0; sim_isr_runs++; tick_isr(); } \
                         } while (0)                           /* C51: EA = 1; */
void sim_reset(void);
#endif
