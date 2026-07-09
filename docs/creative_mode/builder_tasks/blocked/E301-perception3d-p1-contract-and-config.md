# E301 — Perception 3D · P1: contract + config (Mode C, behavior-neutral slice)

**STATUS: STAGED in `blocked/` — release on a clean tree.** First card cut under
`docs/operating_loop_v1.md`. Parent spec: `docs/perception-3d-maximum.md` **v1.1** (§2, §3, §12-P1 —
read the v1.1 corrections header first; anchors verified at HEAD this card-cut).
**Commit:** `claude: planned. codex: …`. Lane: `runtime/ai` — disjoint from the creative debt lane.

## Goal (one line)

Land the 3D-perception **contract**: extend the existing `NpcPerceptionResult` + `NpcBehaviorConfig` with the
new fields, serialized with back-compat — **zero behavior change** (new fields defaulted, unread).

## METRIC PROMISE (checked mechanically at close)

- Receipt golden: **byte-identical** (`git diff tests/golden/` empty).
- Full suite green **+ new pin-tests added** (count goes UP): config defaults, config round-trip, result-struct
  defaults.
- Existing-files-edited ≤ **3** (`NpcBehaviorSystem.hpp`, the `NpcBehaviorConfig` codec site, one test file);
  new files ≤ 1 (a test TU if none fits).

## STOP CONDITION

If `NpcPerceptionResult` (`NpcBehaviorSystem.hpp:72-87`) or `NpcBehaviorConfig` (`:23-35`) differs materially
from what this card states, or the config has no (de)serialization site to extend — **STOP and report; do not
improvise the contract.**

## The changes

**1. Extend `NpcPerceptionResult` (`NpcBehaviorSystem.hpp:72-87`) — append, do not reorder:**
```cpp
  float horizontalAngleDeg = 0.0F;   // [0,180] facing(XZ) vs delta(XZ); 0 = dead ahead
  float verticalAngleDeg   = 0.0F;   // [-90,90] asin(delta.y/dist); + = target above
  bool  inVerticalCone     = false;  // |verticalAngleDeg| <= config.verticalHalfAngleDegrees
  bool  perceived          = false;  // final verdict (wired in P2; stays false in P1)
  enum class Los : std::uint8_t { Clear, Blocked, Unknown };
  Los   los                = Los::Unknown;   // tri-state; Unknown NEVER grants sight
```
`targetInVisionCone` keeps its existing meaning (the **horizontal** cone gate). Do **not** touch the request's
`targetHasLineOfSight` bool yet — that dies in P2/P3 (§16), not here.

**2. Extend `NpcBehaviorConfig` (`:23-35`) — append beside `visionHalfAngleDegrees` (`:31`):**
```cpp
  float verticalHalfAngleDegrees = 30.0F;   // pitch FOV half-angle — the verticality gate
  float guardEyeHeightMeters = 1.6F;
  float targetStandEyeHeightMeters = 1.6F;
  float targetSneakEyeHeightMeters = 0.9F;
  float occlusionMarginMeters = 0.05F;
```

**3. Config (de)serialization:** grep where `NpcBehaviorConfig` round-trips (scenario TOML codec /
`SaveCodec`-adjacent). Add parse + emit for the 5 fields. **Back-compat law:** a missing key → the default
above, never zero (`verticalHalfAngleDegrees=0` would blind the guard when P2 lands).

**4. Pin-tests (the promised new coverage):**
- Default-constructed result: new fields at documented defaults, `los == Unknown`, `perceived == false`.
- Config round-trip: emit → parse → all 5 fields survive; parse-with-missing-keys → defaults (not zero).

## Gates

Build green **all targets incl. `tools/`** · `ctest` green + the new pin-tests · golden byte-identical ·
`docs/perception-3d-maximum.md` P1 line flipped in §12 on landing.

## Explicitly NOT in this card

No 3D math, no cone/LOS behavior change, no debug drawing, no FSM wiring, no deletion of the fail-open bool —
those are P2–P5. This slice must be releasable with **zero observable behavior change**.
