# E141 — `creativeFly` anchor → `CreativeFlyAnchorStore` — GATE-0 PREFLIGHT (v0.2)

**STATUS: BLOCKED — held, not claimable.** Gates before it becomes a `ready/` card:
1. **#2 `activeCreative` delete lands** (E139 done) — one ownership kill at a time.
2. **Gate-1 ratification** — spine-adjacent freshness store
   (`docs/core_spine_work_rules.md`).

**Decision A is LOCKED** (re-seed on world-open drift) and **survives adversarial
verification** (workflow `wuu7jt0jx`, 4 verifiers + completeness critic, 2026-07-07).
The direction is sound; v0.2 folds in one blocker fix + three shape corrections the
verification surfaced. Traced per `docs/ownership_trace_method.md`.

> **v0.1 → v0.2 changelog (what the adversarial pass caught):**
> - **BLOCKER fixed** — the freshness token must be a *stable per-world-open identity*,
>   NOT a content hash. Every hash source in-tree (`Session::stateHash()` /
>   `runtimeStateHash` = `currentStateHash`) fails two ways: it drifts per-tick (re-seeds
>   mid-fly, snapping the camera) *and* two blank creative worlds hash identically (so the
>   blank-A→blank-B swap A exists to fix would silently not fire). **Resolution:** new
>   window-owned monotonic `creativeWorldEpoch`, mirroring `activeRoomRevision`.
> - **Retracted "3 seeders are redundant"** — they have 3 *distinct* behaviors (§3).
> - **Rescoped the leak** (§2) — the creative lane already re-seeds on launch; the real
>   stale-anchor surface is the *bypass* paths (map_maker / raw session swap).
> - **Provenance race** between origin-frame and drift re-seed resolved via epoch
>   precedence (§4).
> - Added the missed second caller, the render-frame fan-out, 3 existing test
>   migrations, and the standalone-app out-of-scope note.
> - **All line numbers are approximate** — `Operations.cpp`/`InputFrame.cpp`/
>   `ProjectionRefresh.cpp` are dirty in the working tree and drifted twice during
>   verification. Cited by **function name**; re-anchor against HEAD at card-cut time.

---

## 1. The field cluster & what is NOT in scope

`ViewportState.hpp:20–21` (nested in `ProductViewportState` → `window.viewport`):

```
bool creativeFlyAnchorValid = false;   // the one-way latch (deleted by this slice)
Vec3 creativeFlyPositionMeters;        // the anchor (moves to the store)
```

**In scope:** the anchor pair only.
**NOT in scope, but entangled (must be handled, not moved):**
- Siblings `creativeFlyActive/Speed/Status/ReasonCode` (ViewportState.hpp:22–25) — stay
  on `window.viewport`. They are **co-mutated with the anchor** in two blocks (§6); the
  store split fractures those blocks, so the integrator/eager-seed callers keep writing
  siblings even after the anchor moves.
- Camera pose `cameraYawDegrees/cameraPitchDegrees` — the eager seeder co-writes these
  (§3); they stay on the viewport.
- **Standalone app parallel anchor** — `apps/iggy3d_creative/main.cpp` keeps its OWN
  local `Vec3 flyPos{0,6,12}` (note: `{0,6,12}`, *different* from the window path's
  `{0,6,10}`), integrated at `main.cpp:~545` and framed at `~643`. It never touches
  `window.viewport`. The store verb is window-scoped and **does not reconcile it** — a
  known parallel-anchor divergence, explicitly out of scope for this slice.

## 2. The confirmed deficit — RESCOPED

**Not a derived cache — a lazily-seeded accumulator whose latch is never reset.**
Verified independently:
- `creativeFlyAnchorValid` is written `=true` by 3 seeders and never `=false` anywhere
  but its default initializer. No `window.viewport = {}` reset exists (the `frame.viewport
  = {...}` hits are render width/height/DPI, a different struct).
- The window is **persistent**: `window` and `activeSession` are separate app-lifetime
  members (`AppKernel.hpp:28–29`), the loop runs once (`AppKernel.cpp:~122`), sessions
  swap mid-loop. So window state outlives sessions.
- **The asymmetry that IS the bug:** `clearProductGameplayLaunchState` (session teardown)
  zeroes `runtimeStateHash`, `activeRoom`, `activeRoomCollision`, and `activeSession` —
  but leaves `creativeFly*` latched. The anchor outlives even a full session drop.

**RESCOPE (v0.2):** the creative launch paths (`frameCreativeStageCameraOnOrigin`, called
on New-World *and* Open-World) already re-seed the anchor to origin on world open — so the
blanket "rides world A into world B, never invalidated" is **false for the creative lane**.
The genuine stale-anchor surface is the paths that **bypass** the origin frame: the legacy
`map_maker` surface and any raw session swap without a creative launch. Decision A is
justified against *that* surface (plus it makes the origin-frame reset principled instead
of incidental).

## 3. Writers & readers — corrected census (classify before touching)

**Writers — 5 callsites, NOT "4 writers", and the seeders are NOT redundant:**

| writer | behavior beyond the anchor | verdict |
|---|---|---|
| `frameCreativeStageCameraOnOrigin` (Operations.cpp `~256`) — called at `~1359` (New World) **and** `~1427` (Open World) | **also writes `cameraYawDegrees=0`, `cameraPitchDegrees=-30`** | eager seed → `store.seedFromOrigin()`; **keep the yaw/pitch writes in the caller** |
| `ensureCreativeFlyAnchor` (InputFrame.cpp `~351`) — called at `~517` (map_maker) **and** `~1263` (creativeNavigate) | **always latches** valid=true, even on the origin `{}` fallback | lazy seed → ensure verb; **two callers**; `~1263` passes `&*context.activeSession` (raw optional deref) — preserve the `navigateActive` precondition against the verb's nullable `Session*` |
| `mapMakerAnchorFor` (ProjectionRefresh.cpp `~353`) | `!playerFound` branch returns `{}` **without latching** (retry next frame); its return value is authoritative (grid `anchorWorld` + `planeY`) | seed-**and-return** verb; **must preserve the no-latch-on-no-player branch** |
| integrator (InputFrame.cpp `~410–416`) | co-writes the 4 siblings in the same block | keep; stamps `provenance = FlyIntegrated` |

The three seeders read **three different sources** (origin constant; Session player via
`activePlayerPositionOrOrigin`; Scene projection via `playerAnchorFromScene`) and have
**three different latch behaviors** — the store's ensure verb takes the source per
callsite; it is not one canonical player read.

**Readers — the camera consumer + its downstream fan-out + receipt:**
- `ProjectionRefresh.cpp:~778–779` — gates `cameraAnchorOverrideAvailable` on
  `creativeFlyAnchorValid`, reads the anchor → `frame.cameraAnchorOverrideMeters`. **Migrate
  in lockstep** (the gate's truth value changes when the latch becomes
  `provenance≠Unseeded && epoch matches`).
- Downstream of the override (read the *derived* frame field, **no edit needed**):
  eye-framing `ProjectionRefresh.cpp:~900`, `ViewportFraming.cpp:~53`,
  `FramePresenter.cpp:~1013`. Listed so the blast radius is honest.
- Receipt `GameplaySceneStateFields.cpp:~207–213` — projection; rebuild from
  `store.anchor()` + `store.provenance()`.

## 4. Proposed shape — the epoch token (blocker resolution)

```cpp
// NEW window field — mirrors activeRoomRevision (ProductAppWindowState.hpp:256).
// Bumped ONCE per world-open, inside frameCreativeStageCameraOnOrigin. Stable
// (not per-tick), distinct per open (two blank worlds differ). NOT a content hash.
std::uint64_t creativeWorldEpoch = 0;

struct CreativeFlyAnchorStore {
  Vec3 positionMeters;
  enum class Provenance { Unseeded, OriginFramed, PlayerSeeded, SceneSeeded, FlyIntegrated };
  Provenance provenance = Provenance::Unseeded;
  std::uint64_t seededFromWorldEpoch = 0;   // which world-open seeded this
};

// Freshness = provenance != Unseeded && seededFromWorldEpoch == window.creativeWorldEpoch.
// Re-seeds when the epoch drifted (new world open) OR unseeded. Records provenance.
const Vec3& ensureFreshCreativeFlyAnchor(ProductAppWindowState& window,
                                         const Session* activeSession);
void seedCreativeFlyAnchorFromOrigin(ProductAppWindowState& window);  // used by the origin frame
```

**Provenance-race resolution:** `frameCreativeStageCameraOnOrigin` is the thing that
**bumps `creativeWorldEpoch`** and calls `seedCreativeFlyAnchorFromOrigin` — so on world
open the origin seed is authoritative *for that epoch*, and the lazy seeders only fire when
the anchor is unseeded for the current epoch. No last-writer race. `creativeFlyAnchorValid`
is deleted; its readers switch to the freshness predicate.

## 5. Truth-gates (3 existing test migrations + 1 net-new)

- **Migrate (existing, will break on the field move):**
  - `product_vulkan_room_frame_tests.cpp:~1344` — fabricator sets the raw anchor; reseed via store.
  - `product_creative_world_launch_tests.cpp:~4134` — co-asserts anchor **and** `yaw==0`,
    `pitch<0`; reroute the anchor reads, keep the yaw/pitch asserts (proves §3's entanglement).
  - `product_window_input_frame_tests.cpp:~824` — asserts valid + integrator-moved z; reroute.
- **Net-new (the bug reproduction):** open world A, seed the anchor, open world B (distinct
  `creativeWorldEpoch`) — assert a re-seed **even if the two sessions' state hashes coincide**
  (guards the blank-A→blank-B corollary). Fails pre-fix, passes post-fix.
- Receipt golden (`RECEIPT_GOLDEN_REGEN=1`): gains `creative_fly_anchor_provenance` +
  `..._world_epoch`, drops the raw `valid` bool. God-struct ownership TSV: two viewport
  fields change owner row; `creativeWorldEpoch` is a new window-owned field.

## 6. Sizing — M, design-heavy, with two entanglement seams

Field-ref census is small (~16 field refs / ~8 non-test — v0.1's "25/27" was wrong).
Churn is trivial. The cost is **policy + entanglement**:
- **Co-mutation block 1 (eager):** anchor + `yaw=0`/`pitch=-30` — split so the caller keeps pose.
- **Co-mutation block 2 (integrator):** anchor + 4 siblings — split so the caller keeps siblings.
- Gate sequence mirrors the collision store: G2 store types + `creativeWorldEpoch` field →
  G3 failing cross-world test + freshness predicate → G4 wire ensure/seedFromOrigin +
  migrate the camera consumer → G5 collapse the 3 seeders (preserving their 3 behaviors) →
  G6 stress → G7 receipt audit.

## 7. Gate-1 checklist (what ratification must confirm)

1. **Token = window-owned `creativeWorldEpoch`** (new field, bumped in the origin frame),
   NOT `Session::stateHash()`/`runtimeStateHash`. Confirm the app-lane-only scope (no
   session/runtime edit → stays out of the core-spine session gate).
2. **Epoch precedence** — origin-frame bumps + seeds; lazy seeders fill only within-epoch.
3. **Preserve the 3 seed behaviors** — origin keeps yaw/pitch; map_maker keeps no-latch-on-
   no-player; both `ensureCreativeFlyAnchor` callers (incl. the `&*activeSession` deref).
4. **Migrate the camera consumer in lockstep** so the override does not flicker on drift.
5. **3 existing tests + 1 new test** in scope; standalone-app anchor explicitly out.

**Once ratified and #2 lands, decompose into G2–G7.** Until then, held in `blocked/`.

## 8. Adversarial-verification audit trail

Workflow `wuu7jt0jx` (transcript under `.../subagents/workflows/wf_9faca7cb-4ab`):
invalidation-hunt + window-lifetime **confirmed** persistence and the never-reset latch;
missed-access-sweep + collapse-safety returned **nuanced**, surfacing the token trap, the
provenance race, the yaw/pitch and no-latch behavioral differences, the second caller, the
render fan-out, and the 3 test migrations. Completeness critic: **decision A survives,
gateSequenceReady = false** until the token blocker + 3 shape corrections are folded in —
which is what v0.2 does.
