#include <stdio.h>
#include <string.h>
#include "pd_phy.h"

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

static void t_crc(void)
{
    const uint8_t s[] = "123456789";
    /* 標準 CRC-32 check value（與 zlib.crc32 / Ethernet FCS 相同） */
    CHECK(pd_crc32(s, 9) == 0xCBF43926ul);
    CHECK(pd_crc32((const uint8_t *)"", 0) == 0x00000000ul);
    /* 手動分段更新要等於一次算完 */
    uint32_t c = pd_crc32_update(PD_CRC32_INIT, s, 4);
    c = pd_crc32_update(c, s + 4, 5);
    CHECK((c ^ 0xFFFFFFFFul) == 0xCBF43926ul);
    /* nibble 查表版必須等價 */
    CHECK((pd_crc32_nibble(PD_CRC32_INIT, s, 9) ^ 0xFFFFFFFFul) == 0xCBF43926ul);
    /* PD 的「剩餘值」特性：把 CRC 以 LE 附在資料後再算，未 xorout 的暫存器 = 0xDEBB20E3（規格寫的 0xC704DD7B 是 MSB-first 表示法的同一個剩餘值） */
    uint8_t m[8] = {0x41, 0x2C, 0xA1, 0x01, 0x00, 0x00, 0x00, 0x00};
    uint32_t crc = pd_crc32(m, 4);
    m[4] = (uint8_t)crc; m[5] = (uint8_t)(crc >> 8); m[6] = (uint8_t)(crc >> 16); m[7] = (uint8_t)(crc >> 24);
    CHECK(pd_crc32_update(PD_CRC32_INIT, m, 8) == 0xDEBB20E3ul);
    CHECK(pd_crc32_nibble(PD_CRC32_INIT, m, 8) == 0xDEBB20E3ul);
}

static void t_4b5b(void)
{
    static const uint8_t exp[16] = {0x1E,0x09,0x14,0x15,0x0A,0x0B,0x0E,0x0F,
                                    0x12,0x13,0x16,0x17,0x1A,0x1B,0x1C,0x1D};
    int i;
    for (i = 0; i < 16; i++) {
        CHECK(pd_4b5b_encode((uint8_t)i) == (int8_t)exp[i]);
        CHECK(pd_4b5b_decode(exp[i]) == i);
    }
    CHECK(pd_4b5b_encode(16) == -1);
    CHECK(pd_4b5b_decode(K_SYNC1) == -1);
    CHECK(pd_4b5b_decode(K_EOP) == -1);
    CHECK(pd_4b5b_decode(0x00) == -1);
    CHECK(pd_4b5b_decode(0x1F) == -1);
    /* 全部 32 個 5b 值：恰好 16 個合法 */
    int ok = 0;
    for (i = 0; i < 32; i++) if (pd_4b5b_decode((uint8_t)i) >= 0) ok++;
    CHECK(ok == 16);
}

static void t_frame(void)
{
    /* GoodCRC header: MsgID=0, PortPowerRole=Source(1), Rev 2.0(01), DataRole=DFP(1), Spec... 簡化用 0x0161 */
    const uint8_t msg[2] = {0x61, 0x01};
    uint8_t sym[64], out[16];
    int n = pd_frame_build(msg, 2, sym, sizeof sym);
    CHECK(n == 4 + 2 * 6 + 1);
    if (n <= 0) return; /* 骨架尚未實作時避免後面越界 */
    CHECK(sym[0] == K_SYNC1 && sym[1] == K_SYNC1 && sym[2] == K_SYNC1 && sym[3] == K_SYNC2);
    CHECK(sym[4] == 0x09 && sym[5] == 0x0E);   /* 0x61: 低 nibble 1 -> 01001, 高 nibble 6 -> 01110 */
    CHECK(sym[n - 1] == K_EOP);
    CHECK(pd_frame_parse(sym, (size_t)n, out, sizeof out) == 2);
    CHECK(memcmp(out, msg, 2) == 0);

    /* 緩衝區不足 */
    CHECK(pd_frame_build(msg, 2, sym, 16) == PD_ERR_LEN);

    /* 錯誤注入 */
    uint8_t bad[64];
    memcpy(bad, sym, (size_t)n);
    bad[0] = K_SYNC2;
    CHECK(pd_frame_parse(bad, (size_t)n, out, sizeof out) == PD_ERR_SOP);
    memcpy(bad, sym, (size_t)n);
    bad[6] = 0x00;
    CHECK(pd_frame_parse(bad, (size_t)n, out, sizeof out) == PD_ERR_SYMBOL);
    memcpy(bad, sym, (size_t)n);
    CHECK(pd_frame_parse(bad, (size_t)n - 1, out, sizeof out) == PD_ERR_EOP);
    memcpy(bad, sym, (size_t)n);
    bad[4] = (uint8_t)pd_4b5b_encode(0x2);  /* 翻掉一個資料 nibble，仍是合法符號 */
    CHECK(pd_frame_parse(bad, (size_t)n, out, sizeof out) == PD_ERR_CRC);
    /* 資料太短（不足 4 bytes CRC） */
    uint8_t shortf[8] = {K_SYNC1, K_SYNC1, K_SYNC1, K_SYNC2, 0x1E, 0x1E, K_EOP};
    CHECK(pd_frame_parse(shortf, 7, out, sizeof out) == PD_ERR_LEN);
    /* 奇數個資料 symbol */
    uint8_t odd[8] = {K_SYNC1, K_SYNC1, K_SYNC1, K_SYNC2, 0x1E, K_EOP};
    CHECK(pd_frame_parse(odd, 6, out, sizeof out) == PD_ERR_SYMBOL || pd_frame_parse(odd, 6, out, sizeof out) == PD_ERR_EOP);
    /* out 太小 */
    CHECK(pd_frame_parse(sym, (size_t)n, out, 3) == PD_ERR_LEN);
}

static void t_roundtrip(void)
{
    uint8_t msg[30], sym[128], out[40];
    int len, k, n;
    uint32_t seed = 12345;
    for (len = 0; len <= 28; len++) {
        for (k = 0; k < len; k++) { seed = seed * 1103515245u + 12345u; msg[k] = (uint8_t)(seed >> 16); }
        n = pd_frame_build(msg, (size_t)len, sym, sizeof sym);
        CHECK(n == 4 + 2 * (len + 4) + 1);
        if (n <= 0) return;
        CHECK(pd_frame_parse(sym, (size_t)n, out, sizeof out) == len);
        CHECK(len == 0 || memcmp(out, msg, (size_t)len) == 0);
    }
}

int main(void)
{
    t_crc(); t_4b5b(); t_frame(); t_roundtrip();
    printf("%d checks, %d failed\n", g_run, g_fail);
    return g_fail ? 1 : 0;
}
