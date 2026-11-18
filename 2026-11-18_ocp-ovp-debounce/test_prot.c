#include <stdio.h>
#include "prot.h"

static int fails, total;
#define CHECK(c, msg) do { total++; if (!(c)) { fails++; printf("FAIL: %s (line %d)\n", msg, __LINE__); } } while (0)
#define EQ(a, b, msg) do { total++; long _a=(long)(a), _b=(long)(b); if (_a!=_b) { fails++; printf("FAIL: %s got %ld expect %ld (line %d)\n", msg, _a, _b, __LINE__);} } while (0)

static void run(prot_t *p, int n, uint16_t v, uint16_t i) { while (n--) prot_tick(p, v, i); }

int main(void)
{
    prot_t p;

    /* --- ADC 換算 --- */
    EQ(adc_to_vbus_mv(0), 0, "vbus adc=0");
    EQ(adc_to_vbus_mv(1023), 36300, "vbus full scale");
    EQ(adc_to_vbus_mv(1), 35, "vbus adc=1 rounding (35.48)");
    EQ(adc_to_vbus_mv(155), 5500, "vbus 5V-ish (5500.0)");
    EQ(adc_to_vbus_mv(556), 19729, "vbus adc=556 (19729.4)");
    EQ(adc_to_ibus_ma(0), 0, "ibus adc=0");
    EQ(adc_to_ibus_ma(1023), 13200, "ibus full scale");
    EQ(adc_to_ibus_ma(1), 13, "ibus adc=1 (12.9)");
    EQ(adc_to_ibus_ma(231), 2981, "ibus 3A-ish (2980.6)");
    CHECK(adc_to_ibus_ma(500) == (uint16_t)((500UL*13200UL + 511UL)/1023UL), "ibus single division");

    /* --- 門檻 --- */
    EQ(ovp_limit_from_contract(5000), 5500, "ovp 5V");
    EQ(ovp_limit_from_contract(20000), 22000, "ovp 20V no overflow");
    EQ(ovp_limit_from_contract(21000), 23100, "ovp 21V");
    EQ(ocp_limit_from_contract(3000), 3600, "ocp 3A");
    EQ(ocp_limit_from_contract(5000), 6000, "ocp 5A");

    /* --- 尚無合約 --- */
    prot_init(&p);
    run(&p, 50, 30000, 30000);
    EQ(p.flags, 0, "no contract -> no trip");
    EQ(prot_try_clear(&p, 0, 0), 0, "no contract clear is noop");

    /* --- OVP：連續 3 次 --- */
    prot_set_contract(&p, 5000, 3000);
    EQ(p.ovp_limit_mv, 5500, "limit set");
    prot_tick(&p, 5501, 0); prot_tick(&p, 5501, 0);
    EQ(p.flags, 0, "2 ticks no trip");
    prot_tick(&p, 5500, 0);          /* 剛好等於門檻 = 正常，歸零 */
    prot_tick(&p, 5600, 0); prot_tick(&p, 5600, 0);
    EQ(p.flags, 0, "glitch resets ovp counter");
    prot_tick(&p, 5600, 0);
    EQ(p.flags, PROT_OVP, "3 consecutive -> OVP");

    /* --- latch 與 clear --- */
    run(&p, 10, 5000, 0);
    EQ(p.flags, PROT_OVP, "latched stays");
    EQ(prot_try_clear(&p, 5300, 0), PROT_OVP, "5300 > 5500-343 -> cannot clear");
    EQ(prot_try_clear(&p, 5157, 0), 0, "5157 <= 5157 -> clear");

    /* --- OCP：漏水桶 --- */
    prot_set_contract(&p, 5000, 3000);
    run(&p, 19, 5000, 3601);
    EQ(p.flags, 0, "19 ticks no trip");
    run(&p, 5, 5000, 3000);          /* 退 5 */
    EQ(p.ocp_cnt, 14, "leaky decrement");
    run(&p, 6, 5000, 3601);
    EQ(p.flags, PROT_OCP, "back to 20 -> OCP");
    /* 計數器不環繞 */
    run(&p, 400, 5000, 9000);
    EQ(p.ocp_cnt, OCP_TRIP_TICKS, "ocp_cnt saturates");
    EQ(prot_try_clear(&p, 5000, 3600), PROT_OCP, "ibus == limit cannot clear");
    EQ(prot_try_clear(&p, 5000, 3599), 0, "ibus < limit clear");
    EQ(p.ocp_cnt, 0, "clear resets counters");

    /* --- 總是低於下限時不下溢 --- */
    prot_set_contract(&p, 9000, 3000);
    run(&p, 300, 9000, 0);
    EQ(p.ocp_cnt, 0, "no underflow");

    /* --- PPS 重新 Request：flags 不清，計數清 --- */
    prot_set_contract(&p, 5000, 3000);
    run(&p, 3, 6000, 0);
    EQ(p.flags, PROT_OVP, "ovp tripped");
    prot_set_contract(&p, 9000, 3000);
    EQ(p.flags, PROT_OVP, "set_contract keeps latched flags");
    EQ(p.ovp_cnt, 0, "set_contract resets cnt");

    /* --- OVP 與 OCP 同時 --- */
    prot_init(&p); prot_set_contract(&p, 5000, 3000);
    run(&p, 20, 6000, 4000);
    EQ(p.flags, PROT_OVP | PROT_OCP, "both flags");

    printf("%d/%d passed\n", total - fails, total);
    return fails ? 1 : 0;
}
