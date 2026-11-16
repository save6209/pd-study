#include "../stack_guard.h"

void stack_paint(uint8_t *idata, uint8_t base, uint8_t limit)
{
    unsigned i;   /* 用較寬型別，避免 limit=0xFF 時 uint8_t 迴圈溢位 */
    for (i = base; i <= limit; i++)
        idata[i] = STACK_PAINT;
}

uint8_t stack_high_water(const uint8_t *idata, uint8_t base, uint8_t limit)
{
    int p;
    for (p = limit; p >= base; p--)
        if (idata[p] != STACK_PAINT)
            return (uint8_t)(p - base + 1);
    return 0;
}

uint8_t stack_free(uint8_t sp, uint8_t limit)
{
    return (sp >= limit) ? 0 : (uint8_t)(limit - sp);
}

int stack_overflowed(const uint8_t *idata, uint8_t limit)
{
    return idata[limit] != STACK_PAINT;
}

static int cost_dfs(const fn_t *g, int n, int f, uint8_t *state, int *memo)
{
    int best = 0, i;
    if (f < 0 || f >= n) return -1;
    if (state[f] == 2) return memo[f];
    if (state[f] == 1) return -1;            /* 回到訪問中的節點 → 遞迴 */
    state[f] = 1;
    for (i = 0; i < MAX_CALLEE && g[f].callee[i] >= 0; i++) {
        int c = cost_dfs(g, n, g[f].callee[i], state, memo);
        if (c < 0) return -1;
        if (c + 2 > best) best = c + 2;
    }
    state[f] = 2;
    memo[f] = g[f].frame + best;
    return memo[f];
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
    int lvl[2] = {0, 0}, i;
    int total = stack_cost(g, n, main_root);
    if (total < 0) return -1;
    for (i = 0; i < nisr; i++) {
        int c = stack_cost(g, n, isrs[i].root);
        if (c < 0) return -1;
        c += 2;                              /* 硬體 LCALL 推 PC */
        if (c > lvl[isrs[i].priority ? 1 : 0]) lvl[isrs[i].priority ? 1 : 0] = c;
    }
    return total + lvl[0] + lvl[1];
}
