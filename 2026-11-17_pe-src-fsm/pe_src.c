#include "pe_src.h"

static void emit(pe_ctx_t *c, pe_act_t t, uint16_t arg)
{
    if (c->act_n < PE_ACT_MAX) {
        c->act[c->act_n].type = t;
        c->act[c->act_n].arg  = arg;
        c->act_n++;
    }
}

static void timer_start(pe_ctx_t *c, uint16_t ms) { c->timer_ms = ms; c->timer_run = 1; }
static void timer_stop(pe_ctx_t *c)               { c->timer_run = 0; c->timer_ms = 0; }

/* 給定範圍內視為合法 Request（本題簡化：5V~20V） */
static uint8_t request_valid(uint16_t mv) { return (mv >= 5000u && mv <= 20000u); }

static void enter(pe_ctx_t *c, pe_state_t s, uint16_t arg);

void pe_init(pe_ctx_t *c)
{
    uint8_t i;
    uint8_t *p = (uint8_t *)c;
    for (i = 0; i < sizeof(*c); i++) p[i] = 0;
    c->state = PE_SRC_STARTUP;
}

void pe_clear_actions(pe_ctx_t *c) { c->act_n = 0; }

/* 進入狀態時的動作 (entry action) */
static void enter(pe_ctx_t *c, pe_state_t s, uint16_t arg)
{
    c->state = s;
    timer_stop(c);
    switch (s) {
    case PE_SRC_SEND_CAPS:
        /* TODO(2): CapsCounter 加 1，並要求送出 Source_Capabilities */
        break;
    case PE_SRC_DISCOVERY:
        /* TODO(2): 啟動 tTypeCSendSourceCap */
        break;
    case PE_SRC_NEGOTIATE_CAP:
        /* TODO(3): 記下 req_mv；用 request_valid() 判斷：
         *   合法 -> 送 Accept，進 TRANSITION_SUPPLY
         *   非法 -> 送 Reject，有 explicit contract 回 READY，否則 WAIT_NEW_CAPS */
        break;
    case PE_SRC_TRANSITION_SUPPLY:
        /* TODO(4): phase=0，啟動 tSrcTransition */
        break;
    case PE_SRC_HARD_RESET:
        /* TODO(5): HardResetCounter++、送 Hard Reset、啟動 tPSHardReset */
        break;
    case PE_SRC_TRANSITION_TO_DEFAULT:
        /* TODO(5): 清 explicit_contract、VBUS 關閉、啟動 tSrcRecover */
        break;
    default:
        break;
    }
}

static void on_timeout(pe_ctx_t *c)
{
    switch (c->state) {
    case PE_SRC_SEND_CAPS:
        /* TODO(6): tSenderResponse 逾時：HardResetCounter <= nHardResetCount 則 HARD_RESET，否則 DISABLED */
        break;
    case PE_SRC_DISCOVERY:
        /* TODO(6): CapsCounter > nCapsCount 則 DISABLED，否則回 SEND_CAPS */
        break;
    case PE_SRC_TRANSITION_SUPPLY:
        /* TODO(4): phase 0 逾時 -> 送 ACT_SET_VBUS(req_mv)、phase=1、啟動 T_VBUS_READY_TIMEOUT
         *          phase 1 逾時 -> HARD_RESET */
        break;
    case PE_SRC_HARD_RESET:
        /* TODO(5): tPSHardReset 到 -> TRANSITION_TO_DEFAULT */
        break;
    case PE_SRC_TRANSITION_TO_DEFAULT:
        /* TODO(5): tSrcRecover 到 -> 送 ACT_VBUS_5V，回 STARTUP */
        break;
    default:
        break;
    }
}

void pe_event(pe_ctx_t *c, pe_event_t ev, uint16_t arg)
{
    /* 全域：任何時候收到對方 Hard Reset（STARTUP / 已在復位流程中除外） */
    if (ev == EV_RX_HARD_RESET &&
        c->state != PE_SRC_STARTUP && c->state != PE_SRC_TRANSITION_TO_DEFAULT &&
        c->state != PE_SRC_DISABLED) {
        enter(c, PE_SRC_TRANSITION_TO_DEFAULT, 0);
        return;
    }

    if (ev == EV_TICK) {
        /* TODO(1): 若 timer 正在跑，扣掉 arg ms；到期（含 arg 大於剩餘）就停掉 timer 並呼叫 on_timeout()。
         *          小心 uint16_t 相減 underflow。 */
        (void)arg;
        return;
    }

    switch (c->state) {
    case PE_SRC_STARTUP:
        if (ev == EV_START) enter(c, PE_SRC_SEND_CAPS, 0);
        break;
    case PE_SRC_SEND_CAPS:
        if (ev == EV_TX_GOODCRC) {
            /* TODO(7): CapsCounter 歸零，啟動 tSenderResponse */
        } else if (ev == EV_TX_NO_GOODCRC) {
            enter(c, PE_SRC_DISCOVERY, 0);
        } else if (ev == EV_RX_REQUEST) {
            enter(c, PE_SRC_NEGOTIATE_CAP, arg);
        }
        break;
    case PE_SRC_TRANSITION_SUPPLY:
        if (ev == EV_VBUS_READY && c->phase == 1) {
            /* TODO(7): 送 PS_RDY、explicit_contract=1、HardResetCounter 歸零、進 READY */
        }
        break;
    case PE_SRC_READY:
        if (ev == EV_RX_REQUEST)          enter(c, PE_SRC_NEGOTIATE_CAP, arg);
        else if (ev == EV_RX_GET_SRC_CAP) enter(c, PE_SRC_SEND_CAPS, 0);
        else if (ev == EV_CAPS_CHANGED)   enter(c, PE_SRC_SEND_CAPS, 0);
        break;
    case PE_SRC_WAIT_NEW_CAPS:
        if (ev == EV_CAPS_CHANGED) enter(c, PE_SRC_SEND_CAPS, 0);
        break;
    default:
        break;
    }
}
