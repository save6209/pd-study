#include "tick.h"
#include "sim.h"

volatile uint8_t tick_b[4];

void tick_set(uint32_t v) { for (int i = 0; i < 4; i++) tick_b[i] = (uint8_t)(v >> (8 * i)); }
uint32_t tick_peek(void)  { return (uint32_t)tick_b[0] | ((uint32_t)tick_b[1] << 8)
                                 | ((uint32_t)tick_b[2] << 16) | ((uint32_t)tick_b[3] << 24); }

/* 已提供：ISR 逐 byte 遞增 */
void tick_isr(void)
{
    for (int i = 0; i < 4; i++) {
        if (++tick_b[i] != 0) break;   /* 沒進位就結束 */
    }
}

/* 已提供（有 bug）：LSB 先讀。若在讀 b0 與 b1 之間 0xFF→0x100，會讀到 0x1FF */
uint32_t tick_read_naive(void)
{
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) {
        v |= (uint32_t)tick_b[i] << (8 * i);
        sim_byte_access();             /* 每讀 1 byte，ISR 可能插入 */
    }
    return v;
}

/* TODO 1：用 EA_DISABLE()/EA_ENABLE() 包住讀取，回傳一致的值。
 *         思考：為什麼 EA_ENABLE 之前不能 return？臨界區要多短？ */
uint32_t tick_read_critical(void)
{
    /* TODO */
    return 0;
}

/* TODO 2：不關中斷。連續讀兩次（每次用 sim_byte_access() 逐 byte 讀），
 *         兩次相同才回傳，否則重讀。為什麼對「單一寫入者、只會遞增」的計數器可行？ */
uint32_t tick_read_retry(void)
{
    /* TODO */
    return 0;
}

/* TODO 3：now/start/timeout 單位 ms，32-bit 會溢位繞回。
 *         回傳 timeout 是否已到。注意 now >= start + timeout 在繞回時會錯！ */
int pd_timer_expired(uint32_t now, uint32_t start, uint32_t timeout_ms)
{
    /* TODO */
    (void)now; (void)start; (void)timeout_ms;
    return 0;
}

/* TODO 4：SPSC ring buffer。head/tail 為 uint8_t，用 (RB_SIZE-1) 遮罩。
 *         規則：滿 = ((head+1)&MASK)==tail（浪費 1 格）；空 = head==tail。
 *         寫入端必須「先寫資料、再推進 head」；讀取端「先讀資料、再推進 tail」。 */
void rb_init(rb_t *r) { r->head = 0; r->tail = 0; }

int rb_put(rb_t *r, uint8_t v)
{
    /* TODO */
    (void)r; (void)v;
    return 0;
}

int rb_get(rb_t *r, uint8_t *v)
{
    /* TODO */
    (void)r; (void)v;
    return 0;
}
