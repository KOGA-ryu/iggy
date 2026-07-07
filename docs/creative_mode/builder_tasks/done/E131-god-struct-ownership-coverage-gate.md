# E131: god-struct ownership coverage gate (the decomposition truth-gate)

## Objective

Make the god-struct decomposition measurable: a **checked-in member→owner map** for every
`ProductAppWindowState` top-level member, plus a **test** that asserts each member is claimed by **exactly one**
target (a Store / the app-global remainder / the delete-set) and that no map entry is stale. A newly-added
member with no ownership decision **fails the test** — forcing the decision instead of growing the junk drawer.

This is the receipt-oracle pattern (`tests/unit/product_receipt_key_order_tests.cpp`) applied to the god-struct.
Source of truth for the assignments: `docs/god_struct_decomposition_target_map.md` §2 + §3 (+ the audit
delete-set).

## Why This Exists

`ProductAppWindowState` (427 LOC, ~154 members) is the substrate every ownership deficit lives on. The
decomposition map named the destination (11 Stores + remainder). Without a gate, we can't tell whether a slice
made progress, and every new feature can silently park another field on the god-struct. This test turns
"are we consolidating?" into a green/red signal and blocks new un-owned fields.

## Required Work

1. **Checked-in owner map** `docs/god_struct_member_ownership.tsv` — one row per top-level member of
   `ProductAppWindowState` (struct body lines 193–425):
   ```
   <memberName>\t<owner>
   ```
   where `<owner>` is a Store name from the decomposition map (`RoomStore`, `CreativeIdentityStore`,
   `CreativeFlyAnchorStore`, `CreativeAuthoringStore`, `SaveSessionStore`, `ViewportStore`, `InputDeviceStore`,
   `GameplayStore`, `DebugHudStore`, `FrontendWindowShell`, `PresentPathStore`), or `app-global-remainder`, or
   `delete`. Populate every member from the map §2/§3 (the map already assigns them; transcribe). Members whose
   store is still provisional (DebugHud/PresentPath/FrontendShell) get their provisional store name — the point
   is *assigned*, not *final*.
2. **Test** `tests/unit/product_god_struct_ownership_coverage_tests.cpp` (register in `cmake/iggy3d_tests.cmake`):
   - Read `src/app/iggy3d/ProductAppWindowState.hpp`; extract top-level member names from the struct body
     (single-line declarations `^  <Type> <name>;` between `struct ProductAppWindowState {` and its closing
     `};`). If a member declaration is multi-line/ambiguous and the regex can't capture it, the test should
     **fail loudly** naming the line (so coverage is never silently under-counted).
   - Read the owner map TSV.
   - Assert **(a)** every extracted member has exactly one TSV row with a valid owner tag; **(b)** every TSV row
     names a member that actually exists (no stale rows); **(c)** print a one-line summary of counts per owner
     (so progress toward DONE is visible: `delete`/`app-global-remainder`/per-store tallies).
   - On mismatch, print the offending member(s) + the fix hint ("add it to god_struct_member_ownership.tsv with
     its target store from docs/god_struct_decomposition_target_map.md").
3. This is a **coverage/telemetry gate only** — it changes NO production behavior and moves NO field. Migration
   happens in the per-store slices; this test just proves nothing is unaccounted.

## Acceptance Notes

- The TSV covers **every** current top-level member (test passes: zero unassigned, zero stale).
- Test registered + green; the per-owner summary prints (e.g. how many members still target each Store vs
  already `delete`/remainder).
- No production file changed; full suite green.

## Do Not

- Do NOT move, delete, or rename any struct member (that is the per-store slices' job).
- Do NOT change production behavior. Do NOT name anything `Kernel`.
- Do NOT stage, commit, or push.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_god_struct_ownership_coverage_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^product_god_struct_ownership_coverage_tests$' --output-on-failure
ctest --test-dir /Users/kogaryu/iggy3d/build
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files changed:
- Member count extracted vs assigned (coverage):
- Per-owner summary (the progress snapshot):
- Multi-line/ambiguous members handled:
- Tests/checks run:
- Concerns/deferred:

---

## Completion Brief - 2026-07-07

- Files changed:
  - `cmake/iggy3d_tests.cmake`
  - `docs/god_struct_member_ownership.tsv`
  - `tests/unit/product_god_struct_ownership_coverage_tests.cpp`
  - `docs/creative_mode/builder_tasks/done/E131-god-struct-ownership-coverage-gate.md`
- Member count extracted vs assigned (coverage):
  - Extracted current top-level `ProductAppWindowState` members: 213.
  - Assigned TSV rows: 213.
  - Coverage result: zero unassigned members, zero stale TSV rows, zero duplicate rows.
- Per-owner summary (the progress snapshot):
  - `CreativeAuthoringStore=86`
  - `SaveSessionStore=32`
  - `GameplayStore=32`
  - `FrontendWindowShell=15`
  - `ViewportStore=12`
  - `InputDeviceStore=11`
  - `app-global-remainder=7`
  - `PresentPathStore=6`
  - `DebugHudStore=5`
  - `delete=4`
  - `RoomStore=3`
- Multi-line/ambiguous members handled:
  - The test accumulates non-comment lines inside `ProductAppWindowState` until `;`, so split string initializers such as stale/pick/wireframe status fields are counted.
  - Any declaration that cannot parse to a single member name fails loudly with the source line and declaration text.
  - Current run found no ambiguous declarations.
- Tests/checks run:
  - `cmake -S /Users/kogaryu/iggy3d -B /Users/kogaryu/iggy3d/build` passed.
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_god_struct_ownership_coverage_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^product_god_struct_ownership_coverage_tests$' --output-on-failure` passed.
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests` printed the 213-row per-owner summary above.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build` passed: 260/260.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files passed.
- Concerns/deferred:
  - `gamepadMenuSelectUsed` appears in both the input-device and frontend-shell prose lists in `docs/god_struct_decomposition_target_map.md`; the TSV assigns it to `InputDeviceStore` with the rest of gamepad device/input state. `mouseMenuSelectUsed` remains `FrontendWindowShell` per the shell list.
  - This card adds only the ownership coverage gate; no production behavior or struct members were changed.
  - `Testing/Temporary/LastTest.log` changed from CTest output and was left untouched.
