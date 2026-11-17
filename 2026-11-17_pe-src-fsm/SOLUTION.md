# SOLUTION（請先自己做完再看！）

完整參考實作：[`solution/pe_src.c`](solution/pe_src.c)，驗證：`make check-solution`。

## 解說重點
**TODO(1) timer 倒數**
```c
if (c->timer_run) {
    if (arg >= c->timer_ms) { timer_stop(c); on_timeout(c); }
    else                    { c->timer_ms -= arg; }
}
```
`timer_ms -= arg` 若 arg > timer_ms 會繞回 65535 附近，timer 變「永不逾時」。主迴圈忙碌時 tick 會累積，這在實機很常見。
先 `timer_stop` 再 `on_timeout`，因為 on_timeout 內可能 `enter()` 新狀態並重新啟動 timer。

**enter() 開頭 `timer_stop`**：離開狀態時殘留的 timer 不會誤觸發下一個狀態，是最常見的 FSM bug 來源。

**TODO(2)(6) Counters**：SEND_CAPS entry 才 `caps_counter++`；DISCOVERY 逾時判斷 `caps_counter > N_CAPS_COUNT`。
故 Source_Capabilities 一共送 `nCapsCount + 1 = 51` 次，之後 Disabled（測試 `t_discovery` 驗證）。
SenderResponse 逾時判斷 `hard_reset_counter <= N_HARD_RESET_COUNT`：counter 為 0、1、2 時各 Hard Reset 一次（共 3 次），第 4 次 Disabled。

**TODO(3)**：Reject 後有 explicit contract 回 READY（維持原合約），沒有則 WAIT_NEW_CAPS 等 DPM 通知 Caps 改變。

**TODO(4) 兩階段**：Accept 之後必須等 tSrcTransition（Sink 需要時間降載）才可 `ACT_SET_VBUS`，
phase 1 才接受 `EV_VBUS_READY`（過早的事件須忽略）；VBUS 逾時視為失敗走 Hard Reset。

**TODO(5) Hard Reset**：本方發出 → HARD_RESET（送訊號、等 tPSHardReset）→ TRANSITION_TO_DEFAULT；
對方發來 → 直接 TRANSITION_TO_DEFAULT（不回送）。TO_DEFAULT：VBUS 關閉、清合約、等 tSrcRecover，再恢復 vSafe5V 回 STARTUP。

**TODO(7)**：GoodCRC 清 `caps_counter` 並啟動 tSenderResponse；PS_RDY 送出代表合約成立，清 `hard_reset_counter`。

## 移植到 C51 的提示
- `pe_ctx_t` 放 XDATA 或 `pdata`；單一 ctx 時用 `static` 避免 reentrant 開銷。
- `enter()` 有遞迴（NEGOTIATE → TRANSITION_SUPPLY），C51 預設不可重入、靠 overlay，遞迴需 `reentrant` 或改寫成迴圈／直接賦值，並注意 stack 深度（見 2026-11-16 題）。
