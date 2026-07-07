# E164 — FrontendWindowShell (god-struct decomposition #10) — RULING + move

**STATUS: STAGED in `blocked/`.** Recon-grounded + spot-verified (workflow `wdnplylk0`).
**Commit convention:** `claude: planned. codex: …`. This is the residual "junk-drawer" resolution.

> **EXECUTION SERIALIZES** on `ProductAppWindowState.hpp`; run AFTER #4/#6/#7/#9/#11 so the true leftover set is
> visible. Re-anchor at slice time.

---

## Ruling — a real store IS worth standing up, but split honestly

After all other stores move out, the leftover top-level members are **not** homogeneous junk — they cluster:

- **STAYS as the app-global remainder (~7, already correct):** genuine SDL/window-lifecycle + present-existence
  bits — `requested`, `sdlAvailable`, `created`, `drawable`, and the surface/swapchain existence bits. **Do NOT
  store these** (§3 of the map); they are the legitimate window-lifecycle remainder.
- **MOVES into `FrontendWindowShell`:** the coherent opening-menu / boot-screen flags — `openingMenuVisible`,
  `menuTextDrawn`, `selectedRowDrawn`, `mouseMenuSelectUsed`, and the gamepad-menu flag (see coordination), plus
  the `productVulkanMenu` present-menu state carved out of #11. ~16 members, **~40 repoints.**
- **FLAG (do not silently absorb):** `automationControl` is a mis-fit here — it's automation/test-harness state,
  not window-shell. Recon recommends flagging it for its own home rather than dumping it in the shell. Surface it
  to the planner; don't fold it in on a guess.

## ⚠ CROSS-CARD COORDINATION — reclaim `gamepadMenuSelectUsed` from InputDeviceStore (E156)

Recon found `gamepadMenuSelectUsed` is **mis-filed into InputDeviceStore #7 (E156)**. It is a *menu-selection*
flag that pairs with `mouseMenuSelectUsed` (they sit adjacent on the god-struct, `:195`/`:197`), not a gamepad
*device* property like `gamepadAvailable`/`gamepadName`/`gamepadMapping`. **Recommended: it belongs in
FrontendWindowShell.**

**Action for the planner/slicer:** this is a judgment call (device-flag vs menu-flag). If accepted:
- **Gate-neutral pre-step** (do while both are still flat, zero code change): correct the TSV row
  `gamepadMenuSelectUsed` from `InputDeviceStore` → `FrontendWindowShell`.
- **Remove `gamepadMenuSelectUsed` from E156's InputDeviceStore field list** so #7 doesn't claim it.
If rejected, leave it in InputDeviceStore and drop it from this card. **Confirm before either store executes.**

## LAW & gates

- **Golden byte-identical** (menu/boot flags emit via string keys; path repoint changes no key/value). A diff = BUG.
- **Compiler-guided, not sed.**
- **TSV:** delete the moved rows, add the `frontendShell` (and any second nested member) row(s); leave the ~7
  remainder rows as their app-global owner. Reconcile the `gamepadMenuSelectUsed` row per the coordination above.
- Build green · ctest 260/260 · golden unchanged · update map #10 → DONE + PRIORITY.md. This move DONE-s the
  decomposition: god-struct = the ~7 lifecycle bits + the 11 store members.

## Suggested slices

- **STEP 0 (gate-neutral):** TSV `gamepadMenuSelectUsed` retarget (if the reclaim is accepted).
- **G1:** stand up `FrontendWindowShell` (+ absorb `productVulkanMenu` from #11), move the menu/boot flags,
  compiler-repoint; build + ctest 260/260.
- **G2:** decide `automationControl`'s home (separate card if it needs one) + final map/PRIORITY docs.
