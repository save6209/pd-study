#ifndef STACK_GUARD_H
#define STACK_GUARD_H
#include <stdint.h>

#define STACK_PAINT   0x55u   /* 填充樣式 */
#define MAX_FN        16
#define MAX_CALLEE    4

/* ---------- Part 1: stack painting（在 PC 上用 uint8_t idata[256] 模擬 8051 IDATA）---------- */

/* 把 idata[base .. limit] 含頭尾全部填成 STACK_PAINT */
void stack_paint(uint8_t *idata, uint8_t base, uint8_t limit);

/* 回傳歷史最高使用量（bytes）。
 * 8051 stack 往上長（PUSH 先 SP++ 再寫入），所以從 limit 往下找
 * 第一個不是 STACK_PAINT 的位置 p，用量 = p - base + 1；全是 PAINT 則 0。 */
uint8_t stack_high_water(const uint8_t *idata, uint8_t base, uint8_t limit);

/* 目前 SP 之上還剩多少 byte 可用（limit - sp）；sp > limit 時回 0 */
uint8_t stack_free(uint8_t sp, uint8_t limit);

/* 判斷 stack 是否已被打穿：最頂端 canary 位置 idata[limit] 不是 PAINT → 1 */
int stack_overflowed(const uint8_t *idata, uint8_t limit);

/* ---------- Part 2: 最深呼叫鏈估算 ---------- */

typedef struct {
    const char *name;
    uint8_t frame;                /* 此函式自己額外 PUSH 的 bytes（不含 return address）*/
    int8_t  callee[MAX_CALLEE];   /* 呼叫的函式 index，-1 結束 */
} fn_t;

/* 函式 root 的最壞 stack 用量（bytes）：
 *   cost(f) = f.frame + max over callee c of ( 2 + cost(c) )
 *   其中 2 = LCALL 推入的 return address（PCL, PCH）
 * 遇到遞迴（呼叫圖有環）回傳 -1。n 為函式總數。 */
int stack_cost(const fn_t *g, int n, int root);

/* ---------- Part 3: ISR 巢狀 ---------- */

typedef struct {
    int root;           /* ISR 在 g[] 裡的 index */
    uint8_t priority;   /* 0 = low, 1 = high（8051 只有兩級硬體優先權）*/
} isr_t;

/* 系統最壞 stack 用量 = sp_init偏移（不含，這裡只算 bytes）
 *   main 的 cost + 同一時間「最多一個 low ISR + 一個 high ISR」
 *   每個 ISR 額外 +2（硬體 LCALL 推 PC）。
 *   low/high 各取該級別 cost 最大者。任何一個 cost 為 -1 → 回 -1。 */
int stack_worst_total(const fn_t *g, int n, int main_root,
                      const isr_t *isrs, int nisr);

#endif
