# 2026-11-19　PD CRC32 + 4b5b + Frame 組裝/解析

**主題分類：Physical layer 編碼 / CRC**　**預計時間：30 分鐘**

## 學習目標
1. 以 C 實作 PD 使用的 CRC-32（與 Ethernet FCS 相同），並做出 **16 項 nibble 查表版**，理解 8051 上 code size vs. 速度的取捨。
2. 實作 4b5b 編/解碼，分辨「資料符號」與「K-code（Sync-1/2、RST-1/2、EOP）」。
3. 組出/解析完整 SOP frame：`Preamble | Sync-1 ×3 + Sync-2 | 資料 | CRC32 | EOP`，並做錯誤偵測。

## 背景知識
- **USB PD 3.x Spec §5.6.2 Table 5-3 / 5-4（4b5b 與 K-code）、§5.6.2.2（Ordered Set，SOP = Sync-1,Sync-1,Sync-1,Sync-2）、§5.6.3（CRC）**
- CRC-32：poly `0x04C11DB7`（reflected 為 `0xEDB88320`）、init `0xFFFFFFFF`、輸入/輸出反射、xorout `0xFFFFFFFF`。標準檢查值 `"123456789" → 0xCBF43926`。
- PD 每個 byte **LSB first**、多 byte 欄位 **little-endian**；所以 reflected 演算法最自然，且 CRC 以 LE 順序附在資料後。
- 剩餘值（residue）特性：資料+CRC(LE) 一起餵進去，未 xorout 的暫存器恆為 `0xDEBB20E3`（規格以 MSB-first 寫成 `0xC704DD7B`）。硬體/韌體常用這招直接驗證，不必另外比對。
- 傳輸順序：每個 byte 先送**低 nibble**，再送高 nibble，每個 nibble 轉成 5 bit symbol。
- Preamble（64 bit 的 0101…）由 BMC 硬體送出，不在本題 symbol 範圍。
- 8051：256 項 CRC 表 = 1 KB CODE（4 byte × 256）；16 項 = 64 bytes，每 byte 多一次迭代，通常是好交易。bitwise 版最省 CODE 但最慢（8 次迴圈/byte）。

## 時間分配
| 時間 | 內容 |
|------|------|
| 5 分 | 閱讀本文與 `pd_phy.h` |
| 20 分 | 完成 `pd_phy.c` 的 TODO 1~6 |
| 5 分 | `make test` 驗證、跑一遍錯誤注入案例 |

## 練習步驟
1. `make test`：先看全紅（骨架可編譯，但測試失敗）。
2. TODO 1：`pd_crc32_update`（bitwise）。通過 `"123456789"` 檢查值。
3. TODO 2：`pd_crc32_nibble`，與 bitwise 結果必須一致。
4. TODO 3/4：4b5b 編/解碼。注意 K-code 不是合法資料符號，解碼要回 -1。
5. TODO 5：`pd_frame_build`。先算好所需 symbol 數再寫，避免緩衝區溢位。
6. TODO 6：`pd_frame_parse`。錯誤優先序：SOP → 非法符號 → EOP 缺失 → 長度 → CRC。
7. `make test` 全綠後，再看 `SOLUTION.md`。

## 延伸思考題
1. 若 CRC 表放在 8051 的 CODE，nibble 版每次查表要用 `MOVC`；試估算與 bitwise 版處理 30 byte 訊息（最大 PD message ≈ 7 個 4-byte 的 data object + header）的機器週期差異。CRC 變數用 `uint32_t` 在 C51 是否 `idata`/`data`？
2. `pd_frame_parse` 要把整個 frame 先放進 RAM；在只有 256 bytes IDATA 的 8051 上，ISR 若逐 symbol 即時解碼並累算 CRC（邊收邊算），RAM 需求會如何改變？用 residue 驗證時還需要暫存 CRC 嗎？
3. 若收到 Hard Reset ordered set（Sync-1, Sync-1, Sync-1, RST-2 等）或 Cable Reset，SOP 檢查要怎麼擴充？PD 規格允許 4 個 K-code 中缺 1 個仍判定成功，這個容錯如何實作？
