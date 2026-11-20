#include <stdio.h>
#include "tick.h"
#include "sim.h"

static int fails, total;
#define CHECK(c, ...) do { total++; if (!(c)) { fails++; printf("  FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static const uint32_t starts[] = { 0x000000FF, 0x0000FFFF, 0x00FFFFFF, 0xFFFFFFFF, 0x12345678, 0x000001FF };
#define NS (int)(sizeof starts / sizeof starts[0])

static void sweep(const char *name, uint32_t (*rd)(void), int maxpoint, int expect_pending_on_enable)
{
    int bad = 0;
    printf("[%s]\n", name);
    for (int s = 0; s < NS; s++)
        for (int p = 1; p <= maxpoint; p++) {
            sim_reset(); tick_set(starts[s]); sim_inject_at = p;
            uint32_t before = starts[s], after = starts[s] + 1;
            uint32_t v = rd();
            if (v != before && v != after) {
                bad++;
                if (bad <= 3) printf("  start=0x%08X inject@%d -> read 0x%08X (torn!)\n", (unsigned)before, p, (unsigned)v);
            }
            if (expect_pending_on_enable) {   /* ISR 不能被吃掉：函式返回後 tick 必須 = before+1 */
                if (sim_isr_runs != 1 || tick_peek() != after) {
                    bad++;
                    if (bad <= 3) printf("  start=0x%08X inject@%d: ISR 遺失或重複 (runs=%d tick=0x%08X)\n",
                                         (unsigned)before, p, sim_isr_runs, (unsigned)tick_peek());
                }
            }
        }
    CHECK(bad == 0, "%s: %d 個情境讀到不一致的值", name, bad);
}

static void test_naive_is_broken(void)
{
    sim_reset(); tick_set(0x000000FF); sim_inject_at = 1;
    uint32_t v = tick_read_naive();
    printf("[naive 示範] 0x000000FF 在第 1 byte 後被中斷 -> 讀到 0x%08X (合法值只有 0xFF / 0x100)\n", (unsigned)v);
    CHECK(v == 0x1FF, "naive 示範預期 0x1FF（若不是，請檢查 sim 是否被改動）");
}

static void test_critical_section_only(void)
{
    /* 臨界區內 ISR 被延後；EA_ENABLE 後 ISR 恰好執行一次 */
    sim_reset(); tick_set(0xFF); sim_inject_at = 2;
    uint32_t v = tick_read_critical();
    CHECK(v == 0xFF, "關中斷期間 ISR 不可改變讀值，應為 0xFF，得到 0x%X", (unsigned)v);
    CHECK(sim_ea == 1, "函式返回時 EA 必須恢復為 1");
}

static void test_timer(void)
{
    printf("[pd_timer_expired]\n");
    CHECK(pd_timer_expired(1000, 900, 100) == 1, "剛好滿 100ms 應為已逾時");
    CHECK(pd_timer_expired(999, 900, 100) == 0, "99ms 未逾時");
    CHECK(pd_timer_expired(1500, 900, 100) == 1, "已逾時");
    /* 繞回：start 接近 2^32 */
    CHECK(pd_timer_expired(0x00000010, 0xFFFFFFF0, 0x20) == 1, "繞回：經過 0x20 應逾時");
    CHECK(pd_timer_expired(0x0000000F, 0xFFFFFFF0, 0x20) == 0, "繞回：經過 0x1F 未逾時");
    CHECK(pd_timer_expired(0xFFFFFFF5, 0xFFFFFFF0, 0x20) == 0, "start 之後不久不應逾時");
    CHECK(pd_timer_expired(5, 5, 0) == 1, "timeout=0 立刻逾時");
}

static void test_rb(void)
{
    printf("[ring buffer]\n");
    rb_t r; rb_init(&r); uint8_t v = 0;
    CHECK(rb_get(&r, &v) == 0, "空 buffer get 應回 0");
    int n = 0; while (rb_put(&r, (uint8_t)(0xA0 + n))) { n++; if (n > 100) break; }
    CHECK(n == RB_SIZE - 1, "容量應為 RB_SIZE-1=%d，實際 %d", RB_SIZE - 1, n);
    CHECK(rb_put(&r, 0x55) == 0, "滿了 put 應回 0");
    for (int i = 0; i < n; i++) { CHECK(rb_get(&r, &v) == 1 && v == 0xA0 + i, "FIFO 順序錯誤 @%d", i); }
    CHECK(rb_get(&r, &v) == 0, "取完應為空");
    /* head/tail 繞回 uint8_t 邊界與 index 遮罩 */
    int ok = 1;
    for (int i = 0; i < 1000; i++) {
        if (!rb_put(&r, (uint8_t)i) || !rb_put(&r, (uint8_t)(i + 1))) { ok = 0; break; }
        uint8_t a, b;
        if (!rb_get(&r, &a) || !rb_get(&r, &b) || a != (uint8_t)i || b != (uint8_t)(i + 1)) { ok = 0; break; }
    }
    CHECK(ok, "長時間輪轉（head/tail 超過 255）資料錯誤");
}

int main(void)
{
    test_naive_is_broken();
    sweep("TODO1 tick_read_critical", tick_read_critical, 4, 1);
    test_critical_section_only();
    sweep("TODO2 tick_read_retry", tick_read_retry, 8, 1);
    test_timer();
    test_rb();
    printf("\n%d/%d checks passed\n", total - fails, total);
    return fails ? 1 : 0;
}
