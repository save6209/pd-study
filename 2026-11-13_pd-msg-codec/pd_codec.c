#include "pd_codec.h"
/* TODO: 依 README 的欄位表實作。每個函式目前是 stub，make test 會失敗。
 * 提示：先 mask 再 shift；電壓/電流要做單位換算(50mV/10mA/100mV/50mA/20mV)。 */

uint16_t pd_header_pack(const pd_header_t *h){ 
    return ( ((uint16_t)(h->extended & 1U)) << 15 |  ((uint16_t)(h->num_do&0x07U)) << 12 |  ((uint16_t)(h->msg_id&0x07U)) << 9 |  ((uint16_t)h->power_role) << 8
        |  ((uint16_t)(h->spec_rev&0x03U)) << 6 |  ((uint16_t)h->data_role) << 5 |  ((uint16_t)(h->msg_type&0x1F)));
}
void pd_header_unpack(uint16_t raw, pd_header_t *h){ 
    h->extended = (raw >> 15) & 0x01U;
    h->num_do = (raw >> 12) & 0x07U;
    h->msg_id = (raw >> 9) & 0x07U;
    h->power_role = (raw >> 8) & 0x01U;
    h->spec_rev = (raw >> 6) & 0x03U;
    h->data_role = (raw >> 5) & 0x01U;
    h->msg_type = raw & 0x1FU;
}
uint8_t pdo_type(uint32_t pdo){ 
    
}
uint32_t pdo_fixed_pack(const pd_fixed_pdo_t *p){ (void)p; /* TODO */ return 0; }
void pdo_fixed_unpack(uint32_t raw, pd_fixed_pdo_t *p){ (void)raw; (void)p; /* TODO */ }
uint32_t pdo_pps_pack(const pd_pps_apdo_t *p){ (void)p; /* TODO: 注意 bit31:30 = 11 */ return 0; }
void pdo_pps_unpack(uint32_t raw, pd_pps_apdo_t *p){ (void)raw; (void)p; /* TODO */ }
uint32_t rdo_fixed_pack(const pd_rdo_fixed_t *r){ (void)r; /* TODO */ return 0; }
void rdo_fixed_unpack(uint32_t raw, pd_rdo_fixed_t *r){ (void)raw; (void)r; /* TODO */ }
uint32_t rdo_pps_pack(const pd_rdo_pps_t *r){ (void)r; /* TODO: Voltage 在 bit20:9 (20mV) */ return 0; }
void rdo_pps_unpack(uint32_t raw, pd_rdo_pps_t *r){ (void)raw; (void)r; /* TODO */ }
req_result_t pd_validate_request(const uint32_t *src_caps, uint8_t num_pdo, uint32_t rdo){
    (void)src_caps; (void)num_pdo; (void)rdo;
    /* TODO: pos 檢查 -> 依 PDO 型別比較電流(Fixed)或電壓範圍+電流(PPS) */
    return REQ_REJECT_POS;
}
