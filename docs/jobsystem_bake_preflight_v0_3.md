# JobSystem Preflight v0.3 — measurement + revalidation → **recommend DEFER**

**Author:** Claude (planner). **Date:** 2026-07-07. **Supersedes** [v0.2](jobsystem_bake_preflight_v0_2.md)
(design unchanged; this doc records the §1 magnitude measurement v0.2 gated on, re-anchors every fact to
HEAD, and corrects three claims a 4-agent adversarial revalidation refuted). **Gate-1: NOT ready.**
**Recommendation: DEFER** — the bake freeze is not user-perceptible; this would be correct architecture
arriving early. Deferring is architecturally free (the seam persists) and reversible on a measured trigger.

---

## A. The measurement (the gate v0.2 set for itself)

Captured headless via the no-window bake harness (`refreshProductCreativeBakedActiveRoom` reading
`bakeElapsedMicroseconds`), fixed 60×95 floor + N crates on a grid. Frame budget @60fps = 16,667µs.

| objects | bake time | fraction of a frame |
|--:|--:|--|
| 11 (realistic room) | **822 µs** | 5% |
| 101 | 1,500 µs | 9% |
| 501 (dense) | 2,814 µs | 17% |
| 1501 (extreme, far past any authored room) | 7,574 µs | 45% |

**Scaling is ~linear** (~700µs fixed floor cost + ~4–5µs/object), **hard-bounded** by the greedy-floor/
reachability cap `policy.maxCells = 1,000,000` (`RoomBake.cpp:577`, `GridTooLarge` cutoff). Sub-frame even
at 1500 objects. **The compute freeze is not perceptible on any realistic room.** (Caveat: this times the
compute bake only — the harness's `activationHook` is a no-op, so the reasoning-graph install cost in §C.2
is *not* in these numbers.)

## B. What is SOLID at HEAD (deferring costs nothing architecturally)

The four premises a job system depends on all hold — verified against HEAD:
1. **Compute/install boundary is clean.** `buildRoomAssetFromCreativeDocument` (`RoomBake.cpp:664-770`)
   returns a self-contained value `CreativeRoomBakeResult`, touches no GPU/session/window/editor state; all
   mutation is quarantined in the separate main-thread `installBakedRoom` (`Operations.cpp:702-727`). A job
   system splits exactly at `execute()` (compute `Operations.cpp:657` → install `:612`).
2. **`CreativeDocument` is a Rule-of-Zero value container** (`Document.hpp:161-247`; only `=default` ctor;
   all members value types incl. `CreativeObject`). The §6 snapshot is a sound one-line deep copy —
   `CreativeDocument snapshot = *request.document;`.
3. **The only worker-reachable global is `kDescriptors`** (`ObjectDescriptor.cpp:1697`) — `constexpr`,
   immutable, race-free. No mutable static/singleton/logger in the compute tree. *This is the key
   de-risking fact for the engine's first-ever thread.*
4. **A clean drain seam exists** in `Loop.cpp` (see §C.3 for the corrected line).

## C. Three gaps v0.2 understated (found by adversarial revalidation)

1. **BLOCKER — stale-reject-at-apply (§5.6) does not exist.** `installBakedRoom` applies
   *unconditionally*; `result_.bakedDocumentRevision` is captured (`Operations.cpp:654`) but never compared
   to the live `document.revision()`. The `creativeBakedRoomStale` flag (`ProductAppWindowState.hpp:269-274`)
   is a same-frame diagnostic driving auto-rebake, **not** an apply gate. The invariant is only *vacuously*
   true today because the engine is single-threaded. **Must be BUILT in slice 1** (small guarded add — the
   field to compare already exists), not "wired." v0.2 presents it as partly-existing; it isn't.
2. **MAJOR — the freeze is underscoped.** The install hook runs `buildReasoningGraph` (via
   `activateCreativeReasoningGraph`, `Operations.cpp:712-716`) — O(n²) over nodes with a physics raycast per
   candidate edge (`ReasoningGraph.cpp:189-202`) — **synchronously on the main thread**, and slice 1 offloads
   only the room bake. So slice 1 would **not eliminate the full freeze**. Nodes = anchors + patrol
   waypoints (tens), so it's modest at typical scale, but it is **unmeasured** and must be measured + either
   moved to the worker (its inputs are worker-available; `setReasoningGraph` mutation stays install-side) or
   documented as a known residual.
3. **MAJOR — drain-idiom miscite.** v0.2 §6 cites a `pendingExecutionSequences` drain "at `Loop.cpp:257`"
   as the mirror. That vector lives on the runtime `SessionState` (`SessionState.hpp:68`) and drains on the
   **simulation lane** (`Session.cpp:1343/1389`) — `Loop.cpp` has **no** drain of any kind today. The correct
   insertion seam is **`Loop.cpp:263`** (after the input-frame call closes at `:262`, before render prep at
   `:264-289`), not `:257`.

## D. Corrected anchors (v0.2 line-refs drifted ~40–50 lines vs HEAD)

| symbol | v0.2 cite | HEAD |
|---|---|---|
| bake compute call + self-timing | `Operations.cpp:694` / `:686-697` | **`Operations.cpp:655-658`** (compute call `:657`) |
| `buildRoomAssetFromCreativeDocument` | `RoomBake.cpp:664-770` | `RoomBake.cpp:664-770` (matches) |
| `installBakedRoom` | `Operations.cpp:740-766` | **`Operations.cpp:702-727`** |
| `refreshProductCreativeBakedActiveRoom` | `Operations.cpp:1590-1608` | **`Operations.cpp:1544-1562`** |
| `CreativeRoomBakeRequest.document` ptr | `RoomBake.hpp:22` | **`RoomBake.hpp:34`** (populated live `Operations.cpp:641`) |
| input-frame call / present | `Loop.cpp:242-256` / `:285` | **`Loop.cpp:248-262`** / **`:291-295`** |
| drain seam | `Loop.cpp:257` | **`Loop.cpp:263`** |
| trigger A / B | `InputFrame.cpp:1088-1098` / `1320-1329` | `InputFrame.cpp:1091-1102` / `1323-1332` |

## E. Decision — DEFER, with a measured revisit trigger

**Recommendation:** do **not** spend the engine's first thread on a sub-millisecond-to-few-millisecond,
hard-bounded, non-perceptible compute cost. Building now = mutex/join/shutdown-ordering/TSan complexity
plus two build-required gaps (§C.1, §C.2) to solve a problem the measurement says we don't have yet.

**Revisit (build slice 1) when any holds — and re-run this measurement to confirm:**
- a real captured bake crosses **~5 ms** on a *plausible authored* room (not a synthetic 1500-crate grid); or
- the **reasoning-graph** build (§C.2), once measured, crosses perceptible on anchor/waypoint-dense rooms; or
- **auto-rebake-on-edit** (`InputFrame.cpp:1323-1332`) fires the bake at interactive frequency, where even
  1–2 ms compounds into visible stutter while dragging.

The recon is banked: when the trigger fires, the design (v0.2) + these corrections + the three gap-fixes are
a ratifiable Gate-1 in one pass.

## F. If we build anyway (the ratifiable path)

v0.2's design stands; add to slice-1 scope: (1) **build** the revision-compare stale-reject gate at the
drain; (2) place the drain at `Loop.cpp:263`; (3) decide the reasoning-graph residual (measure → move or
document). Then G1–G7 per v0.2 §12. Gate-1 would need the user to accept that slice 1 relieves only the
compute freeze unless the reasoning-graph is also offloaded.
