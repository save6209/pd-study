# 參考解答（請先自己做完再看！）

完整程式在 `solution/tick.c`，`make check-solution` 可驗證。

## TODO 1：關中斷臨界區
```c
EA_DISABLE();
for (i = 0; i < 4; i++) v |= tick_b[i] << (8*i);
EA_ENABLE();
return v;
```
- 臨界區只含 4 次 byte 搬移，約幾個機器週期；關中斷期間到來的 Timer 中斷會被硬體掛住（IE flag），EA=1 後才執行，所以**不會丟 tick**（但若關超過 1 ms 就會丟）。
- C51 實務：`bit ea_save = EA; EA = 0; ...; EA = ea_save;` 才可巢狀。

## TODO 2：double-read 重試
```c
do { a = raw_read(); b = raw_read(); } while (a != b);
```
- 成立條件：單一寫入者、值單調遞增（或只會往前變）。若兩次讀值相同，中間 ISR 最多遞增整數圈（>49 天，不可能），所以不會撞到 torn 值。
- 優點：不影響 ISR 延遲（對 PD BMC 這類對時序敏感的系統很重要）。缺點：最壞需重試，WCET 不固定；ISR 頻率高於讀取速度時可能 livelock。
- 在較簡單的 16-bit 情境，`hi → lo → hi` 若 hi 沒變即一致。

## TODO 3：wrap-safe 逾時
```c
return (uint32_t)(now - start) >= timeout_ms;
```
`now - start` 在模 2^32 下永遠是「經過時間」。錯誤寫法 `now >= start + timeout` 在 `start + timeout` 溢位時永遠為假（或剛開始就為真）。限制：timeout 必須 < 2^31，且須在逾時前被檢查。C51 若 tick 只用 `uint16_t`，最長只能量測 ~32 s。

## TODO 4：SPSC ring buffer
```c
put: next=(head+1)&MASK; if(next==tail) return 0; buf[head]=v; head=next;
get: if(head==tail) return 0; *v=buf[tail]; tail=(tail+1)&MASK;
```
- 每個 index 只有一方寫入，且 uint8_t 讀寫原子 → 不需鎖。
- **順序很重要**：先寫資料再推進 head，否則主迴圈可能在資料寫入前就看到新 head。在多核心 / 有 cache 的系統還需 memory barrier；8051 單核無 cache，`volatile` 即可。

## 延伸思考題提示
1. `hi,lo,hi`：與 TODO 2 同理，是只讀一半的變種；C51 big-endian 時 hi 在低地址，讀序要對應。
2. 巢狀會提前開中斷，外層臨界區被破壞；解法是保存 EA 再還原。
3. 主迴圈 `if (rx_done) { handle(); rx_done = 0; }` 在 `handle()` 之後、清旗標之前若 ISR 又收到訊息並設旗標，這次訊息會被一起清掉而遺失。改用 ring buffer / 計數器（ISR ++、主迴圈 --，且對計數器的修改要原子）可避免。
