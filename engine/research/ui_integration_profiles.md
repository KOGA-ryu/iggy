# UI Integration Profiles

This note records the intended high-level UI shape for later integration. It is
not an implementation order and does not open UI/Edi mutation work.

## Principle

The UI should display the game first, not the engine. Authoring and debugging
surfaces should be available as separate workspace profiles so normal play,
scenario construction, validation, and engine inspection do not compete for the
same screen.

The existing UI direction can support multiple far-left profile tabs. Those tabs
should represent workspace modes.

## Far-Left Workspace Profiles

### Play

Purpose: play the scenario.

Primary content:
- World/level view.
- Player character, NPCs, doors, items, and interaction targets.
- Minimal HUD.
- Current objective or local context.
- Inventory quick view.
- Interaction prompts.
- Pause/menu/save controls later.

Hidden by default:
- TOML source-plan details.
- Validator diagnostics.
- Runtime frame internals.

### Build

Purpose: arrange scenario content.

Primary content:
- Level/scenario canvas.
- Terrain, wall, and floor placement.
- Player start placement.
- NPC placement.
- Item/drop placement.
- Interaction target placement.
- Region placement.

This should be spatial and visual. It should not expose raw engine registries as
the primary interaction model.

### Script

Purpose: author scenario sequence facts without becoming a generic scripting
language.

Primary content:
- Authored frame flow.
- Player commands: move, interact, pickup.
- NPC controls: wait, seek, movement mode, target.
- Interaction/pickup timing.
- Frame ids and ordering.
- Expectation authoring entry points.

Non-goal:
- Arbitrary expressions, loops, condition language, or event scripting.

### Check

Purpose: prove the authored scenario works.

Primary content:
- Lint/check/run status.
- Expected vs actual results.
- Final rows.
- Trace frame playback.
- Diagnostics with source locations.
- Summary counts.

This profile should be backed by `RuntimeGameplayAuthoringPreviewModel` and the
existing authoring facade/projection APIs.

### Actors

Purpose: inspect and author NPC/player-facing actor facts.

Primary content:
- NPC actor list.
- Profiles and trait sets.
- Actor positions.
- Actor control preview.
- Movement/debug overlays where useful.

### Items

Purpose: inspect and author inventory/drop facts.

Primary content:
- Item drops.
- Pickup targets.
- Inventory expectations.
- Item ids/counts.
- Future item definitions if a catalog is approved.

### Interactions

Purpose: inspect and author interaction target/effect facts.

Primary content:
- Interaction targets.
- Toggle/required-item state.
- Enabled/disabled preview.
- Required item ids.
- Existing effect metadata.
- Future inspect/talk/event hooks only after gates approve them.

### Package

Purpose: inspect authored package metadata and boundaries.

Primary content:
- Package title.
- Description.
- Authoring version.
- Main scenario path.
- Package validation status.
- Package-level diagnostics.

Non-goal:
- Recursive package discovery.
- Dependency management.
- Asset catalogs.

### Debug

Purpose: inspect engine/runtime state for developers.

Primary content:
- Runtime state tree.
- Command queue.
- Player intents.
- NPC actor/control registries.
- Interaction target/effect state.
- Inventory/drop state.
- Frame summaries.
- Occupancy/collision overlays.
- AI map overlay.
- Navigation/path overlay.
- Save/load snapshot status later.
- Performance counters later.

This profile is for engine inspection, not normal authoring.

### Docs / Examples

Purpose: provide example-driven authoring guidance.

Primary content:
- Canonical fixtures.
- Regression examples.
- Package examples.
- Supported TOML table/key shape.
- "Open as scenario" entry points.

## First UI Milestone

Status: complete for the existing Qt shell read-only preview consumer.

The first safe UI milestone is a read-only preview consumer:
- Accept one explicit TOML file path or package path through
  `iggy_qt_shell --preview PATH`.
- Select preview mode with `--preview-mode run|trace|check|lint`.
- Call the runtime read-only preview model.
- Project preview state through `UiAuthoringPreviewPanelModel`.
- Render the `panel:authoring_preview` panel in the existing Qt shell.
- Show package metadata when present.
- Show diagnostics.
- Show summary/final rows.
- Show trace frames.
- Show expectation result.

Hard stops for the first milestone:
- No editing.
- No save/load mutation.
- No file watching.
- No package scanning.
- No UI-owned parsing.
- No new scenario semantics.

## Product Play UI Projection

Status: complete for the read-only product play panel model, context seam, and
Qt `--play` launch/load/build consumer plus ready-state focus toggle and
ready/focused keyboard product input mapping; runtime/product presentation
camera policy, manual frame request wrapper, product input context projection,
product input accumulator, and Qt manual Step with accumulator output plus
transient current-player-tile context projection are complete; Qt product frame
pump toggle is complete as replaceable app-shell timing; runtime/product actor
debug/material quad projection is complete; runtime/product pointer projection
and explicit primary point/tile accumulator event preservation are complete.
Qt product viewport ownership is complete as an app-shell surface.
Thin Qt mouse `PrimaryTile` consumer is complete for focused ready left-clicks
on the viewport. Scene-only `InteractionTargetSpatialQuery2D` is complete as a
pure spatial lookup primitive, and runtime/product
`RuntimeGameplayProductInteractionTargetQuery` is complete as a read-only
point/tile-center target query plus reach report, and
`RuntimeGameplayProductInputTargetContext` is complete as transient
hovered-target binding context enrichment from that report. Runtime/product
`RuntimeGameplayProductInputFrameTargetContext` is complete as opt-in pre-frame
target-context enrichment over the latest eligible `PrimaryTile` pressed event.
Qt manual Step and Product Frame Pump now consume that helper through the shared
one-frame path, and compact read-only target-context diagnostics now project into
the product play panel. Thin Qt product viewport render command drawing is
complete as a temporary app-shell consumer of existing latest-frame quad
commands. Thin Qt target highlight overlay is complete as a visual annotation
over latest target-context diagnostics. Textured sprites/animation/material
policy, other model-slot file binding, glTF/glb parsing under the future
constrained subset, asset
registry/catalog, materials/textures/descriptors/samplers, package/authoring
asset policy, renderer expansion, backend validation, richer overlays/labels,
richer diagnostics, frame request/play-surface ownership, explicit interaction
execution, product UX/save semantics, and broader UI execution remain gated.

Native `iggy_native_play` scripted controls/debugger is complete as a no-Qt
app-shell harness over the same product input path as keyboard controls. It
supports `--scripted-controls LIST`, `--scripted-control-interval-ms`,
`--debug-scripted-controls`, `--dump-final-state`,
`--expect-player-tiles 'x,y;x,y'`, and `--quit-after-script`; debugger output
reports before/after player tile, frame request/play mode/surface/loop statuses,
input/ignored event counts, accepted/blocked/rejected counts, `npcMoved`, and
render command count. Final-state dump reports player tile, next frame index,
render command count, active input count, and held input count. Expectation
mismatch exits nonzero and reports the actual tile. It does not change gameplay
semantics, runtime/product APIs, Qt behavior, persistence, or render
asset/material/glTF policy.

Native scene draw-list extraction is complete as no-Qt renderer prep:
`NativeSceneDrawList.hpp` defines backend-neutral app-local draw items and pure
transform helpers under `iggy::native_play`, with nullable runtime gameplay state
plus seconds as input. `IggyNativePlay.cpp` adapts current product play state
through `nativeSceneDrawState()` and keeps Vulkan command recording as the
consumer of returned draw items. Fallback player cube, floor y/x order, wall
`tileAt(...)` policy, NPC registry order, player-last ordering, transforms,
tints, inclusion policy, and vector order are intended unchanged. Vulkan
mesh-buffer ownership, renderer/swapchain/pipeline/command-buffer extraction,
SDL/input/scripted-control/free-play/gameplay stepping, Qt behavior, CLI/debugger
output, glTF/assets/textures/materials/animation/shader policy, and visual
behavior changes remain separate.

Native product session extraction is complete as no-Qt app-local product
orchestration: `NativeProductSession.hpp/.cpp` own/delegate product load/play
state, input accumulator, active movement controls, latest frame, presentation
camera, scripted-control cadence/state, final dump state, movement guard, camera
request config, and one-frame product request/tick flow. `IggyNativePlay.cpp`
keeps app-shell responsibilities for CLI parsing/help, SDL key mapping and
event/window lifecycle, plus draw-list/camera orchestration; later native
renderer skeleton extraction moves Vulkan lifetime/draw submission behind
`NativeVulkanRenderer`. The session loads the product scenario before parsing raw
scripted-control specs, preserving pre-extraction load-before-parse ordering;
`ParseArgs` still uses the shared parser only for expected-player-tile count
validation. Output/error strings and normal scripted final-state behavior are
intended unchanged. Runtime/product/scene/server/render-command APIs, gameplay/
input/scripted-control semantics, CLI/debugger output, SDL extraction, renderer
class/skeleton extraction, mesh-buffer ownership, `NativePlayMath.hpp`,
`NativeSceneDrawList.hpp`, glTF/assets/textures/material registry/animation/
shader work remain separate.

Native Vulkan renderer skeleton extraction is complete as no-Qt app-local
renderer separation: `NativeVulkanRenderer.hpp/.cpp` define
`iggy::native_play::NativeVulkanRenderer` with a minimal pimpl surface and
`NativeVulkanFrameInput` carrying a view-projection matrix plus borrowed
draw-item vector pointer. Vulkan lifetime/resources, swapchain, render pass,
pipeline, depth, framebuffers, command pool, command buffers, sync, cube mesh,
recording, acquire/submit/present, recreate, and cleanup now live in
`NativeVulkanRenderer.cpp`; CMake registers that source for `iggy_native_play`.
`IggyNativePlay.cpp` remains CLI/help/validation, MoltenVK fallback setup, SDL
init/window/event loop/destruction/quit, SDL key mapping, product session,
seconds/camera/draw-list orchestration, and per-frame renderer input owner. The
renderer stores only a non-owning `SDL_Window *` for Vulkan interop, consumes
per-call draw items synchronously, and does not depend on `NativeProductSession`,
runtime gameplay state, product loop/frame request, scripted controls, or
`BuildNativeSceneDrawItems(...)`. Debugger CLI/output strings, gameplay/product/
session/input/scripted-control semantics, runtime/product/scene/server/render-
command APIs, `NativePlayMath.hpp`, `NativeSceneDrawList.hpp`, shader behavior,
generalized mesh/resource ownership, glTF/assets/textures/material registry/
animation, Linux/dGPU policy, and backend abstraction remain separate.

Native GPU mesh resource wrapping is complete as no-Qt renderer-private resource
cleanup inside `NativeVulkanRenderer.cpp`: the existing cube mesh path now uses
private `NativeVulkanBufferResource` and `NativeVulkanMeshResource` structs plus
`HasBuffer`/`HasMesh` readiness helpers. Cube vertex/index upload remains
host-visible/coherent and keeps caller-provided usage flags; cube data, all
scene model ids mapping to the single cube mesh, draw order, shader interface,
push constants, tint behavior, and `VK_INDEX_TYPE_UINT16` indexed draw
parameters are intended unchanged. `destroyBuffer(...)` and
`destroyMeshResource(...)` centralize buffer-before-memory destruction,
index-before-vertex cleanup, and reset-to-default idempotence. Public renderer
API, `IggyNativePlay.cpp`, CMake, shader files, runtime/product/scene APIs,
`NativeSceneDrawList.hpp`, CLI/debugger output, app-shell behavior, staging/
device-local upload policy, mesh registries/model slots, asset loaders, file IO,
glTF/assets/textures/materials/animation, Linux/dGPU policy, and backend
abstraction remain separate. The known allocation failure-path risk between
`vkCreateBuffer` and ownership assignment pre-existed the wrapper and remains a
future RAII/allocation exception-safety gate.

Native pipeline/shader resource wrapping is complete as no-Qt renderer-private
resource cleanup inside `NativeVulkanRenderer.cpp`: the existing render pass,
pipeline layout, graphics pipeline, and shader module handles now use private
`NativeVulkanShaderModuleResource` and `NativeVulkanPipelineResource` structs
plus `HasShaderModule`/`HasPipeline` readiness helpers. `createRenderPass()`
fills `pipeline_.renderPass`; `createShaderModule(...)` returns a wrapped shader
module; `destroyShaderModule(...)` centralizes shader module destruction/reset;
command recording and draw code use `pipeline_.renderPass`, `pipeline_.layout`,
and `pipeline_.graphics`; and `destroyPipelineResource(...)` centralizes
graphics pipeline, pipeline layout, and render pass destruction/reset.
Shader filenames, shader stage setup, `pName = "main"`, vertex input, fixed
pipeline state, push constant range, render pass semantics, command-buffer bind
behavior, and swapchain recreate behavior are intended unchanged. Public
renderer API, `IggyNativePlay.cpp`, CMake, shader files, runtime/product/scene
APIs, product session, draw-list data, CLI/debugger output, app-shell behavior,
descriptors/samplers/materials/textures, model slots, asset/glTF loaders,
staging/device-local upload policy, Linux/dGPU policy, and backend abstraction
remain separate.

Native model-slot/cube fallback registry is complete as no-Qt renderer-private
groundwork inside `NativeVulkanRenderer.cpp`: `NativeSceneModelId` now maps to
renderer-private `NativeVulkanModelSlot` values, then `NativeVulkanModelRegistry`
resolves slots to mesh bindings. The initial registry bound `Floor`, `Wall`,
`NpcActor`, and `Player` slots to the same cube fallback; later procedural mesh
packets now bind `Player` to the bean mesh and `NpcActor` to the NPC marker mesh
while the later floor/wall asset binding packet supersedes the terrain slots
when loaded assets validate. `meshForSceneModel(...)`
delegates through `ModelSlotForSceneModel(...)` and `meshForModelSlot(...)`;
draw-item vector iteration/order is unchanged. The same scene model ids, mesh
readiness behavior, draw parameters, shader pipeline, push constants, and tint
behavior are intended unchanged. Public renderer API, `IggyNativePlay.cpp`,
`NativeVulkanRenderer.hpp`, `NativeSceneDrawList.hpp`, CMake, shaders,
runtime/product/scene APIs, product session, tests, CLI/debugger output,
gameplay semantics, descriptors/samplers, textures/materials/assets/glTF, asset
loaders, model file IO, package discovery, authoring asset policy, staging/
device-local upload policy, Linux/dGPU policy, and backend abstraction remain
separate.

Native static mesh asset data model is complete as no-Qt backend-free CPU mesh
groundwork: `NativeStaticMeshAsset.hpp` defines `NativeStaticMeshVertex` with
`Vec3 position` plus `std::array<float, 3> color`, `NativeStaticMeshAsset`
vertices plus `std::uint16_t` indices, inline validation for non-empty vertices/
indices, index range checks, and current `std::uint32_t` draw-count fit, plus
`NativeCubeStaticMeshAsset()` carrying the previous cube positions, colors, and
indices exactly. `NativeVulkanRenderer.cpp` consumes `NativeStaticMeshAsset` for
the cube fallback upload; vertex binding/attributes use `NativeStaticMeshVertex`
while preserving the two-`vec3` shader inputs; upload remains host-visible/
coherent; indexed draw remains `VK_INDEX_TYPE_UINT16`; and the cube fallback was
still the `Floor`/`Wall` mesh at that stage while later procedural helpers used
the same CPU mesh shape for player and NPC model slots. Later floor/wall loaded
assets supersede those terrain slots when valid. This is no-loader static mesh data
groundwork only. Public renderer API, app shell, product session, runtime/
product/scene APIs, draw-list data, shader files/interfaces, CMake, tests,
loader/file IO, glTF/static model parsing, materials/textures/descriptors/
samplers, resource catalog, staging/device-local upload, Linux/dGPU policy,
backend abstraction, model slot binding behavior, CLI/debugger output, and
gameplay/session/input/scripted-control behavior remain separate.

Native procedural bean mesh slot binding is complete as the first no-Qt
non-cube mesh proof: `NativeStaticMeshAsset.hpp` now provides
`NativeBeanStaticMeshAsset()` as an in-memory procedural mesh using the existing
position/color vertex shape and `std::uint16_t` indexed triangles.
`NativeVulkanRenderer.cpp` creates a separate `playerMesh_` from that asset and
registers only `NativeVulkanModelSlot::Player` to it. `Floor` and `Wall` still
used the cube fallback at that stage, and `NpcActor` is covered by the NPC
marker binding below. Later floor/wall loaded assets supersede those terrain
slots when valid.
The existing shader interface, vertex binding, push constants, item tint,
host-visible/coherent upload path, `VK_INDEX_TYPE_UINT16` draw path, app shell,
product session, draw-list order, camera, CLI/debugger output, and public
renderer API remain unchanged. Loader work, file IO, glTF/static model parsing,
asset registries/catalogs,
materials/textures/descriptors/samplers, staging/device-local upload, Linux/
dGPU policy, backend abstraction, and gameplay/session/input behavior remain
separate.

Native procedural NPC mesh slot binding is complete as the second no-Qt
non-cube mesh proof: `NativeStaticMeshAsset.hpp` now provides
`NativeNpcMarkerStaticMeshAsset()` as an in-memory tapered marker mesh using the
existing position/color vertex shape and `std::uint16_t` indexed triangles.
`NativeVulkanRenderer.cpp` creates a separate `npcMesh_` from that asset and
registers `NativeVulkanModelSlot::NpcActor` to it. `Player` stays on the bean
mesh, and `Floor`/`Wall` still used the cube fallback at that stage. Later
floor/wall loaded assets supersede those terrain slots when valid. The existing shader
interface, vertex binding, push constants, item tint, host-visible/coherent
upload path, `VK_INDEX_TYPE_UINT16` draw path, app shell, product session,
draw-list order, camera, CLI/debugger output, and public renderer API remain
unchanged. Loader work, file IO, glTF/static model parsing, asset registries/
catalogs, materials/textures/descriptors/samplers, staging/device-local upload,
Linux/dGPU policy, backend abstraction, and gameplay/session/input behavior
remain separate.

Native static mesh text loading is complete as app-local no-Qt mesh-file
groundwork: `NativeStaticMeshAssetLoader.hpp` parses a minimal text format into
the existing `NativeStaticMeshAsset` CPU shape. The format supports comments,
blank lines, `v x y z r g b` vertex records, and `tri i0 i1 i2` triangle
records. The text and file loaders return parsed mesh data plus structured
issues for open failures, unknown directives, malformed records, out-of-range
indices, extra tokens, and invalid final meshes. A focused
`native_static_mesh_asset_loader_tests` target covers valid text/file loading
and the main failure modes. The loader packet itself did not bind loaded files
to renderer slots, add CLI options, package discovery, glTF parsing, asset
registries/catalogs, materials/textures/descriptors/samplers, shader changes,
staging/device-local upload, Linux/dGPU policy, backend abstraction, app-shell
behavior, or gameplay/session/input behavior.

Native player mesh asset binding is complete as the first no-Qt loaded mesh
slot binding: `engine/apps/native_play/assets/player.igmesh` is a checked-in
minimal native text mesh asset, `iggy_native_play` receives
`IGGY_NATIVE_PLAY_ASSET_DIR` pointing at `apps/native_play/assets`, and
`NativeVulkanRenderer.cpp` loads `player.igmesh` through
`LoadNativeStaticMeshAssetFile(...)` while creating scene meshes. A valid loaded
asset becomes `playerMesh_` and remains bound to `NativeVulkanModelSlot::Player`;
if loading fails or validates false, the procedural bean remains the silent
fallback. `native_static_mesh_asset_loader_tests` validates the checked-in
player asset fixture. This binds one loaded text mesh to one renderer-private
model slot only: no public renderer API, CLI option, package discovery, glTF/
static model parsing, asset registry/catalog, material/texture/descriptor/
sampler policy, shader change, staging/device-local upload, runtime/product/
scene API, app shell behavior, CLI/debugger output, or gameplay behavior change
is included.

Native NPC mesh asset binding is complete as the second no-Qt loaded mesh slot
binding: `engine/apps/native_play/assets/npc.igmesh` is a checked-in minimal
native NPC text mesh asset, and `NativeVulkanRenderer.cpp` loads `npc.igmesh`
through `LoadNativeStaticMeshAssetFile(...)` while creating scene meshes. A
valid loaded asset becomes `npcMesh_` and remains bound to
`NativeVulkanModelSlot::NpcActor`; if loading fails or validates false, the
procedural NPC marker remains the silent fallback.
`native_static_mesh_asset_loader_tests` validates the checked-in NPC asset
fixture. This binds one loaded text mesh to one renderer-private NPC model slot
only: no public renderer API, CLI option, package discovery, glTF/static model
parsing, asset registry/catalog, material/texture/descriptor/sampler policy,
shader change, staging/device-local upload, runtime/product/scene API, app shell
behavior, CLI/debugger output, or gameplay behavior change is included.

Native floor/wall mesh asset binding is complete as no-Qt loaded terrain mesh
slot binding: `engine/apps/native_play/assets/floor.igmesh` and
`engine/apps/native_play/assets/wall.igmesh` are checked-in native text mesh
assets, and `NativeVulkanRenderer.cpp` loads them through
`LoadNativeStaticMeshAssetFile(NativePlayAssetPath(...))` while creating scene
meshes. Successful loaded assets become `floorMesh_` and `wallMesh_`, and
`NativeVulkanModelSlot::Floor` / `Wall` register to those meshes only when
`HasMesh(...)` succeeds. If either file load fails or validates false, that slot
reuses the existing `cubeMesh_` fallback without duplicate cube GPU mesh upload.
`native_static_mesh_asset_loader_tests` validates the checked-in floor and wall
fixture counts, and Player/NPC bindings remain unchanged. This binds loaded
text mesh assets to renderer-private floor/wall slots only: no public renderer
API, CLI option, CMake, package discovery, glTF/static model parsing, asset
registry/catalog, material/texture/descriptor/sampler policy, shader change,
staging/device-local upload, Linux/dGPU policy, backend abstraction,
runtime/product/scene/draw-list API, app shell behavior, CLI/debugger output, or
gameplay/session/input/scripted-control change is included.

Native static model slot policy is complete as app-local no-Qt value-only model
selection policy: `NativeStaticModelPolicy.hpp` defines `NativeStaticModelSlot`
for `Floor`, `Wall`, `NpcActor`, and `Player`; `NativeStaticModelAssetRef` for
`{ slot, meshFilename }`; `NativeStaticModelPolicy` with a `models` vector; a
stable default `.igmesh` policy for `floor.igmesh`, `wall.igmesh`, `npc.igmesh`,
and `player.igmesh`; and `FindNativeStaticModelAsset(...)` first-match lookup.
The policy has no filesystem/file loading/parsing/GPU/Vulkan knowledge.
`NativeVulkanRenderer.cpp` consumes it only for filenames while preserving the
existing loaded `.igmesh` behavior and fallbacks. This is not a glTF/glb parser,
JSON/GLB dependency, shared render-server move, public renderer API change, app
shell change, runtime/product/scene/server/draw-list API change, shader/material/
texture/descriptor/sampler policy, staging/device-local upload policy,
Linux/dGPU policy, backend abstraction, CLI/debugger output change, or gameplay/
session/input/scripted-control change. Future glTF/glb work remains separately
gated to a constrained subset: one mesh, one primitive, triangles, required
positions, optional vertex colors/default later, indexed `uint16` first, and no
materials, textures, normals, UVs, animation, skins, scene graph, or transforms.

Native static model load reporting is complete as backend-free app-local
inspection over the value-only policy and `.igmesh` loader:
`NativeStaticModelLoadReport.hpp` iterates `Floor`, `Wall`, `NpcActor`, and
`Player`; records per-slot filename/status/fallback/issues/vertex/index counts;
aggregates loaded/failed/missing counts; maps report-only fallbacks to cube,
cube, procedural NPC marker, and procedural bean; and loads only explicit policy
refs through `FindNativeStaticModelAsset(...)` plus
`LoadNativeStaticMeshAssetFile(assetRoot / meshFilename)`. It does not touch
`NativeVulkanRenderer.cpp`, mutate renderer state, or add GPU/Vulkan/SDL
behavior. It is not file discovery, package discovery, registry/catalog,
manifest policy, glTF/glb parsing, JSON/GLB parsing, dependency work, shared
render-server ownership, public renderer API work, shader/material/texture/
descriptor/sampler policy, staging/device-local upload, Linux/dGPU policy,
backend abstraction, CLI/debugger output, or gameplay/input/scripted-control
behavior.

Native static model load report CLI dumping is complete as no-Qt app-shell asset
diagnostics: `iggy_native_play --dump-static-model-load-report` builds the
default policy report against `IGGY_NATIVE_PLAY_ASSET_DIR`, prints compact
stdout, and exits before `NativeVulkanApp` construction/run. It returns 0 only
when all fixed slots are loaded and nonzero for any missing/failed fixed slot.
It does not require `--play`, scripted controls, SDL display availability, or
Vulkan renderer initialization beyond normal binary linkage. Output is an
aggregate `static-model-load-report loaded=N failed=N missing=N` plus one row
per slot with filename/status/fallback/counts/issues; current checked-in assets
report Floor 4/6, Wall 8/36, NpcActor 7/30, and Player 6/24. Existing
play/scripted/debug/final-state output remains preserved except for the added
help option. This does not add glTF/glb/JSON parsing, `.igmesh` schema changes,
discovery/catalog/manifest policy, renderer mutation, runtime/product/scene API
changes, shader/material/texture policy, staging/device-local upload, Linux/dGPU
policy, backend abstraction, or gameplay/input/scripted-control semantics.

Native static mesh text writing is complete as pure app-local `.igmesh`
roundtrip support: `NativeStaticMeshAssetWriter.hpp` serializes the current
loaded text format with a fixed header, ordered vertex rows, ordered triangle
rows, and deterministic newline-terminated output. It reports `InvalidMesh` or
`NonTriangleIndexCount` instead of mutating or repairing non-serializable input,
emits no text on failure, and leaves `IsNativeStaticMeshAssetValid(...)`
semantics unchanged. This is not glTF/glb/JSON parsing, `.igmesh` schema
expansion, file writing, discovery/scanning/catalog/manifest policy, renderer
behavior, app-shell/CLI behavior, public renderer API, draw-list/runtime/product/
scene/server API, shader/material/texture policy, or gameplay/input/scripted-
control semantics.

Native static mesh fixture writer roundtrip coverage is complete as test-only
validation: `native_static_mesh_asset_writer_tests.cpp` now roundtrips the
checked-in renderer-bound floor, wall, NPC, and player `.igmesh` fixtures through
load, write, reload, and canonical second write/idempotence checks, with counts
4/6, 8/36, 7/30, and 6/24 respectively. It also adds procedural NPC marker
write/reload count coverage beside the existing procedural bean coverage. CMake
only adds the test asset root compile definition to the writer test. This does
not change production source, renderer behavior, app shell/CLI, fixtures,
schemas, file writing/export, discovery/catalog/manifest policy, public renderer
API, draw-list/runtime/product/scene/server API, shader/material/texture policy,
or gameplay/input/scripted-control semantics.

Native static mesh built-in export CLI is complete as a no-Qt app-shell
diagnostic: `iggy_native_play --dump-static-mesh-asset NAME` supports exactly
`cube`, `bean`, and `npc-marker`, serializes the selected existing built-in
procedural mesh with `WriteNativeStaticMeshAssetText(...)`, prints raw
deterministic `.igmesh` text to stdout, and exits before `NativeVulkanApp`,
SDL, or Vulkan launch. Unknown names fail nonzero with compact errors, and the
option conflicts with `--dump-static-model-load-report` to avoid ambiguous
stdout formats. This adds only a help/diagnostic path; it does not write files,
rewrite fixtures, accept arbitrary asset paths, normalize checked-in assets,
add glTF/glb/JSON parsing, expand `.igmesh` schema, change renderer behavior,
change runtime/product/scene/server/draw-list APIs, or change gameplay/scripted/
final-state semantics.

Native static mesh export policy is complete as app-local value-only metadata:
`NativeStaticMeshExportPolicy.hpp` records stable built-in export refs for
`cube`, `bean`, and `npc-marker` with default filenames `cube.igmesh`,
`bean.igmesh`, and `npc-marker.igmesh`, provides first-match lookup, and maps
ids to the existing built-in CPU mesh assets. The native export CLI now resolves
through this policy before writing raw `.igmesh`, while preserving accepted
names, unknown-name and conflict failures, and raw stdout output. This is not an
output writer, arbitrary path input, asset registry/catalog, discovery/scanning,
glTF/glb/JSON parser, `.igmesh` schema expansion, renderer behavior change, or
gameplay/scripted/final-state semantic change.

Native static mesh output directory export CLI is complete as a constrained
no-Qt app-shell/file-export diagnostic: `NativeStaticMeshFileExport.hpp`
validates policy lookup, existing output directory, directory type, target
nonexistence, writer success, file open, and write success, returning structured
status without printing or throwing for expected validation failures.
`iggy_native_play --dump-static-mesh-asset NAME --output-dir DIR` writes
`DIR/defaultFilename` from the export policy and prints compact status such as
`static-mesh-export name=cube output=/tmp/iggy-native-export-smoke/cube.igmesh bytes=523`.
Raw stdout dumping remains unchanged without `--output-dir`, and the CLI still
rejects output-dir without dump, unknown assets, report conflicts, and existing
targets. This does not create directories, overwrite, force, delete, rename,
rewrite fixtures, accept arbitrary output paths, add discovery/catalog/manifest
policy, parse glTF/glb/JSON, expand `.igmesh`, change renderer behavior, or
change gameplay/input/scripted/final-state semantics.

Native static mesh file export status text helper extraction is complete without
changing visible export output or CLI behavior. The central
`NativeStaticMeshFileExportStatusText(...)` helper now lives in
`NativeStaticMeshFileExport.hpp` beside `NativeStaticMeshFileExportStatus`.
Single-export and batch-export compact CLI failure paths use it, preserving
failure prefixes and status strings including `TargetAlreadyExists`,
`InvalidPolicy`, and `WriteFailed`. This does not change single/batch success
output, export status assignment, preflight/write order, issue counts, output
path selection, sidecar writes, no-overwrite/no-create-directory behavior, CLI
parser/dispatch/conflicts, verifier/package-directory/report/generated text
behavior, renderer/model-slot behavior, or parser scope.

Native static mesh built-in batch export CLI is complete as a constrained
no-Qt app-shell batch export: `ExportNativeStaticMeshPolicyToDirectory(...)`
preflights the output directory and all default target filenames before writing,
then `iggy_native_play --export-static-mesh-assets --output-dir DIR` writes
`cube.igmesh`, `bean.igmesh`, and `npc-marker.igmesh` and prints a compact
status such as
`static-mesh-export-batch output=/tmp/iggy-native-export-batch-smoke exported=3 bytes=33879`.
Single-asset stdout and single-asset output-dir behavior remains unchanged, and
batch conflicts with single-asset dump and static model load report modes. This
does not add arbitrary output paths, overwrite/force/delete/rename behavior,
directory creation, fixture rewrites, package discovery/scanning, registry/
catalog/manifest policy, glTF/glb/JSON parsing, `.igmesh` schema expansion,
renderer behavior, or gameplay/scripted/final-state semantic changes.

Native static mesh export report CLI is complete as a no-write app-shell
diagnostic: `NativeStaticMeshExportReport.hpp` reports export name, default
filename, built-in id, writable status, issue count, vertex count, index count,
and writer byte count for built-in export policy assets.
`iggy_native_play --dump-static-mesh-export-report` prints aggregate and per-
asset rows, exits before native app construction or SDL/Vulkan startup, and
conflicts with output-dir, single export, batch export, and static model load
report modes. It does not write files, validate output dirs, inspect the
filesystem, change renderer behavior, expand `.igmesh`, add asset discovery, or
change gameplay/scripted/final-state semantics.

Native static mesh export report status text helper extraction is complete
without changing visible report output or CLI behavior. The central
`NativeStaticMeshExportReportStatusText(...)` helper now lives in
`NativeStaticMeshExportReport.hpp` beside `NativeStaticMeshExportReportStatus`.
Export report row rendering uses it, preserving the default summary and asset
rows byte-for-byte, including `status=Writable`, vertices, indices, bytes,
issues, row order, and trailing newlines. This does not change report
construction, writer/policy behavior, CLI parser/help/dispatch/conflicts,
filesystem/write behavior, verifier/package-directory/file export/manifest/
package manifest behavior, renderer/model-slot behavior, or parser scope.

Native static mesh export policy validation is complete as backend-free and
filesystem-free guard logic: `ValidateNativeStaticMeshExportPolicy(...)` reports
empty names, duplicate names, empty default filenames, filename separators, and
duplicate default filenames before file export work starts. Single and batch
exports now return `InvalidPolicy` with issue counts before lookup, directory
checks, target preflight, writer work, or writes; the default policy and valid
default report/batch outputs remain unchanged. This does not add package
manifests, sidecar output, discovery/catalog policy, fixture canonicalization,
overwrite/create-directory policy, renderer loading cleanup, glTF/glb/JSON
parsing, `.igmesh` schema changes, or gameplay/scripted/final-state changes.

Native static mesh export manifest CLI is complete as a no-write app-shell
diagnostic: `NativeStaticMeshExportManifest.hpp` builds deterministic manifest
text by validating the export policy and reusing the export report for stable
asset counts and writer byte counts. `iggy_native_play
--dump-static-mesh-export-manifest` prints rows such as
`static-mesh-export-manifest version=1 assets=3 bytes=33879`, then `cube`,
`bean`, and `npc-marker` filename/count/byte rows, and exits before native app
construction or SDL/Vulkan startup. The mode conflicts with output-dir, batch
export, export report, single mesh dump, and static model load report modes.
Existing valid export report and batch export output remain unchanged. This
does not write sidecar files, create package/export manifest files on disk,
create directories, overwrite files, accept arbitrary output paths, rewrite
fixtures, add discovery/catalog policy, parse JSON/glTF/glb, change renderer
behavior, or change gameplay/scripted/final-state semantics.

Native static mesh batch manifest sidecar export is complete as constrained
batch-export file output: `iggy_native_play --export-static-mesh-assets
--output-dir DIR` now writes the three built-in `.igmesh` files plus
`static-mesh-export-manifest.txt`. The sidecar content is exactly the manifest
builder text, and batch success output includes `manifest=...` and
`manifestBytes=...` fields. Batch export preflights the sidecar target before
mesh writes and reports `TargetAlreadyExists` on the sidecar path without
overwriting or writing mesh files. Single-asset stdout and output-dir export
remain unchanged and write no sidecar. This does not add package discovery,
registry/catalog expansion, package semantics, overwrite/force/create-directory
policy, fixture rewrites, renderer behavior, JSON/glTF/glb parsing, `.igmesh`
schema expansion, or gameplay/scripted/final-state changes.

Native static mesh export directory verification CLI is complete as a read-only
app-shell diagnostic: `VerifyNativeStaticMeshExportDirectory(...)` validates
policy, requires an existing output directory, compares
`static-mesh-export-manifest.txt` exactly to the manifest builder text, compares
`static-mesh-export-package-manifest.txt` exactly to package manifest builder
text, then loads only expected policy files and checks counts against the export
report.
`iggy_native_play --verify-static-mesh-export --output-dir DIR` exits before
native app construction or SDL/Vulkan startup and prints
`static-mesh-export-verify ... verified=3 manifest=ok packageManifest=ok` on
success. Missing mesh sidecars fail with `MissingManifest`; missing or
mismatched package sidecars fail with `MissingPackageManifest` or
`PackageManifestMismatch`.
Package sidecar verification happens after mesh manifest equality and before
asset geometry checks. The verifier now reads the package sidecar through the
explicit-file reader before exact deterministic text comparison; malformed or
unreadable sidecars fail as `PackageManifestReadFailed` and report
`packageManifest=invalid`. Parse-valid exact mismatches remain
`PackageManifestMismatch` with `packageManifest=mismatch`. For
`PackageManifestReadFailed`, structured package manifest read issues are
preserved on the verification result and report rows are emitted after the
summary and before asset rows, for example
`packageManifestReadIssue code=MalformedHeader line=1 token=static-mesh-export-package`.
Missing package sidecars, parse-valid mismatches, and mesh-manifest failures do
not emit package read issue rows. Extra unrelated files are ignored; the
verifier remains read-only and explicit-directory-only and does not add package
directory reading, semantic package acceptance, discovery/scanning, package
semantics, source mutation, overwrite/create-directory policy, fixture rewrites,
renderer behavior, JSON/glTF/glb parsing, `.igmesh` schema changes, or
gameplay/scripted/final-state changes.

Native static mesh export directory verification status text helper extraction
is complete without changing visible report text or CLI behavior. The central
`NativeStaticMeshExportDirectoryVerificationStatusText(...)` helper now lives in
`NativeStaticMeshExportDirectoryVerification.hpp` beside the verifier status
enum. Verification report summary and per-asset statuses plus both compact CLI
failure paths use it, preserving strings including `MissingManifest`,
`PackageManifestReadFailed`, and `PackageManifestBuildFailed`. This does not
change CLI parser/dispatch/help/success output, verifier logic, status ordering,
issue counts, problem paths, verified flags, entry data, sidecar matching,
generated sidecar/export behavior, package acceptance, package-directory
diagnostics, renderer/model-slot behavior, or parser scope.

Native static mesh package manifest read issue text helper extraction is
complete without changing visible report text or CLI behavior. The central
`NativeStaticMeshExportPackageManifestReadIssueCodeText(...)` helper now lives in
`NativeStaticMeshExportPackageManifest.hpp`; verification report and package-
directory report package manifest read issue rows route through it. The nested
mesh manifest issue-code mapper remains separate because it maps
`NativeStaticMeshExportManifestReadIssueCode`. This does not change row order,
issue counts, reader/verifier/package-directory read behavior, exact sidecar
matching, generated sidecar/export behavior, package acceptance, renderer/model-
slot behavior, `.igmesh` loading beyond existing verifier behavior, parser
scope, or gameplay semantics.

Native static mesh export manifest read issue text helper extraction is complete
without changing package-directory report text or CLI behavior. The central
`NativeStaticMeshExportManifestReadIssueCodeText(...)` helper now lives in
`NativeStaticMeshExportManifest.hpp`; package-directory report nested mesh
`manifestReadIssue` rows route through it. This mirrors the package manifest
helper extraction while staying on the separate
`NativeStaticMeshExportManifestReadIssueCode` enum. This does not change summary
fields, row order, issue counts, tokens, trailing newlines, `readOk()`, CLI exit
behavior, reader/package-directory read behavior, exact verification, generated
sidecar/export behavior, package acceptance, package manifest helper behavior,
renderer/model-slot behavior, `.igmesh` loading beyond existing verifier
behavior, parser scope, or gameplay semantics.

Native static mesh package directory reading is complete as a separate
header-only explicit-directory helper. `ReadNativeStaticMeshExportPackageDirectory(...)`
reads only `static-mesh-export-package-manifest.txt` through the explicit-file
reader, then projects the nested mesh manifest path and asset row paths in
manifest row order. It does not compare generated default text, verify nested
mesh manifests, parse mesh export manifests, load `.igmesh` assets, check
geometry, check nested file existence, scan directories, reject extra files,
reconstruct export policy/built-in ids, integrate CLI/export/verification/
renderer/package loading, or change native app behavior.

Native static mesh package directory read status text helper extraction is
complete without changing visible report text or CLI behavior. The
`NativeStaticMeshExportPackageDirectoryReadStatusText(...)` helper now lives in
`NativeStaticMeshExportPackageDirectoryReader.hpp` beside the read status/result
types and returns `Read`, `MissingDirectory`, `DirectoryNotDirectory`,
`PackageManifestReadFailed`, or `Unknown`. Package-directory report summary
rendering and compact CLI failure status text keep using the same helper name
through includes. This does not change row order, issue counts, `readOk()`, CLI
parser/dispatch/exit behavior, package-directory reader status/data behavior,
exact verification, generated sidecar/export behavior, package acceptance,
write policy, renderer/model-slot behavior, `.igmesh` loading beyond existing
verifier behavior, parser scope, or gameplay semantics.

Native static mesh package directory reporting is complete as a no-write report
builder over the directory reader. The report summary includes status,
directory, asset count, issue count, and available package/nested manifest
paths; successful reports print parsed asset rows in package-manifest row order,
and package manifest read failures print deterministic
`packageManifestReadIssue` rows. It also parses the already-projected nested
mesh manifest path for diagnostics only, appending `manifestRead=ok|invalid`
and `manifestReadIssues=N` summary facts and deterministic
`manifestReadIssue` rows without changing `readOk()`, CLI exit behavior, asset
rows, package directory reader status/data, verification, export, or package
loading. When the nested manifest reads successfully, it also emits
diagnostic-only `manifestAsset=` rows for each parsed mesh manifest asset before
the package-declared `asset=` rows. It does not load `.igmesh`, validate
geometry, compare package rows to mesh rows, inspect directories beyond known
paths, or change native app behavior.

Native static mesh package directory report CLI is complete as a thin native
app-shell dump over the report builder. `iggy_native_play
--dump-static-mesh-export-package-directory-report --output-dir DIR` prints the
report before SDL/Vulkan startup and returns success only when the directory
reader reports `Read`; failures print report text first, then use the existing
compact `iggy_native_play:` error path. The CLI remains non-verifying: missing
nested mesh manifests still report `Read` with projected paths, file facts, and
`manifestRead=invalid` diagnostics, and the flag does not add package loading,
scanning, export mutation, renderer behavior, or native app play/session
behavior changes.

Native static mesh package directory presence diagnostics are complete as
read-only report fields. The package directory report now appends
`manifestExists=1|0` when the nested manifest path is available and
`exists=1|0` on each declared asset row. These facts do not change `readOk()`,
CLI exit status, verification semantics, export behavior, directory scanning,
mesh loading, generated-text comparison, or package loading.

Native static mesh package directory file fact diagnostics are complete as
additional read-only report fields. The package sidecar summary now includes
`packageManifestExists`, `packageManifestRegularFile`, and
`packageManifestBytes`; the nested manifest summary includes
`manifestRegularFile` and `manifestBytes`; asset rows include `regularFile` and
`bytes`. Missing paths, directories, and size failures report
`regularFile=0 bytes=0`, and these facts do not change `readOk()`, CLI exit
status, verification semantics, export behavior, directory scanning, mesh
loading, generated-text comparison, or package loading.

Native static mesh export manifest text reading is complete as an in-memory
reader for the generated mesh export manifest grammar. It validates version,
count, byte, row, basename-only filename, unsigned numeric row fact, duplicate,
and total byte invariants without package-directory integration.

Native static mesh export manifest file reading is complete as an explicit
supplied-path helper over that text reader. It opens the supplied path in binary
mode, reports one `FileOpenFailed` issue with `line=0` and
`token=path.string()` when opening fails, and delegates successful reads to the
unchanged text reader. The nested mesh manifest remains unintegrated with
verification/export behavior: no default path composition outside explicit
callers, `.igmesh` loading, geometry validation, CLI/export/verification
behavior change, or policy/built-in id reconstruction was added.

Native static mesh package directory manifest read diagnostics are complete as a
report-only use of that explicit file reader. When the package directory reader
succeeds, the report reads the already-projected nested mesh manifest path and
prints `manifestRead=ok manifestReadIssues=0` or
`manifestRead=invalid manifestReadIssues=N`, with deterministic
`manifestReadIssue` rows for missing or malformed nested manifests. `readOk()`
and CLI exit status are unchanged, so missing/malformed nested mesh manifests
remain `status=Read` diagnostics and do not suppress package asset rows. This
does not add package directory reader status/data changes, verification/export
semantics, generated-text comparison, package acceptance validation,
package-vs-mesh row comparisons, `.igmesh` loading, geometry validation,
discovery/scanning, source mutation, or write behavior.

Native static mesh package directory manifest asset rows are complete as
diagnostic-only report rows. Valid nested mesh manifests now add
`manifestAsset=<name> filename=<filename> vertices=<vertexCount> indices=<indexCount> bytes=<byteCount>`
rows after any manifest read issues and before package asset rows. Missing or
malformed nested mesh manifests still emit no `manifestAsset=` rows and keep
package asset rows present with `status=Read` / CLI exit 0. These rows do not
load `.igmesh`, validate geometry, traverse asset paths, add mesh-manifest file
facts, change renderer behavior, or change write behavior.

Native static mesh package directory manifest row comparison diagnostics are
complete as diagnostic-only report rows. When the nested manifest reads
successfully, the report compares package sidecar rows to parsed mesh manifest
rows by asset name and filename only, appends
`manifestMatches=N manifestMismatches=N manifestComparisonIssues=N`, and emits
deterministic `manifestComparison` rows for missing-from-manifest,
missing-from-package, and filename-mismatch cases. `manifestComparisonIssues`
equals the emitted comparison row count and does not alter core `issues=` or
`report.read.issueCount`. Valid default exports report
`manifestMatches=3 manifestMismatches=0 manifestComparisonIssues=0` and no
comparison rows. Missing/malformed nested manifests emit no comparison summary
or rows and keep existing `manifestReadIssue` diagnostics. These rows do not
add package acceptance, verification, nonzero CLI behavior,
issue-count/status semantics, generated-text comparison, `.igmesh` loading,
geometry validation, policy reconstruction, discovery/scanning, source mutation,
or write behavior.

Native static mesh package directory manifest comparison helper extraction is
complete as a behavior-preserving app-local report structure. The extracted
surface is `NativeStaticMeshExportPackageDirectoryManifestComparisonResult` plus
`CompareNativeStaticMeshExportPackageDirectoryManifestRows(...)`, and it
preserves row ordering, summary counts, `readOk()`, CLI exit behavior, and report
output text. Comparison rows remain package-order `MissingFromManifest` /
`FilenameMismatch` first, then manifest-order `MissingFromPackage`. Combined
mismatch coverage proves `manifestMatches=1 manifestMismatches=3
manifestComparisonIssues=3` with row order `FilenameMismatch`,
`MissingFromManifest`, `MissingFromPackage`; missing or malformed nested
manifests still emit no comparison summary/rows and keep
`manifestRead=invalid manifestReadIssues=1`. This does not add package
acceptance, verification, nonzero CLI behavior, core `issues=` changes,
package-directory reader status changes, `.igmesh` loading, geometry validation,
policy reconstruction, discovery/scanning, source mutation, renderer behavior,
or write behavior.

Native static mesh package directory structured manifest diagnostics are
complete without changing report text. `NativeStaticMeshExportPackageDirectoryReport`
now exposes `manifestReadAttempted`, `manifestRead`, and `manifestComparison`;
the builder routes existing nested manifest read and comparison data through
those fields instead of local-only variables. `manifestReadAttempted` is set only
after successful package directory read with a projected nested manifest path,
`manifestRead` stores the explicit nested manifest file reader result when
attempted, and `manifestComparison` is populated only when that read succeeds.
Focused coverage proves the structured fields mirror existing output for valid
export, combined mismatch, missing/malformed nested manifests, and
missing/malformed package sidecars. This does not change summary/row text,
`readOk()`, CLI exit behavior, core `issues=`, package directory reader status,
manifest reader behavior, verification/export behavior, package acceptance,
`.igmesh` loading, geometry validation, policy reconstruction, renderer
behavior, or write behavior.

Native static mesh package directory structured file facts are complete without
changing report text. The package directory report now exposes
`NativeStaticMeshExportPackageDirectoryPathFacts`,
`ReadNativeStaticMeshExportPackageDirectoryPathFacts(...)`,
`NativeStaticMeshExportPackageDirectoryAssetFacts`,
`packageManifestFactsRecorded`, `packageManifestFacts`, `manifestFactsRecorded`,
`manifestFacts`, and `assetFacts`. Package sidecar facts are recorded when the
package sidecar path is known, nested mesh manifest facts are recorded when the
nested manifest path is known, and `assetFacts` mirrors package-declared assets
in package row order. The report does not add file facts for nested
mesh-manifest-declared-only rows. Existing text for valid exports, missing or
malformed sidecars/manifests, missing declared assets, and directory-at-asset
cases is unchanged. This does not add rows, summary fields, status changes,
`readOk()` changes, CLI exit changes, core `issues=` changes, verification or
package acceptance semantics, `.igmesh` loading, geometry validation, policy
reconstruction, discovery/scanning, renderer behavior, or write behavior.

Native static mesh package directory report text renderer extraction is complete
without changing visible report text. The new helper
`BuildNativeStaticMeshExportPackageDirectoryReportText(const NativeStaticMeshExportPackageDirectoryReport &report)`
serializes the existing structured report data, and
`BuildNativeStaticMeshExportPackageDirectoryReport(...)` assigns `report.text`
from that helper after collecting the report. The renderer reads existing
structured fields only: `read`, package and nested manifest file facts, package
manifest read issues, nested manifest read diagnostics, manifest comparison, and
asset facts. Summary rows, field order, issue rows, manifest asset rows,
comparison rows, package asset rows, counts, tokens, paths, and newlines are
preserved byte-for-byte. This does not add rows, summary fields, status changes,
`readOk()` changes, CLI exit changes, core `issues=` changes, verification or
package acceptance semantics, `.igmesh` loading, geometry validation, policy
reconstruction, discovery/scanning, renderer behavior, or write behavior.

Native static mesh package directory report data builder extraction is complete
without changing visible report text or CLI behavior. The new helper
`BuildNativeStaticMeshExportPackageDirectoryReportData(const std::filesystem::path &directory)`
collects structured diagnostics only and returns `text` empty. It owns package
directory read, nested mesh manifest read attempt/result, package-vs-nested-
manifest comparison, package sidecar facts, nested manifest facts, and package-
declared asset facts. The full `BuildNativeStaticMeshExportPackageDirectoryReport(...)`
still calls that helper and then assigns `report.text` through
`BuildNativeStaticMeshExportPackageDirectoryReportText(...)`. This does not add
rows, summary fields, status changes, `readOk()` changes, CLI exit changes, core
`issues=` changes, verification or package acceptance semantics, `.igmesh`
loading, geometry validation, policy reconstruction, discovery/scanning,
renderer behavior, or write behavior.

Native static mesh package directory report builder parity coverage is complete
as a test-only update. Existing data-builder/full-builder parity and
text-renderer parity assertions now cover missing-from-manifest comparison,
missing-from-package comparison, filename mismatch comparison, missing directory,
file path instead of directory, malformed package sidecar data-builder parity,
malformed nested manifest, directory at declared asset path, and extra unrelated
file ignored branches. This does not change production source, report text, CLI
behavior, package-directory read behavior/status/issue counts/rows, manifest
comparison row/count semantics, file facts, verifier behavior, exact sidecar
matching, generated sidecar/export behavior, package acceptance, package
directory report/reader internals, package loading/discovery, `.igmesh` loading
beyond existing verifier behavior, renderer/model-slot behavior, or parser
scope.

Native static mesh export verification report CLI is complete as a read-only
report over the existing verifier: `iggy_native_play
--dump-static-mesh-export-verification-report --output-dir DIR` prints a
summary plus per-asset rows with actual/expected counts before native app
construction or SDL/Vulkan startup. Summary rows include compact sidecar states
as `manifest=... packageManifest=...`, using `not-checked` when the package
manifest was not reached after a mesh manifest failure. Failed verification
prints the report before returning nonzero through the compact
`iggy_native_play:` error style. This adds no writes, repair, scanning,
package/catalog/parser/schema/material/texture
behavior, renderer behavior, native app CMake source registration, or gameplay/
scripted/final-state changes.

Native static mesh export verification report text renderer extraction is
complete without changing visible report text or CLI behavior. The pure helper
`BuildNativeStaticMeshExportDirectoryVerificationReportText(const NativeStaticMeshExportDirectoryVerificationReport &report)`
serializes the existing verification report data, while
`BuildNativeStaticMeshExportDirectoryVerificationReport(policy, directory)` still
verifies through `VerifyNativeStaticMeshExportDirectory(policy, directory)` and
assigns `report.text` from the helper. Summary row fields, sidecar state fields,
optional problem path, package manifest read issue rows, asset rows, row order,
paths, tokens, counts, and trailing newlines are preserved byte-for-byte. This
does not change verification data/status/order/issue counts, CLI exit behavior,
exact sidecar matching, generated sidecar/export behavior, package acceptance,
package directory report behavior, package loading/discovery, `.igmesh` loading
beyond existing verifier behavior, renderer/model-slot behavior, or parser
scope.

Native static mesh export verification report data builder extraction is
complete without changing visible report text, verifier behavior, or CLI
behavior. The no-text helper
`BuildNativeStaticMeshExportDirectoryVerificationReportData(const NativeStaticMeshExportPolicy &policy, const std::filesystem::path &directory)`
constructs the report surface, stores
`VerifyNativeStaticMeshExportDirectory(policy, directory)` in
`report.verification`, and leaves `report.text` empty. The full
`BuildNativeStaticMeshExportDirectoryVerificationReport(policy, directory)` now
calls that data builder and then assigns text through
`BuildNativeStaticMeshExportDirectoryVerificationReportText(report)`. Statuses,
verified counts, issue counts, problem paths, sidecar state fields, package read
issue rows, entries, entry statuses/counts, CLI exit behavior, exact sidecar
matching, generated sidecar/export behavior, package acceptance, package
directory report/reader behavior, package loading/discovery, `.igmesh` loading
beyond existing verifier behavior, renderer/model-slot behavior, and parser
scope are preserved.

Native static mesh verification report builder parity coverage is complete as a
test-only update. Existing data-builder/full-builder parity and text-renderer
parity assertions now cover missing output directory, file path instead of
directory, manifest mismatch, package manifest mismatch, corrupt asset/load
failure, and extra unrelated file ignored branches. This does not change
production source, report text, CLI behavior, verifier behavior/status/count/row
semantics, package read issue semantics, exact sidecar matching, generated
sidecar/export behavior, package acceptance, package directory report/reader
behavior, package loading/discovery, `.igmesh` loading beyond existing verifier
behavior, renderer/model-slot behavior, or parser scope.

Native static mesh export package policy is complete as value-only metadata:
`NativeStaticMeshExportPackagePolicy.hpp` defines the stable format id
`iggy:native-static-mesh-export-package`, version `1`, and manifest filename
`static-mesh-export-manifest.txt`, then wraps the default mesh export policy.
Its validation is deterministic and filesystem-free, covering unsupported
metadata, manifest filename separators/collisions, and invalid nested mesh
export policies. It does not add CLI behavior, package IO, discovery/scanning,
parsing, renderer behavior, schema/material/texture changes, or gameplay
semantics.

Native static mesh export package manifest CLI is complete as a no-write
app-shell diagnostic: `NativeStaticMeshExportPackageManifest.hpp` builds
deterministic package manifest text after validating package policy.
`iggy_native_play --dump-static-mesh-export-package-manifest` prints the package
format/version, manifest filename, asset count, and asset filename rows before
native app construction or SDL/Vulkan startup. It does not add package file IO,
reader/parser syntax, package verification integration, discovery/scanning,
write/repair behavior, renderer behavior, `.igmesh` schema changes, or gameplay
semantics.

Native static mesh package manifest text reader is complete as a dependency-free
and filesystem-free in-memory parser for the generated package manifest grammar.
It accepts only the generated header and `asset=NAME filename=FILENAME` rows,
validates metadata, counts, basename-only filenames, duplicate rows, missing
fields, extra tokens, and malformed rows, and does not reconstruct export
policy or built-in ids. It is not wired into CLI, verification, export, file IO,
package directory reading, discovery/scanning, repair/loading, renderer
behavior, schema behavior, or gameplay semantics.

Native static mesh package manifest file reader is complete as an explicit-file
wrapper over that text reader. It opens only the supplied path in binary mode,
reads the full file, delegates to the text parser, and reports `FileOpenFailed`
with line `0` and the supplied path token when opening fails. It does not infer
directories, validate companion meshes, reconstruct policy/built-in ids, or
integrate with CLI, verification/report, export, discovery/scanning, renderer
behavior, schema behavior, or gameplay semantics.

Native static mesh batch package manifest sidecar export is complete as
batch-only file output. `iggy_native_play --export-static-mesh-assets
--output-dir DIR` now writes `static-mesh-export-package-manifest.txt` alongside
the mesh files and `static-mesh-export-manifest.txt`, reports
`packageManifest=...` and `packageManifestBytes=...`, and preflights the package
sidecar before mesh writes. Single export still writes no sidecars, the package
manifest dump remains no-write, and verifier/report paths now require the
package sidecar. This adds no package parser/reader, package discovery,
exact-extra-file validation, renderer behavior, `.igmesh` schema changes, or
gameplay semantics.

The current product play UI projection:
- Extends `UiFeatureContext` with direct product play build/state/latest-frame
  pointers and a presence helper.
- Registers `feature:product_play` and `panel:product_play` in the runtime
  workspace model.
- Projects supplied product play pointers through
  `UiProductPlayModePanelModel`.
- Populates the product play panel only when product play context exists.
- Emits no missing-context diagnostic when product play context is absent.
- Keeps the product play panel hidden by default through existing panel
  assignments/settings behavior.
- Displays rows/counts from existing runtime fields: build/loop status,
  identity paths, loaded/focus/frame index/scenario frame count, latest
  frame/surface status, ignored input count, adapter counts, binding counts,
  step status/frame/counts, and presentation/render counts.

Qt launch consumer:
- `iggy_qt_shell --play PATH` accepts one explicit TOML file, package directory,
  or `package.toml` path for product load/build/play-mode build.
- The shell stores load, loop, play-mode build results and play-mode state in
  `IggyQtShellWindow` for stable UI context pointers.
- The shell sets product play context pointers and reveals the existing
  read-only `panel:product_play`.
- Latest product play frame remains absent until a later frame-step/tick gate.
- `--play` and `--preview` are mutually exclusive and exit 2 when combined.
- Missing `--play` path exits 2.
- Bad filesystem paths still open the shell and display failed load/build state.

Qt focus toggle:
- The View menu exposes a checkable `Product Input Focus` action for ready
  `--play` sessions.
- The action is enabled/applicable only for ready product play state.
- Toggling updates only the durable current product play focus bit, keeps
  product play context pointers stable, clears latest product play frame to
  absent, and refreshes the read-only product play panel.
- Failed-load product play sessions keep the action disabled/non-applicable.
- Launch still starts focused by default through runtime play-mode build
  defaults.

Qt keyboard input mapping:
- Ready, focused `--play` sessions map supported Qt key press/release events
  into app-shell-owned transient product accumulator state.
- The shell stores product controls/events only, not raw `QKeyEvent` objects or
  pointers.
- Product input events are recorded only when product play mode exists,
  play-mode build status is ready, and product input focus is enabled.
- Disabled focus, failed-load, and not-ready play sessions record no event; when
  focus is disabled, accumulator state is cleared.
- Auto-repeat and unsupported keys are ignored. Arrow/WASD map to movement,
  `E`/Return/Enter to interact, `I` to inspect, Space to wait, and Escape to
  cancel.
- Binding context is supplied later when building request frame output; no
  selected target, hovered target, scene/UI model exposure, or settings exposure
  was added.

Runtime presentation camera policy:
- `RuntimeGameplayProductPresentationCamera` is an app-neutral runtime/product
  policy that chooses transient caller-owned `CameraState` plus
  `LevelRenderFrame2DConfig` from product play state and caller-owned config.
- It supports not-loaded previous/fallback camera selection, loaded player
  initialization, previous-camera player follow through existing `CameraRig`,
  follow-disabled previous/fallback behavior, clamp result flags, and render
  config forwarding.
- It does not execute frames, call play-surface build, call product input
  adapter/binding, persist presentation state, or add Qt/UI/CLI behavior.

Runtime manual frame request wrapper:
- `RuntimeGameplayProductFrameRequest` composes presentation camera policy first
  and then calls `RuntimeGameplayProductPlayMode::frame(...)` exactly once with
  selected transient camera/render config.
- It maps nested play-mode frame status to request status, returns carried next
  play-mode state, projects supplied input event count and nested ignored input
  count, and preserves nested camera/play-mode frame results.
- Caller code owns transient input-frame clearing/draining, previous-camera
  storage, latest-frame storage, and presentation state ownership.
- It does not add Qt/UI/CLI behavior, a manual Step button, automatic tick
  loop/frame pump, mouse screen-to-world/tile mapping, raw input persistence,
  save/load, UX semantics, or gameplay semantics.

Runtime input context projection:
- `RuntimeGameplayProductInputContext` projects transient
  `PlayerInputBindingContext2D` for product play input.
- It reports `NotLoaded`, `LoadedWithoutPlayer`, or `Projected`.
- It returns default binding gates and sets only current player tile from loaded
  product play state when a player exists.
- It does not set selected/hovered targets, inspect interaction state, search
  targets, map mouse input, synthesize `PrimaryPoint`/`PrimaryTile`, or add
  cadence/pump behavior.

Runtime input accumulator:
- `RuntimeGameplayProductInputAccumulator` stores product-level transient input
  state only: held movement controls and pending one-shot product events.
- Held movement controls are `MoveNorth`, `MoveSouth`, `MoveWest`, and
  `MoveEast`; one-shot controls are `Interact`, `Inspect`, `Wait`, and
  `Cancel`, plus explicit `PrimaryPoint` and `PrimaryTile`.
- Held movement presses are deduplicated, releases remove held movement, and
  one-shot and primary releases are no-ops.
- Frame output emits held movement `Pressed` events each requested frame before
  one-shot events, carries supplied binding context, preserves held controls,
  preserves explicit primary point/tile payloads, drains one-shots, and never
  synthesizes `PrimaryPoint`/`PrimaryTile`.
- `clear(...)` returns empty transient state.

Runtime pointer projection:
- `RuntimeGameplayProductPointerProjection` maps viewport-local points through
  existing `CameraView` normalization to world point plus `tileForPoint(...)`
  tile.
- It uses `CameraState`, `CameraView`, `CameraViewConfig`, and `TileCoord` only;
  no Qt types.
- It preserves `CameraView` behavior for negative viewport dimensions,
  non-positive zoom, and zero-axis degenerate viewports.
- It returns status/flags/world point/tile for a future consumer to choose
  `PrimaryTile`, `PrimaryPoint`, or both under a separate policy gate.
- Point-vs-tile / `PrimaryPoint` emission policy remains future work.

Qt product viewport owner:
- Product play sessions create `QFrame#productViewport` inside `QFrame#mainSlot`,
  including failed/not-ready play sessions.
- The viewport uses zero-margin/zero-spacing layout, expanding size policy, and
  local `QFrame#productViewport` styling only.
- `productViewport_` is reset to null on main-slot rebuilds where no viewport is
  created.
- The viewport is a stable event/render target boundary, not long-term renderer
  semantics, runtime truth, or scene/UI model truth.

Qt product mouse primary-tile consumer:
- `productViewport_` installs a viewport-only event filter for product play
  sessions.
- Focused, ready left mouse press records exactly one transient
  `RuntimeGameplayProductInputEvent2D` with `PrimaryTile` pressed and
  `tile = projection.tile` into the product input accumulator.
- The handler accepts/returns true only when that event is recorded; otherwise it
  falls through.
- The handler normalizes `QMouseEvent::position()` by the viewport pixel size
  into the configured product camera-view span from
  `productPresentationCameraConfig().cameraView.viewportSize` before calling
  `RuntimeGameplayProductPointerProjection`.
- It builds `RuntimeGameplayProductPresentationCamera` read-only from
  `productPlayState_` and the product presentation camera config and stores no
  camera or pointer state.
- Product Step and Product Frame Pump consume the accumulator later.

Interaction target spatial query:
- `InteractionTargetSpatialQuery2D` is scene-only and performs pure spatial
  lookup over `InteractionTarget2DRegistry`.
- It scans enabled targets in registry order, compares Euclidean distance to
  `max(0, target.radius) + max(0, extraRadius)`, picks the nearest eligible
  target, preserves first registry entry for exact distance ties, and supports
  point or tile-center queries.
- It does not project selected/hovered context, execute interactions, check
  reach/LOS/occupancy/pathfinding, define kind-priority/z-order/layer policy,
  or wire Qt/product input.

Runtime product interaction target query:
- `RuntimeGameplayProductInteractionTargetQuery` is an app-neutral runtime/product
  read-only report over current product play state interaction targets.
- It accepts query kind `None`, `Point`, or `TileCenter`, delegates point and
  tile-center lookup to `InteractionTargetSpatialQuery2D`, copies found target
  id/payload into the result, and annotates reach with `InteractionReach2D` when
  the state has a player.
- `NotLoaded`, `MissingQuery`, `TargetNotFound`, and `TargetFound` remain report
  statuses only; there is no top-level `LoadedWithoutPlayer` status and found
  targets remain visible without a player.
- It does not mutate selected/hovered target context, convert `PrimaryTile` to
  `Interact`, execute click-to-interact, use `InteractionPlan2D`, change
  command/gate/effect semantics, persist query results, or add Qt behavior.

Runtime product input target context:
- `RuntimeGameplayProductInputTargetContext` copies a base
  `PlayerInputBindingContext2D` and enriches it from an already-computed product
  interaction target query report.
- Valid `TargetFound` reports with `hasTarget` and a non-empty target id set
  hovered target fields, copy a diagnostic hovered id, and return
  `TargetProjected`.
- Not-loaded, missing-query, target-not-found, invalid found reports, and empty
  target ids return `Unchanged` with the base context copied exactly.
- It preserves selected target fields, existing hover on unchanged paths, current
  player tile, and input gate flags; valid found reports replace only hover.
- Reachability is not required, and reach remains query/report annotation only.
- It does not own hover lifecycle, clear hover on misses, synthesize
  `Interact`/`Inspect` targets, mutate query/product/gameplay/input state, or
  wire frame request/play-surface behavior.

Runtime product input frame target context:
- `RuntimeGameplayProductInputFrameTargetContext` is an opt-in pre-frame helper
  between accumulator output and frame request.
- It copies the input frame, scans events in order, and uses the latest eligible
  `PrimaryTile` pressed event with a tile payload.
- If no eligible event exists, it returns `NoEligiblePrimaryTile` with the copied
  frame unchanged.
- Missing tile payloads and `PrimaryTile` releases are ineligible and preserved
  for existing adapter behavior.
- It queries `RuntimeGameplayProductInteractionTargetQuery` as `TileCenter`,
  forwards spatial/reach configs, applies `RuntimeGameplayProductInputTargetContext`
  to the input frame's base binding context, and replaces only the copied frame
  binding context when projection succeeds.
- It preserves all events unchanged and in order, treats reach as diagnostic
  only, and does not synthesize `Interact`/`Inspect`, inject target ids into
  events, mutate product/input state, or change frame request/play-surface/input
  adapter behavior.

Qt product input frame target context consumer:
- `IggyQtShellWindow::runProductFrameRequestOnce()` calls
  `RuntimeGameplayProductInputFrameTargetContext {}.enrich(...)` after
  accumulator `buildFrame(...)` and after storing `productInputAccumulator_ =
  frame.state`, but before `RuntimeGameplayProductFrameRequest`.
- Manual `Product Step` and `Product Frame Pump` both use this path.
- The call uses current `productPlayState_`, the built frame, and default
  spatial/reach configs.
- Qt passes `enrichedFrame.frame` to the frame request input. `Unchanged` and
  `NoEligiblePrimaryTile` paths still pass the copied unchanged frame without
  branching.
- Qt stores the latest transient helper result beside existing transient input,
  latest-frame, and camera state, then wires a stable const pointer through
  `UiFeatureContext`.
- Product Input Focus disable clears accumulator state plus latest target-context
  diagnostics/pointer; enabling focus does not synthesize diagnostics.
- Widget/body refreshes do not clear the member; the pointer remains stable
  through rebuilds.
- Qt exposes no diagnostics in settings, logs, or status text.
- Product Input Focus gating, mouse event handling, keyboard mapping, pump
  timing, accumulator one-shot drain, held-control behavior, and manual Step
  availability are unchanged.
- Qt remains a thin caller and does not own target lookup semantics.

Product input target-context diagnostics projection:
- `UiFeatureContext` carries a const diagnostics pointer, but
  `uiFeatureContextHasProductPlayMode(...)` ignores it, so diagnostics alone do
  not create product play context.
- `UiRuntimeWorkspaceModel` passes diagnostics into the product panel only when
  product play context is already present.
- `UiProductPlayModePanelModel` adds compact `targetContext` rows and does not
  require latest-frame rows.
- No diagnostics pointer produces no target-context rows.
- Rows are source-shaped and compact: `targetContext.status`,
  `targetContext.hasPrimaryTileEvent`, conditional
  `targetContext.primaryTileEventIndex`, conditional `targetContext.primaryTile`,
  `targetContext.target.status`, `targetContext.target.hasTarget`, conditional
  `targetContext.target.targetId`, `targetContext.target.hasPlayer`,
  `targetContext.target.hasReach`, conditional `targetContext.target.reachable`,
  and `targetContext.projection.status`.
- The projection does not dump copied frames, all events, full target payloads,
  or nested structs.

Qt manual Step consumer:
- The View menu exposes `Product Step` for ready `--play` sessions only.
- The action is independent of `Product Input Focus`; when focus is false,
  existing play-surface behavior ignores input but still consumes one available
  frame with empty intents/context.
- Qt stores accumulator state instead of a latest-edge `productInputFrame_`.
- Key recording maps supported keys to product controls/events and records into
  accumulator state without clearing per key event.
- Focus disable clears accumulator state.
- Executing Step builds accumulator frame output with projected binding context,
  stores the returned accumulator state, enriches a copied input frame target
  context, and builds `RuntimeGameplayProductFrameRequestInput` from current
  `productPlayState_`, enriched frame output, and shell-owned presentation camera
  config, then calls
  `RuntimeGameplayProductFrameRequest {}.run(input)` exactly once.
- The shell updates only replaceable transient app-shell state:
  `productPlayState_`, latest product play frame/context pointer, and
  `productPresentationCamera_`.
- The shell preserves held movement across steps and drains one-shots after each
  executed Step.
- The projected context is not written back into accumulator state.
- If Step is unavailable, accumulator state is not silently cleared.
- The existing read-only product play panel is refreshed after the request.
- Camera/config defaults are shell presentation defaults only and are not
  settings/save truth.

Qt frame pump toggle:
- The View menu exposes a checkable `Product Frame Pump` action for ready
  `--play` sessions only.
- The pump is Qt/app-shell-owned replaceable timing through a window-owned
  `QTimer` at 250 ms / 4 Hz.
- Manual Step and timer ticks share the same one-frame helper: accumulator
  output plus projected input context, returned accumulator state storage, one
  `RuntimeGameplayProductFrameRequest` call, transient shell state updates, and
  product play panel refresh.
- The pump is independent of `Product Input Focus`; focus remains only the input
  gate.
- The timer stops when product play becomes unavailable or not ready, but does
  not auto-stop on `NoFrameAvailable`.
- Pump enabled state, interval, and keybindings are not persisted.

Product actor render projection:
- Runtime presentation can now include untextured debug/material quad commands
  for the current player and present modern NPC actors through
  `RuntimeGameplayProductActorRenderCommands`.
- The commands are appended after level render output by
  `RuntimeGameplayProductPresentationFrame` and can reach Qt product play
  latest-frame presentation data through the existing frame request path.
- Qt does not own actor render semantics, actor state, materials, assets, or
  render persistence.

Thin Qt product viewport render command drawer:
- `QFrame#productViewport` is now a Qt-local paint-capable widget that draws
  existing latest product play frame render commands.
- It consumes only stable, transient app-shell latest-frame data from
  `latestProductPlayModeFrame_`: the level-frame command vector and
  latest-frame camera-view bounds.
- It maps command world bounds to current widget pixel rectangles using latest
  frame camera-view bounds as visible world bounds, normalizes command world
  bounds and mapped `QRectF`, maps x and y directly with no flip, and preserves
  command vector order with no Qt-side layer/order sorting.
- It skips commands when there is no latest frame, widget size is non-positive,
  or camera-view width/height is zero; background still paints normally.
- It draws only `RenderCommand2DType::Quad` commands as untextured flat
  rectangles, ignores texture payloads, and uses Qt-local hardcoded debug colors
  for floor, wall, player, NPC actor, legacy NPC, and fallback materials.
- It remains a temporary app-shell/debug-material renderer over existing
  latest-frame data, not the long-term renderer.

Thin Qt product target highlight overlay:
- `ProductViewportWidget` accepts a const pointer to the latest transient
  `RuntimeGameplayProductInputFrameTargetContextResult` diagnostics.
- `buildMainSlot()` passes
  `hasLatestProductInputFrameTargetContext_ ? &latestProductInputFrameTargetContext_ : nullptr`
  into the viewport.
- The paint path draws render command quads first, then draws the target overlay
  afterward.
- The overlay draws only when latest frame exists, diagnostics pointer exists,
  diagnostics target has `hasTarget = true`, camera-view bounds are
  non-degenerate, and widget size is positive.
- It uses copied diagnostics target payload only: target position and
  non-negative radius. It does not run target queries from paint.
- It maps a world-space marker from position plus/minus radius through the
  existing viewport world-to-pixel helper and uses a Qt-local minimum marker for
  tiny or zero-radius targets.
- Reach is visual annotation only: reachable, unreachable, and no-reach choose
  different local styles, and unreachable targets are still shown.
- The overlay is outline/tint only: no label, target id text, selected marker,
  trail, click animation, command execution, or richer overlay.

Hard stops for product play UI projection:
- No product frame execution, `RuntimeGameplayProductPlayMode::frame(...)`,
  `RuntimeGameplayProductPlaySurfaceFrame::build(...)`, app tick loop, or frame
  pump from scene/UI projection code.
- No raw Qt/device event persistence; supported keyboard mapping may produce
  only transient product input events inside the Qt shell.
- No scene/UI projection-owned execution, frame stepping, app tick loop, or
  frame pump; the Qt shell pump is the separately documented replaceable
  app-shell timing path.
- No product loader/file/package/TOML APIs called from scene/UI model code.
- No calls to `RuntimeGameplayProductPlayMode::frame`,
  `RuntimeGameplayProductPlaySurfaceFrame::build`, loader APIs, or product
  step/run functions from scene/UI.
- No default camera/render config, product input events, presentation frame, or
  latest frame result synthesis in Qt launch.
- No raw Qt key/mouse/focus event routing into product input events from the
  focus toggle.
- No product input events, default camera/render config, presentation frames,
  latest-frame synthesis, frame stepping/manual stepping, app tick loop, or
  frame pump from the focus toggle.
- No `RuntimeGameplayProductInputAdapter::map(...)` or
  `PlayerInputBinding2D::bind(...)` calls from Qt input mapping.
- No Qt mouse behavior beyond viewport-only focused ready left-click
  `PrimaryTile`; no Qt types in runtime/scene/product APIs.
- No `PrimaryPoint`, world-point payload, right/middle/move/wheel/double-click/
  drag/hover behavior, or target lookup from Qt input mapping.
- No settings persistence or keyboard shortcut for the focus toggle.
- No product play mode mutation from scene/UI projection code; Qt launch/focus
  code may update only the durable current focus bit through
  `RuntimeGameplayProductPlayMode::withInputFocus(...)`. Runtime/gameplay state
  mutation remains prohibited.
- No raw Qt event or product input event persistence in
  gameplay/session/product-loop/play-mode/save/settings/scene-UI truth.
- No camera, presentation, render commands, actor render config, or render-frame
  persistence in gameplay/session/product-loop/play-mode/save truth.
- No presentation camera policy output stored in scene/UI models or settings.
- No product frame request invocation, input-frame clearing/draining, previous
  camera storage, latest-frame storage, or presentation state ownership inside
  scene/UI projection code.
- No projected binding context persistence in runtime/session/gameplay/
  product-loop/play-mode state, snapshots, saves, settings, scene/UI models, or
  accumulator state.
- No selected/hovered target discovery, interaction target search, reach lookup,
  mouse screen-to-world/tile mapping, `PrimaryPoint`, or `PrimaryTile` synthesis
  from input context projection. Scene-only spatial lookup is available as a
  primitive, product target query reports are available read-only, and product
  input target context enrichment plus opt-in frame enrichment exist. Qt consumes
  the frame helper only as a thin caller in the shared one-frame path; frame
  request/play-surface ownership remains absent.
- No accumulator state persistence in runtime/session/gameplay/product-loop/
  play-mode state, snapshots, saves, settings, or scene/UI models.
- No raw Qt key/event storage, cadence/rate policy, accumulator-owned frame
  pump, pointer synthesis, target search, or reach lookup in accumulator state.
- No runtime/product semantic ownership by Qt frame pump.
- No pump enabled state, interval, keybindings, input accumulator, pump state,
  camera, latest frame, or render-frame persistence in runtime/session/
  gameplay/product-loop/play-mode snapshots, saves, settings, or scene/UI model
  truth.
- No textured sprite/animation sampling, new art/assets/material registry,
  package discovery, additional Qt mouse behavior, target context wiring,
  target search, reach lookup beyond the read-only product query/enrichment
  helpers, automatic frame request/play-surface ownership of frame enrichment,
  diagnostics persistence or display beyond compact read-only panel rows,
  pause/retry/reset/completion/failure/save-load productization, package
  scanning/watching/discovery, source mutation, raw Qt event persistence,
  projected pointer persistence, viewport geometry/state persistence, render
  command/config persistence, render command drawing beyond the approved Qt
  latest-frame drawer, canvas polish, target highlighting beyond the approved
  thin Qt marker, or runtime/product semantic changes from Qt frame pump,
  viewport ownership, or viewport painting.
- No settings persistence or keyboard shortcut for Qt manual Step.
- No hidden default camera/render config inside the UI model.
- No pause/retry/reset, completion/failure, save/load productization, package
  scanning/watching/discovery, source mutation, or new gameplay semantics.

## Later Milestones

1. Source-linked diagnostics.
2. Visual trace playback.
3. Target discovery/search/reach and interaction execution.
4. Latest-frame presentation integration beyond the read-only panel, if needed.
5. Automatic app tick loop / frame pump.
7. Build canvas for placement.
8. Structured authoring controls for existing facts.
9. Source/TOML roundtrip only after an explicit gate.

## Open Decisions

- Should Play and Build be separate app modes or tabs in one shell?
- Should TOML source view be always visible, optional, or debug-only?
- What is the first editable fact: terrain, actors, interactions, or frame
  commands?
- When does source mutation become safe enough to support?
