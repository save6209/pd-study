#include "pd_phy.h"

/* 4b5b 表 (PD spec Table 5-3)，index = nibble */
static const uint8_t k_enc[16] = {
    0x1E, 0x09, 0x14, 0x15, 0x0A, 0x0B, 0x0E, 0x0F,
    0x12, 0x13, 0x16, 0x17, 0x1A, 0x1B, 0x1C, 0x1D
};

/* 16 項 nibble CRC 表：只需 64 bytes CODE，比 256 項 (1KB) 省很多 */
static const uint32_t k_crc_nib[16] = {
    0x00000000ul, 0x1DB71064ul, 0x3B6E20C8ul, 0x26D930ACul,
    0x76DC4190ul, 0x6B6B51F4ul, 0x4DB26158ul, 0x5005713Cul,
    0xEDB88320ul, 0xF00F9344ul, 0xD6D6A3E8ul, 0xCB61B38Cul,
    0x9B64C2B0ul, 0x86D3D2D4ul, 0xA00AE278ul, 0xBDBDF21Cul
};

uint32_t pd_crc32_update(uint32_t crc, const uint8_t *data, size_t len)
{
    /* TODO 1: 逐 byte、逐 bit 計算 reflected CRC32。
     *   crc ^= byte; 重複 8 次: if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320; else crc >>= 1;
     * 想想：為什麼 PD 的 byte 是 LSB first，所以用 reflected 版本最自然？ */
    (void)data; (void)len;
    return crc;
}

uint32_t pd_crc32_nibble(uint32_t crc, const uint8_t *data, size_t len)
{
    /* TODO 2: 用 k_crc_nib[] 每次處理 4 bit：
     *   crc ^= byte; crc = (crc >> 4) ^ k_crc_nib[crc & 0x0F]; 再做一次。
     * 結果必須與 pd_crc32_update 完全相同。 */
    (void)data; (void)len; (void)k_crc_nib;
    return crc;
}

uint32_t pd_crc32(const uint8_t *data, size_t len)
{
    return pd_crc32_update(PD_CRC32_INIT, data, len) ^ 0xFFFFFFFFul;
}

int8_t pd_4b5b_encode(uint8_t nibble)
{
    /* TODO 3: 查 k_enc[]；nibble > 15 回 -1 */
    (void)nibble; (void)k_enc;
    return -1;
}

int8_t pd_4b5b_decode(uint8_t symbol)
{
    /* TODO 4: 反查。8051 上不要用 32 項表浪費 RAM/CODE 以外的東西也行，
     * 先求正確：掃 k_enc[]（16 項）找到回 index，找不到（含 K-code）回 -1。 */
    (void)symbol;
    return -1;
}

int pd_frame_build(const uint8_t *msg, size_t len, uint8_t *sym, size_t sym_max)
{
    /* TODO 5:
     *  1) 計算需要的 symbol 數 = 4(SOP) + 2*(len+4) + 1(EOP)，不夠回 PD_ERR_LEN
     *  2) 寫 SOP: K_SYNC1 x3, K_SYNC2
     *  3) 逐 byte 先送低 nibble 再送高 nibble（4b5b 編碼）
     *  4) CRC32 = pd_crc32(msg,len)，以 little-endian 4 bytes 同樣方式編碼
     *  5) 最後 K_EOP */
    (void)msg; (void)len; (void)sym; (void)sym_max;
    return PD_ERR_LEN;
}

int pd_frame_parse(const uint8_t *sym, size_t nsym, uint8_t *out, size_t out_max)
{
    /* TODO 6:
     *  1) 檢查 SOP（4 個 symbol），否則 PD_ERR_SOP
     *  2) 從第 4 個 symbol 起，兩兩一組解成 byte（低 nibble 先，全部先寫進 out，含 CRC），直到遇到 K_EOP
     *     遇到非法 symbol -> PD_ERR_SYMBOL；到結尾沒 EOP -> PD_ERR_EOP
     *  3) 解出的 byte 數 n 必須 >= 4（含 CRC）；寫 out 前先檢查 n < out_max，否則 PD_ERR_LEN
     *  4) 取最後 4 bytes 當 CRC(LE)，與 pd_crc32(payload) 比較，不符 PD_ERR_CRC
     *  5) 回傳 payload 長度 n-4
     * 提示：只有奇數個 symbol 的資料也要視為錯誤 (PD_ERR_SYMBOL)。 */
    (void)sym; (void)nsym; (void)out; (void)out_max;
    return PD_ERR_SOP;
}
