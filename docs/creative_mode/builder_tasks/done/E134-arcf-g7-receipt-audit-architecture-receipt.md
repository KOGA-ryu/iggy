# E134 (E-ARCF-G7): freshness store finish — receipt instrumentation + ownership audit + architecture receipt

## Objective

Gate 7 (the finish) of the freshness guard: **surface the freshness `reasonCode` into the receipt** (so a
stale/rebake leaves an observable trail — the core-spine instrumentation requirement), run the **naming +
ownership audit**, and write the **architecture receipt** next to the store. After this the
`ActiveRoomCollisionFreshnessStore` slice is complete.

**Gated spine work — the finish gate.** Owner design:
`docs/active_room_collision_freshness_preflight_v0_2.md` (Gate table G7; §8 reasonCode enum; `core_spine_work_rules.md:57-78`).

## Why This Exists

`core_spine_work_rules.md:63-64` requires instrumentation ("no counters means debugging by incense") + an
architecture receipt (why · owns · not-owns · thread · shutdown · first consumer · known limitation). G7
delivers both and closes the audit checklist, so the store is a finished, observable, documented spine piece.

## Required Work

1. **Store the frame-boundary freshness result** so the receipt can read it. The frame-boundary `ensure`
   (`window/InputFrame.cpp:526`) currently discards its `ProductActiveRoomCollisionFreshnessResult`. Add ONE
   window field to hold the last result's observable bits (e.g.
   `ProductActiveRoomCollisionFreshnessResult activeRoomCollisionFreshness;` on `ProductAppWindowState`, or a
   flat `{rebaked, reasonCode}` pair) and assign it from that `ensure` call. Keep it minimal.
2. **Emit it in the receipt** — in `src/app/iggy3d/receipt/ActiveRoomFields.cpp`, append fields for the
   freshness `reasonCode` (and optionally `rebaked`) alongside the existing `active_room_collision_*` fields.
   Use the closed reasonCode enum (§8): `skipped_fresh / rebaked_room / rebaked_session / rebaked_both /
   rebaked_unloaded / rebaked_empty`.
3. **Two truth-gates will fire — satisfy BOTH:**
   - **Receipt key-order oracle** (`product_receipt_key_order_tests`) will fail on the new field(s) — that is
     expected. **Regenerate the golden**: `RECEIPT_GOLDEN_REGEN=1 ./build/product_receipt_key_order_tests` from
     the repo root, and commit the updated `tests/golden/product_receipt_key_order.golden`.
   - **God-struct coverage gate** (`product_god_struct_ownership_coverage_tests`) will fail on the new window
     member — **add it to `docs/god_struct_member_ownership.tsv` with owner `RoomStore`** (it's part of the
     collision freshness truth).
4. **Naming + ownership audit** (`core_spine_work_rules.md:68-78`): confirm `Store`/`ensure` did not inflate
   scope; confirm no stray `buildProductActiveRoomCollision(` outside the store + the two definitions + the
   justified `EditingState.cpp:129` mirror; confirm `bumpActiveRoomRevision` is the sole revision mutator.
   Record the greps in the brief.
5. **Architecture receipt** — a comment block at the top of
   `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.cpp`: **why it exists · what it owns · what it
   does NOT own · thread rules · shutdown rules · first consumer · known limitation** (the coarse
   `currentStateHash` conservatism, and the documented-not-enforced I7 precondition).
6. **C2 — final I7 re-audit**: grep `entity.active`/`setActive` outside the hashed session command path; must be
   empty. Record.

## Acceptance Notes

- Freshness `reasonCode` appears in the receipt (`ActiveRoomFields.cpp`), driven by the frame-boundary result.
- **Receipt golden regenerated + committed**; `product_receipt_key_order_tests` green with the new field(s).
- **New window field added to the ownership TSV (`RoomStore`)**; `product_god_struct_ownership_coverage_tests`
  green.
- Naming/ownership audit clean; coverage grep empty (modulo the justified mirror); `bumpActiveRoomRevision`
  sole mutator.
- Architecture receipt present at the store. C2 I7 re-audit recorded.
- Full suite green (`product_creative_no_window_bake_scenario_tests` still passes — the bake path is unchanged;
  only a new receipt field is added).

## Do Not

- Do NOT change the freshness logic or the reasonCode decision order (that shipped in G3). Do NOT reorder
  existing receipt fields (append only). Do NOT name anything `Kernel`.
- Do NOT stage, commit, or push.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
RECEIPT_GOLDEN_REGEN=1 ./build/product_receipt_key_order_tests   # from repo root, after the receipt edit
ctest --test-dir /Users/kogaryu/iggy3d/build
grep -rn "buildProductActiveRoomCollision(" --include='*.cpp' /Users/kogaryu/iggy3d/src | grep -v test \
  | grep -vE "ActiveRoomCollisionFreshnessStore.cpp|ActiveRoomCollision.cpp:11[0-9]|ActiveRoomCollision.cpp:12[0-9]|EditingState.cpp:129"   # expect empty
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files changed:
- Window field added (name + owner tsv row):
- Receipt fields emitted (keys + enum):
- Receipt golden regenerated (field count before→after):
- Naming/ownership audit greps (result):
- Architecture receipt (location):
- C2 I7 re-audit:
- Suite:
- Concerns/deferred:

## Completion Brief - Codex

- Files changed:
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/receipt/ActiveRoomFields.cpp`
  - `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.cpp`
  - `src/app/iggy3d/gameplay/TapeRunner.cpp`
  - `docs/god_struct_member_ownership.tsv`
  - `tests/golden/product_receipt_key_order.golden`
- Window field added (name + owner tsv row): added `ProductActiveRoomCollisionFreshnessResult activeRoomCollisionFreshness` on `ProductAppWindowState`; ownership row is `activeRoomCollisionFreshness	RoomStore`.
- Receipt fields emitted (keys + enum): appended `active_room_collision_freshness_rebaked` and `active_room_collision_freshness_reason_code` after existing `active_room_collision_*` fields. The reason code is sourced from the store result and stays in the closed set `skipped_fresh`, `rebaked_room`, `rebaked_session`, `rebaked_both`, `rebaked_unloaded`, `rebaked_empty`.
- Receipt golden regenerated (field count before->after): `RECEIPT_GOLDEN_REGEN=1 ./build/product_receipt_key_order_tests` wrote 1031 fields; prior golden line count was 1029. New golden rows are `active_room_collision_freshness_rebaked	false` and `active_room_collision_freshness_reason_code	skipped_fresh`.
- Naming/ownership audit greps (result): no stray `buildProductActiveRoomCollision(` production callsites outside `ActiveRoomCollisionFreshnessStore.cpp`, `ActiveRoomCollision.cpp` definitions, and the justified `room_editor/EditingState.cpp:129` mirror. Production `activeRoomRevision` direct-write audit is clean outside the default field and `bumpActiveRoomRevision` owner implementation; the legacy TapeRunner no-window adapter now seeds its temporary window via `bumpActiveRoomRevision(window)`.
- Architecture receipt (location): comment block added at the top of `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.cpp`, covering why, owns, not-owns, thread rules, shutdown rules, first consumer, and known limitations.
- C2 I7 re-audit: clean. `entity.active =` scan outside expected save/runtime fixture paths produced no output. `setActive(` scan outside expected world/interaction/creative seams produced no output.
- Suite:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
  - `RECEIPT_GOLDEN_REGEN=1 ./build/product_receipt_key_order_tests` passed and regenerated the golden.
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_receipt_key_order_tests product_god_struct_ownership_coverage_tests product_active_room_collision_tests product_creative_no_window_bake_scenario_tests product_gameplay_tape_runner_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_receipt_key_order_tests|product_god_struct_ownership_coverage_tests|product_active_room_collision_tests|product_creative_no_window_bake_scenario_tests|product_gameplay_tape_runner_tests)$' --output-on-failure` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build` passed, 260/260.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over source/docs/card files passed. The regenerated golden still contains pre-existing intentional blank-value tab rows for empty receipt values; `diff --check` is clean.
- Concerns/deferred: no stage/commit/push. `Testing/Temporary/LastTest.log` remains dirty from CTest output and was left untouched.
