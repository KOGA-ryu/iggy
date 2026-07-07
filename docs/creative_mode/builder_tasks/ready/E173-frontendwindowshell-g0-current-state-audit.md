# E173 — FrontendWindowShell G0: Current-State Audit And Ownership Reconciliation

**STATUS: READY.** Parent: `blocked/E164-frontendwindowshell-ruling.md`.

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
