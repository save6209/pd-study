# 參考解答（請先自己做完再看！）

完整程式碼見 `solution/prot.c`，以下是關鍵解說。

## 1. ADC 換算
```c
uint32_t x = (uint32_t)adc * 3300u * 11u;      /* 最大 1023*36300 = 37,134,900，需 32 bit */
return (x + 1023u/2u) / 1023u;                 /* 四捨五入 */
```
- 一定要先把 `adc` 轉成 `uint32_t` 再乘；`adc * 3300u` 在 16-bit `int` 的 C51 下會先以 16 bit 計算而溢位（常見 bug）。
- IBUS：`adc * 3300 * 4 / 1023`，單次除法。誤差 ≤ 0.5 mA；若「先 mV 再 ×4」誤差可達 2 mA。
- 進階：若要省掉 32 位除法，可預先算出定點係數 `K = round(36300 * 65536 / 1023)`，`mv = (adc*K) >> 16`，但要驗證誤差並注意 `adc*K` 仍是 32 bit。

## 2. 門檻
```c
return (uint16_t)(((uint32_t)mv * 110u) / 100u);
```
`mv*110` 在 20000 mV 時為 2,200,000，超過 65535，因此要 32 bit；或改寫成 `mv + mv/10`（精確度略不同，需與測試一致時用 32 bit 版本）。

## 3. prot_tick
- OVP：連續計數。任一次正常就歸零，避免雜訊累積誤跳，同時 3 ms 內可以反應。
- OCP：漏水桶（超限 +1、正常 -1、下限 0）。短突波（如容性負載啟動）不會跳，持續過載會在平均 > 50% 時間超限時累積到 20。
- 計數器飽和於門檻，避免 `uint8_t` 環繞（255→0 會讓已超限的情況變成沒超限，是實務上典型 bug）。
- flags 為 latched，只能由 `prot_try_clear` 清除。

## 4. prot_try_clear
`vbus <= ovp_limit - ovp_limit/16`（約 6% hysteresis）且 `ibus < ocp_limit` 才清，避免在門檻邊緣抖動地反覆 trip/clear。尚無合約（limit = 0）時直接返回。

## 5. prot_set_contract
只清計數，不清 flags：否則 PPS 重新 Request 可以「洗掉」尚未處理的 fault。

## 延伸題提示
1. 主迴圈寫 2-byte 變數時 ISR 可能讀到半新半舊的值：寫入前後 `EA = 0 / EA = 1`（critical section），或用「先寫 shadow、再用單 byte flag 通知 ISR 切換」。
2. 常見做法：Transition_Supply 期間 `prot_set_contract` 用「max(舊,新) 合約」的門檻，或暫停 OVP 並設上限為新電壓 110%，並以 tPSTransition 逾時兜底；風險是遮蔽期間無保護，需靠硬體 OVP。
3. 不夠；軟體 1 ms 取樣只能做慢速保護（過載、過溫），快速短路必須靠硬體比較器 / 內建硬體 OCP 直接關 gate，軟體只負責 latch 與通報。
