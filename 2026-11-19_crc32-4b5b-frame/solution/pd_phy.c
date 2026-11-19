#include "pd_phy.h"

static const uint8_t k_enc[16] = {
    0x1E, 0x09, 0x14, 0x15, 0x0A, 0x0B, 0x0E, 0x0F,
    0x12, 0x13, 0x16, 0x17, 0x1A, 0x1B, 0x1C, 0x1D
};

static const uint32_t k_crc_nib[16] = {
    0x00000000ul, 0x1DB71064ul, 0x3B6E20C8ul, 0x26D930ACul,
    0x76DC4190ul, 0x6B6B51F4ul, 0x4DB26158ul, 0x5005713Cul,
    0xEDB88320ul, 0xF00F9344ul, 0xD6D6A3E8ul, 0xCB61B38Cul,
    0x9B64C2B0ul, 0x86D3D2D4ul, 0xA00AE278ul, 0xBDBDF21Cul
};

uint32_t pd_crc32_update(uint32_t crc, const uint8_t *data, size_t len)
{
    uint8_t i;
    while (len--) {
        crc ^= *data++;
        for (i = 0; i < 8; i++)
            crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320ul : (crc >> 1);
    }
    return crc;
}

uint32_t pd_crc32_nibble(uint32_t crc, const uint8_t *data, size_t len)
{
    while (len--) {
        crc ^= *data++;
        crc = (crc >> 4) ^ k_crc_nib[crc & 0x0F];
        crc = (crc >> 4) ^ k_crc_nib[crc & 0x0F];
    }
    return crc;
}

uint32_t pd_crc32(const uint8_t *data, size_t len)
{
    return pd_crc32_update(PD_CRC32_INIT, data, len) ^ 0xFFFFFFFFul;
}

int8_t pd_4b5b_encode(uint8_t nibble)
{
    return (nibble > 15) ? -1 : (int8_t)k_enc[nibble];
}

int8_t pd_4b5b_decode(uint8_t symbol)
{
    uint8_t i;
    for (i = 0; i < 16; i++)
        if (k_enc[i] == symbol) return (int8_t)i;
    return -1;
}

static size_t put_byte(uint8_t *sym, size_t pos, uint8_t b)
{
    sym[pos++] = (uint8_t)pd_4b5b_encode(b & 0x0F);
    sym[pos++] = (uint8_t)pd_4b5b_encode(b >> 4);
    return pos;
}

int pd_frame_build(const uint8_t *msg, size_t len, uint8_t *sym, size_t sym_max)
{
    size_t need = 4 + 2 * (len + 4) + 1, pos = 0, i;
    uint32_t crc;
    if (need > sym_max) return PD_ERR_LEN;
    sym[pos++] = K_SYNC1; sym[pos++] = K_SYNC1;
    sym[pos++] = K_SYNC1; sym[pos++] = K_SYNC2;
    for (i = 0; i < len; i++) pos = put_byte(sym, pos, msg[i]);
    crc = pd_crc32(msg, len);
    for (i = 0; i < 4; i++) pos = put_byte(sym, pos, (uint8_t)(crc >> (8 * i)));
    sym[pos++] = K_EOP;
    return (int)pos;
}

int pd_frame_parse(const uint8_t *sym, size_t nsym, uint8_t *out, size_t out_max)
{
    size_t pos = 4, n = 0;
    uint32_t crc_rx;
    int8_t lo, hi;
    if (nsym < 4 || sym[0] != K_SYNC1 || sym[1] != K_SYNC1 ||
        sym[2] != K_SYNC1 || sym[3] != K_SYNC2) return PD_ERR_SOP;
    for (;;) {
        if (pos >= nsym) return PD_ERR_EOP;
        if (sym[pos] == K_EOP) break;
        if (pos + 1 >= nsym) return PD_ERR_EOP;
        lo = pd_4b5b_decode(sym[pos]);
        hi = pd_4b5b_decode(sym[pos + 1]);
        if (lo < 0 || hi < 0) return PD_ERR_SYMBOL;
        if (n >= out_max) return PD_ERR_LEN;
        out[n++] = (uint8_t)(lo | (hi << 4));
        pos += 2;
    }
    if (n < 4) return PD_ERR_LEN;
    n -= 4;
    crc_rx = (uint32_t)out[n] | ((uint32_t)out[n + 1] << 8) |
             ((uint32_t)out[n + 2] << 16) | ((uint32_t)out[n + 3] << 24);
    if (crc_rx != pd_crc32(out, n)) return PD_ERR_CRC;
    return (int)n;
}
