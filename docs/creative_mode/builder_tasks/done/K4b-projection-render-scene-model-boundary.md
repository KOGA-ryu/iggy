# K4b - Projection/Render Scene Model Boundary

## Status

READY. K4 batch step 2 of 6. Claim only after K4a is committed and in `done/`.
Authority:
docs/creative_mode/builder_tasks/blocked/K4-department-dependency-dag-plan.md.

On green completion, commit this card, move it to `done/`, and continue to K4c
without waiting for Reviewer. A STOP pauses the whole K4 batch.

## Goal

Remove the sole forbidden projection -> render include by moving stable bean
model identity into the projection scene value boundary. Preserve all rendered
model ids, mesh geometry, sizes, and GPU behavior.

## Scope

Allowed files:

- add src/projection/scene/SceneModel.hpp
- add src/projection/scene/SceneModel.cpp
- update CMakeLists.txt to compile SceneModel.cpp in iggy3d
- update src/projection/scene/SceneProjection.cpp
- update src/render/mesh/BeanMesh.hpp
- update src/render/mesh/BeanMesh.cpp
- update src/render/vulkan/BufferImageResources.cpp
- update focused bean/projection tests only as needed

No content/runtime migration, policy manifest, receipts/goldens, or app/window
changes.

## Required Ownership

SceneModel.hpp/.cpp owns:

    enum class SceneModelKind : std::uint8_t {
      PlayerBean,
      NpcBean,
      CodexProbeBean,
    };

    std::string_view sceneModelId(SceneModelKind kind);
    bool parseSceneModelId(std::string_view value, SceneModelKind& out);

The stable strings remain byte-identical: bean_player, bean_npc, and
bean_codex_probe.

- SceneProjection.cpp uses SceneModelKind and sceneModelId; it must not include
  a render header.
- BeanMesh consumes SceneModelKind for mesh selection while retaining all
  render-owned geometry/size policy.
- BufferImageResources.cpp uses the same projection-owned model kind when
  parsing room model references.
- Remove BeanModelKind, beanModelId, and parseBeanModelId; do not retain aliases
  or compatibility wrappers.

## Required Invariants

- Player/NPC scene modelRef values are unchanged.
- All three ids still parse to the same geometry and default size.
- No save, replay, receipt, golden, or Vulkan submission behavior changes.
- Graph after this card: no projection -> render; the only remaining SCC is
  content/runtime. The graph is exactly 328 edges, 17 directed pairs, and 16
  unordered pairs.

## Verification

    cmake -S . -B build
    cmake --build build --target bean_mesh_tests projection_tests
    ctest --test-dir build -R '^(bean_mesh_tests|projection_tests)$' --output-on-failure
    python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --format json
    rg -n '#include[[:space:]]+[<"]render/' src/projection
    git diff --check

The projection include grep must return no match.

## Stop Conditions

- A stable model-ref string, bean mesh size, vertex/index result, or GPU path
  would change.
- The boundary requires a render alias or a new shared/common folder.
- Any content/runtime or policy file would need to change.

## Completion Brief

Append the deleted render dependency, new projection value owner, exact focused
test results, graph delta, and confirmation that stable ids/mesh behavior are
unchanged. On success, commit and continue to K4c.
