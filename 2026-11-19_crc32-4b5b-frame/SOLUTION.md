# 參考解答與解說

> ⚠️ 請先自己做完再看！參考實作在 `solution/pd_phy.c`，驗證：`make check-solution`。

## 1. CRC32
```c
crc ^= byte;
for (8 次) crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : (crc >> 1);
```
- 輸入/輸出反射 → 用 reflected poly，不需要另外做 bit reverse。
- `pd_crc32_update` 不做 xorout，方便分段更新；`pd_crc32` 才包 init 與 xorout。
- nibble 版：`crc = (crc >> 4) ^ tab[crc & 0xF]` 連做兩次。表內容為 `tab[i]` = 把 `i` 當 4 bit 餵進 bitwise 演算法 4 次的結果。
- 常見錯誤：忘了 xorout；把 CRC 以 big-endian 附上；init 用 0。
- 殘餘值：資料+LE CRC → 暫存器 `0xDEBB20E3`（= finalized `0x2144DF1C`）。

## 2. 4b5b
資料 0~F 對應 `1E 09 14 15 0A 0B 0E 0F 12 13 16 17 1A 1B 1C 1D`。K-code（Sync-1 `0x18`、Sync-2 `0x11`、RST-1 `0x07`、RST-2 `0x19`、EOP `0x0D`）不在表內，因此解碼回 -1。32 個 5b 值中只有 16 個是資料。反查可用 16 項線性掃描（省 CODE），或 32 項表（快、多 32 byte）。

## 3. Frame
```
sym[0..3] = Sync-1 Sync-1 Sync-1 Sync-2
per byte  : enc(b & 0xF), enc(b >> 4)       // 低 nibble 先
CRC       : crc32(msg) 以 LE 4 bytes 同樣編碼
last      : EOP
symbol 數 = 4 + 2*(len+4) + 1
```
範例：header `0x0161` → bytes `61 01` → symbols `09 0E 09 1E`…。

## 4. Parse 要點
- 先驗 SOP，再逐對解碼直到 EOP；`pos+1 >= nsym` 表示資料 symbol 落單。
- 寫入 `out` 前檢查 `n < out_max`，`out_max` 需含 CRC 4 bytes。
- `n < 4` 為長度錯；然後取最後 4 bytes 組 CRC（LE）與 `pd_crc32(out, n-4)` 比對。
- 實務上 ISR 會邊收邊更新 CRC，結尾檢查 residue 即可，省下一份 RAM 拷貝。

## 延伸思考題提示
1. bitwise ≈ 8×(約 4~6 指令×32-bit 移位)/byte，nibble 版 2×(MOVC + 4 byte XOR/移位)；差一個數量級左右，但 8051 32-bit 移位都很貴，查表通常勝出。
2. 邊收邊算：只需 4 byte CRC 暫存 + nibble 狀態，RAM 恆定；用 residue 比對不必保存收到的 CRC。
3. 規格要求 SOP* 的 4 個 K-code 中至少 3 個正確即可辨識（§5.6.1.2 的 Ordered Set 偵測）；實作時對 4 個位置各自比對並計數 ≥ 3，同時須能對應到唯一的 ordered set。
