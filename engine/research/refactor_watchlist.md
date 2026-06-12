# Refactor Watchlist

Purpose: track APIs likely to need cleanup after feature paths stabilize.

## Active Watch Items

### Runtime render-cache mirror fields

Files:
- `engine/src/runtime/RuntimeSessionState.hpp`

Watch:
- `RuntimeSessionState::renderCache`
- `RuntimeSessionState::hasRenderCache`
- `RuntimeSessionState::derivedCaches`

Reason:
- Legacy render-only fields mirror the newer `LevelDerivedCacheState` path.

Cleanup trigger:
- All render-frame/session code can consume `derivedCaches.render` directly.

Risk:
- Mirror drift if a builder updates one path but not the other.

### Collision world source overloads

Files:
- `engine/src/runtime/RuntimeSessionCommandTick.hpp`
- `engine/src/runtime/RuntimeSessionCommandTickRunner.hpp`

Watch:
- overloads that accept explicit `CollisionWorld2D`
- no-override overloads that use `RuntimeCollisionWorldProvider`

Reason:
- The override overloads may be permanent test/runtime hooks, but the policy should stay explicit.

Cleanup trigger:
- Decide whether explicit collision overrides are permanent API.

Risk:
- Callers may not know whether session cache or explicit world is used.

### Full-rebuild derived cache updates

Files:
- `engine/src/scene/level/LevelCollisionCacheState.hpp`
- `engine/src/scene/level/LevelDerivedCacheState.hpp`

Watch:
- collision cache update behavior from changed tiles
- render dirty chunk update behavior

Reason:
- Render cache has chunk-level dirty updates. Collision cache currently favors rebuild simplicity.

Cleanup trigger:
- Performance or API pressure proves object-level collision dirty replacement is needed.

Risk:
- Premature incremental collision complexity before mutation semantics stabilize.

### Runtime command pipeline width

Files:
- `engine/src/runtime/RuntimePlayerCommandStep.hpp`
- `engine/src/runtime/RuntimeSessionCommandTick.hpp`
- `engine/src/runtime/RuntimeSessionCommandTickRunner.hpp`

Watch:
- config/result struct growth
- diagnostics nesting depth

Reason:
- The pipeline preserves diagnostics well, but caller ergonomics may degrade.

Cleanup trigger:
- First real caller finds the API awkward or repetitive.

Risk:
- Hiding diagnostics in a convenience wrapper too early.

### Stale ownership assertion in level derived cache tests

Files:
- `engine/tests/level_derived_cache_state_tests.cpp`

Watch:
- `TestRuntimeAndLevelRuntimeDoNotReferenceDerivedCacheState`

Reason:
- The test name and assertion text predate the accepted migration where `RuntimeSessionState` intentionally carries `LevelDerivedCacheState`.
- The ownership rule is now: `LevelRuntimeState` remains authoritative and cache-free, while `RuntimeSessionState` may carry rebuildable derived caches.

Cleanup trigger:
- Next test-code cleanup or next runtime/session cache slice.

Risk:
- Leaving stale assertions in the test file makes future reviewers distrust the ownership map, even if current test execution remains green.

## Review Cadence

Every 5-8 build slices, check:

- Can a compatibility bridge be removed?
- Are mirrors still tested?
- Did a subsystem start importing a layer it should not know about?
- Did CMake fragment organization remain clean?
- Are docs still describing current code?
