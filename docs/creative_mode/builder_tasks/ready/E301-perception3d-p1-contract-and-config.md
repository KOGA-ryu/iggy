# E301 v2 — Perception 3D · P1: contract + config (Mode C, behavior-neutral slice)

> **v2 (premise corrected after a CORRECT stop):** builder invoked the STOP CONDITION — there is **no**
> `NpcBehaviorConfig` serialization site. Verified: saves persist `behavior_profile_id` only
> (`SaveCodec.cpp:1148,1839`; `FixtureScenarioLoader.cpp:690`); config is **derived truth**, expanded from the
> in-code profile catalog by `configFromNpcBehaviorProfile` (`NpcBehaviorProfile.cpp`). So the config work lands
> in the **profile → derivation path**, not a codec. Step 3 rewritten; no save/TOML work in this card.

**STATUS: READY — released from `blocked/` after a clean-tree check.** First card cut under
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

If `NpcPerceptionResult` (`NpcBehaviorSystem.hpp:72-87`), `NpcBehaviorConfig` (`:23-35`), or
`NpcBehaviorProfile`/`configFromNpcBehaviorProfile` (`NpcBehaviorProfile.hpp/.cpp`) differs materially from what
this card states — **STOP and report; do not improvise the contract.**

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

**3. Thread the fields through the profile layer (`NpcBehaviorProfile.{hpp,cpp}`)** — config is derived from
profiles; the profile IS the data layer:
- Mirror the 5 new fields on `NpcBehaviorProfile` with the same defaults.
- Copy them in `configFromNpcBehaviorProfile` (currently copies 8 fields; becomes 13).
- **Heal the pre-existing asymmetry:** `visionHalfAngleDegrees` is in config but NOT profile-carried (built-in
  profiles silently always use 60°). Add it to the profile + derivation too — behavior-neutral because profile
  default == config default (60), and it makes the vision cone per-archetype tunable alongside the new
  vertical field.
- Built-in catalog profiles (`default`/`melee_training`/`passive`) need no edits — member defaults carry them.
- **Back-compat law (restated for this architecture):** default-constructed profile → config carries exactly
  the documented defaults; nothing may default to zero. No codec/TOML work exists or is added.

**4. Pin-tests (the promised new coverage):**
- Default-constructed result: new fields at documented defaults, `los == Unknown`, `perceived == false`.
- Derivation: `configFromNpcBehaviorProfile(NpcBehaviorProfile{})` → all 6 threaded fields (5 new +
  `visionHalfAngleDegrees`) at documented defaults, none zero.
- Thread-through: a profile with overridden values (e.g. `verticalHalfAngleDegrees=45`, `visionHalfAngle=30`)
  → config carries them (proves the copy isn't dropped).

## Gates

Build green **all targets incl. `tools/`** · `ctest` green + the new pin-tests · golden byte-identical ·
`docs/perception-3d-maximum.md` P1 line flipped in §12 on landing.

## Explicitly NOT in this card

No 3D math, no cone/LOS behavior change, no debug drawing, no FSM wiring, no deletion of the fail-open bool —
those are P2–P5. This slice must be releasable with **zero observable behavior change**.
