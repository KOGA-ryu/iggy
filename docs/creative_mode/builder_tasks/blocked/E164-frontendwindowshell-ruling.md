# E164 — FrontendWindowShell (god-struct decomposition #10) — RULING + move

**STATUS: STAGED in `blocked/`.** Recon-grounded + spot-verified (workflow `wdnplylk0`).
**Commit convention:** `claude: planned. codex: …`. This is the residual "junk-drawer" resolution.
Children released so far: `done/E173-frontendwindowshell-g0-current-state-audit.md`;
`done/E174-frontendwindowshell-g1-scalar-menu-state.md`;
`done/E175-frontendwindowshell-g2-startup-state.md`;
`ready/E176-frontendwindowshell-g3-product-vulkan-menu-state.md`.

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
- **RULED (planner-confirmed): `automationControl` STAYS app-global — do NOT put it in the shell.** It's a
  distinct cross-cutting domain (127 refs; its own `automation_control_*` receipt fields; the test/scripting
  driver state), already a clean nested struct. It stays as its own top-level member in the app-global
  remainder — analogous to `activeSession`/`creativeApp` — **not** a store, **not** the shell. Keep its TSV row
  under the app-global-remainder owner label (same as `requested`/`sdlAvailable`/…). No move; no code change to it.

## CROSS-CARD COORDINATION — `gamepadMenuSelectUsed` reclaimed from InputDeviceStore (E156) — CONFIRMED

**Planner-confirmed:** `gamepadMenuSelectUsed` moves to **FrontendWindowShell**, out of InputDeviceStore #7.
Verified: it is a *menu-selection* flag written beside `mouseMenuSelectUsed` in `InputFrame.cpp` menu-select
tracking (`:1453`/`:1542`) — not a device property like `gamepadAvailable`/`gamepadName`/`gamepadMapping`. It
pairs with `mouseMenuSelectUsed` (already in the shell). **E156 has been updated to drop it (now 10 fields).**
- **Gate-neutral pre-step** (while both are still flat, zero code change): retarget the TSV row
  `gamepadMenuSelectUsed` from `InputDeviceStore` → `FrontendWindowShell`.
- It then moves into `FrontendWindowShell` with `mouseMenuSelectUsed` in G1.

## LAW & gates

- **Golden byte-identical** (menu/boot flags emit via string keys; path repoint changes no key/value). A diff = BUG.
- **Compiler-guided, not sed.**
- **TSV:** delete the moved rows, add the `frontendShell` (and any second nested member) row(s); leave the ~7
  remainder rows as their app-global owner. Reconcile the `gamepadMenuSelectUsed` row per the coordination above.
- Build green · ctest 260/260 · golden unchanged · update map #10 → DONE + PRIORITY.md. This move DONE-s the
  decomposition: god-struct = the ~7 lifecycle bits + the 11 store members.

## Suggested slices

- **STEP 0 (gate-neutral):** TSV `gamepadMenuSelectUsed` retarget (if the reclaim is accepted).
- **G1:** stand up `FrontendWindowShell`, move scalar menu/status fields, and retarget `automationControl`
  ownership to `app-global-remainder`. **DONE as E174.**
- **G2:** move `startup` into the existing `FrontendWindowShell`. **DONE as E175.**
- **G3:** move `productVulkanMenu` into `FrontendWindowShell`; final map/PRIORITY docs. **READY as E176.**
