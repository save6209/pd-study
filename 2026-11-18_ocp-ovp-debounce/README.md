# 2026-11-18 OCP/OVP 保護：ADC 定點換算與 Debounce

**主題分類：保護機制 / ADC 定點運算**　**預計時間：30 分鐘**

## 學習目標
- 用純整數（不用 float）把 10-bit ADC 原始值換成 VBUS mV / IBUS mA，並正確四捨五入。
- 理解 C51 上 `uint16_t` 相乘溢位：中間值必須提升到 `uint32_t`（C51 的 `long` = 4 bytes，但運算慢，要懂取捨）。
- 設計兩種 debounce：**連續 N 次**（OVP，要快且嚴）與**漏水桶**（OCP，容許短暫突波）。
- 保護 latch、釋放條件（hysteresis），以及 PPS 重新 Request 時門檻如何更新而不誤清 fault。

## 背景知識
- USB PD 3.x §7.1（Source 輸出電壓容差，vSrcNew/vSrcValid）、§7.2.x（過壓/過流是 Source 自行實作的保護，規格要求 Source 在故障時進入 Hard Reset / 關 VBUS 的行為，見 §8.3.3 PE_SRC_Hard_Reset）。
- 8051：沒有 FPU，float 由函式庫軟體模擬，吃 code size 與時間；ISR 中更應避免。`uint32_t` 乘除也貴，所以 **每個 tick 只做必要的運算**，門檻在協商時算一次（`prot_set_contract`）而不是每 ms 重算。
- 四捨五入的整數寫法：`(x + d/2) / d`。
- 誤差累積：先算 mV 再乘 4 得 mA，會把 ±0.5 mV 誤差放大成 ±2 mA；請合併成單次除法。

本題假設：Vref 3.3V、10-bit、VBUS 分壓 1/11、sense 5 mΩ × 增益 50（參數在 `prot.h`）。

## 時間分配
| 時間 | 內容 |
|------|------|
| 5 分鐘 | 讀本文、`prot.h`，瀏覽 `test_prot.c` |
| 20 分鐘 | 依 TODO(1)~(6) 實作 `prot.c` |
| 5 分鐘 | `make test` 全綠；對照 SOLUTION.md |

## 練習步驟
1. `make test`（此時大量 FAIL 是正常的）。
2. TODO(1)(2)：`adc_to_vbus_mv` / `adc_to_ibus_ma`。
3. TODO(3)：門檻計算，注意 20V × 110 會溢位 16 bit。
4. TODO(4)(5)：`prot_set_contract` 與 `prot_tick`（OVP 連續計數 + OCP 漏水桶，計數器飽和不環繞）。
5. TODO(6)：`prot_try_clear` 的 hysteresis 條件。
6. `make test` 全綠後再看 SOLUTION.md。（驗證參考解答：`make check-solution`）

## 延伸思考題
1. `prot_tick` 若在 1 ms timer ISR 裡呼叫，而 `prot_set_contract` 在主迴圈呼叫（PE 協商完成），會有什麼競爭？`ovp_limit_mv` 是 2 byte，8051 一次寫一 byte，需要怎麼保護？
2. PPS 從 5V 切到 20V 的 transition 期間，VBUS 本來就會高於舊合約的 110% 門檻。你會如何在 `Transition_Supply` 期間處理（遮蔽、放寬門檻、或用新門檻）？各有什麼風險？
3. 若 ADC 每 1 ms 取樣一次但 OCP 要求 < 200 µs 反應，軟體 debounce 還夠嗎？應該交給哪個硬體（比較器 / 硬體 OCP）？
