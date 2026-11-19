#ifndef PD_PHY_H
#define PD_PHY_H
#include <stdint.h>
#include <stddef.h>

/* 5-bit K-code / 控制符號 (USB PD 3.x spec Table 5-4) */
#define K_SYNC1 0x18u /* 11000 */
#define K_SYNC2 0x11u /* 10001 */
#define K_RST1  0x07u /* 00111 */
#define K_RST2  0x19u /* 11001 */
#define K_EOP   0x0Du /* 01101 */

/* 錯誤碼 */
#define PD_ERR_SOP     (-1) /* 開頭不是 Sync-1 x3 + Sync-2 */
#define PD_ERR_SYMBOL  (-2) /* 資料區出現非法 5b 符號 */
#define PD_ERR_EOP     (-3) /* 沒有 EOP */
#define PD_ERR_CRC     (-4) /* CRC32 不符 */
#define PD_ERR_LEN     (-5) /* 長度不合理 / 緩衝區不足 */

#define PD_CRC32_INIT 0xFFFFFFFFul

/* --- CRC32 (poly 0x04C11DB7, reflected 0xEDB88320, init FFFFFFFF, xorout FFFFFFFF) --- */
uint32_t pd_crc32_update(uint32_t crc, const uint8_t *data, size_t len); /* bitwise, 不做 xorout */
uint32_t pd_crc32_nibble(uint32_t crc, const uint8_t *data, size_t len); /* 16 項查表，不做 xorout */
uint32_t pd_crc32(const uint8_t *data, size_t len);                       /* 已完成：init + update + xorout */

/* --- 4b5b --- */
int8_t pd_4b5b_encode(uint8_t nibble);  /* 0..15 -> 5-bit symbol；超出回 -1 */
int8_t pd_4b5b_decode(uint8_t symbol);  /* 5-bit symbol -> 0..15；K-code 或非法回 -1 */

/* --- Frame --- */
/* 建 SOP frame：Sync-1 x3, Sync-2, 資料(每 byte 低 nibble 先)、CRC32(LE)、EOP。
 * 回傳 symbol 數；sym_max 不足回 PD_ERR_LEN。*/
int pd_frame_build(const uint8_t *msg, size_t len, uint8_t *sym, size_t sym_max);
/* 解 frame，成功回傳 payload 長度（不含 CRC）；失敗回 PD_ERR_*。
 * out_max 是 out 的總容量，必須容得下 payload + 4 bytes CRC（解碼時 CRC 也會先寫進 out）。*/
int pd_frame_parse(const uint8_t *sym, size_t nsym, uint8_t *out, size_t out_max);

#endif
