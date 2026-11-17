#ifndef PE_SRC_H
#define PE_SRC_H
#include <stdint.h>

/* 時間常數 (ms)。tSenderResponse 等為 PD3.x Table 6.68 (Time Values) 的上限取值；
 * T_VBUS_READY_TIMEOUT 為本題自訂（代表電源實際爬升/放電的最長等待）。 */
#define T_SENDER_RESPONSE      30u   /* tSenderResponse 24~30 ms        */
#define T_TYPEC_SEND_SRC_CAP   150u  /* tTypeCSendSourceCap 100~200 ms  */
#define T_SRC_TRANSITION       30u   /* tSrcTransition 25~35 ms         */
#define T_PS_HARD_RESET        30u   /* tPSHardReset 25~35 ms           */
#define T_SRC_RECOVER          800u  /* tSrcRecover 660~1000 ms         */
#define T_VBUS_READY_TIMEOUT   500u  /* 本題自訂                         */
#define N_CAPS_COUNT           50u   /* nCapsCount                      */
#define N_HARD_RESET_COUNT     2u    /* nHardResetCount                 */

typedef enum {
    PE_SRC_STARTUP = 0,
    PE_SRC_SEND_CAPS,
    PE_SRC_DISCOVERY,
    PE_SRC_NEGOTIATE_CAP,
    PE_SRC_TRANSITION_SUPPLY,
    PE_SRC_READY,
    PE_SRC_WAIT_NEW_CAPS,
    PE_SRC_HARD_RESET,
    PE_SRC_TRANSITION_TO_DEFAULT,
    PE_SRC_DISABLED
} pe_state_t;

typedef enum {
    EV_START,          /* VBUS=5V 穩定、Attach 完成            */
    EV_TICK,           /* arg = 經過的 ms                      */
    EV_TX_GOODCRC,     /* 我方送出的訊息收到 GoodCRC           */
    EV_TX_NO_GOODCRC,  /* CRCReceiveTimer 逾時，沒收到 GoodCRC */
    EV_RX_REQUEST,     /* 收到 Request，arg = 要求電壓 (mV)    */
    EV_RX_GET_SRC_CAP, /* 收到 Get_Source_Cap                  */
    EV_RX_HARD_RESET,  /* 收到對方 Hard Reset                  */
    EV_VBUS_READY,     /* 電源已爬升到目標電壓                 */
    EV_CAPS_CHANGED    /* DPM 通知 Source Caps 已改變          */
} pe_event_t;

typedef enum {
    ACT_TX_SRC_CAP = 1,
    ACT_TX_ACCEPT,
    ACT_TX_REJECT,
    ACT_TX_PS_RDY,
    ACT_TX_HARD_RESET,
    ACT_SET_VBUS,      /* arg = 目標電壓 (mV) */
    ACT_VBUS_OFF,
    ACT_VBUS_5V
} pe_act_t;

typedef struct { pe_act_t type; uint16_t arg; } pe_action_t;

#define PE_ACT_MAX 16u

typedef struct {
    pe_state_t state;
    uint16_t   timer_ms;        /* 剩餘 ms，僅 timer_run != 0 時有效 */
    uint8_t    timer_run;
    uint8_t    phase;           /* TRANSITION_SUPPLY: 0=等 tSrcTransition, 1=等 VBUS */
    uint8_t    caps_counter;
    uint8_t    hard_reset_counter;
    uint8_t    explicit_contract;
    uint16_t   req_mv;
    pe_action_t act[PE_ACT_MAX];
    uint8_t    act_n;
} pe_ctx_t;

void pe_init(pe_ctx_t *c);
void pe_event(pe_ctx_t *c, pe_event_t ev, uint16_t arg);
void pe_clear_actions(pe_ctx_t *c);

#endif
