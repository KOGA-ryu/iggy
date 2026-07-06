# E100: Kernel W3 - Frustum Cull At A Const-Safe Consumption Seam

## Objective

Wire the shipped `core/math/Frustum` kernel into the standalone render submit
path or another const-safe scene-consumption seam so off-screen room meshes can
be culled before drawing.

## Source Brief

- `docs/creative_mode/kernel_wiring_brief_v0_1.md`, section W3.

## Scope

- Start with a read of the standalone frame build/submit path and render
  consumption seam.
- Implement only if there is a const-safe place to filter mesh submission
  without casting away `FrameInput::projections.scene`.
- Likely files:
  - `apps/iggy3d_creative/main.cpp`
  - or a render/backend consumption file if that is the correct const-safe seam.
- Add focused diagnostics/tests only if needed to prove cull counts.

## Required Behavior

- Once per frame, build planes with:
  - `frustumPlanesFromClip(frame.camera.clipFromWorld, ClipDepthRange::ZeroToOne)`
- For each mesh, build its AABB from `position +/- size * 0.5`.
- Keep meshes where `aabbInFrustum(...)` is true.
- Non-finite or degenerate mesh sizes must not crash; handle them conservatively
  and document the policy in the completion brief.

## Hazards

- `FrameInput::projections.scene` is const. Do not cast away const at the frame
  build site.
- Rotated objects may be conservatively over-kept by AABB cull. That is fine.
- This is an invisible perf feature; acceptance needs counts and a no-visual-
  disappearance proof, not just green tests.

## Do Not

- Do not change scene ownership broadly.
- Do not alter RoomBake policy or mesh roles.
- Do not touch product UI routing, save/load, mutation, or descriptors.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Required Reads

- `docs/creative_mode/kernel_wiring_brief_v0_1.md`
- `src/core/math/Frustum.hpp`
- `src/render/FrameInput.hpp`
- `apps/iggy3d_creative/main.cpp`
- the render/backend submit path selected for the implementation.

## Acceptance

- Capture scene still renders correctly when in view.
- A cull diagnostic proves mesh count drops when the camera faces away from a
  large set of room meshes.
- No visible object disappears while on-screen.
- The completion brief identifies the exact const-safe seam used.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative -j10
/Users/kogaryu/iggy3d/build/iggy3d_creative --capture /tmp/iggy3d_kernel_w3_final.png --frames 32 > /tmp/iggy3d_kernel_w3_final.log 2>&1
git -C /Users/kogaryu/iggy3d diff --check
```

Run focused render/unit tests only if touched.

## Completion Brief

Append:

- Files changed:
- Const-safe cull seam:
- Mesh-count evidence:
- Capture artifact:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - E100

- Files changed:
  - `apps/iggy3d_creative/StandaloneFrustumCull.hpp`
  - `apps/iggy3d_creative/StandaloneFrustumCull.cpp`
  - `apps/iggy3d_creative/main.cpp`
  - `tests/unit/standalone_frustum_cull_tests.cpp`
  - `CMakeLists.txt`
  - `cmake/iggy3d_tests.cmake`
  - builder queue bookkeeping files for claim/done movement
- Const-safe cull seam:
  - Standalone app builds the normal `FrameInput` first so
    `frame.camera.clipFromWorld` is available.
  - Immediately before `backend->submitFrame(frame)`, the app builds a local
    culled `SceneProjectionResult` copy using
    `frustumPlanesFromClip(frame.camera.clipFromWorld,
    ClipDepthRange::ZeroToOne)`.
  - `frame.projections.scene` is then pointed at that local culled scene copy.
    No `const_cast` is used, and the original scene ownership remains unchanged.
- Mesh-count evidence:
  - `standalone_frustum_cull_tests` proves a 4-mesh scene keeps 3 meshes,
    culls 1 off-screen mesh, and conservatively keeps 2 malformed meshes.
  - `standalone_frustum_cull_tests` proves an intersecting boundary mesh remains
    visible.
  - `standalone_frustum_cull_tests` proves a 13-mesh scene drops to 1 kept mesh
    when 12 room meshes are wholly outside the frustum.
  - Runtime standalone logs now include `frustumInputMeshes`,
    `frustumKeptMeshes`, `frustumCulledMeshes`, and
    `frustumConservativeMeshes` on the first submit line and final line.
- Capture artifact:
  - Not run for this slice. The card says not to launch a window, and the
    current workflow is avoiding the standalone capture/object-placement script.
    The cull contract is covered by the focused no-window test target.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_frustum_cull_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^standalone_frustum_cull_tests$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - `rg -n "[[:blank:]]$" /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandaloneFrustumCull.hpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandaloneFrustumCull.cpp /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp /Users/kogaryu/iggy3d/tests/unit/standalone_frustum_cull_tests.cpp /Users/kogaryu/iggy3d/CMakeLists.txt /Users/kogaryu/iggy3d/cmake/iggy3d_tests.cmake /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/claimed/E100-kernel-w3-frustum-cull-consumption-seam.md /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/PRIORITY.md`
- Concerns/deferred:
  - Invalid/non-finite/zero-sized mesh position or size is conservatively kept,
    not culled, to avoid accidental visual disappearance.
  - Rotated objects still use the room mesh AABB policy requested by the card;
    this may over-keep, which is acceptable for W3.
  - The next visual proof can use the newly logged cull counts when capture
    runs are allowed again.
