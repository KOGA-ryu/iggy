# E141 — `creativeFly` anchor → `CreativeFlyAnchorStore` — GATE-0 PREFLIGHT

**STATUS: BLOCKED — held, not claimable.** Two gates must clear first:
1. **#2 `activeCreative` delete must land** (E139 done) — sequencing, one ownership
   kill at a time.
2. **Gate-1 ratification by the user** — this is a spine-adjacent freshness store
   (`docs/core_spine_work_rules.md`); it does not become a `ready/` card until
   ratified with the §7 open question resolved.

Deficit #3 from `docs/ownership_deficit_audit.md` (row 3, score 3.0, rated **M** —
**preflight CONFIRMS M**, unlike #2 (S→L) and #1 (9.0→hold)). Traced per
`docs/ownership_trace_method.md`. Recon only — **no code edits**, disjoint from the
in-flight `activeCreative` work.

---

## 1. The field cluster

`ViewportState.hpp:20–21` (nested in `ProductViewportState`, itself nested in the
god-struct's `window.viewport`):

```
bool creativeFlyAnchorValid = false;   // 20  — the latch
Vec3 creativeFlyPositionMeters;        // 21  — the anchor
// siblings NOT in scope for this slice: creativeFlyActive/Speed/Status/ReasonCode (22–25)
```

## 2. The confirmed deficit (grep → read → verified)

**Not a derived cache — a lazily-seeded accumulator with no invalidation authority.**

**4 writers, 3 seed provenances + 1 integrator:**

| site | role | provenance |
|---|---|---|
| `Operations.cpp:298–299` `frameCreativeStageCameraOnOrigin` | seed (eager) | origin `{0,6,10}`, pitch −30° |
| `InputFrame.cpp:357–359` `ensureCreativeFlyAnchor` | seed (lazy, `if !valid`) | player-or-origin |
| `ProjectionRefresh.cpp:363–364` (getter) | seed (lazy, `if !valid`) | scene player pose |
| `InputFrame.cpp:416` fly-integrate | **integrator** (`if fly.applied`) | accumulates delta each frame |

**The three defects, all verified:**
1. **Ambiguous provenance** — whichever seeder hits the `!valid` guard *first* wins,
   seeding from a *different source*; nothing records which. (`grep creativeFly.*session` → **zero** stamps.)
2. **No invalidation authority** — `grep 'creativeFlyAnchorValid = false'` → **only the
   default**. Never reset. No `window.viewport = {}` reset exists in product code (the
   two `frame.viewport = {...}` hits are the render frame's width/height/DPI — a
   different struct). Once latched true, true for the life of the **persistent**
   `ProductAppWindowState`.
3. **Session-lifetime mismatch** — the window persists while sessions are swapped in
   (same shape that forced the collision store's `sessionHash` token). So a seed from
   world A's player rides into world B: `frame.cameraAnchorOverrideMeters`
   (`ProjectionRefresh.cpp:778–779`) and the receipt read stale coordinates until the
   next fly input drags them elsewhere.

## 3. Classification of every reader (7 logical sites)

| site | class | migration |
|---|---|---|
| `ProjectionRefresh.cpp:778–779` → `frame.cameraAnchorOverrideMeters` | **truth read** (the camera consumer) | read through `store.anchor()` after `ensureFreshAnchor` |
| `ProjectionRefresh.cpp:356–357` (getter guard + return) | **seed-or-return** — collapses into the ensure verb | becomes the store's ensure body |
| `InputFrame.cpp:354` (`ensureCreativeFlyAnchor` guard) | **seed** — collapses into ensure verb | delete; call `ensureFreshAnchor` |
| `InputFrame.cpp:409` (feeds `applyProductCreativeFlyInput`, in/out) | **integrator input** (legitimate owner mutation) | keep; mutate through store |
| `GameplaySceneStateFields.cpp:207–213` (receipt: valid + xyz) | **projection** | rebuild from `store.anchor()` + `store.provenance()` |
| `Operations.cpp:298` (origin frame) | **eager seed** | route through `store.seedFromOrigin()` (explicit provenance) |

**No hit conflates the anchor with an unrelated value; the 3 seeders are genuinely
redundant and collapse to one verb.** This is a real ownership kill, not a re-house.

## 4. Proposed shape (exemplar, one level smaller)

One owner holding the anchor + provenance + session token; one ensure verb:

```cpp
struct CreativeFlyAnchorStore {
  Vec3 positionMeters;
  enum class Provenance { Unseeded, OriginFramed, PlayerSeeded, SceneSeeded, FlyIntegrated };
  Provenance provenance = Provenance::Unseeded;
  std::uint64_t seededFromSessionHash = 0;   // 0 = unseeded
};

// The ONE verb. Re-seeds iff unseeded OR the session drifted; otherwise keeps the
// integrated value. Records provenance. Mirrors ensureActiveRoomCollisionFresh.
const Vec3& ensureFreshCreativeFlyAnchor(ProductAppWindowState& window,
                                         const Session* activeSession);
```

- The 3 lazy/eager seeders → callers of `ensureFreshCreativeFlyAnchor` (or an explicit
  `seedFromOrigin` for the eager stage-frame path).
- `InputFrame.cpp:416` integrator stays, but stamps `provenance = FlyIntegrated`.
- `creativeFlyAnchorValid` **is deleted** — replaced by `provenance != Unseeded &&
  seededFromSessionHash == currentHash`. The one-way latch becomes a real freshness token.

## 5. Truth-gates that fire by design

- **Receipt golden** (`RECEIPT_GOLDEN_REGEN=1`) — the receipt gains
  `creative_fly_anchor_provenance` / session-stamp keys, loses the raw `valid` bool.
- **God-struct ownership coverage TSV** — the two viewport fields change owner row.
- **New behavioral test** — the exemplar's own test shape: seed in session A, swap to
  session B, assert the anchor re-seeds (today it does NOT — this test fails pre-fix,
  passes post-fix; it *is* the bug reproduction).

## 6. Sizing — confirmed M, design-heavy not churn-heavy

25 total refs (17 non-test), ~7 logical readers. Churn is trivial next to #1's 477.
The cost is **policy**, concentrated in §7. Gate sequence mirrors the collision store
(G2 types → G3 dirty-check + failing cross-session test → G4 wire the ensure seam →
G5 remove scattered seeders → G6 stress → G7 receipt audit).

## 7. THE OPEN QUESTION for Gate-1 (must be answered before this is built)

**When is a re-seed the correct behavior, vs. sticky-by-design?**

The camera anchor being sticky *within* a session is correct (you don't want it
snapping back while you fly). The question is the **boundary**: on world/session load,
should the fly anchor reset to that world's player/origin, or persist the last
position? Recon proves it currently **persists** (never invalidated) — but I cannot
tell from the code whether that is intended UX or a latent bug. Two ratifiable answers:

- **(A) Re-seed on session drift** (full exemplar): `sessionHash` mismatch forces a
  re-seed. Fixes the cross-world stale-anchor. **Recommended** — matches the collision
  store's session discipline and the "no stored state outlives its provenance" law.
- **(B) Sticky is intended** (lighter): keep persistence, but still collapse the 3
  seeders to one verb and record provenance for the receipt. Drops the session token.
  Smaller, but leaves the anchor able to outlive its seeding session.

**Do not build until the user picks A or B.** The whole store shape (token vs.
no-token) hinges on it. Recommendation: **A**.
