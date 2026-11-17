# 2026-11-17 PE Source 狀態機模擬器（PE_SRC_*）

**主題分類：Policy Engine 狀態機 / PD Timer**　**預計時間：30 分鐘**

## 學習目標
- 把 PD3.x Source Policy Engine 的主流程寫成「事件驅動 + entry action + timer」的 C 狀態機（可直接移植到 C51）。
- 搞懂 tSenderResponse / tTypeCSendSourceCap / tSrcTransition / tPSHardReset / tSrcRecover 各自卡在哪個狀態。
- 理解 CapsCounter、HardResetCounter 的歸零與上限（nCapsCount=50、nHardResetCount=2）。
- 實作 8051 友善的 timer tick：`uint16_t` 倒數 **不可 underflow**。

## 背景知識
規格：USB PD 3.x **§8.3.3.2 Policy Engine in Source Port State Diagram**、**§6.6 Timers**、**§6.7 Counters**。

```
STARTUP ─EV_START→ SEND_CAPS ─GoodCRC→(等 Request, tSenderResponse)
   ▲                 │ 無 GoodCRC            │ Request           │ 逾時
   │                 ▼                       ▼                   ▼
   │             DISCOVERY             NEGOTIATE_CAP         HARD_RESET
   │       (tTypeCSendSourceCap)        │Accept   │Reject        │ tPSHardReset
   │        ≤nCapsCount→SEND_CAPS      ▼         ▼              ▼
   │        >nCapsCount→DISABLED  TRANSITION_SUPPLY  READY/WAIT_NEW_CAPS
   │                               │tSrcTransition→SET_VBUS→VBUS_READY→PS_RDY→READY
   └──── tSrcRecover ◄── TRANSITION_TO_DEFAULT ◄── 任何狀態收到對方 Hard Reset
```

**本題的簡化**：只做 Source 端主流程；Request 合法性＝5000~20000 mV；不處理 PD2.0/3.0 版本、Swap、VDM；
GoodCRC/逾時以事件 (`EV_TX_GOODCRC` / `EV_TX_NO_GOODCRC`) 餵入，不模擬 PHY。HardResetCounter 在 PS_RDY 送出（合約建立）時歸零。
`T_VBUS_READY_TIMEOUT` 為自訂值。

## 時間分配
| 時間 | 內容 |
|------|------|
| 5 分鐘 | 讀本文與 `pe_src.h`、瀏覽 `test_pe_src.c` |
| 20 分鐘 | 依 TODO(1)~(7) 順序實作 `pe_src.c` |
| 5 分鐘 | `make test` 全綠；對照 SOLUTION.md |

## 練習步驟
1. `cd 2026-11-17_pe-src-fsm && make test`（此時大量 FAIL 是正常的）。
2. **TODO(1)** `pe_event` 的 `EV_TICK`：timer 倒數。注意 `arg >= timer_ms` 要先比較，再相減。
3. **TODO(2)** `SEND_CAPS`、`DISCOVERY` 的 entry。
4. **TODO(3)** `NEGOTIATE_CAP`：Accept / Reject 分支（有無 explicit contract 去向不同）。
5. **TODO(4)** `TRANSITION_SUPPLY` 兩階段：先等 tSrcTransition 才能動電源（phase 0），再等 VBUS（phase 1）。
6. **TODO(5)** `HARD_RESET`、`TRANSITION_TO_DEFAULT` 的 entry 與逾時。
7. **TODO(6)** `SEND_CAPS`、`DISCOVERY` 逾時：兩個 counter 的判斷（`<=` vs `>`，容易差一）。
8. **TODO(7)** GoodCRC 與 PS_RDY 的處理。
9. `make test` 全綠後，再看 `SOLUTION.md`。驗證解答：`make check-solution`。

## 延伸思考題
1. 實際 C51 上 `EV_TICK` 來自 1 ms Timer ISR。`pe_event` 在主迴圈呼叫、ISR 只累加 `tick_pending`（uint8_t）時，要怎麼避免丟 tick 與 race？（提示：臨界區、一次取走）
2. `PE_SRC_Send_Capabilities` 沒收到 GoodCRC 與收到 GoodCRC 卻沒有 Request，為何走不同路徑（Discovery vs Hard Reset）？對「Sink 未接/未支援 PD」有什麼意義？
3. 每個狀態各自用 `timer_ms` 單一計時器，哪個真實情境會需要同時兩個 timer？（提示：SenderResponseTimer 與 CRCReceiveTimer、tSourceCap）
