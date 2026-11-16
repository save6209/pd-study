# 參考解答與解說

> ⚠️ 請先自己做完再看！完整程式在 [solution/stack_guard.c](solution/stack_guard.c)，
> 驗證：`make check-solution`。

## Part 1：painting
```c
unsigned i;                       /* 不用 uint8_t，避免 limit=0xFF 時無窮迴圈 */
for (i = base; i <= limit; i++) idata[i] = STACK_PAINT;
```
`stack_high_water` 從 `limit` 往下掃第一個 `!= 0x55`，回傳 `p - base + 1`。
資料巧合等於 0x55 只會讓「中間」看起來是空的，但因為我們由頂端往下掃，
只要更深處有被寫過就會先被找到，所以結果不受中間 0x55 影響（極端情況：最深一格剛好寫 0x55 會少算 1 byte，
實務上 +1~2 bytes 餘量即可，或選不常見的樣式）。

`stack_free`：`sp >= limit ? 0 : limit - sp`。
`stack_overflowed`：檢查 `idata[limit]`（canary）。

## Part 2：呼叫圖 DFS
- `cost(f) = frame(f) + max_c(2 + cost(c))`，葉節點 = `frame`。
- 三色標記：`state==1`（訪問中）再被走到 → 有環 → 回 `-1`；`state==2` 直接用 memo。
- 範例：`crc=3`，`phy_send=2+(2+3)=7`，`pd_task=4+(2+7)=13`，`main=1+(2+13)=16`。

## Part 3：ISR 巢狀
- 每個 ISR 成本 = `cost(isr) + 2`（硬體 LCALL）。
- 同優先權不會巢狀，所以各級取最大值，總和 `main + low_max + high_max`。
- 範例：low = max(12, 9) = 12、high = 14，total = 16 + 12 + 14 = 42。

## 實務備註
- 這是**靜態上界**，函式指標、`reentrant` 函式、C51 library（如 `?C?` 乘除法輔助函式）的 stack 需另外加。
- 靜態估算 + 實機 painting 量測互相驗證，兩者取大再加餘量。
