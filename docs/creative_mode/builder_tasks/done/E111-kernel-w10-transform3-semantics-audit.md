# E111: Kernel W10 - Transform3 Semantics Audit

## Objective

Do a read-only decision packet for `Transform3` semantics before any
implementation.

This is a core-kernel audit card. The risk is that `Transform3` stores
`rotationEulerRadians`, while `transformPoint(const Transform3&, Vec3)` applies
only scale + translation. `OrientedBox` separately honors rotation. Before
renaming helpers or adding full-TRS helpers, map current intent and call sites.

## Scope

Read-only except moving this task card through the bucket and appending the
completion brief.

Do not edit source, tests, CMake, docs outside this task card, or commit.

## Required Reads

- `src/core/math/Transform3.hpp`
- `src/core/math/Transform3.cpp`
- `src/core/math/OrientedBox.hpp`
- `src/core/math/OrientedBox.cpp`
- `src/core/math/Mat4.hpp`
- `src/core/math/Mat4.cpp`
- `tests/unit/math_tests.cpp`
- `tests/unit/oriented_box_tests.cpp`
- all direct call sites of `transformPoint(` where overload ambiguity matters

Useful scans:

```sh
rg -n "Transform3|transformPoint\\(" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/apps /Users/kogaryu/iggy3d/tests/unit
rg -n "rotationEulerRadians|makeOrientedBox|toTransform3" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/apps /Users/kogaryu/iggy3d/tests/unit
```

## Questions To Answer

1. Which current `transformPoint(...)` calls use `Transform3`, and what do they
   expect: scale+translate, full TRS, or unclear?
2. Which current `transformPoint(...)` calls use `Mat4`, and are they confusing
   because they share the same function name?
3. Where is `rotationEulerRadians` actually honored today?
4. Would changing `Transform3::transformPoint` to full TRS break runtime,
   creative, or tests?
5. What is the smallest safe implementation slice after this audit?

## Expected Analysis Shape

Produce a concrete table like:

```text
Surface: Transform3 point transform
File/function:
Current behavior:
Expected semantic:
Risk if full TRS:
Suggested owner/API:
Test coverage:
Recommended action:
```

## Likely Outcomes To Evaluate

Do not implement these yet; rank them.

- Add an explicitly named `transformPointScaleTranslate(...)` and leave
  `transformPoint(...)` temporarily as a compatibility wrapper.
- Add a separate `transformPointTrs(...)` that honors Euler rotation through the
  same convention as `OrientedBox`.
- Rename/deprecate only in a later slice after call sites are made explicit.
- If the current ambiguity is harmless, document why and add only guard tests in
  a later slice.

## Do Not

- Do not change `Transform3` behavior in this card.
- Do not add full matrix/quaternion systems.
- Do not alter `OrientedBox`, standalone picking, runtime collision, render,
  save/load, or Creative descriptors.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Verification

Read-only checks only:

```sh
git -C /Users/kogaryu/iggy3d status --short
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files inspected:
- Transform3 call-site table:
- Mat4 overload findings:
- Rotation ownership findings:
- Recommended next implementation slice:
- Risks/blockers:

---

## Completion Brief - 2026-07-06

- Files inspected:
  - `src/core/math/Transform3.hpp`
  - `src/core/math/Transform3.cpp`
  - `src/core/math/OrientedBox.hpp`
  - `src/core/math/OrientedBox.cpp`
  - `src/core/math/Mat4.hpp`
  - `src/core/math/Mat4.cpp`
  - `tests/unit/math_tests.cpp`
  - `tests/unit/oriented_box_tests.cpp`
  - `src/runtime/collision/EntityHitQuery.cpp`
  - `apps/iggy3d_creative/StandalonePicking.cpp`
  - `apps/iggy3d_creative/StandalonePreviewProxies.cpp`
  - `apps/iggy3d_creative/main.cpp`
  - `src/render/vulkan/RenderLoop.cpp`
  - `src/projection/scene/SceneProjection.cpp`
  - `src/projection/debug/DebugProjection.cpp`
  - `src/runtime/save/SaveCodec.cpp`
  - `src/runtime/replay/StateHash.cpp`
  - `tests/unit/entity_hit_query_tests.cpp`
  - `tests/unit/standalone_picking_tests.cpp`

- Transform3 call-site table:

```text
Surface: Transform3 point transform implementation
File/function: src/core/math/Transform3.cpp:19, src/core/math/Transform3.hpp:16
Current behavior: localPoint is component-scaled, then translated. rotationEulerRadians is ignored.
Expected semantic: existing API behaves as scale+translate.
Risk if full TRS: every caller would silently start honoring rotation under the same name.
Suggested owner/API: core/math should split explicit names before behavior changes.
Test coverage: tests/unit/math_tests.cpp:40 validates identity and scale+translate with zero rotation only.
Recommended action: add transformPointScaleTranslate(...) and keep transformPoint(...) as compatibility wrapper first.

Surface: Runtime entity hit-query bounds
File/function: src/runtime/collision/EntityHitQuery.cpp:28
Current behavior: transforms localBounds min/max by scale+translation and builds a sorted AABB.
Expected semantic: scale+translate AABB for runtime entity hurt/target volume. Runtime scene/debug projection also uses localBounds + transform.position at src/projection/scene/SceneProjection.cpp:15 and src/projection/debug/DebugProjection.cpp:17, so runtime entity bounds are not generally rotated today.
Risk if full TRS: using only rotated min/max corners would not be a correct enclosing rotated AABB and would diverge from scene/debug projection. It could change ability/entity-hit behavior for any nonzero entity rotation.
Suggested owner/API: runtime/collision should call transformPointScaleTranslate(...) explicitly until a separate rotated entity bounds policy exists.
Test coverage: tests/unit/entity_hit_query_tests.cpp covers hit order/filter/radius/invalid input but not rotated transforms.
Recommended action: add a guard test pinning nonzero rotation is ignored by current entity hit bounds before any API split.

Surface: Core Transform3 tests
File/function: tests/unit/math_tests.cpp:40
Current behavior: tests identity fields and scale+translate point result with zero rotation.
Expected semantic: scale+translate; rotation omission is not directly pinned.
Risk if full TRS: current tests would not fail for zero rotation, so a breaking semantic change could slip through.
Suggested owner/API: core/math tests.
Test coverage: incomplete for nonzero rotation.
Recommended action: add explicit nonzero-rotation guard for transformPointScaleTranslate(...) and transformPointTrs(...).

Surface: Creative/standalone rotated visual bounds
File/function: apps/iggy3d_creative/StandalonePreviewProxies.cpp:195
Current behavior: converts CreativeTransform to Transform3, then uses makeOrientedBox(...), not transformPoint(Transform3).
Expected semantic: full TRS is owned by OrientedBox for rotated visual/hit proxies.
Risk if full TRS: no direct call-site break, but naming ambiguity can mislead future code into bypassing OrientedBox.
Suggested owner/API: OrientedBox keeps full-TRS geometry; Transform3 gains explicit helper names.
Test coverage: tests/unit/standalone_picking_tests.cpp:102, :120, :141, :162 cover rotated visual picking.
Recommended action: do not alter OrientedBox in the split; reuse its Euler convention for transformPointTrs(...).
```

- Mat4 overload findings:
  - `transformPoint(const Mat4&, Vec3)` is declared at `src/core/math/Mat4.hpp:16` and implemented at `src/core/math/Mat4.cpp:32`. It applies a homogeneous row-major matrix transform and divides by finite non-0/non-1 `w`.
  - Mat4 call sites are projection/render-screen math, not Transform3 placement:
    - `apps/iggy3d_creative/StandalonePicking.cpp:198` and `:226` project world points to NDC for selection/hit UI.
    - `apps/iggy3d_creative/main.cpp:1547` projects the selection label center.
    - `src/render/vulkan/RenderLoop.cpp:197` projects runtime debug/HUD points.
    - `tests/unit/math_tests.cpp:69` covers identity matrix point transform.
  - Compile-time overload resolution is clear because the first argument is `Mat4` vs `Transform3`. Human grep/review ambiguity is real because both overloads are named `transformPoint` while their semantics differ.

- Rotation ownership findings:
  - `Transform3::rotationEulerRadians` is stored on `Transform3`, checked by `isFinite(...)`, serialized/deserialized in `src/runtime/save/SaveCodec.cpp:850` and `:1484`, and included in replay hashing at `src/runtime/replay/StateHash.cpp:45`.
  - Rotation is actually applied by `OrientedBox`, not by `Transform3::transformPoint`. `src/core/math/OrientedBox.hpp:22` documents that this is the first place rotation is honored, and `src/core/math/OrientedBox.cpp:35` defines full TRS placement as `world = translate + rotate(scale . local)`.
  - The Euler convention is explicit in `src/core/math/OrientedBox.cpp:9`: intrinsic X-then-Y-then-Z, `R = Rz * Ry * Rx`, Y-up.
  - Standalone creative rotated picks consume this through `makeOrientedBox(...)`; runtime entity scene/debug bounds and hit-query bounds remain axis-aligned/translation-based today.

- Recommended next implementation slice:
  1. Add `transformPointScaleTranslate(const Transform3&, Vec3)` in `core/math/Transform3.*` with current behavior.
  2. Keep `transformPoint(const Transform3&, Vec3)` as a compatibility wrapper to `transformPointScaleTranslate(...)`.
  3. Add `transformPointTrs(const Transform3&, Vec3)` using the same Euler convention as `OrientedBox::placeLocal(...)`.
  4. Update direct `Transform3` call sites to explicit helpers:
     - `EntityHitQuery.cpp` should use `transformPointScaleTranslate(...)`.
     - core math tests should call and pin both explicit helpers.
  5. Do not rename/remove the old wrapper until call sites are explicit and any runtime rotated-entity policy is deliberately designed.

- Risks/blockers:
  - Changing `transformPoint(const Transform3&, ...)` directly to full TRS is not safe. The runtime hit-query caller would change behavior under the same name, and two rotated min/max corners are not enough to compute a correct enclosing rotated AABB.
  - Current tests do not directly guard nonzero `Transform3::rotationEulerRadians` for the Transform3 overload, so a guard-test slice should precede or accompany the helper split.
  - There is no single runtime-wide rotated entity bounds policy. Scene/debug projection still uses `localBounds + position`, while creative rotated visuals use `OrientedBox`.

- Verification commands run:
  - `git -C /Users/kogaryu/iggy3d status --short`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "transformPoint\\(" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/apps /Users/kogaryu/iggy3d/tests/unit`
  - `rg -n "rotationEulerRadians|makeOrientedBox|toTransform3" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/apps /Users/kogaryu/iggy3d/tests/unit`
