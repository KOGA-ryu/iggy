# E173 — FrontendWindowShell G0: Current-State Audit And Ownership Reconciliation

**STATUS: DONE.** Parent: `blocked/E164-frontendwindowshell-ruling.md`.

## Goal

Do a source-neutral current-state audit before any `FrontendWindowShell` move.
E164 was written before the store moves completed; the true leftover set is now
visible after E172. Do not start the move until this audit resolves the field
list and the `automationControl` ownership contradiction.

## Scope

Inspect:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `docs/god_struct_member_ownership.tsv`
- `docs/god_struct_decomposition_target_map.md`
- `docs/creative_mode/builder_tasks/blocked/E164-frontendwindowshell-ruling.md`
- current call sites for the likely shell fields:
  - `openingMenuVisible`
  - `menuTextDrawn`
  - `selectedRowDrawn`
  - `mouseMenuSelectUsed`
  - `gamepadMenuSelectUsed`
  - `selectedSettingsTab`
  - `launchAction`
  - `launchStatus`
  - `packageLoadStatus`
  - `startup`
  - `productVulkanMenu`
  - `framesPresented`
  - `eventPollCount`
  - `menuRowCount`
  - `status`

Also inspect `automationControl`. E164 says it should stay app-global, but the
current TSV may still assign it to `FrontendWindowShell`. Report the live truth
and the safest correction.

## Rules

- Read-only source card: do not edit source, tests, CMake, receipt golden, or
  production docs except moving/appending this task card.
- Do not create `FrontendWindowShell` in this card.
- Do not move `automationControl`.
- Do not retarget TSV rows in this card; report the needed correction and put it
  into the drafted implementation card.
- Do not run broad CTest.

## Required Commands

Run and report:

```sh
rg -n "struct ProductAppWindowState|\\b(openingMenuVisible|menuTextDrawn|selectedRowDrawn|mouseMenuSelectUsed|gamepadMenuSelectUsed|selectedSettingsTab|launchAction|launchStatus|packageLoadStatus|startup|productVulkanMenu|framesPresented|eventPollCount|menuRowCount|status|automationControl)\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp

rg -n "^(openingMenuVisible|menuTextDrawn|selectedRowDrawn|mouseMenuSelectUsed|gamepadMenuSelectUsed|selectedSettingsTab|launchAction|launchStatus|packageLoadStatus|startup|productVulkanMenu|framesPresented|eventPollCount|menuRowCount|status|automationControl)\\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv

rg -n "\\b(openingMenuVisible|menuTextDrawn|selectedRowDrawn|mouseMenuSelectUsed|gamepadMenuSelectUsed|selectedSettingsTab|launchAction|launchStatus|packageLoadStatus|startup|productVulkanMenu|framesPresented|eventPollCount|menuRowCount|status|automationControl)\\b" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'

git -C /Users/kogaryu/iggy3d diff --check
```

## Deliverable

Append a completion brief to this card and move it to `done/`.

The brief must include:

- exact current top-level `ProductAppWindowState` member list and count;
- exact fields recommended for `FrontendWindowShell`;
- exact fields recommended to remain app-global;
- exact fields recommended for deletion or separate future cleanup;
- `automationControl` ruling and whether the TSV currently disagrees;
- direct call-site count and hottest files for the recommended shell fields;
- a drafted E174 implementation card outline with:
  - field list,
  - expected touched files,
  - required greps,
  - test/golden gates,
  - whether TSV retarget of `automationControl` is included.

No stage, commit, push, source edits, CMake edits, window launch, or broad CTest.

## Completion Brief

- Files inspected:
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `docs/god_struct_member_ownership.tsv`
  - `docs/god_struct_decomposition_target_map.md`
  - `docs/creative_mode/builder_tasks/blocked/E164-frontendwindowshell-ruling.md`
  - direct call sites in `src/app/iggy3d` and `tests/unit` for the candidate
    shell fields and `automationControl`
- Current top-level `ProductAppWindowState` member list and count:
  - Count: 30 top-level members.
  - Members, in declaration order:
    - `requested`
    - `sdlAvailable`
    - `created`
    - `drawable`
    - `openingMenuVisible`
    - `menuTextDrawn`
    - `selectedRowDrawn`
    - `mouseMenuSelectUsed`
    - `gamepadMenuSelectUsed`
    - `inputDevice`
    - `debugHud`
    - `creativeAuthoring`
    - `selectedSettingsTab`
    - `gameplay`
    - `launchAction`
    - `launchStatus`
    - `packageLoadStatus`
    - `startup`
    - `room`
    - `saveSession`
    - `runtimeStateHash`
    - `creativeWorldEpoch`
    - `viewport`
    - `automationControl`
    - `presentPath`
    - `productVulkanMenu`
    - `framesPresented`
    - `eventPollCount`
    - `menuRowCount`
    - `status`
- Fields recommended for `FrontendWindowShell`:
  - `openingMenuVisible`
  - `menuTextDrawn`
  - `selectedRowDrawn`
  - `mouseMenuSelectUsed`
  - `gamepadMenuSelectUsed`
  - `selectedSettingsTab`
  - `launchAction`
  - `launchStatus`
  - `packageLoadStatus`
  - `startup`
  - `productVulkanMenu`
  - `framesPresented`
  - `eventPollCount`
  - `menuRowCount`
  - `status`
- Fields recommended to remain app-global:
  - `requested`
  - `sdlAvailable`
  - `created`
  - `drawable`
  - `automationControl`
- Fields recommended for deletion or separate future cleanup:
  - `runtimeStateHash`: still top-level and still TSV-owned by `delete`; keep it
    out of FrontendWindowShell and handle through a future delete/rederive
    cleanup.
  - `creativeWorldEpoch`: still top-level and TSV-owned by `ViewportStore`;
    keep it out of E174 and leave it for a dedicated viewport/fly-anchor cleanup
    if planner wants to remove the remaining top-level scalar.
  - Existing store members `inputDevice`, `debugHud`, `creativeAuthoring`,
    `gameplay`, `room`, `saveSession`, `viewport`, and `presentPath` stay as
    store members and are not FrontendWindowShell fields.
- `automationControl` ruling:
  - E164 says `automationControl` stays app-global and must not move into
    FrontendWindowShell.
  - Live usage supports that ruling: `automationControl` has 133 total direct
    `*.window.automationControl` occurrences in `src/app/iggy3d` + `tests/unit`,
    including 127 production occurrences, concentrated in automation execution
    and automation receipt code.
  - Current TSV disagrees: `docs/god_struct_member_ownership.tsv` line 24 still
    assigns `automationControl` to `FrontendWindowShell`.
  - Safest correction: E174 should retarget the TSV row to
    `app-global-remainder`, remove `automationControl` from the target-map
    FrontendWindowShell field list, and make no code move for
    `automationControl`.
- Required command results:
  - `ProductAppWindowState.hpp` candidate scan found the live top-level
    candidates at lines 36, 41-45, 49, 51-54, 61, and 63-67.
  - TSV candidate scan found all shell candidates assigned to
    `FrontendWindowShell`; it also found the `automationControl` contradiction
    at line 24.
  - Full field-name scan over `src/app/iggy3d` + `tests/unit` was run. It is
    noisy for generic names like `status`, so direct `*.window.<field>` counts
    below are the actionable move estimate.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Direct call-site counts for recommended shell fields:
  - Total direct `*.window.<shell-field>` occurrences:
    - 341 in `src/app/iggy3d` + `tests/unit`.
    - 198 in production `src/app/iggy3d` only.
  - Per-field direct occurrence counts across `src/app/iggy3d` + `tests/unit`:
    - `productVulkanMenu`: 121
    - `startup`: 107
    - `launchStatus`: 52
    - `status`: 22
    - `framesPresented`: 7
    - `launchAction`: 6
    - `menuTextDrawn`: 4
    - `packageLoadStatus`: 4
    - `selectedSettingsTab`: 4
    - `openingMenuVisible`: 3
    - `selectedRowDrawn`: 3
    - `eventPollCount`: 2
    - `gamepadMenuSelectUsed`: 2
    - `menuRowCount`: 2
    - `mouseMenuSelectUsed`: 2
  - Hottest production files for the recommended shell fields:
    - `src/app/iggy3d/Operations.cpp`: 69
    - `src/app/iggy3d/receipt/StartupProbeFields.cpp`: 33
    - `src/app/iggy3d/window/RendererLifecycle.cpp`: 23
    - `src/app/iggy3d/window/Loop.cpp`: 17
    - `src/app/iggy3d/receipt/FeedbackSurfaceAutomationVulkanFields.cpp`: 14
    - `src/app/iggy3d/window/FramePresenter.cpp`: 12
    - `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`: 7
    - `src/app/iggy3d/save/Flow.cpp`: 3
    - `src/app/iggy3d/receipt/TailFields.cpp`: 3
    - `src/app/iggy3d/menu/ActionHandlers.cpp`: 3
    - `src/app/iggy3d/ascii_room/Activation.cpp`: 3
- Drafted E174 implementation card outline:
  - Title:
    - `E174 — FrontendWindowShell G1: Move Shell Fields And Reconcile Automation Ownership`
  - Objective:
    - Create `FrontendWindowShell` and move only the 15 shell fields listed
      above into `ProductAppWindowState::frontendShell` or local naming chosen
      by planner.
    - Preserve receipt golden byte-for-byte.
    - Retarget `automationControl` ownership docs to `app-global-remainder`
      without moving code.
  - Field list:
    - `openingMenuVisible`
    - `menuTextDrawn`
    - `selectedRowDrawn`
    - `mouseMenuSelectUsed`
    - `gamepadMenuSelectUsed`
    - `selectedSettingsTab`
    - `launchAction`
    - `launchStatus`
    - `packageLoadStatus`
    - `startup`
    - `productVulkanMenu`
    - `framesPresented`
    - `eventPollCount`
    - `menuRowCount`
    - `status`
  - Do not move:
    - `automationControl`
    - `runtimeStateHash`
    - `creativeWorldEpoch`
    - `requested`
    - `sdlAvailable`
    - `created`
    - `drawable`
    - existing store members
  - Expected touched production files:
    - `src/app/iggy3d/ProductAppWindowState.hpp`
    - new `src/app/iggy3d/window/FrontendWindowShell.hpp`
    - `src/app/iggy3d/Operations.cpp`
    - `src/app/iggy3d/save/Flow.cpp`
    - `src/app/iggy3d/window/RendererLifecycle.cpp`
    - `src/app/iggy3d/window/Loop.cpp`
    - `src/app/iggy3d/window/FramePresenter.cpp`
    - `src/app/iggy3d/window/InputFrame.cpp`
    - `src/app/iggy3d/menu/ActionHandlers.cpp`
    - `src/app/iggy3d/ascii_room/Activation.cpp`
    - `src/app/iggy3d/gameplay/TapeRunner.cpp`
    - `src/app/iggy3d/AppKernel.cpp`
    - `src/app/iggy3d/receipt/StartupProbeFields.cpp`
    - `src/app/iggy3d/receipt/FeedbackSurfaceAutomationVulkanFields.cpp`
    - `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`
    - `src/app/iggy3d/receipt/TailFields.cpp`
  - Expected touched tests:
    - compiler-guided repoints in product window, startup/menu, Vulkan menu,
      creative UI receipt preservation, and creative world launch tests.
  - Docs:
    - `docs/god_struct_member_ownership.tsv`: delete the 15 moved top-level
      rows, add one `frontendShell` row, and retarget `automationControl` to
      `app-global-remainder`.
    - `docs/god_struct_decomposition_target_map.md`: remove
      `automationControl` from #10 and mark FrontendWindowShell done if this is
      the final shell move.
    - `docs/creative_mode/builder_tasks/PRIORITY.md`: update only after gates.
  - Required greps:
    - No old flat storage:
      `rg -n "window\\.(openingMenuVisible|menuTextDrawn|selectedRowDrawn|mouseMenuSelectUsed|gamepadMenuSelectUsed|selectedSettingsTab|launchAction|launchStatus|packageLoadStatus|startup|productVulkanMenu|framesPresented|eventPollCount|menuRowCount|status)\\b" src tests --glob '*.cpp' --glob '*.hpp'`
    - No moved-field declarations in `ProductAppWindowState.hpp`.
    - No old top-level ownership rows for the 15 moved fields in
      `god_struct_member_ownership.tsv`.
    - Confirm `automationControl` remains as `window.automationControl` and is
      not under `frontendShell`.
    - Confirm `runtimeStateHash` and `creativeWorldEpoch` were not touched.
  - Test/golden gates:
    - `cmake --build /Users/kogaryu/iggy3d/build -j10`
    - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`
    - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests`
    - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure`
    - `git -C /Users/kogaryu/iggy3d diff --check`
    - focused trailing-whitespace scan over touched files
    - `git diff -- tests/golden/product_receipt_key_order.golden` must be
      empty
  - `automationControl` TSV retarget:
    - Include in E174.
    - It is a documentation ownership correction only; no source move.
- No source, tests, CMake, receipt golden, or production docs were edited. Only
  this task card was claimed and appended.
- No stage, commit, push, window launch, or broad CTest.
