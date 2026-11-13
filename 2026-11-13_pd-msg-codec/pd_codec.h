#ifndef PD_CODEC_H
#define PD_CODEC_H
#include <stdint.h>

/* ---- Message Header (USB PD R3.x, Table 6.1) ---- */
typedef struct {
    uint8_t extended;      /* bit15 */
    uint8_t num_do;        /* bit14:12 Number of Data Objects */
    uint8_t msg_id;        /* bit11:9  MessageID (0..7) */
    uint8_t power_role;    /* bit8     1 = Source */
    uint8_t spec_rev;      /* bit7:6   0=1.0 1=2.0 2=3.x */
    uint8_t data_role;     /* bit5     1 = DFP */
    uint8_t msg_type;      /* bit4:0 */
} pd_header_t;

uint16_t pd_header_pack(const pd_header_t *h);
void     pd_header_unpack(uint16_t raw, pd_header_t *h);

/* ---- Source PDO ---- */
typedef struct {
    uint8_t  dual_role_power, usb_suspend, unconstrained, usb_comm;
    uint8_t  dual_role_data, unchunked, epr_capable, peak_current;
    uint16_t voltage_mv;   /* 50 mV/LSB, 10 bits */
    uint16_t current_ma;   /* 10 mA/LSB, 10 bits */
} pd_fixed_pdo_t;

typedef struct {
    uint8_t  power_limited;
    uint16_t max_mv;       /* 100 mV/LSB, 8 bits */
    uint16_t min_mv;       /* 100 mV/LSB, 8 bits */
    uint16_t max_ma;       /* 50 mA/LSB, 7 bits  */
} pd_pps_apdo_t;

#define PDO_TYPE_FIXED 0
#define PDO_TYPE_BATT  1
#define PDO_TYPE_VAR   2
#define PDO_TYPE_APDO  3

uint8_t  pdo_type(uint32_t pdo);                 /* bit31:30 */
uint32_t pdo_fixed_pack(const pd_fixed_pdo_t *p);
void     pdo_fixed_unpack(uint32_t raw, pd_fixed_pdo_t *p);
uint32_t pdo_pps_pack(const pd_pps_apdo_t *p);
void     pdo_pps_unpack(uint32_t raw, pd_pps_apdo_t *p);

/* ---- Request Data Object ---- */
typedef struct {
    uint8_t  obj_pos;      /* 1-based index into Source_Capabilities */
    uint8_t  mismatch, usb_comm, no_suspend, unchunked, epr_capable;
    uint16_t op_ma;        /* Fixed: 10 mA/LSB */
    uint16_t max_ma;       /* Fixed: 10 mA/LSB */
} pd_rdo_fixed_t;

typedef struct {
    uint8_t  obj_pos;
    uint8_t  mismatch, usb_comm, no_suspend, unchunked, epr_capable;
    uint16_t out_mv;       /* 20 mV/LSB, 12 bits */
    uint16_t op_ma;        /* 50 mA/LSB, 7 bits  */
} pd_rdo_pps_t;

uint32_t rdo_fixed_pack(const pd_rdo_fixed_t *r);
void     rdo_fixed_unpack(uint32_t raw, pd_rdo_fixed_t *r);
uint32_t rdo_pps_pack(const pd_rdo_pps_t *r);
void     rdo_pps_unpack(uint32_t raw, pd_rdo_pps_t *r);

/* ---- Request validation (Source side, PE_SRC_Negotiate_Capability) ---- */
typedef enum { REQ_ACCEPT = 0, REQ_REJECT_POS, REQ_REJECT_CURRENT,
               REQ_REJECT_VOLTAGE } req_result_t;

/* src_caps: array of num_pdo raw PDOs; rdo: raw RDO from Sink. */
req_result_t pd_validate_request(const uint32_t *src_caps, uint8_t num_pdo,
                                 uint32_t rdo);
#endif
