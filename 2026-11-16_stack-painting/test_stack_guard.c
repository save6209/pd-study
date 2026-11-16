#include <stdio.h>
#include <string.h>
#include "stack_guard.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static void t_paint(void)
{
    uint8_t ram[256];
    memset(ram, 0, sizeof ram);
    stack_paint(ram, 0x30, 0xFF);               /* limit = 0xFF 邊界 */
    CHECK(ram[0x2F] == 0 && ram[0x30] == STACK_PAINT && ram[0xFF] == STACK_PAINT);
    CHECK(stack_high_water(ram, 0x30, 0xFF) == 0);
    CHECK(!stack_overflowed(ram, 0xFF));

    /* 模擬使用到 0x30+9：寫入 10 bytes */
    for (int i = 0; i < 10; i++) ram[0x30 + i] = (uint8_t)(i + 1);
    CHECK(stack_high_water(ram, 0x30, 0xFF) == 10);
    /* 中間值剛好等於 0x55 的陷阱：最深處寫了 0x55，之後又被別的值蓋過 */
    ram[0x30 + 4] = STACK_PAINT;
    CHECK(stack_high_water(ram, 0x30, 0xFF) == 10);

    ram[0xFF] = 0x12;
    CHECK(stack_overflowed(ram, 0xFF));
    CHECK(stack_high_water(ram, 0x30, 0xFF) == 0xD0);   /* 0xFF-0x30+1 */
}

static void t_free(void)
{
    CHECK(stack_free(0x40, 0xFF) == 0xBF);
    CHECK(stack_free(0xFF, 0xFF) == 0);
    CHECK(stack_free(0xFF, 0x7F) == 0);   /* sp 超過 limit */
}

/* 0 main -> 1 pd_task -> 2 phy_send -> 3 crc ; main -> 3 */
static const fn_t G[] = {
    /*0*/ {"main",     1, {1, 3, -1, -1}},
    /*1*/ {"pd_task",  4, {2, -1, -1, -1}},
    /*2*/ {"phy_send", 2, {3, -1, -1, -1}},
    /*3*/ {"crc",      3, {-1, -1, -1, -1}},
    /*4*/ {"isr_tmr",  5, {3, -1, -1, -1}},   /* low  */
    /*5*/ {"isr_uart", 7, {-1, -1, -1, -1}},  /* low  */
    /*6*/ {"isr_cc",   3, {2, -1, -1, -1}},   /* high */
    /*7*/ {"rec_a",    1, {8, -1, -1, -1}},
    /*8*/ {"rec_b",    1, {7, -1, -1, -1}},
};
#define NG ((int)(sizeof G / sizeof G[0]))

static void t_cost(void)
{
    CHECK(stack_cost(G, NG, 3) == 3);
    CHECK(stack_cost(G, NG, 2) == 2 + 2 + 3);                 /* 7 */
    CHECK(stack_cost(G, NG, 1) == 4 + 2 + 7);                 /* 13 */
    CHECK(stack_cost(G, NG, 0) == 1 + 2 + 13);                /* 16 */
    CHECK(stack_cost(G, NG, 7) == -1);                        /* 遞迴 */
    CHECK(stack_cost(G, NG, 8) == -1);
}

static void t_total(void)
{
    isr_t isrs[] = { {4, 0}, {5, 0}, {6, 1} };
    /* main=16; low: isr_tmr=5+2+3=10 (+2)=12, isr_uart=7(+2)=9 → 12;
       high: isr_cc=3+2+7=12 (+2)=14 → 16+12+14=42 */
    CHECK(stack_worst_total(G, NG, 0, isrs, 3) == 42);
    /* 只有 low ISR */
    CHECK(stack_worst_total(G, NG, 0, isrs, 2) == 16 + 12);
    /* 沒有 ISR */
    CHECK(stack_worst_total(G, NG, 0, isrs, 0) == 16);
    /* ISR 含遞迴 */
    isr_t bad[] = { {7, 0} };
    CHECK(stack_worst_total(G, NG, 0, bad, 1) == -1);
}

int main(void)
{
    t_paint(); t_free(); t_cost(); t_total();
    if (fails) { printf("%d check(s) failed\n", fails); return 1; }
    printf("ALL PASS\n");
    return 0;
}
