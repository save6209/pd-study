#include <stdio.h>
#include <string.h>
#include "pe_src.h"

static int fails, checks;
#define CHECK(cond) do { checks++; if (!(cond)) { fails++; \
    printf("  FAIL line %d: %s\n", __LINE__, #cond); } } while (0)

static int has_act(pe_ctx_t *c, pe_act_t t, uint16_t arg, int check_arg)
{
    uint8_t i;
    for (i = 0; i < c->act_n; i++)
        if (c->act[i].type == t && (!check_arg || c->act[i].arg == arg)) return 1;
    return 0;
}
static int count_act(pe_ctx_t *c, pe_act_t t)
{
    uint8_t i; int n = 0;
    for (i = 0; i < c->act_n; i++) if (c->act[i].type == t) n++;
    return n;
}

/* 走到 SEND_CAPS 且收到 GoodCRC */
static void to_caps_acked(pe_ctx_t *c)
{
    pe_init(c);
    pe_event(c, EV_START, 0);
    pe_event(c, EV_TX_GOODCRC, 0);
}
/* 走完 Negotiate -> Transition_Supply -> Ready */
static void to_ready(pe_ctx_t *c, uint16_t mv)
{
    to_caps_acked(c);
    pe_event(c, EV_RX_REQUEST, mv);
    pe_event(c, EV_TICK, T_SRC_TRANSITION);
    pe_event(c, EV_VBUS_READY, 0);
}

static void t_startup(void)
{
    pe_ctx_t c; printf("[startup]\n");
    pe_init(&c);
    CHECK(c.state == PE_SRC_STARTUP);
    pe_event(&c, EV_TICK, 1000);          /* STARTUP 不該被 tick 帶走 */
    CHECK(c.state == PE_SRC_STARTUP);
    pe_event(&c, EV_START, 0);
    CHECK(c.state == PE_SRC_SEND_CAPS);
    CHECK(has_act(&c, ACT_TX_SRC_CAP, 0, 0));
    CHECK(c.caps_counter == 1);
}

static void t_timer(void)
{
    pe_ctx_t c; printf("[timer]\n");
    to_caps_acked(&c);
    pe_event(&c, EV_TICK, T_SENDER_RESPONSE - 1);
    CHECK(c.state == PE_SRC_SEND_CAPS);   /* 差 1ms 不能逾時 */
    CHECK(c.timer_ms == 1);
    pe_event(&c, EV_TICK, 1);
    CHECK(c.state == PE_SRC_HARD_RESET);  /* 剛好到期 */
    /* 一次 tick 遠大於剩餘時間：不可 underflow */
    to_caps_acked(&c);
    pe_event(&c, EV_TICK, 5000);
    CHECK(c.state == PE_SRC_HARD_RESET);
    CHECK(c.timer_ms == T_PS_HARD_RESET);
}

static void t_negotiate_ok(void)
{
    pe_ctx_t c; printf("[negotiate ok]\n");
    to_caps_acked(&c);
    pe_clear_actions(&c);
    pe_event(&c, EV_RX_REQUEST, 9000);
    CHECK(c.state == PE_SRC_TRANSITION_SUPPLY);
    CHECK(has_act(&c, ACT_TX_ACCEPT, 0, 0));
    CHECK(!has_act(&c, ACT_SET_VBUS, 0, 0));      /* 還要等 tSrcTransition */
    pe_event(&c, EV_VBUS_READY, 0);               /* 太早的 VBUS_READY 要忽略 */
    CHECK(c.state == PE_SRC_TRANSITION_SUPPLY);
    pe_event(&c, EV_TICK, T_SRC_TRANSITION);
    CHECK(has_act(&c, ACT_SET_VBUS, 9000, 1));
    pe_event(&c, EV_VBUS_READY, 0);
    CHECK(c.state == PE_SRC_READY);
    CHECK(has_act(&c, ACT_TX_PS_RDY, 0, 0));
    CHECK(c.explicit_contract == 1);
}

static void t_negotiate_reject(void)
{
    pe_ctx_t c; printf("[negotiate reject]\n");
    to_caps_acked(&c);
    pe_clear_actions(&c);
    pe_event(&c, EV_RX_REQUEST, 28000);           /* 超出能力 */
    CHECK(has_act(&c, ACT_TX_REJECT, 0, 0));
    CHECK(c.state == PE_SRC_WAIT_NEW_CAPS);       /* 尚無 explicit contract */
    pe_event(&c, EV_CAPS_CHANGED, 0);
    CHECK(c.state == PE_SRC_SEND_CAPS);

    to_ready(&c, 9000);
    pe_clear_actions(&c);
    pe_event(&c, EV_RX_REQUEST, 0);               /* 已有合約：Reject 後回 Ready */
    CHECK(has_act(&c, ACT_TX_REJECT, 0, 0));
    CHECK(c.state == PE_SRC_READY);
    CHECK(c.explicit_contract == 1);
}

static void t_vbus_timeout(void)
{
    pe_ctx_t c; printf("[vbus timeout]\n");
    to_caps_acked(&c);
    pe_event(&c, EV_RX_REQUEST, 15000);
    pe_event(&c, EV_TICK, T_SRC_TRANSITION);
    CHECK(c.state == PE_SRC_TRANSITION_SUPPLY);
    pe_event(&c, EV_TICK, T_VBUS_READY_TIMEOUT);
    CHECK(c.state == PE_SRC_HARD_RESET);
}

static void t_hard_reset_flow(void)
{
    pe_ctx_t c; printf("[hard reset flow]\n");
    to_ready(&c, 9000);
    pe_clear_actions(&c);
    pe_event(&c, EV_RX_HARD_RESET, 0);            /* 對方 Hard Reset */
    CHECK(c.state == PE_SRC_TRANSITION_TO_DEFAULT);
    CHECK(c.explicit_contract == 0);
    CHECK(has_act(&c, ACT_VBUS_OFF, 0, 0));
    CHECK(!has_act(&c, ACT_TX_HARD_RESET, 0, 0)); /* 對方發的，我方不回送 */
    pe_event(&c, EV_TICK, T_SRC_RECOVER - 1);
    CHECK(c.state == PE_SRC_TRANSITION_TO_DEFAULT);
    pe_event(&c, EV_TICK, 1);
    CHECK(c.state == PE_SRC_STARTUP);
    CHECK(has_act(&c, ACT_VBUS_5V, 0, 0));
}

static void t_discovery(void)
{
    pe_ctx_t c; int round, sends = 0; printf("[discovery / nCapsCount]\n");
    pe_init(&c);
    pe_event(&c, EV_START, 0);
    pe_event(&c, EV_TX_NO_GOODCRC, 0);
    CHECK(c.state == PE_SRC_DISCOVERY);
    pe_event(&c, EV_TICK, T_TYPEC_SEND_SRC_CAP - 1);
    CHECK(c.state == PE_SRC_DISCOVERY);
    pe_event(&c, EV_TICK, 1);
    CHECK(c.state == PE_SRC_SEND_CAPS);

    /* 沒有 Sink：Source_Capabilities 共送 nCapsCount+1 次後 Disabled */
    pe_init(&c);
    pe_event(&c, EV_START, 0);
    for (round = 0; round < 200 && c.state != PE_SRC_DISABLED; round++) {
        sends += count_act(&c, ACT_TX_SRC_CAP);   /* act[] 只有 16 格，每輪清掉 */
        pe_clear_actions(&c);
        if (c.state == PE_SRC_SEND_CAPS) pe_event(&c, EV_TX_NO_GOODCRC, 0);
        else                             pe_event(&c, EV_TICK, T_TYPEC_SEND_SRC_CAP);
    }
    CHECK(c.state == PE_SRC_DISABLED);
    CHECK(sends + count_act(&c, ACT_TX_SRC_CAP) == (int)N_CAPS_COUNT + 1);
}

static void t_hard_reset_limit(void)
{
    pe_ctx_t c; int i; printf("[hard reset counter]\n");
    /* Sink 回 GoodCRC 但永遠不送 Request */
    pe_init(&c);
    pe_event(&c, EV_START, 0);
    for (i = 0; i < 3; i++) {                     /* 前 3 次 (counter 0,1,2) 都走 Hard Reset */
        pe_event(&c, EV_TX_GOODCRC, 0);
        pe_event(&c, EV_TICK, T_SENDER_RESPONSE);
        CHECK(c.state == PE_SRC_HARD_RESET);
        pe_event(&c, EV_TICK, T_PS_HARD_RESET);
        pe_event(&c, EV_TICK, T_SRC_RECOVER);
        CHECK(c.state == PE_SRC_STARTUP);
        pe_event(&c, EV_START, 0);
    }
    CHECK(c.hard_reset_counter == 3);
    pe_event(&c, EV_TX_GOODCRC, 0);
    pe_event(&c, EV_TICK, T_SENDER_RESPONSE);
    CHECK(c.state == PE_SRC_DISABLED);
}

int main(void)
{
    t_startup(); t_timer(); t_negotiate_ok(); t_negotiate_reject();
    t_vbus_timeout(); t_hard_reset_flow(); t_discovery(); t_hard_reset_limit();
    printf("\n%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
