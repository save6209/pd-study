#include "stack_guard.h"

void stack_paint(uint8_t *idata, uint8_t base, uint8_t limit)
{
    /* TODO 1: 注意 limit 可能是 0xFF，用 uint8_t 迴圈變數會無窮迴圈！ */
    (void)idata; (void)base; (void)limit;
}

uint8_t stack_high_water(const uint8_t *idata, uint8_t base, uint8_t limit)
{
    /* TODO 2: 從 limit 往下找第一個 != STACK_PAINT */
    (void)idata; (void)base; (void)limit;
    return 0;
}

uint8_t stack_free(uint8_t sp, uint8_t limit)
{
    /* TODO 3 */
    (void)sp; (void)limit;
    return 0;
}

int stack_overflowed(const uint8_t *idata, uint8_t limit)
{
    /* TODO 4 */
    (void)idata; (void)limit;
    return 0;
}

/* 提示：用 state[] 做 DFS 三色標記（0=未訪, 1=訪問中, 2=完成），
 * 遇到 state==1 的節點就是遞迴。可再加 memo[] 避免重複計算。 */
static int cost_dfs(const fn_t *g, int n, int f, uint8_t *state, int *memo)
{
    /* TODO 5 */
    (void)g; (void)n; (void)f; (void)state; (void)memo;
    return 0;
}

int stack_cost(const fn_t *g, int n, int root)
{
    uint8_t state[MAX_FN] = {0};
    int memo[MAX_FN] = {0};
    return cost_dfs(g, n, root, state, memo);
}

int stack_worst_total(const fn_t *g, int n, int main_root,
                      const isr_t *isrs, int nisr)
{
    /* TODO 6: main + max(low ISR) + max(high ISR)，各 ISR +2 */
    (void)g; (void)n; (void)main_root; (void)isrs; (void)nisr;
    return 0;
}
