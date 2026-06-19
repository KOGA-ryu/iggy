# 59 Native Static Mesh Asset Data Model Status Sync

Status: complete.

## Goal

Sync planning and API docs after native no-Qt rendering added a backend-free CPU
static mesh asset data model for the existing cube fallback path.

## Integrated Commit

- `ec62cde0 Add native static mesh asset data`

## Integrated Surface

- `engine/apps/native_play/NativeStaticMeshAsset.hpp` adds an app-local
  backend-free CPU mesh asset model.
- `NativeStaticMeshVertex` uses `Vec3 position` and
  `std::array<float, 3> color` to preserve the current vertex-color pipeline
  shape.
- `NativeStaticMeshAsset` stores vertices plus `std::uint16_t` indices.
- Inline validation covers non-empty vertices, non-empty indices, index range
  checks, and current `std::uint32_t` draw-count fit.
- `NativeCubeStaticMeshAsset()` preserves the previous cube positions, colors,
  and indices exactly.
- `NativeVulkanRenderer.cpp` now consumes `NativeStaticMeshAsset` for cube
  upload.
- Vertex binding/attributes use `NativeStaticMeshVertex` while preserving the
  two `vec3` shader inputs.
- Upload remains host-visible/coherent, and indexed draw remains
  `VK_INDEX_TYPE_UINT16`.
- Existing model slot bindings and cube fallback behavior remain unchanged:
  `Floor`, `Wall`, `NpcActor`, and `Player` still use the same cube mesh.
- This is no-loader static mesh data groundwork only.

## Source Verification Facts

- Source reviewer confirmed a clean worktree at `ec62cde0`.
- `git diff --name-only 956263de..HEAD` showed two approved source files:
  `engine/apps/native_play/NativeStaticMeshAsset.hpp` and
  `engine/apps/native_play/NativeVulkanRenderer.cpp`.
- `git diff --check 956263de..HEAD` passed.
- Native build passed.
- Focused product CTest regex passed 10/10.
- Focused render/resource CTest regex passed 9/9.
- `./engine/build/iggy_native_play --help` passed.
- Builder native smoke passed with display access and preserved debug/final-state
  lines.

## Boundaries Preserved

- No `.cpp`, CMake, tests, public renderer API, app shell, product session,
  runtime/product/scene API, draw-list, shader, docs, loader, file IO, glTF,
  material/texture/descriptor/sampler, resource catalog, staging/device-local
  upload, Linux/dGPU, or backend abstraction changes in the source packet.
- No model slot binding behavior changes.
- No CLI/debugger output or gameplay/session/input/scripted-control changes.
- Real file loading, glTF/static model parsing, asset registry/catalog,
  materials/textures/descriptors/samplers, model slot binding to non-cube
  assets, staging/device-local upload, and shared render-server ownership remain
  future gates.

## Verification

- `git merge-base --is-ancestor ec62cde0 HEAD`
- `git diff --check`
- `git status --short --branch`
