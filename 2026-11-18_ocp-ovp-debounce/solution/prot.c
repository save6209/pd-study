#include "prot.h"

uint16_t adc_to_vbus_mv(uint16_t adc)
{
    uint32_t x = (uint32_t)adc * ADC_VREF_MV * VBUS_DIV_NUM;
    return (uint16_t)((x + ADC_BITS_MAX / 2u) / ADC_BITS_MAX);
}

uint16_t adc_to_ibus_ma(uint16_t adc)
{
    /* 1000/(5*50) = 4，單次除法四捨五入 */
    uint32_t x = (uint32_t)adc * ADC_VREF_MV * (1000u / (ISENSE_MOHM * ISENSE_GAIN));
    return (uint16_t)((x + ADC_BITS_MAX / 2u) / ADC_BITS_MAX);
}

uint16_t ovp_limit_from_contract(uint16_t mv)
{
    return (uint16_t)(((uint32_t)mv * 110u) / 100u);
}

uint16_t ocp_limit_from_contract(uint16_t ma)
{
    return (uint16_t)(((uint32_t)ma * 120u) / 100u);
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

void prot_set_contract(prot_t *p, uint16_t mv, uint16_t ma)
{
    p->contract_mv = mv;
    p->ovp_limit_mv = ovp_limit_from_contract(mv);
    p->ocp_limit_ma = ocp_limit_from_contract(ma);
    p->ovp_cnt = 0;
    p->ocp_cnt = 0;
}

uint8_t prot_tick(prot_t *p, uint16_t vbus_mv, uint16_t ibus_ma)
{
    if (p->ovp_limit_mv == 0) return p->flags;   /* 尚無合約 */

    if (vbus_mv > p->ovp_limit_mv) {
        if (p->ovp_cnt < OVP_TRIP_TICKS) p->ovp_cnt++;
    } else {
        p->ovp_cnt = 0;
    }
    if (ibus_ma > p->ocp_limit_ma) {
        if (p->ocp_cnt < OCP_TRIP_TICKS) p->ocp_cnt++;
    } else if (p->ocp_cnt > 0) {
        p->ocp_cnt--;
    }
    if (p->ovp_cnt >= OVP_TRIP_TICKS) p->flags |= PROT_OVP;
    if (p->ocp_cnt >= OCP_TRIP_TICKS) p->flags |= PROT_OCP;
    return p->flags;
}

uint8_t prot_try_clear(prot_t *p, uint16_t vbus_mv, uint16_t ibus_ma)
{
    if (p->ovp_limit_mv == 0) return p->flags;
    if (vbus_mv <= (uint16_t)(p->ovp_limit_mv - p->ovp_limit_mv / 16u) &&
        ibus_ma < p->ocp_limit_ma) {
        p->flags = 0;
        p->ovp_cnt = 0;
        p->ocp_cnt = 0;
    }
    return p->flags;
}
