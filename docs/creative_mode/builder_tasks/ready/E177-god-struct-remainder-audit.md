# E177 - God Struct Remainder Audit

## Status

Ready.

## Objective

Do a read-only current-state audit of the remaining top-level
`ProductAppWindowState` members after E176. Do not implement moves or deletes in
this card.

The store folds are now complete through FrontendWindowShell. The remaining
fields are a smaller mixed set, and the next implementation should be based on a
fresh map rather than assumptions from older audit counts.

## Scope

Inspect:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `docs/god_struct_member_ownership.tsv`
- `docs/god_struct_decomposition_target_map.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`
- direct source/test call sites for the remaining top-level fields

Current expected remaining top-level fields at E176 are:

- app-global/lifecycle candidates: `requested`, `sdlAvailable`, `created`,
  `drawable`, `automationControl`
- existing store members: `frontendShell`, `inputDevice`, `debugHud`,
  `creativeAuthoring`, `gameplay`, `room`, `saveSession`, `viewport`,
  `presentPath`
- cleanup/freshness candidates: `runtimeStateHash`, `creativeWorldEpoch`

## Do Not Edit

- No source changes.
- No tests/CMake changes.
- No receipt golden changes.
- Do not move or delete `runtimeStateHash`.
- Do not move or delete `creativeWorldEpoch`.
- Do not move `automationControl`.
- Do not create implementation cards in `ready/` unless the audit result is
  exact and reviewable. Draft follow-up card text inside this done card instead.

## Required Audit

1. List the current top-level `ProductAppWindowState` members in declaration
   order and count them.
2. Reconcile every current top-level member against
   `docs/god_struct_member_ownership.tsv`.
3. Reconcile the remaining fields against
   `docs/god_struct_decomposition_target_map.md`, especially the claim that the
   final target is a thin composition plus app-global lifecycle bits.
4. Count direct call sites for:
   - `requested`
   - `sdlAvailable`
   - `created`
   - `drawable`
   - `automationControl`
   - `runtimeStateHash`
   - `creativeWorldEpoch`
5. Classify each remaining scalar as:
   - keep app-global;
   - delete/rederive candidate;
   - move into an existing store;
   - needs owner decision before implementation.
6. Specifically answer:
   - Is `runtimeStateHash` still used, and can it be deleted safely in one
     future card?
   - Is `creativeWorldEpoch` correctly owned by `ViewportStore`, or should it
     remain app-global because it is a window-owned generation token?
   - Is `automationControl` correctly app-global, or is it a store candidate
     with too much blast radius?

## Suggested Commands

```sh
sed -n '1,140p' src/app/iggy3d/ProductAppWindowState.hpp
sed -n '1,80p' docs/god_struct_member_ownership.tsv
rg -n "runtimeStateHash|creativeWorldEpoch|automationControl|requested|sdlAvailable|drawable|\\bcreated\\b" src/app/iggy3d tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "runtimeStateHash|creativeWorldEpoch|automationControl|app-global-remainder|delete|ViewportStore" docs/god_struct_decomposition_target_map.md docs/god_struct_member_ownership.tsv docs/creative_mode/builder_tasks/PRIORITY.md
git -C /Users/kogaryu/iggy3d diff --check
```

Use additional `rg` commands as needed, but report the exact commands and
counts.

## Deliverable

Append the audit to this card and move it to `done/`.

The completion brief must include:

- current top-level member count and ordered member list;
- TSV reconciliation result;
- direct call-site counts by field and hottest files;
- classification table for remaining scalars;
- recommendation for the next one or two implementation cards;
- whether any stale wording remains in `docs/god_struct_decomposition_target_map.md`
  or `PRIORITY.md`;
- confirmation that no source/test/CMake/receipt golden files were edited.

## Verification

Run:

```sh
git -C /Users/kogaryu/iggy3d status --short
git -C /Users/kogaryu/iggy3d diff --check
```

No build or broad CTest is required for this read-only audit unless the builder
edits files beyond the task card by mistake.
