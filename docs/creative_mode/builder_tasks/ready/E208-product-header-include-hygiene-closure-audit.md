# E208: Product Header Include Hygiene Closure Audit

## Status

Ready.

## Context

E201-E207 removed the obvious unnecessary `ProductAppWindowState.hpp`
transitive includes from production and test-support headers.

Current quick scan after E207 shows only three `.hpp` files still directly
including `app/iggy3d/ProductAppWindowState.hpp`:

- `src/app/iggy3d/AppKernel.hpp`
- `src/app/iggy3d/window/Loop.hpp`
- `tests/unit/ProductAsciiRoomWindowTestSupport.hpp`

This card is a read-only closure audit. Its job is to prove whether the
include-hygiene lane should close, not to make another code change.

## Objective

Classify the remaining direct `ProductAppWindowState.hpp` header includes and
state whether any safe follow-up include-hygiene implementation card remains.

## Scope

Read-only. Do not edit source, tests, CMake, fixtures, receipt golden, or
production docs.

Allowed file move/edit:

- this task card only

Inspect at minimum:

- `src/app/iggy3d/AppKernel.hpp`
- `src/app/iggy3d/window/Loop.hpp`
- `tests/unit/ProductAsciiRoomWindowTestSupport.hpp`
- `docs/creative_mode/builder_tasks/PRIORITY.md`
- the E201-E207 done cards if needed for context

## Required Work

1. Run the current direct-header scan:

   ```sh
   rg -l '#include "app/iggy3d/ProductAppWindowState.hpp"' /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.hpp' | sort
   ```

2. For each remaining header, classify whether the complete type is required:
   - by-value member or parameter;
   - local object construction in an inline helper;
   - field read/write in an inline helper;
   - or forward-declarable reference/pointer-only use.
3. Confirm whether `AppKernel.hpp` and `window/Loop.hpp` still need a complete
   type because they store `ProductAppWindowState` by value.
4. Confirm whether `ProductAsciiRoomWindowTestSupport.hpp` still needs a
   complete type because it constructs a `ProductAppWindowState` and writes
   fields in an inline helper.
5. Run a broader count for reporting:

   ```sh
   grep -rl 'app/iggy3d/ProductAppWindowState.hpp' /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/apps /Users/kogaryu/iggy3d/tests | wc -l
   ```

6. Run:

   ```sh
   git -C /Users/kogaryu/iggy3d diff --check
   ```

## Decision Requirements

Report one of:

- **Lane closed:** all remaining header includes require complete
  `ProductAppWindowState`; no safe follow-up implementation card should be
  released for this lane.
- **Follow-up needed:** name the exact header(s), explain why they are still
  forward-declarable, and draft the next narrow implementation card.

If the lane is closed, include a short suggested `PRIORITY.md` wording update
for reviewer/planner to apply later, but do not edit it in this audit card.

## Completion Brief Requirements

Report:

- card moved to done;
- direct-header scan output;
- broad includer count;
- per-header classification;
- lane decision;
- any suggested priority wording update;
- `diff --check` result;
- confirmation that no source, test, CMake, fixture, receipt golden, production
  docs, staging, commit, push, or window launch was performed.
