# PD Message Header / PDO / RDO 編解碼器（30 分鐘）

## 學習目標
- 熟悉 USB PD R3.x 的 Message Header、Source Capabilities（Fixed / PPS APDO）、Request Data Object 位元欄位
- 練習 bit mask / shift，並做單位換算（50mV、10mA、100mV、50mA、20mV）
- 實作 Source 端 `PE_SRC_Negotiate_Capability` 會用到的 Request 驗證

## 背景知識（USB PD R3.x，第 6 章 Protocol Layer）
**Message Header（6.2.1.1）**：`15 Ext | 14:12 Num DO | 11:9 MsgID | 8 Port Power Role(Source=1) | 7:6 Spec Rev(10=3.x) | 5 Port Data Role(DFP=1) | 4:0 Msg Type`

**Fixed Supply PDO（6.4.1.2.2）**：`31:30=00 | 29 DRP | 28 USB Suspend | 27 Unconstrained | 26 USB Comm | 25 Dual-Role Data | 24 Unchunked Ext | 23 EPR | 21:20 Peak I | 19:10 電壓(50mV) | 9:0 最大電流(10mA)`

**PPS APDO（6.4.1.3.4）**：`31:30=11 | 29:28=00 | 27 PPS Power Limited | 24:17 Max V(100mV) | 15:8 Min V(100mV) | 6:0 Max I(50mA)`

**RDO Fixed（6.4.2）**：`30:28 Object Position(從1開始) | 26 Mismatch | 25 USB Comm | 24 No Suspend | 23 Unchunked | 22 EPR | 19:10 Operating I(10mA) | 9:0 Max Operating I(10mA)`

**RDO PPS**：`30:28 Pos | 26..22 同上旗標 | 20:9 Output Voltage(20mV) | 6:0 Operating I(50mA)`

C51 提示：8051 上 `uint32_t` 移位很貴；實務上常用 `unsigned char` 陣列/union 直接存取 byte。本練習先在 PC 上把邏輯做對。

## 時間分配
- 5 分鐘：讀 `pd_codec.h` 與上方欄位表
- 20 分鐘：完成 `pd_codec.c` 內所有 TODO
- 5 分鐘：`make test` 驗證，全部通過為止

## 練習步驟
1. `cd 2026-11-13_pd-msg-codec && make test`（一開始會失敗）
2. 依序實作：header → Fixed PDO → PPS APDO → RDO(Fixed/PPS) → `pd_validate_request`
3. 驗證規則：`obj_pos` 為 0 或超過 PDO 數 → `REQ_REJECT_POS`；Fixed 的 op/max 電流不可超過 PDO；PPS 的輸出電壓需在 [min,max]、電流不超過 max
4. 全部通過後才看 `SOLUTION.md`

## 延伸思考題
1. 若 RDO 的 Capability Mismatch=1，Source 是否仍應該 Accept？規格怎麼說？
2. Pack 時 `voltage_mv/50` 遇到非 50 的倍數（如 5025mV）該無條件捨去、四捨五入還是回報錯誤？各有什麼風險？
3. 在 8051 上如何避免 32-bit 移位的 code size 與時間成本（提示：union + byte access、big/little endian）？
