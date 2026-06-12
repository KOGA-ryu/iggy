# Compatibility Debt

Purpose: track temporary bridges introduced to keep build slices source-compatible. Each item should have an intended removal or review condition.

## Active Debt

### Runtime render-cache mirror

Current bridge:
- `RuntimeSessionState::renderCache`
- `RuntimeSessionState::hasRenderCache`

New source:
- `RuntimeSessionState::derivedCaches`
- `LevelDerivedCacheState::render`
- `LevelDerivedCacheState::hasRenderCache`

Reason:
- Existing render-frame call sites and tests used render-only fields before collision cache existed.
- The additive migration lets runtime carry render and collision caches without breaking existing callers.

Removal condition:
- All runtime/render-frame call sites consume `derivedCaches.render` directly.
- Tests cover render-frame cache path through `LevelDerivedCacheState`.
- No builder or runner test needs legacy `renderCache` / `hasRenderCache`.

Review risk:
- Divergent mirror state. Any builder or updater that changes one render cache path must assert the mirror remains consistent.

### Runtime command tick explicit-world overloads

Current bridge:
- `RuntimeSessionCommandTick::run(input, collisionWorld)`
- `RuntimeSessionCommandTickRunner::run(input, collisionWorld)`

New source:
- `RuntimeCollisionWorldProvider`
- session-carried `derivedCaches.collision`

Reason:
- Existing callers can still pass an explicit collision world.
- The provider makes precedence explicit before session cache use becomes common.

Removal condition:
- Decide whether explicit collision override remains a permanent testing/runtime hook.
- If permanent, keep it documented as highest-precedence override instead of treating it as debt.

Review risk:
- Hiding collision source selection inside command execution without tests for precedence.

### Derived cache full rebuild bias

Current bridge:
- collision cache update can rebuild from current map and changed tiles rather than precise object-level replacement.

Reason:
- Keeps cache update policy simple while tile mutation semantics are still new.

Removal condition:
- Only optimize if full rebuild is measurably too expensive or prevents an expected use case.

Review risk:
- Adding incremental complexity before map mutation and runtime integration stabilize.

## Recently Resolved Or Stable

### CMake source/test fragments

Status:
- Root `engine/CMakeLists.txt` delegates explicit source and test lists to subsystem fragments under `engine/cmake/`.

Rule:
- New source/test wiring should update the relevant subsystem fragment, not re-expand the root file.

### Shared test fixtures

Status:
- Shared support helpers exist for AABB assertions, player fixtures, and command frame fixtures.

Rule:
- Add shared fixtures only for repeated mechanics. Keep scenario-specific helpers local when they preserve test intent.

## Debt Review Questions

Ask at every checkpoint:

- Is this bridge still serving compatibility, or has it become accidental API?
- Is there a test proving mirrors stay consistent?
- Can callers use the newer path without losing diagnostics?
- Would removing this now cause broad churn or clarify ownership?

