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
