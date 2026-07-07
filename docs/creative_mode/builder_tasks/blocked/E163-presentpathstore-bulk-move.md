# E163 — PresentPathStore bulk-move (god-struct decomposition #11) — PARENT

**STATUS: STAGED in `blocked/`.** Recon-grounded + spot-verified (workflow `wdnplylk0`).
**Commit convention:** `claude: planned. codex: …`.

> **EXECUTION SERIALIZES** on `ProductAppWindowState.hpp`; re-anchor at slice time.

---

## Ruling — STAND UP a store owning ALL 9 `productVulkan*` status members

The map row 11 lists ~6 and its §3 wanted `productVulkanSurfaceCreated/SwapchainReady/FrameSubmitted` left on
the app-global remainder. **Recon rejects that split** — verified (HEAD) that the god-struct carries **9**
`productVulkan*` status/mirror members that are written + read as one present-loop unit; splitting them would
leave the receipt emitter reading half-old/half-new. Move all 9. **Single atomic slice, ~92 repoints.**

## The 9 fields → `window.presentPath`

```
productVulkanRenderer productVulkanSurfaceCreated productVulkanSwapchainReady productVulkanFrameSubmitted
productVulkanFrameSubmittedCount productVulkanStatus productVulkanReasonCode productVulkanRenderingPath
productVulkanRecordMode
```

**Out of scope:** `productVulkanMenu` (a `ProductVulkanMenuState`, the adjacent member) is assigned to
**FrontendWindowShell #10 (E164)** — do NOT move it here.

## Method

New header `src/app/iggy3d/window/PresentPathStore.hpp` — `struct PresentPathStore { …9 members verbatim
(types + defaults)… };`. (The recon suggests optionally renaming to shorter members like `renderer`/`status`
inside the struct, but **keep the names verbatim to keep the diff mechanical and the golden safe** — rename is
a separate, later cleanup.) Replace the 9 flat god-struct fields with `PresentPathStore presentPath;`. Build;
repoint each `no member named` error `window.<field> → window.presentPath.<field>`.

## LAW & gates

- **Golden byte-identical** — emitted via string keys in `FeedbackSurfaceAutomationVulkanFields.cpp`; path
  repoint changes no key/value. A diff = BUG.
- **Compiler-guided, not sed** (low-risk; production uniform bare `window`).
- **TSV:** delete the 9 `productVulkan*␉PresentPathStore` rows, add one `presentPath␉PresentPathStore` row.
- Build green · ctest 260/260 · golden unchanged · update map #11 → DONE + PRIORITY.md.

## Suggested slices

**One gate.** The cluster is read/written as a unit; a partial split breaks the emitter.
