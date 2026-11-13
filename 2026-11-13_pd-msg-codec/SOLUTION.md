# 參考解答（請先自己做完再看！）

```c
#include "pd_codec.h"
uint16_t pd_header_pack(const pd_header_t *h){
    return (uint16_t)(((h->extended&1u)<<15)|((h->num_do&7u)<<12)|((h->msg_id&7u)<<9)|
           ((h->power_role&1u)<<8)|((h->spec_rev&3u)<<6)|((h->data_role&1u)<<5)|(h->msg_type&0x1Fu));
}
void pd_header_unpack(uint16_t r, pd_header_t *h){
    h->extended=(r>>15)&1; h->num_do=(r>>12)&7; h->msg_id=(r>>9)&7; h->power_role=(r>>8)&1;
    h->spec_rev=(r>>6)&3; h->data_role=(r>>5)&1; h->msg_type=r&0x1F;
}
uint8_t pdo_type(uint32_t p){ return (uint8_t)(p>>30); }
uint32_t pdo_fixed_pack(const pd_fixed_pdo_t *p){
    return ((uint32_t)(p->dual_role_power&1)<<29)|((uint32_t)(p->usb_suspend&1)<<28)|
      ((uint32_t)(p->unconstrained&1)<<27)|((uint32_t)(p->usb_comm&1)<<26)|
      ((uint32_t)(p->dual_role_data&1)<<25)|((uint32_t)(p->unchunked&1)<<24)|
      ((uint32_t)(p->epr_capable&1)<<23)|((uint32_t)(p->peak_current&3)<<20)|
      ((uint32_t)((p->voltage_mv/50)&0x3FF)<<10)|((uint32_t)(p->current_ma/10)&0x3FF);
}
void pdo_fixed_unpack(uint32_t r, pd_fixed_pdo_t *p){
    p->dual_role_power=(r>>29)&1; p->usb_suspend=(r>>28)&1; p->unconstrained=(r>>27)&1;
    p->usb_comm=(r>>26)&1; p->dual_role_data=(r>>25)&1; p->unchunked=(r>>24)&1;
    p->epr_capable=(r>>23)&1; p->peak_current=(r>>20)&3;
    p->voltage_mv=(uint16_t)(((r>>10)&0x3FF)*50u); p->current_ma=(uint16_t)((r&0x3FF)*10u);
}
uint32_t pdo_pps_pack(const pd_pps_apdo_t *p){
    return (3ul<<30)|((uint32_t)(p->power_limited&1)<<27)|
      ((uint32_t)((p->max_mv/100)&0xFF)<<17)|((uint32_t)((p->min_mv/100)&0xFF)<<8)|
      ((uint32_t)(p->max_ma/50)&0x7F);
}
void pdo_pps_unpack(uint32_t r, pd_pps_apdo_t *p){
    p->power_limited=(r>>27)&1; p->max_mv=(uint16_t)(((r>>17)&0xFF)*100u);
    p->min_mv=(uint16_t)(((r>>8)&0xFF)*100u); p->max_ma=(uint16_t)((r&0x7F)*50u);
}
uint32_t rdo_fixed_pack(const pd_rdo_fixed_t *r){
    return ((uint32_t)(r->obj_pos&7)<<28)|((uint32_t)(r->mismatch&1)<<26)|((uint32_t)(r->usb_comm&1)<<25)|
      ((uint32_t)(r->no_suspend&1)<<24)|((uint32_t)(r->unchunked&1)<<23)|((uint32_t)(r->epr_capable&1)<<22)|
      ((uint32_t)((r->op_ma/10)&0x3FF)<<10)|((uint32_t)((r->max_ma/10)&0x3FF));
}
void rdo_fixed_unpack(uint32_t w, pd_rdo_fixed_t *r){
    r->obj_pos=(w>>28)&7; r->mismatch=(w>>26)&1; r->usb_comm=(w>>25)&1; r->no_suspend=(w>>24)&1;
    r->unchunked=(w>>23)&1; r->epr_capable=(w>>22)&1;
    r->op_ma=(uint16_t)(((w>>10)&0x3FF)*10u); r->max_ma=(uint16_t)((w&0x3FF)*10u);
}
uint32_t rdo_pps_pack(const pd_rdo_pps_t *r){
    return ((uint32_t)(r->obj_pos&7)<<28)|((uint32_t)(r->mismatch&1)<<26)|((uint32_t)(r->usb_comm&1)<<25)|
      ((uint32_t)(r->no_suspend&1)<<24)|((uint32_t)(r->unchunked&1)<<23)|((uint32_t)(r->epr_capable&1)<<22)|
      ((uint32_t)((r->out_mv/20)&0xFFF)<<9)|((uint32_t)((r->op_ma/50)&0x7F));
}
void rdo_pps_unpack(uint32_t w, pd_rdo_pps_t *r){
    r->obj_pos=(w>>28)&7; r->mismatch=(w>>26)&1; r->usb_comm=(w>>25)&1; r->no_suspend=(w>>24)&1;
    r->unchunked=(w>>23)&1; r->epr_capable=(w>>22)&1;
    r->out_mv=(uint16_t)(((w>>9)&0xFFF)*20u); r->op_ma=(uint16_t)((w&0x7F)*50u);
}
req_result_t pd_validate_request(const uint32_t *caps, uint8_t n, uint32_t rdo){
    uint8_t pos=(rdo>>28)&7;
    if(pos==0||pos>n) return REQ_REJECT_POS;
    uint32_t pdo=caps[pos-1];
    if(pdo_type(pdo)==PDO_TYPE_FIXED){
        pd_rdo_fixed_t r; pd_fixed_pdo_t p; rdo_fixed_unpack(rdo,&r); pdo_fixed_unpack(pdo,&p);
        if(r.op_ma>p.current_ma||r.max_ma>p.current_ma) return REQ_REJECT_CURRENT;
        return REQ_ACCEPT;
    } else if(pdo_type(pdo)==PDO_TYPE_APDO){
        pd_rdo_pps_t r; pd_pps_apdo_t p; rdo_pps_unpack(rdo,&r); pdo_pps_unpack(pdo,&p);
        if(r.out_mv<p.min_mv||r.out_mv>p.max_mv) return REQ_REJECT_VOLTAGE;
        if(r.op_ma>p.max_ma) return REQ_REJECT_CURRENT;
        return REQ_ACCEPT;
    }
    return REQ_REJECT_POS;
}
```

## 解說
- **先 mask 再 shift**：每個欄位 pack 前先用寬度遮罩，避免超出欄位污染相鄰 bit（如 voltage 超過 10 bits）。
- **PPS APDO 的 bit31:30 必須是 11**，所以用 `3ul<<30`；`pdo_type()` 才能辨識。
- **uint32_t 常數用 `ul`**：在 8051（int=16-bit）上 `3<<30` 會溢位，這是常見 bug。
- **驗證**：Fixed 要同時檢查 Operating 與 Max Operating 電流；PPS 檢查 20mV 精度換算後的電壓區間與 50mA 電流。
- 思考題 1：Mismatch=1 時 Source 仍可 Accept（Sink 表示給不夠需求但仍願意接受），並依 Sink 後續行為處理；Source 不應因此拒絕。
- 思考題 2：建議捨去（不超過 Source 能力）並在驗證端用換算後的值比較；四捨五入可能讓輸出超過限制。
- 思考題 3：用 union { uint32_t l; uint8_t b[4]; }，C51 為 big-endian，欄位按 byte 取用可省去 32-bit shift。
