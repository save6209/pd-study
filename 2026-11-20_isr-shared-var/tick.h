#ifndef TICK_H
#define TICK_H
#include <stdint.h>

/* 1ms tick，由 Timer ISR 遞增。以 4 個 byte 存放，模擬 8051 上 unsigned long
 * 需要 4 次 MOV 才能讀完（C51 為 big-endian，這裡為了教學用 b[0]=LSB）。 */
extern volatile uint8_t tick_b[4];

void     tick_set(uint32_t v);
uint32_t tick_peek(void);            /* 測試用：直接看真值，不經過模擬 */
void     tick_isr(void);             /* 已提供：逐 byte 遞增並進位 */

uint32_t tick_read_naive(void);      /* 已提供：有 bug 的示範，觀察 torn read */

/* ---- 你要完成的 ---- */
uint32_t tick_read_critical(void);   /* TODO 1：關中斷法 */
uint32_t tick_read_retry(void);      /* TODO 2：不關中斷，連讀兩次比對 */
int      pd_timer_expired(uint32_t now, uint32_t start, uint32_t timeout_ms); /* TODO 3 */

/* TODO 4：ISR(寫入端) / 主迴圈(讀取端) 單生產者單消費者 ring buffer */
#define RB_SIZE 8                     /* 必須是 2 的次方，index 用 uint8_t */
typedef struct {
    volatile uint8_t buf[RB_SIZE];
    volatile uint8_t head;            /* 只有 ISR 會改 */
    volatile uint8_t tail;            /* 只有主迴圈會改 */
} rb_t;
void rb_init(rb_t *r);
int  rb_put(rb_t *r, uint8_t v);      /* ISR 呼叫；滿了回 0，成功回 1 */
int  rb_get(rb_t *r, uint8_t *v);     /* 主迴圈呼叫；空回 0，成功回 1 */
#endif
