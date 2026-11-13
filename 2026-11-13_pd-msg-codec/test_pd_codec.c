#include <stdio.h>
#include <string.h>
#include "pd_codec.h"
static int fails=0, total=0;
#define CHECK(c) do{ total++; if(!(c)){ fails++; printf("FAIL line %d: %s\n",__LINE__,#c);} }while(0)

int main(void){
    /* Header: Source_Cap, 2 PDO, MsgID=3, Source/DFP, Rev3.0 -> 0x2 | ... */
    pd_header_t h={0,2,3,1,2,1,1}, g;
    uint16_t raw=pd_header_pack(&h);
    CHECK(raw==0x27A1);  /* 0010 0111 1010 0001 */
    pd_header_unpack(raw,&g);
    CHECK(g.num_do==2&&g.msg_id==3&&g.power_role==1&&g.spec_rev==2&&g.data_role==1&&g.msg_type==1&&g.extended==0);

    /* 5V/3A fixed, unconstrained + USB comm = 0x0C01912C */
    pd_fixed_pdo_t f={0,0,1,1,0,0,0,0,5000,3000}, fg;
    uint32_t p=pdo_fixed_pack(&f);
    CHECK(p==0x0C01912Cul);
    CHECK(pdo_type(p)==PDO_TYPE_FIXED);
    pdo_fixed_unpack(p,&fg);
    CHECK(fg.voltage_mv==5000&&fg.current_ma==3000&&fg.unconstrained==1&&fg.usb_comm==1);
    f.voltage_mv=20000; f.current_ma=5000; f.peak_current=0;
    CHECK((pdo_fixed_pack(&f)&0x3FF)==500 && ((pdo_fixed_pack(&f)>>10)&0x3FF)==400);

    /* PPS 3.3-11V 5A: max=110, min=33, cur=100 */
    pd_pps_apdo_t a={0,11000,3300,5000}, ag;
    uint32_t ap=pdo_pps_pack(&a);
    CHECK(ap==0xC0DC2164ul);
    CHECK(pdo_type(ap)==PDO_TYPE_APDO);
    pdo_pps_unpack(ap,&ag);
    CHECK(ag.max_mv==11000&&ag.min_mv==3300&&ag.max_ma==5000);

    /* RDO fixed: pos 2, 2A op / 3A max */
    pd_rdo_fixed_t rf={2,0,1,0,0,0,2000,3000}, rfg;
    uint32_t rd=rdo_fixed_pack(&rf);
    CHECK(((rd>>28)&7)==2 && ((rd>>10)&0x3FF)==200 && (rd&0x3FF)==300 && ((rd>>25)&1)==1);
    rdo_fixed_unpack(rd,&rfg);
    CHECK(rfg.obj_pos==2&&rfg.op_ma==2000&&rfg.max_ma==3000&&rfg.usb_comm==1);

    /* RDO PPS: pos 3, 9.0 V, 2 A */
    pd_rdo_pps_t rp={3,0,1,0,0,0,9000,2000}, rpg;
    uint32_t rpw=rdo_pps_pack(&rp);
    CHECK(((rpw>>9)&0xFFF)==450 && (rpw&0x7F)==40 && ((rpw>>28)&7)==3);
    rdo_pps_unpack(rpw,&rpg);
    CHECK(rpg.out_mv==9000&&rpg.op_ma==2000&&rpg.obj_pos==3);

    /* Validation */
    pd_fixed_pdo_t f5={0,0,1,1,0,0,0,0,5000,3000}, f9={0,0,0,0,0,0,0,0,9000,2000};
    uint32_t caps[3]={pdo_fixed_pack(&f5),pdo_fixed_pack(&f9),pdo_pps_pack(&a)};
    pd_rdo_fixed_t q={1,0,0,0,0,0,3000,3000};
    CHECK(pd_validate_request(caps,3,rdo_fixed_pack(&q))==REQ_ACCEPT);
    q.obj_pos=0; CHECK(pd_validate_request(caps,3,rdo_fixed_pack(&q))==REQ_REJECT_POS);
    q.obj_pos=4; CHECK(pd_validate_request(caps,3,rdo_fixed_pack(&q))==REQ_REJECT_POS);
    q.obj_pos=2; q.op_ma=2500; q.max_ma=2500;
    CHECK(pd_validate_request(caps,3,rdo_fixed_pack(&q))==REQ_REJECT_CURRENT);
    q.op_ma=2000; q.max_ma=2500; /* max_ma 超過也應拒絕 */
    CHECK(pd_validate_request(caps,3,rdo_fixed_pack(&q))==REQ_REJECT_CURRENT);
    pd_rdo_pps_t pq={3,0,0,0,0,0,9000,2000};
    CHECK(pd_validate_request(caps,3,rdo_pps_pack(&pq))==REQ_ACCEPT);
    pq.out_mv=3000; CHECK(pd_validate_request(caps,3,rdo_pps_pack(&pq))==REQ_REJECT_VOLTAGE);
    pq.out_mv=11020; CHECK(pd_validate_request(caps,3,rdo_pps_pack(&pq))==REQ_REJECT_VOLTAGE);
    pq.out_mv=9000; pq.op_ma=5050; CHECK(pd_validate_request(caps,3,rdo_pps_pack(&pq))==REQ_REJECT_CURRENT);

    printf("%d/%d passed\n", total-fails, total);
    return fails?1:0;
}
