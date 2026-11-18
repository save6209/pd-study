#include "prot.h"

/* TODO(1) 10-bit ADC -> VBUS mV
 *   vbus = adc * VREF / 1023 * 11，四捨五入。
 *   提示：先乘完再除以 1023（避免先除損失精度），中間值要用 uint32_t
 *   （C51 的 long 是 4 byte，adc*3300*11 最大約 37M > 65535）。
 *   四捨五入 = (x + 1023/2) / 1023。 */
uint16_t adc_to_vbus_mv(uint16_t adc)
{
    (void)adc;
    return 0; /* TODO */
}

/* TODO(2) 10-bit ADC -> IBUS mA
 *   Vadc(mV) = adc*3300/1023；I(mA) = Vadc * 4。
 *   請「合併成一次除法」做四捨五入，不要先算 Vadc 再乘 4（誤差會被放大 4 倍）。 */
uint16_t adc_to_ibus_ma(uint16_t adc)
{
    (void)adc;
    return 0; /* TODO */
}

/* TODO(3) OVP = mv*110/100；OCP = ma*120/100。20V*110 = 2200000 會溢位 uint16！ */
uint16_t ovp_limit_from_contract(uint16_t mv)
{
    (void)mv;
    return 0; /* TODO */
}

uint16_t ocp_limit_from_contract(uint16_t ma)
{
    (void)ma;
    return 0; /* TODO */
}

void prot_init(prot_t *p)
{
    p->contract_mv = 0;
    p->ovp_limit_mv = 0;
    p->ocp_limit_ma = 0;
    p->ovp_cnt = 0;
    p->ocp_cnt = 0;
    p->flags = 0;
}

/* TODO(4) 更新 contract / 兩個門檻，ovp_cnt、ocp_cnt 歸零，flags 保持不變 */
void prot_set_contract(prot_t *p, uint16_t mv, uint16_t ma)
{
    (void)p; (void)mv; (void)ma;
}

/* TODO(5) 每 1ms 呼叫：
 *   OVP：vbus_mv > ovp_limit_mv 則 ovp_cnt++，否則歸零；ovp_cnt >= OVP_TRIP_TICKS -> flags |= PROT_OVP
 *   OCP：ibus_ma > ocp_limit_ma 則 ocp_cnt++，否則「若 >0 則 --」；ocp_cnt >= OCP_TRIP_TICKS -> flags |= PROT_OCP
 *   注意：uint8_t 計數器不要無限 ++（到門檻就停，避免環繞）；
 *         已 latch 後仍可繼續更新計數，但 flags 只增不減。 */
uint8_t prot_tick(prot_t *p, uint16_t vbus_mv, uint16_t ibus_ma)
{
    (void)vbus_mv; (void)ibus_ma;
    return p->flags;
}

/* TODO(6) 條件見 prot.h；清除時兩個 flags 一起清並把兩個計數歸零。
 *   若 contract 尚未設定（ovp_limit_mv == 0）不得清除。 */
uint8_t prot_try_clear(prot_t *p, uint16_t vbus_mv, uint16_t ibus_ma)
{
    (void)vbus_mv; (void)ibus_ma;
    return p->flags;
}
