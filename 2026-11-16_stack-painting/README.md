# 8051 Stack Painting 與最深呼叫鏈估算

> 日期：2026-11-16　｜　預計時間：30 分鐘　｜　語言：C（gcc 可編譯，模擬 8051 IDATA）

## 學習目標
1. 理解 8051 stack 只能放在 IDATA（最多 256 bytes，與 DATA 變數、register bank、bit-addressable 區共用）。
2. 實作 stack painting：開機填 `0x55`，執行一段時間後掃描「歷史最高水位」。
3. 用呼叫圖估算最深 stack 用量，並加入 ISR 巢狀（8051 只有 2 級硬體優先權）。
4. 偵測遞迴（C51 預設不支援一般遞迴，非 `reentrant` 函式遞迴會直接壞掉）。

## 背景知識
- **8051 stack 往上長**：`PUSH` = `SP++` 再寫入；`LCALL` 推 2 bytes（PCL 先、PCH 後）；`ACALL` 同樣 2 bytes。
- Keil C51 啟動碼 `STARTUP.A51` 的 `?C_START` 前會設定 SP，stack 起點通常接在 DATA/IDATA 變數之後；
  所有 IDATA 剩餘空間就是 stack 的最大可用量。M51 map 檔的 `IDATA` 區末端即可算出 `limit`。
- **ISR**：進入時硬體推 PC（2 bytes），Keil 再依用到的暫存器 `PUSH ACC/B/DPL/DPH/PSW/Rn`，
  這些計入該 ISR 的 `frame`。高優先權 ISR 可打斷低優先權 ISR，最壞情況：
  `main + low ISR + high ISR`（同級不會互相巢狀）。
- **Stack painting 陷阱**：
  - 填充值被合法資料碰巧寫成 `0x55`，所以從**頂端往下找第一個非 0x55**，而不是找「第一個 0x55」。
  - `limit = 0xFF` 時，`uint8_t i; i <= limit` 是無窮迴圈。
  - 最頂端一格當 canary，被改掉代表已經溢位（可能已覆寫別的資料）。
- C51 的 `-O` overlay 會把非重入函式的區域變數靜態分配，不吃 stack；`reentrant` 函式才用 stack，
  這也是 `frame` 的來源之一。
- 參考：USB-PD 的 PE/Protocol 通常在主迴圈，PHY 的 CC 中斷（RX/TX、GoodCRC timeout）為高優先 ISR，
  stack 預算特別容易被它吃掉。

## 時間分配
| 時間 | 內容 |
|------|------|
| 5 分鐘 | 讀本文與 `stack_guard.h` |
| 20 分鐘 | 完成 `stack_guard.c` 的 TODO 1~6 |
| 5 分鐘 | `make test` 驗證、思考延伸題 |

## 練習步驟
1. `make test`：骨架可編譯，但測試會失敗（預期）。
2. **Part 1（TODO 1~4）**：`stack_paint` / `stack_high_water` / `stack_free` / `stack_overflowed`。
   測試包含 `limit=0xFF` 邊界與「資料恰為 0x55」的案例。
3. **Part 2（TODO 5）**：`stack_cost` 以 DFS 計算 `cost(f) = frame + max(2 + cost(callee))`，遇環回 `-1`。
4. **Part 3（TODO 6）**：`stack_worst_total` 把 main 與 low/high 各一個最大 ISR 加總（每個 ISR +2）。
5. 全部 `ALL PASS` 後再看 [SOLUTION.md](SOLUTION.md)。

## 延伸思考題
1. 若實機上 stack painting 量到歷史最高 `0xC8`，但 IDATA 只剩 `0xD0`，你還敢出貨嗎？該用多大安全餘量？
   最壞路徑（例如 Hard Reset + 同時 Liquid Detect ISR）在量測時真的跑到了嗎？
2. 函式指標呼叫（例如 PE state handler table）讓靜態呼叫圖變得不完整，你會怎麼把它納入估算？
3. 若把 `main` 裡的大型區域陣列改成 `static` 或 `xdata`，對 stack 與執行效率各有什麼影響？
