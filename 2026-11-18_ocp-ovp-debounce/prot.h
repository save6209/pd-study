/* OCP/OVP 保護：ADC 定點換算 + debounce（可直接移植 C51，不使用 float） */
#ifndef PROT_H
#define PROT_H
#include <stdint.h>

/* ---- 硬體參數（假設值，可自行對照你的板子） ---- */
#define ADC_BITS_MAX     1023u   /* 10-bit ADC */
#define ADC_VREF_MV      3300u   /* 參考電壓 3.3V */
#define VBUS_DIV_NUM     11u     /* VBUS 分壓 1/11 (100k:10k) -> 還原乘 11 */
#define ISENSE_MOHM      5u      /* sense resistor 5 mOhm */
#define ISENSE_GAIN      50u     /* 電流放大器增益 50 倍 */
/* 因此 I(mA) = Vadc(mV) * 1000 / (5*50) = Vadc(mV) * 4 */

/* ---- debounce 參數（每 1ms 呼叫一次 prot_tick） ---- */
#define OVP_TRIP_TICKS   3u      /* 連續 3 次超限才跳 OVP（任一次正常即歸零） */
#define OCP_TRIP_TICKS   20u     /* OCP 漏水桶：超限 +1，正常 -1（下限 0），到 20 跳 */

#define PROT_OVP         0x01u
#define PROT_OCP         0x02u

typedef struct {
    uint16_t contract_mv;
    uint16_t ovp_limit_mv;
    uint16_t ocp_limit_ma;
    uint8_t  ovp_cnt;
    uint8_t  ocp_cnt;
    uint8_t  flags;          /* PROT_OVP | PROT_OCP，latched */
} prot_t;

/* 10-bit ADC 原始值 -> VBUS (mV)，四捨五入，全程整數 */
uint16_t adc_to_vbus_mv(uint16_t adc);
/* 10-bit ADC 原始值 -> IBUS (mA)，四捨五入，全程整數 */
uint16_t adc_to_ibus_ma(uint16_t adc);

/* OVP 門檻 = 合約電壓 * 110%；OCP 門檻 = 合約電流 * 120%（注意 uint16 溢位） */
uint16_t ovp_limit_from_contract(uint16_t mv);
uint16_t ocp_limit_from_contract(uint16_t ma);

void    prot_init(prot_t *p);
/* 協商完成 / PPS 重新 Request 時呼叫：更新門檻、清 debounce 計數，但「不清」已 latch 的 flags */
void    prot_set_contract(prot_t *p, uint16_t mv, uint16_t ma);
/* 每 1ms 一次，回傳目前 flags */
uint8_t prot_tick(prot_t *p, uint16_t vbus_mv, uint16_t ibus_ma);
/* 嘗試清除 latch：僅當 vbus <= ovp_limit - ovp_limit/16 且 ibus < ocp_limit 才清，回傳清除後 flags */
uint8_t prot_try_clear(prot_t *p, uint16_t vbus_mv, uint16_t ibus_ma);

#endif
