# Gate-0 PREFLIGHT — activeRoom → activeRoomCollision derived-truth freshness guard (v0.2)

**Status:** Gate 0 design — **pending Gate-1 ratification** (reviewer signs §1–§12). Gates the implementation.
**Doctrine:** `docs/core_spine_work_rules.md`. **Template shape:** `docs/jobsystem_bake_preflight_v0_2.md`.
**Classification:** core-spine (window/session derived-truth coherence). Paranoia, not casual.
**Slice family:** complexity-audit finding #4 (`docs/complexity_audit_v0_1.md`). It is the single-threaded
*on-ramp* to the discipline the future job system generalizes — *one owner ensures a derived truth is fresh
before any reader consumes it* — but it is **not** the job system and **not** async (§11).

**v0.2 changes from v0.1** (Gate-0 critique + independent verification): (1) the mid-tick rebake is
reclassified from "use-after-free" to a **mid-frame stale read of valid memory** — verified below; (2) the
`revision` counter moves **onto the window** so whole-struct `activeRoom` copies cannot stomp it; (3)
`ensureActiveRoomCollisionFresh` is the **sole provenance-stamper**, resolving the I3 contradiction; (4) a
new invariant pins the session-hash no-bypass precondition; (5) a TapeRunner equivalence lemma; (6) a closed
`reasonCode` enum + decision order; (7) disambiguated multi-write bump discipline.

## Gate 1 — RATIFIED 2026-07-07 (with conditions)

Reviewer signed §1–§12. Approved decisions: **(1)** the frame-boundary seam; **(2)** I7 accepted as a named
load-bearing precondition; **(3)** staged bake removal (direct writer-bakes may remain through G4; G5 must
remove them and prove readers now depend on the freshness seam); **(4)** naming — with a tweak.

**Ratification conditions (binding on all downstream gates):**
- **C1 (naming):** the type/path is **`ActiveRoomCollisionFreshnessStore`** (an owned freshness store). Where
  the body below or the G2 card writes `ActiveRoomCollisionFreshness.{hpp,cpp}`, read
  `ActiveRoomCollisionFreshnessStore.{hpp,cpp}`. The public verb remains `ensureActiveRoomCollisionFresh`; the
  POD remains `ProductActiveRoomCollisionFreshnessResult`. No `Kernel`.
- **C2 (I7 re-audit every gate):** because I7 is documented, not structurally enforced, **every gate G2–G7
  must re-run the I7 audit** (grep for `entity.active`/`setActive` writes outside the hashed session command
  path — must stay empty) until I7 is either structurally enforced or the session-hash dependency is removed.
- **C3 (removal checklist):** the direct-rebake callsite inventory is produced at
  `docs/active_room_collision_rebake_removal_checklist.md` — the removal checklist for G4/G5. G2 re-runs the
  greps and reconciles drift; G2 does not remove any callsite.

---

**Chosen name — `ActiveRoomCollisionFreshnessStore` (an owned freshness `Store` + one `ensure` verb).**
One-line responsibility: *the single owner of the `activeRoom → activeRoomCollision` derived-truth pair — it
stamps every collision bake with the room revision + session hash it was built from, and rebakes on demand
iff either has drifted, so no reader observes a stale blob.* Naming compliance (`core_spine_work_rules.md:20-25`):
**NOT `Kernel`** — a `Store` owns the freshness provenance; the public API is one free function plus one bump
helper. (Alt `Scheduler` rejected — it schedules nothing across time; it is a synchronous dirty-check over
owned data, i.e. a `Store`.)

---

## 1. Problem — the concrete break today

`window.activeRoomCollision` is a **derived truth** (a `SpatialSurfaceSet` + counts) computed from two inputs:
(a) room geometry in `window.activeRoom.room`, and (b) live session door state (`SessionState` entity
`active` flags). It is rebuilt at **~15 call sites** from a **~12-writer** input, with **NO revision or dirty
guard**. Every mutation site must *remember by convention* to rebake collision before the next reader touches
it. Two concrete breaks exist in shipping code:

- **Mid-frame, order-dependent stale read.** `gameplay/Controller.cpp:2236-2241` (inside
  `submitProductGameplayCommand`) rebakes collision **only** when `kind == Interact && accepted &&
  gameplayTickAdvanced`. Any *other* accepted+ticked command that mutates a door's `active` (e.g. the sibling
  `Attack` branch at `Controller.cpp:2244`, or a scripted door toggle) advances `session.state()` but leaves
  collision built from the **pre-tick** world. Within a single `applyProductGameplayActions` frame the
  wall-run phase (`Controller.cpp:1166`, via `:2436`) reads `*request.collisionSurfaces`, so **which surface
  set a reader sees depends on handler ordering within the frame** — a door opened by a non-Interact command
  stays a solid blocker for gameplay until some later Interact happens to rebake.

  **This is a stale read of VALID memory, not a use-after-free** (v0.1 misdiagnosed it; verified for v0.2):
  `productActiveRoomCollisionSurfaces` returns `&collision.surfaces` (`ActiveRoomCollision.cpp:133`) — the
  address of the `SpatialSurfaceSet surfaces` **member** (`ActiveRoomCollision.hpp:26`) of the long-lived
  `window.activeRoomCollision`. The mid-tick `window.activeRoomCollision = build...()` at
  `Controller.cpp:2240` assigns *into that existing member in place* — it swaps the vector's internal buffer
  but never relocates or frees the pointed-to object. The later deref reads a fully-constructed (just-rebaked)
  set. So the defect is **wrong-timing data, not memory unsafety**; the guard's fix is correctness/determinism,
  and the pre-slice baseline is well-defined (tests can diff against it — §10).

- **Pair-assignment is convention, not structure.** `activeRoom` and `activeRoomCollision` are written as two
  independent assignments at every writer (`Operations.cpp:452-453` clear; `installBakedRoom` `:748-749`;
  `activateProductAsciiRoomPreview` `Activation.cpp:71-72`). A writer that sets `window.activeRoom` without the
  paired collision line leaves a **ready collision pointing at the wrong (or unloaded) room** — a
  `querySurfaceCount>0` blob whose `activeRoom.loaded==false`. Stale-cases **(a)** (room replaced) and **(c)**
  (cleared room, stale-ready collision).

No field on either struct lets a reader *detect* staleness: `ProductActiveRoomState`
(`ActiveRoomState.hpp:16-38`) and `ProductActiveRoomCollisionState` (`ActiveRoomCollision.hpp:12-27`) share
only a copied `roomId` **string** (`ActiveRoomCollision.cpp:18`), stable per-room and unchanged when the *same*
room is re-baked with new geometry. `RoomAsset.version` is dead (hardcoded `1`, `RoomBake.cpp:671`, never
bumped). Correctness rests entirely on 12 writers never forgetting. **That is the pain.**

## 2. Existing path — real files + symbols

**Derived-truth structs (no freshness token exists):** `gameplay/ActiveRoomState.hpp:16-38`
(`ProductActiveRoomState`; `roomId` :21, embedded `RoomAsset room` :36) · `gameplay/ActiveRoomCollision.hpp:12-27`
(`ProductActiveRoomCollisionState`; copies `roomId`; no provenance) · `gameplay/ActiveRoomCollision.cpp:85-126`
(`buildProductActiveRoomCollisionImpl` + one-arg **nullptr** overload `:117-119` (no door filtering) and two-arg
**runtime** overload `:122-125` (filters doors via `roomWithRuntimeFilteredSurfaces` `:52-83`, reading
`owner->active` `:73-76`)).

**ACTIVE-ROOM writers (~11 sites; each pairs a collision assignment by hand):**
`Operations.cpp` — `createProductSessionFromPackage` (:138; clears at :183, sets :187, collision :189-190),
`createCreativeBlankSession` (:226; ~:283-287), `clearProductGameplayLaunchState` (:447-455; terminal clear
:452-453), `ProductCreativeBakedActiveRoomRefreshExecutor::{handleRejectedBake (:714), installBakedRoom
(:740-749)}`, `launchProductNewWorld` (:1288; room :1327 + collision :1328-1329), `launchProductSaveSlot`/
`launchProductContinueSave` (:1610+; `buildProductActiveRoomFromSavedAuthoredRoom` :1663, collision :1677-1679);
`ascii_room/Activation.cpp:71-72` (no-session bake :72) then `:117-118` (with-session re-bake after
`Session::create`); `automation/AutomationRoomEditing.cpp:88-89` `copyRoomEditingStateToWindow` **(whole-struct
copy — `window.activeRoom = state.activeRoom`; the revision-stomp hazard, §5-I2)**; `room_editor/EditingState.cpp:119/129`
`buildProductRoomEditingState` (writes `ProductRoomEditingState`, NOT window; collision baked WITHOUT session :129).

**COLLISION-only extra writers (the ad-hoc rebakes this slice subsumes):** `gameplay/Controller.cpp:2236-2241`
mid-tick Interact rebake **← delete at G5**; `gameplay/TapeRunner.cpp:221-227` `refreshActiveRoomCollision`
(unconditional; called :493) **← redirect to the guard at G5**.

**Sync-dependent READERS:** `receipt/ActiveRoomFields.cpp:6-64` · `window/InputFrame.cpp:525-526` (captures
`const SpatialSurfaceSet* collisionSurfaces = productActiveRoomCollisionSurfaces(window.activeRoomCollision)`,
threads by value) · `gameplay/Controller.cpp:732/1166/1247/1293` · `gameplay/ProjectionRefresh.cpp:742-746,803-804`
· `view/PrimitiveDrawList.cpp:706-720` (door markers — cross-reads BOTH truths) · `gameplay/ScriptedDriver.cpp:75/101`
· `gameplay/TapeRunner.cpp:209/211-213/557` · `automation/AutomationGameplay.cpp:68/90`.

**Freshness inputs already in the system:** `runtime/session/SessionState.hpp:118` `currentStateHash` (uint64)
+ `:78` `stateHashDirty`; `entity.active` participates (`replay/StateHash.cpp:122-128`). Recomputed +
`stateHashDirty` cleared synchronously at the end of every tick/submitCommand via `markDirtyAndHash`
(`Session.cpp:1345/1379/1390/1508`). **← reuse for the session half; no new session field.** `window.runtimeStateHash`
(`Operations.cpp:194`) tracks session but NOT room geometry — insufficient alone.

**Behavioral anchor:** `tests/unit/product_creative_no_window_bake_scenario_tests.cpp` (real window+session,
`NoWindow`; asserts `activeRoomCollision.ready` and `querySurfaceCount==3` :168-171) — E121. Stays green unchanged.

## 3. Minimal types — smallest possible set

No new class. **One window-owned counter + two provenance fields + one bump helper + one owner verb.**

| Symbol | Kind | One-line responsibility | Ownership |
|---|---|---|---|
| `ProductAppWindowState::activeRoomRevision` | `std::uint64_t` (new, default 0) | Monotonic room-geometry generation for the **window's** active room. Lives on the window, NOT inside `activeRoom`, so whole-struct `activeRoom` copies (`AutomationRoomEditing.cpp:88`) cannot stomp it. | Field of `window`. |
| `ProductActiveRoomCollisionState::bakedFromRoomRevision` | `std::uint64_t` (new, default 0) | The `window.activeRoomRevision` this blob was baked from. `0` = unstamped ⟹ always stale. | Field of `window.activeRoomCollision`. |
| `ProductActiveRoomCollisionState::bakedFromSessionHash` | `std::uint64_t` (new, default 0) | The `SessionState.currentStateHash` this blob filtered doors against; `0` = no session. | Field of `window.activeRoomCollision`. |
| `bumpActiveRoomRevision(ProductAppWindowState&)` | free fn (`ActiveRoomState.*`) | The **only** sanctioned way to advance the counter: `++window.activeRoomRevision`. Every writer calls it after the FINAL `activeRoom` write (§ G4 discipline). Copy-safe: it advances the window counter, never reads the copied struct. | Ops helper. |
| `ensureActiveRoomCollisionFresh(ProductAppWindowState&, const Session*)` | free fn (new `gameplay/ActiveRoomCollisionFreshness.*`) | The **single owner + sole provenance-stamper**: if `(bakedFromRoomRevision, bakedFromSessionHash) != (activeRoomRevision, effectiveSessionHash)`, rebake + stamp; else no-op. | The Store's public verb. |
| `ProductActiveRoomCollisionFreshnessResult` | POD | Instrumentation `{bool rebaked; uint64 observedRoomRevision; uint64 observedSessionHash; std::string reasonCode;}` (`core_spine_work_rules.md:64`). | Returned by value. |

`effectiveSessionHash(session)` = `session->state().currentStateHash` if a session is present, else `0`.

## 4. Boundary — Owns / Does NOT own

| **Owns** | **Does NOT own** |
|---|---|
| The freshness decision for the pair (the "is it stale?" predicate). | Computing collision surfaces — stays `buildProductActiveRoomCollision*` (the Store *calls* it). |
| **Stamping** `bakedFromRoomRevision`/`bakedFromSessionHash` — the SOLE stamper (I3). | Bumping `activeRoomRevision` — the **writer's** duty via `bumpActiveRoomRevision`; the Store only *reads* it. |
| The invariant that no reader sees a stale blob (being the one call at the read seam). | Mutating `activeRoom.room` geometry, session/door state, or authoring truth (`CreativeDocument`). |
| The nullptr-vs-runtime overload selection at bake time. | Session lifetime (`Session::create`/`reset`); the Store takes a `const Session*` borrow, never stored. |
| Idempotence (second call = no-op) + instrumentation of rebake-vs-skip. | Ordering of other per-tick work; persistence (nothing new saved/hashed, §7). |

Firewall: anything not "decide fresh/stale for this one pair and rebake+stamp if stale" does **not** belong
here. Collision math, room mutation, session ticking keep their homes.

## 5. Invariants — always true

- **I1 (freshness at read):** every reader reads `window.activeRoomCollision` **behind a preceding `ensure`
  at its read seam**. After `ensure`, either `activeRoom.loaded == false && collision.ready == false`, or
  `collision.bakedFromRoomRevision == window.activeRoomRevision && collision.bakedFromSessionHash ==
  effectiveSessionHash(session)`. Equivalently: **the read blob is a pure function of exactly the
  (revision, hash) it advertises.**
- **I2 (monotonic, copy-safe revision):** `window.activeRoomRevision` strictly increases across the window's
  lifetime, advancing by **≥1 on the FINAL `activeRoom` write of every writer** (clears included). Because it
  lives on the window, whole-struct copies into `window.activeRoom` (e.g. `AutomationRoomEditing.cpp:88`)
  **cannot** overwrite it; the copy is followed by `bumpActiveRoomRevision(window)`. Never resets, never
  decreases.
- **I3 (single stamper):** `bakedFromRoomRevision`/`bakedFromSessionHash` are written **only** inside
  `ensureActiveRoomCollisionFresh`. Direct writer-bakes (retained through G4 for install correctness) leave
  them at the default `0` — an **unstamped `(0, …)` blob is by construction "stale"** (I1 mismatch, since
  `activeRoomRevision ≥ 1` after any write), so `ensure` rebakes+stamps it at the first read seam. At G5 the
  direct bakes are removed and `ensure` becomes the sole baker; I3 then holds with zero redundant bakes.
- **I4 (loaded ⟹ pairing):** after any `ensure`, `collision.ready == true` implies `activeRoom.loaded == true`
  — enforced structurally (`ActiveRoomCollision.cpp:90-95`), so stale-case (c) is unrepresentable post-`ensure`.
- **I5 (idempotence):** two consecutive `ensure` with no intervening room write or session tick perform
  **exactly one** bake; the second returns `rebaked==false`, counts byte-identical.
- **I6 (session-half honesty):** a nullptr-baked blob advertises `bakedFromSessionHash == 0`; once a session
  exists, `effectiveSessionHash != 0 ⟹` stale ⟹ rebake with door filtering. The window-less pre-session bake
  (`Activation.cpp:72`) can never masquerade as runtime-filtered.
- **I7 (no un-hashed door mutation — the session-half soundness precondition):** every `entity.active` (door)
  mutation flows through session tick/`submitCommand` → `markDirtyAndHash` (`Session.cpp:1345/1379/1390`), so
  `currentStateHash` is authoritative for door state. **Verified today:** traversal/movement use
  `mutableStateForOwnedSystems` (`Controller.cpp:406/1308`) but never touch door `.active`; no path toggles a
  door outside the hashed session path. This invariant is **audited at G5/G7** (grep for `entity.active` writes
  bypassing the hashed path — must stay empty). If ever violated, the freshness scheme silently breaks — hence
  it is a named invariant, not an assumption.

## 6. Thread & lifetime rules

- **Today: single-threaded, main-thread only.** Verified: no async producer of activeRoom/collision exists
  (no `std::thread`/`std::async`/`JobSystem` touches them); all ~11 writers and ~15 readers run on the main
  path. `ensure` runs main-thread, reads `window.activeRoom`+`activeRoomRevision`+`session.state()`, writes
  `window.activeRoomCollision` in place — obeying the **main-thread law** (`core_spine_work_rules.md:48`).
- **Call contract:** readers do not each call it ad hoc. The guard runs at a **single frame-boundary seam** —
  immediately after the session tick settles (`markDirtyAndHash` has run, `stateHashDirty` cleared) and
  **before** the read fan-out (input apply, projection, draw list, receipt). Mutate-then-read entry points
  (e.g. `TapeRunner`) call it at their read seam, replacing their bespoke rebake. The mid-tick Controller
  branch is deleted (G5) because the frame-boundary call subsumes it.
- **Lifetime:** the Store owns **no state** — a stateless verb over `window`+`session` borrows; nothing
  outlives its owner (`core_spine_work_rules.md:56`) trivially. The `const Session*` is valid only for the
  call; never stored.
- **Shaped for future async (NOT built):** the freshness key is a **plain value pair** (two uint64s), not a
  pointer or a live read. That is the snapshot-discipline seam: when the bake later moves to a worker, the
  worker receives `{roomRevisionSnapshot, sessionHashSnapshot, immutable RoomAsset copy}` and the main thread
  validates `window.activeRoomRevision == roomRevisionSnapshot` before applying — today's `ensure` comparison,
  relocated. **We build only the synchronous comparison now** (§11).

## 7. Memory rules

- **Long-lived:** one uint64 on the window + two on the collision struct (24 bytes), no heap; lives as long as
  the window.
- **Per-bake (stale branch only):** exactly what `buildSpatialSurfaceSet`/`roomWithRuntimeFilteredSurfaces`
  allocate today (`ActiveRoomCollision.cpp:98`) — no *additional* allocation over the current rebake.
- **Hot-loop:** the **fresh path allocates nothing** — the dirty check skips the bake when `(revision, hash)`
  match. Steady-state readers (frame after frame with no room/door change) go from ~15 unconditional rebakes
  to zero. **Net steady-state allocation decreases.** Honest caveat for the transition: **through G4** the
  install/clear writers still bake directly AND `ensure` rebakes their unstamped `(0,…)` blob once at the first
  read seam — so an install path bakes **at most twice**; **at G5** the direct bakes are removed and each
  install bakes **once** (inside `ensure`). The redundant install bake is bounded (one per room install, not
  per frame) and is the price of keeping install-time correctness (and the E121 anchor) intact during the
  staged migration.
- **Persistence:** the three fields are **runtime-only** — NOT added to `SaveCodec`/`SaveEnvelope`/`StateHash`.
  Revision is session-local (reproduces from the load-path writer bumping on install). Mirrors the
  `reasoningGraph`/`outcomeTable` transient-in-persistence precedent (`SessionState.hpp:103-116`).

## 8. Failure behavior — every error path

`ensure` decision order (deterministic): **(1)** `!activeRoom.loaded` → rebake to the unloaded blob
(`ready==false`, `status=="active_room_collision_unavailable"`, `querySurfaceCount==0`,
`ActiveRoomCollision.cpp:90-95`), stamp provenance, `reasonCode="rebaked_unloaded"` (resolves stale-case (c)).
**(2)** else if `(bakedFromRoomRevision,bakedFromSessionHash) == (activeRoomRevision, effectiveSessionHash)` →
no-op, `reasonCode="skipped_fresh"`. **(3)** else bake (runtime overload iff `session != nullptr`), stamp, and
classify: room-half mismatch only → `rebaked_room`; session-half only → `rebaked_session`; both → `rebaked_both`;
degenerate empty room (`ready==false`, `reasonCode` surface = `"active_room_collision_missing_surfaces"`,
`:102-106`) → stamp anyway (so it does not thrash-rebake every frame, I5), `reasonCode="rebaked_empty"`.

- **`session == nullptr`:** one-arg overload, `effectiveSessionHash=0`, doors unfiltered by design (I6). No error.
- **`session != nullptr` && `stateHashDirty == true`:** the frame-boundary contract calls `ensure` *after* the
  tick settles the hash; `markDirtyAndHash` clears `stateHashDirty` synchronously, so this path is not normally
  reachable. If it ever is, `ensure` stamps the currently-read hash and the next `ensure` after recompute
  rebakes — no incorrect blob is *published* because readers sit behind the same settled-hash seam. (One-line
  note, not a deferred limitation — the real dependency is I7.)
- **No exceptions, no new fallible resource** — same allocators as today.
- **Every path observable:** closed enum `reasonCode ∈ {skipped_fresh, rebaked_room, rebaked_session,
  rebaked_both, rebaked_unloaded, rebaked_empty}` (plus the transitional `g2_stub_no_rebake`, deleted at G3),
  surfaced into `receipt/ActiveRoomFields.cpp` at G7 so a stale read leaves a trail.

## 9. Shutdown behavior

- **Nothing to drain, cancel, or block** — synchronous stateless verb, no owned thread/queue/handle.
- **Ordering vs teardown:** it mutates `window.activeRoomCollision`, destroyed with the window; the Store holds
  no reference past the call, so teardown order is unconstrained.
- **On `activeSession.reset()`:** the next `ensure` sees `session == nullptr`, takes the window-less path,
  re-stamps `bakedFromSessionHash=0` — no dangling read of a destroyed session (I6 + borrow-not-store, §6).
- **Regression guard:** the terminal clear (`Operations.cpp:452`) bumps `activeRoomRevision`, so any deferred
  `ensure` observes `loaded==false` → unloaded blob (I4).

## 10. Tests

Harness = `product_creative_no_window_bake_scenario_tests` (real window+session, `NoWindow`); type-level asserts
alongside `product_active_room_state_tests` + `product_active_room_collision_tests` (door-filter fixture :126-133).

**Regression pins (3 stale cases) — each asserts BOTH provenance equality AND the observable count delta.
Because the pre-slice baseline is valid (well-defined, not UB — §1), each test diffs against it directly:**
- **T-a (room replaced, collision not rebuilt):** floor+crate baseline → `spatialSurfaceCount==3`,
  `querySurfaceCount==3`, `bakedFromRoomRevision==activeRoomRevision`. Overwrite `window.activeRoom` with
  floor-only + `bumpActiveRoomRevision(window)`, WITHOUT touching collision. Before `ensure`:
  `querySurfaceCount(3) != spatialSurfaceCount(1)`, provenance mismatch. After `ensure`: `querySurfaceCount==1`,
  `roomId==newRoomId`, provenance equal; reader-parity `productActiveRoomCollisionSurfaces(collision)->size()==1`.
- **T-b (baked without runtime session — door not filtered):** room with a surface whose
  `runtimeOwnerStableName` maps to a `Door EntityState{active=true}`. Baseline (two-arg): `runtimeOwnedSurfaceCount==1`,
  `activeDoorBlockerSurfaceCount==1`, `runtimeFilteredSurfaceCount==0`. Open the door via a ticked non-Interact
  path (the `Controller.cpp:2244` gap; bumps `currentStateHash`). After `ensure`: `runtimeFilteredSurfaceCount==1`,
  `activeDoorBlockerSurfaceCount==0`, `querySurfaceCount` drops by exactly 1, `bakedFromSessionHash==currentStateHash`.
  Assert the guard chose the **two-arg** overload.
- **T-c (clear leaves stale-ready collision):** loaded baseline. Set `window.activeRoom={}` +
  `bumpActiveRoomRevision(window)` WITHOUT the paired collision clear. Before: `loaded==false` but `ready==true`
  (stale-ready). After `ensure`: `ready==false`, `status=="active_room_collision_unavailable"`,
  `reasonCode=="rebaked_unloaded"`, `querySurfaceCount==0`, `productActiveRoomCollisionSurfaces()==nullptr`.

**Order-independence (headline proof) — T-order:** one frame, two ticked commands each flipping a distinct
door's `active`: an `Attack`-class (`Controller.cpp:2244`, today does NOT rebake) and an `Interact` (today
does). Run `[Attack, Interact]` then `[Interact, Attack]` from identical baselines. Assert reader-visible
`window.activeRoomCollision` (`querySurfaceCount`, `activeDoorBlockerSurfaceCount`,
`productActiveRoomCollisionSurfaces()->size()`) is **byte-identical across orderings** after the frame-boundary
`ensure`. Contrast: with the mid-tick branch (pre-slice) the two orderings **diverge** — proving the guard
removes the handler-order dependence. This is I1 executable.

**TapeRunner equivalence lemma — T-tape (G4):** redirecting the unconditional `refreshActiveRoomCollision`
(`TapeRunner.cpp:493`) to `ensure` is output-equivalent because (a) TapeRunner re-derives the surface pointer
per step (`currentCollisionSurfaces` :209), so no captured pointer is invalidated, and (b) when a tick changes
no hashed field and no room geometry, the unconditional rebake produces a byte-identical blob that `ensure`
correctly skips. The test exercises **both**: a `Wait`/no-op-tick step (skip path) and a door-toggle step
(rebake path), asserting reader-visible surfaces identical to the pre-slice unconditional-rebake baseline.

**Determinism/idempotence (all tests):** a second `ensure` immediately after the first returns `rebaked==false`,
counts unchanged (I5). **Anchor:** E121 assertions stay green unchanged (output-preserving on the happy install
path). **I7 audit (G5/G7):** grep proves no `entity.active` write bypasses the hashed session path.

## 11. Non-goals — must NOT be built

- **NOT the job system** — no `JobSystem`, no `MainThreadCompletionQueue`, no worker; bake stays synchronous.
- **NOT async** — no snapshot copy, deferred apply, or cancellation. The token is *shaped* for a future
  validate-on-apply seam (§6) but only the synchronous comparison is built.
- **NOT moving off `ProductAppWindowState`** — fields land on the existing window/structs in place.
- **NOT a general dirty-channel framework** — it guards exactly one pair; no generalization to projection/receipt.
- **NOT persisting revision** — not added to save/hash (§7).
- **NOT changing collision math or overload semantics** — the Store only chooses *which* existing overload to call.

## 12. Rollback / deletion path

- **Removal criteria:** if the dirty check never skips (inputs churn every frame) or an always-rebake-at-frame-
  boundary proves measurably free, delete it.
- **Clean deletion (bounded, reversible):** (1) delete `gameplay/ActiveRoomCollisionFreshness.{hpp,cpp}` + the
  frame-boundary callsite; (2) restore the mid-tick branch `Controller.cpp:2236-2241` and
  `TapeRunner::refreshActiveRoomCollision` from git; (3) drop the 3 fields + `bumpActiveRoomRevision` calls
  (additive, unread by persistence — mechanical); (4) delete the T-* tests. No other system depends on the
  token (not saved, hashed, or read by creative/AI/movement lanes), so deletion cannot infect unrelated
  systems — the §4 firewall bounds blast radius to these files plus the two rebake sites.
- **Fallback retention:** a half-landed slice degrades to today's behavior (per-writer rebakes become redundant
  but harmless no-ops once the guard runs first), never worse.

---

## Review-gate slice sequence (Gates 1–7) — one card per gate

| Gate | Slice | Deliverable | Acceptance |
|---|---|---|---|
| **G1** | *This PREFLIGHT accepted.* | Design ratified; name `ActiveRoomCollisionFreshness` approved (non-`Kernel`). | Reviewer signs §1–§12. |
| **G2** | **Types + counter, no consumer.** | Add `window.activeRoomRevision`; add `bakedFromRoomRevision`/`bakedFromSessionHash`; add `bumpActiveRoomRevision(window)`; add `ensureActiveRoomCollisionFresh` **stub** (observes+returns, no rebake) + `FreshnessResult`. **No writer bumps; no reader ensures.** | Compiles; suite green + unchanged; fields default 0. |
| **G3** | **Unit tests pass.** | Implement the real dirty-check + stamp in `ensure` (decision order §8); add T-a/T-b/T-c + idempotence, driven directly. Delete the `g2_stub_no_rebake` string. | New tests green; E121 anchor untouched. |
| **G4** | **One real consumer wired.** | Bump `activeRoomRevision` at the FINAL `activeRoom` write of all ~11 writers (incl. the `AutomationRoomEditing.cpp:88` copy + terminal clears; § bump discipline below); call `ensure` at the frame-boundary seam + `TapeRunner` read seam. Add T-order + T-tape. | T-order proves order-independence; tape identical. |
| **G5** | **Old blocking path removed.** | Delete mid-tick branch `Controller.cpp:2236-2241`; redirect `TapeRunner::refreshActiveRoomCollision` to `ensure`; remove the ~9 direct collision bakes now subsumed; bypass-removal audit (grep stray `buildProductActiveRoomCollision(` at non-Store/non-install sites — none). Run the I7 audit. | No ad-hoc rebake survives; audit clean; green. |
| **G6** | **Stress + shutdown.** | Order-permutation stress (N random door-toggles → identical reader state); session-reset-then-ensure (§9); empty-room thrash test (I5). | No divergence; no dangling-session read; no thrash. |
| **G7** | **Naming + ownership audit + receipt.** | Confirm `Store`/`ensure` did not inflate scope; architecture receipt near `ActiveRoomCollisionFreshness.cpp` (why · owns · not-owns · thread · shutdown · first consumer · known limitation = coarse `currentStateHash` conservatism); wire `reasonCode` into `receipt/ActiveRoomFields.cpp`; final I7 audit. | Audit checklist (`core_spine_work_rules.md:68-78`) passes; receipt + counter present. |

**G4 bump discipline (deterministic):** bump exactly once after the **FINAL** `window.activeRoom` write in each
function. Terminal clears (`Operations.cpp:452`) bump on the clear. Intermediate clear-then-set
(`Operations.cpp:183→187`, `:283→287`) do NOT bump on the intermediate clear. Whole-struct copies
(`AutomationRoomEditing.cpp:88` `window.activeRoom = state.activeRoom`) bump `window` *after* the copy (the
counter lives on the window, so the copy cannot carry a foreign value — I2). `Activation.cpp:72`'s intermediate
nullptr-baked collision is allowed to ship unstamped — it is synchronously superseded at `:118` before any
reader runs.

---

## FIRST implementation card (Gate 2) — drafted; seeded to `ready/` only AFTER Gate-1 sign-off

```
E-CARD: E-ARCF-G2 — ActiveRoomCollisionFreshness: window revision counter + provenance fields (NO consumer)

GATE: 2 (minimal types added, no consumer). Blocks on: G1 (this preflight ratified).
LANE: core-spine / product window derived-truth (claude). OWNER-DOC: this preflight (§3, §5-I2/I3).

GOAL
  Introduce the freshness TYPES ONLY. After this card the counter + provenance fields exist and default to 0,
  the two functions exist, but NO writer bumps and NO reader ensures. Existing behavior byte-identical.

DO
  1. src/app/iggy3d/ProductAppWindowState.hpp
     - Add to ProductAppWindowState, near the activeRoom/activeRoomCollision members:
         std::uint64_t activeRoomRevision = 0;  // monotonic room generation; window-owned (preflight §3/§5-I2)
  2. src/app/iggy3d/gameplay/ActiveRoomState.hpp / .cpp
     - Declare + define: void bumpActiveRoomRevision(ProductAppWindowState& window);
         { ++window.activeRoomRevision; }   // the ONLY sanctioned mutator (§5-I3). fwd-declare ProductAppWindowState.
  3. src/app/iggy3d/gameplay/ActiveRoomCollision.hpp
     - Add to ProductActiveRoomCollisionState (after activeDoorBlockerSurfaceCount, before surfaces):
         std::uint64_t bakedFromRoomRevision = 0;   // provenance: window.activeRoomRevision baked from (§3)
         std::uint64_t bakedFromSessionHash  = 0;   // provenance: SessionState.currentStateHash (0 = no session)
  4. NEW src/app/iggy3d/gameplay/ActiveRoomCollisionFreshness.hpp
     - struct ProductActiveRoomCollisionFreshnessResult {
         bool rebaked = false; std::uint64_t observedRoomRevision = 0; std::uint64_t observedSessionHash = 0;
         std::string reasonCode = "skipped_fresh"; };
     - Declare: ProductActiveRoomCollisionFreshnessResult ensureActiveRoomCollisionFresh(
         ProductAppWindowState& window, const Session* session);
       (fwd-declare ProductAppWindowState + Session; include <cstdint>,<string>.)
  5. NEW src/app/iggy3d/gameplay/ActiveRoomCollisionFreshness.cpp
     - STUB BODY (no dirty-check yet — that is G3). Records observations, does NOT rebake or stamp:
         result.observedRoomRevision = window.activeRoomRevision;
         result.observedSessionHash  = session ? session->state().currentStateHash : 0;
         result.reasonCode = "g2_stub_no_rebake";
         return result;   // window.activeRoomCollision untouched.
     - Add the .cpp to the app target in CMakeLists.txt (mirror ActiveRoomCollision.cpp's entry).

DO NOT (scope firewall — later gates)
  - Do NOT call bumpActiveRoomRevision from any writer (G4).
  - Do NOT call ensureActiveRoomCollisionFresh from any reader/frame boundary (G4).
  - Do NOT touch Controller.cpp:2236-2241 (G5).  Do NOT implement the dirty-check (G3).
  - Do NOT add fields to SaveCodec/SaveEnvelope/StateHash (runtime-only — §7).
  - Do NOT rename/change any buildProductActiveRoomCollision overload.

TESTS (G2 — type-level only)
  - product_active_room_state_tests: default window.activeRoomRevision == 0; bumpActiveRoomRevision advances
    by exactly 1 per call; three calls -> 3.
  - product_active_room_collision_tests: default bakedFromRoomRevision == 0 && bakedFromSessionHash == 0.
  - ensure-stub test: default window, session==nullptr -> rebaked==false, observedSessionHash==0,
    reasonCode=="g2_stub_no_rebake", querySurfaceCount unchanged.

ACCEPTANCE
  - Full build compiles (app + tests). ENTIRE existing suite green + UNCHANGED (product_receipt_key_order_tests
    + product_creative_no_window_bake_scenario_tests pass verbatim). New type-level tests green.
  - grep confirms zero call sites of bumpActiveRoomRevision and zero non-test call sites of
    ensureActiveRoomCollisionFresh (proves "no consumer").

ROLLBACK
  - Purely additive: delete the two new files, remove the 3 fields + the helper, revert the CMakeLists line.
```
