# E140 — `activeRoom` → `RoomStore` — PREFLIGHT / RECON

**STATUS: BLOCKED — do not claim. This is a recon verdict, not a buildable card.**
Verdict: **hold and re-rank** (see §5). The ownership payoff the audit projected is
already banked by the collision freshness store; the residual is a high-churn
mechanical regroup with no ownership win. Kept in `blocked/` so the builder cannot
pull it and so the finding is on the record.

Traced with `docs/ownership_trace_method.md` (grep → read → classify). Run in
parallel with the in-flight `activeCreative` delete (E136–E139) because it touches a
disjoint field set and produced **no code edits** — only this document.

---

## 1. What the audit claimed (and why it was directional only)

`docs/ownership_deficit_audit.md` ranked `activeRoom` **#1, score 9.0, rated M**, with
the plan: *"absorb `{activeRoom, activeRoomCollision, revision}` into the collision
store's G4/G5 gates, killing the window↔roomEditing duplicate at near-zero marginal
cost."* The 9.0 was earned **entirely by the free ride** — "it rides in on a fix that
is already touching every paired site."

That free ride no longer exists. The collision preflight (`docs/active_room_collision_freshness_preflight_v0_2.md`, §11)
deliberately scoped the collision slice to **rebake removal only** and pushed
`activeRoom` out to *"its OWN preflight (NOT folded into collision G5)."* So the
near-zero marginal cost is gone; this slice must now stand on its own legs. It does
not. Same lesson as #2 (`activeCreative`, rated "cheapest S", proved L): **audit
S/M/L and scores are directional; the preflight is the truth.**

## 2. The trace (grep → read)

**Owner cluster on the god-struct** (`ProductAppWindowState.hpp:255–259`):

```
ProductActiveRoomState                 activeRoom;                     // 255
std::uint64_t                          activeRoomRevision = 0;         // 256  (freshness guard, shipped)
ProductActiveRoomCollisionState        activeRoomCollision;            // 258
ProductActiveRoomCollisionFreshnessResult activeRoomCollisionFreshness;// 259
```

**The "duplicate"** (`EditingState.hpp:19–20`, inside `ProductRoomEditingState`):

```
ProductActiveRoomState          activeRoom;             // 19
ProductActiveRoomCollisionState activeRoomCollision;    // 20
+ 5 derived counts (24–28), all filled from the nested copy
```

**Inventory** (`src apps tests`):

| grep | count | meaning |
|---|---|---|
| `\.activeRoom\b` non-test | 62 | reader surface in product code |
| `\.activeRoom\b` incl. tests | 287 | **225 of these are tests** — the churn tax |
| `window\.activeRoom` | 252 | the dominant form — single-owner truth-reads |
| `(roomEditing\|editing\|state)\.activeRoom` | 24 | the nested copy |
| `activeRoom =` writers (non-test) | ~16 | see below |
| `ProductActiveRoomState` type refs | 73 | type-level footprint of any regroup |

**The ~7 window-copy producers** (all promote *into* the single window owner):
`Operations.cpp:184/188/284/454/722/745-748/1328/1664`, `Activation.cpp:72`,
`TapeRunner.cpp:223`, `AutomationRoomEditing.cpp:90`.

**The producer + promotion pair:**
- `EditingState.cpp:119` — `state.activeRoom = buildProductActiveRoomFromRoomAuthoringSnapshot(state.authoringSnapshot)` — builds the **editor's own** room from its controller.
- `AutomationRoomEditing.cpp:90` — `copyRoomEditingStateToWindow` promotes `state.activeRoom → window.activeRoom`, then `bumpActiveRoomRevision` + `ensureActiveRoomCollisionFresh`. This is publish, not shared ownership.

## 3. Classification (the read that overturns the audit)

| site class | verdict |
|---|---|
| `window.activeRoom` ×252 | **Truth read/write of a single-owner published value.** The window solely owns it; ~7 producers *promote* into it. `activeRoomRevision` already guards the one derived consequence (collision). No shared-ownership bug. |
| `roomEditing.activeRoom` (nested) | **NOT a redundant cache — a producer working-set.** Built from `controller.snapshot()`; the editor legitimately needs its own room *before* promotion. **Deleting it breaks the editor.** |
| receipt reads of `roomEditing.activeRoom.*` (`WorldAuthoringFields.cpp:163–175`, 7 fields) | **Distinct projection.** Answers "what has the editor authored" — a *different question* from `window.activeRoom` ("what room is the app showing"). Not duplication; both are legitimate, non-interchangeable receipt inputs. |
| `fillCounts` internal reads (`EditingState.cpp:12–16`) | Internal to the producer. Fine. |

**No hit conflates the two.** Nothing reads `roomEditing.activeRoom` expecting it to
equal `window.activeRoom`. The "two divergent copies" the audit feared are a
**producer copy** and a **published copy** — divergence is *correct* (the editor can
hold an in-progress room the app hasn't adopted). The real staleness bug — the
collision derived from an un-tokened room — **was the collision store's job, and it is
done.**

## 4. So what is actually left in #1?

Two separable things the audit conflated into one 9.0:

- **(A) Structural move-off-god-struct** — regroup `{activeRoom, activeRoomRevision,
  activeRoomCollision, activeRoomCollisionFreshness}` into a `RoomStore` sub-struct on
  the window. Pure field-count reduction toward the decomposition goal
  (`docs/god_struct_decomposition_target_map.md`). **Cost: 252 window refs + 225 test
  sites rewritten** (`window.activeRoom` → `window.room.activeRoom` or a `room()`
  accessor). Compiler-guarded, mechanical, low risk — but the **churniest** pending
  slice, and it buys **zero ownership** (the value already has one owner).

- **(B) Ownership kill** — *there is none left to make here.* The freshness deficit is
  banked; the nested copy is load-bearing and must stay. (B) is a no-op.

## 5. Verdict — hold, re-rank

**#1 as an "ownership kill" is already banked. As a standalone it is a high-churn,
zero-ownership mechanical regroup — the wrong thing to spend the next slice on.**

Recommended re-rank of the pending ownership queue:

1. **#2 `activeCreative` delete** — IN FLIGHT (E136–E139). Real mirror kill.
2. **#3 `creativeFly` anchor** — **the next real ownership kill.** Unfixed genuine
   freshness deficit in the exact exemplar shape already proven by the collision
   store: 3 writers set the anchor with different provenance, none stamps who/when
   (`docs/ownership_deficit_audit.md` row 3). Moderate churn, high ownership value.
   **This should be the next preflight, not #1.**
3. **#1 `activeRoom` regroup (this doc)** — defer until the decomposition is *actually
   gating* the kernel on-ramp. When it is, run it as an explicit **structural** card
   with eyes open on the 225-test churn — **and an explicit "do NOT delete
   `roomEditing.activeRoom`; it is the editor's producer copy" guardrail**, or a
   builder will re-house the editor's room into a bug.

## 6. If/when #1 is finally built — the guardrails

- Preserve `roomEditing.activeRoom` and its receipt reads verbatim; this slice moves
  only the **window's** four fields.
- Prefer an accessor (`window.room()`) over a raw member rename to keep the 252-site
  diff reviewable and to give the future `RoomStore` a seam.
- Regenerate the receipt golden (`RECEIPT_GOLDEN_REGEN=1`) and the god-struct
  ownership coverage TSV; both truth-gates fire by design on the field move.
- Negative-grep gate at delete-last: `window\.activeRoom\b` → 0 (all migrated to the
  accessor/store), `roomEditing\.activeRoom` → **unchanged** (must survive).
