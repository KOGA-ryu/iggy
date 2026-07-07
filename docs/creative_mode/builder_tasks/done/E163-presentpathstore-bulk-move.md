# E163 — PresentPathStore bulk-move (god-struct decomposition #11) — PARENT

**STATUS: DONE.** Recon-grounded + spot-verified (workflow `wdnplylk0`).
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

## Completion Brief

- Files changed:
  - `src/app/iggy3d/window/PresentPathStore.hpp`
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `src/app/iggy3d/receipt/FeedbackSurfaceAutomationVulkanFields.cpp`
  - `src/app/iggy3d/window/FramePresenter.cpp`
  - `src/app/iggy3d/window/Loop.cpp`
  - `src/app/iggy3d/window/RendererLifecycle.cpp`
  - `tests/unit/product_vulkan_room_frame_tests.cpp`
  - `tests/unit/product_window_renderer_lifecycle_tests.cpp`
  - `docs/god_struct_member_ownership.tsv`
  - `docs/god_struct_decomposition_target_map.md`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/creative_mode/builder_tasks/claimed/E163-presentpathstore-bulk-move.md`
- Store/API shape:
  - Added `iggy3d::PresentPathStore` with exactly the nine scoped present-path
    fields: `productVulkanRenderer`, `productVulkanSurfaceCreated`,
    `productVulkanSwapchainReady`, `productVulkanFrameSubmitted`,
    `productVulkanFrameSubmittedCount`, `productVulkanStatus`,
    `productVulkanReasonCode`, `productVulkanRenderingPath`, and
    `productVulkanRecordMode`.
  - Replaced the nine flat `ProductAppWindowState` members with
    `ProductAppWindowState::presentPath`.
  - Left `ProductAppWindowState::productVulkanMenu` top-level for E164.
- Repoint policy:
  - Removed the flat fields first and used the build plus scoped grep evidence
    to repoint the exact nine present-path names.
  - Did not repoint or move `productVulkanMenu`.
  - Final direct-field grep produced no output for top-level window/request
    access to the nine moved fields.
- Receipt/ownership result:
  - Receipt golden output is byte-identical; `git diff -- tests/golden/product_receipt_key_order.golden`
    produced no output.
  - Ownership TSV now has one `presentPath	PresentPathStore` row and keeps
    `productVulkanMenu	FrontendWindowShell`.
  - `product_god_struct_ownership_coverage_tests` reports `PresentPathStore=1`
    and `app-global-remainder=4`.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10`
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure`
    - `100% tests passed, 0 tests failed out of 260`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing-whitespace scan over touched files
  - direct moved-field grep:
    `rg -n "\b(window|request\.window|collisionWindow|menuWindow|renderWindow)\.(productVulkanRenderer|productVulkanSurfaceCreated|productVulkanSwapchainReady|productVulkanFrameSubmitted|productVulkanFrameSubmittedCount|productVulkanStatus|productVulkanReasonCode|productVulkanRenderingPath|productVulkanRecordMode)\b" src tests tools`
    produced no output.
- Concerns/deferred:
  - None for E163. `productVulkanMenu` remains intentionally deferred to
    FrontendWindowShell/E164.
