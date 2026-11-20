# 2026-11-20　ISR 與主迴圈共享變數：Torn Read、Critical Section、SPSC Ring Buffer

**主題分類：ISR / 並行與共享變數**　**預計時間：30 分鐘**

## 學習目標
1. 親眼重現 8051 上「多 byte 變數非原子讀取」造成的 torn read（例如 tick 由 `0x00FF` → `0x0100` 時讀到 `0x01FF`）。
2. 用兩種方法修正：**關中斷臨界區**、**不關中斷的 double-read 重試**，並比較適用情境。
3. 寫出 wrap-safe 的 PD timer 逾時判斷（tSenderResponse、tPSTransition 等都靠它）。
4. 實作單生產者單消費者（SPSC）ring buffer：ISR 收 byte、主迴圈處理，不需鎖。

## 背景知識
- **8051 是 8 位元 CPU**：`unsigned int` / `unsigned long` 需要 2 / 4 次 `MOV`，ISR 可在任兩條指令間插入。
- Keil C51：共享變數必須宣告 `volatile`，否則編譯器可能把它放暫存器或合併讀取；ISR 用 `void timer0_isr(void) interrupt 1`。
- 臨界區：`EA = 0; ...; EA = 1;`。要短！PD 的 BMC 收發對時序敏感，關太久會漏 edge（Physical layer ISR）。若呼叫者可能已關中斷，應「保存再還原」EA 而非無條件 `EA = 1`。
- 8051 上 **uint8_t / bit 讀寫是原子的**，所以 SPSC ring buffer 的 `head`/`tail` 用 uint8_t 就不需要鎖。
- USB PD 3.x Spec §6.6 Timers：tSenderResponse（24–30 ms）、tTypeCSendSourceCap（100–200 ms）、tPSTransition（450–550 ms）、tSrcRecover 等，韌體普遍用一個 1 ms tick 計數器 + 「起始時間」實作，逾時判斷若寫錯，在 tick 繞回時（32-bit 約 49.7 天、16-bit 僅 65.5 秒！）會誤觸發 Hard Reset。
- 本題以 `sim.h` 模擬「ISR 在第 N 次 byte 存取後插入」：自己的 byte 存取後呼叫 `sim_byte_access()`，測試會掃過所有插入點與多種邊界值（`0xFF`、`0xFFFF`、`0xFFFFFF`、`0xFFFFFFFF`）。

## 時間分配
| 時間 | 內容 |
|------|------|
| 5 分 | 閱讀本文、`tick.h`、`tick.c` 的 naive 示範，跑 `make test` 看 torn read |
| 20 分 | 完成 TODO 1~4 |
| 5 分 | `make test` 全綠，回答延伸思考題 |

## 練習步驟
1. `make test`：觀察 `0x1FF` 的 torn read 示範。
2. TODO 1 `tick_read_critical`：`EA_DISABLE` → 逐 byte 讀 → `EA_ENABLE` → return。測試會檢查 ISR 沒被吃掉（掛住的中斷在 EA=1 後必須執行恰好一次）。
3. TODO 2 `tick_read_retry`：不關中斷，連讀兩次比較，不同就重讀。想想為何只在「單一寫入者且單調遞增」時成立。
4. TODO 3 `pd_timer_expired`：用無號減法 `now - start`，不要寫 `now >= start + timeout`。
5. TODO 4 `rb_put` / `rb_get`：浪費一格判滿；先寫資料再推進 index。
6. 全綠後看 `SOLUTION.md`（驗證參考解答：`make check-solution`）。

## 延伸思考題
1. 若 tick 改成 `uint16_t`（2 byte），讀取時只需「讀 hi → 讀 lo → 再讀 hi」即可？這和 TODO 2 的做法有何關係？C51 以 big-endian 存放，對讀取順序有何影響？
2. 臨界區裡若呼叫端本來就已經 `EA = 0`（例如在另一個臨界區內），無條件 `EA = 1` 會有什麼後果？如何寫出可巢狀的 save/restore？
3. 如果 PD 收訊 ISR 會把 header 與 data object 放進 ring buffer 後設一個 `bit rx_done` 旗標，主迴圈清旗標與 ISR 設旗標之間會有什麼 race（lost message）？用「計數器」或「ring buffer 非空」取代旗標有何好處？
