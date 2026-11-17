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
        c->caps_counter++;
        emit(c, ACT_TX_SRC_CAP, 0);
        break;
    case PE_SRC_DISCOVERY:
        timer_start(c, T_TYPEC_SEND_SRC_CAP);
        break;
    case PE_SRC_NEGOTIATE_CAP:
        c->req_mv = arg;
        if (request_valid(arg)) {
            emit(c, ACT_TX_ACCEPT, 0);
            enter(c, PE_SRC_TRANSITION_SUPPLY, 0);
        } else {
            emit(c, ACT_TX_REJECT, 0);
            enter(c, c->explicit_contract ? PE_SRC_READY : PE_SRC_WAIT_NEW_CAPS, 0);
        }
        break;
    case PE_SRC_TRANSITION_SUPPLY:
        c->phase = 0;
        timer_start(c, T_SRC_TRANSITION);
        break;
    case PE_SRC_HARD_RESET:
        c->hard_reset_counter++;
        emit(c, ACT_TX_HARD_RESET, 0);
        timer_start(c, T_PS_HARD_RESET);
        break;
    case PE_SRC_TRANSITION_TO_DEFAULT:
        c->explicit_contract = 0;
        emit(c, ACT_VBUS_OFF, 0);
        timer_start(c, T_SRC_RECOVER);
        break;
    default:
        break;
    }
}

static void on_timeout(pe_ctx_t *c)
{
    switch (c->state) {
    case PE_SRC_SEND_CAPS:          /* tSenderResponse：GoodCRC 後等不到 Request */
        if (c->hard_reset_counter <= N_HARD_RESET_COUNT) enter(c, PE_SRC_HARD_RESET, 0);
        else                                             enter(c, PE_SRC_DISABLED, 0);
        break;
    case PE_SRC_DISCOVERY:          /* tTypeCSendSourceCap */
        if (c->caps_counter > N_CAPS_COUNT) enter(c, PE_SRC_DISABLED, 0);
        else                                enter(c, PE_SRC_SEND_CAPS, 0);
        break;
    case PE_SRC_TRANSITION_SUPPLY:
        if (c->phase == 0) {        /* tSrcTransition 到：可以開始改變電源 */
            emit(c, ACT_SET_VBUS, c->req_mv);
            c->phase = 1;
            timer_start(c, T_VBUS_READY_TIMEOUT);
        } else {                    /* VBUS 一直沒到 */
            enter(c, PE_SRC_HARD_RESET, 0);
        }
        break;
    case PE_SRC_HARD_RESET:
        enter(c, PE_SRC_TRANSITION_TO_DEFAULT, 0);
        break;
    case PE_SRC_TRANSITION_TO_DEFAULT:  /* tSrcRecover 後恢復 vSafe5V */
        emit(c, ACT_VBUS_5V, 0);
        enter(c, PE_SRC_STARTUP, 0);
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
        if (c->timer_run) {
            if (arg >= c->timer_ms) {   /* 注意：uint16 相減會 underflow，先比較 */
                timer_stop(c);
                on_timeout(c);
            } else {
                c->timer_ms -= arg;
            }
        }
        return;
    }

    switch (c->state) {
    case PE_SRC_STARTUP:
        if (ev == EV_START) enter(c, PE_SRC_SEND_CAPS, 0);
        break;
    case PE_SRC_SEND_CAPS:
        if (ev == EV_TX_GOODCRC) {
            c->caps_counter = 0;
            timer_start(c, T_SENDER_RESPONSE);
        } else if (ev == EV_TX_NO_GOODCRC) {
            enter(c, PE_SRC_DISCOVERY, 0);
        } else if (ev == EV_RX_REQUEST) {
            enter(c, PE_SRC_NEGOTIATE_CAP, arg);
        }
        break;
    case PE_SRC_TRANSITION_SUPPLY:
        if (ev == EV_VBUS_READY && c->phase == 1) {
            emit(c, ACT_TX_PS_RDY, 0);
            c->explicit_contract = 1;
            c->hard_reset_counter = 0;
            enter(c, PE_SRC_READY, 0);
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
