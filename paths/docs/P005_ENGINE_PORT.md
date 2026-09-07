# P005: Gallery Workshop engine port

Status: Automated Green; Manual Test Needed. Engine-port implementation and
standalone build/offscreen verification are complete.
Interactive window/pointer acceptance has not been performed. Changes remain
uncommitted. The parent engine and source math cards are unchanged.

## Result and ownership

`paths_gallery` is a separate startup using the Paths native host. It displays
a depth-tested 3D room with a floor/back grid and moving coloured targets.
The developer can navigate the camera, pick objects, create boxes/ramps/open
frames, change position/size/yaw/colour, and edit four waypoint offsets with
loop/ping-pong traversal, dwell times, segment speed multipliers, and scrubbing.
The copied pure waypoint kernel supports up to 32 points; this first UI exposes
four. Patrols use fixed 60 Hz ticks and freeze when paused.

`GalleryScene::dispatch` owns every scene change. UI and scripts submit the
same semantic actions. The scene publishes camera matrices and the exact mesh
used for rendering; picking uses that published frame, the ported ray/AABB
broad phase, and a triangle narrow phase. Frame openings and ramp slopes have
matching visible and selectable geometry. Geometry/camera changes invalidate
old-frame picking until the next frame is published. The right panel is outside
the gallery viewport. Room scenery occludes picking but is not editable.

`NativeVulkanHost` owns the sole device, render pass, buffers, depth image, UI,
and capture. Its scene path is enabled only by the gallery startup. The
existing `paths` executable retains its UI-only startup and question models.

## Port contents

- Exact copies: Vec3, Mat4, ray/AABB and Euler math; fly camera; orbit/pan/dolly
  navigation; both first-room GLSL shaders.
- Bounded extractions: open-frame geometry layout, waypoint types and
  validators, route building, fixed-tick motion planning, and preview sampling.
- Adaptations: parent box/ramp topology, thin-box grid construction, camera
  matrices, and the vertex-colour pipeline. Dynamic rendering was replaced
  by compatibility with the existing Paths render pass and a D32 attachment.
- New integration: pure gallery scene/actions, mesh-based narrow-phase picking,
  workshop controls/CLI, fixed-capacity GPU buffers, embedded shader generation,
  and scene/integration tests.

The exact parent source revision, byte hashes, adaptations, and initial port
hashes are in `MIGRATION_SEED_MANIFEST.json` (entries with purpose
`P005 standalone gallery engine port`). World, collision/rider publication,
Creative document mutation, and the full editor UI were not imported.

## Build and start

From the Paths folder:

```sh
cmake -S . -B build/gallery-port -DCMAKE_BUILD_TYPE=Release
cmake --build build/gallery-port --target paths_gallery -j 6
./build/gallery-port/paths_gallery
```

Dependencies: C++20, CMake 3.24+, SDL3, Vulkan, glslangValidator/glslang.
On this Mac, the existing SDK discovery selects MoltenVK. Shader source and
copied engine helpers are inside Paths; generated SPIR-V is embedded, so the
executable does not need a parent checkout or shader folder at runtime.

For pure models without SDL/Vulkan:

```sh
cmake -S . -B build/gallery-model -DPATHS_BUILD_NATIVE=OFF
cmake --build build/gallery-model --target paths_scene_tests -j 6
ctest --test-dir build/gallery-model -R '^paths_scene_tests$' --output-on-failure
```

## Controls

| Input | Action |
| --- | --- |
| Left click in gallery | Pick the visible object |
| Right drag | Look around |
| WASD with pointer in gallery | Horizontal movement |
| Q / E | Down / up |
| Shift while moving | Faster movement |
| Middle drag | Pan |
| Alt + middle drag | Orbit |
| Mouse wheel | Dolly/zoom |
| F | Frame the room |
| Escape / Pause | Freeze patrols and camera input |
| Reset view | Restore the gallery camera |

Creating a primitive places it in front of the camera and selects it. Numeric
fields edit its origin, size and yaw; waypoint offsets are relative to that
origin. Edits restart the selected route. Loop off means ping-pong. Pause
before scrubbing. Losing window focus pauses automatically. The window has an
800x600 minimum so the scene and controls remain usable.

## Headless integration checks

The script runner accepts one semantic action per frame. With a script,
simulation advances only through explicit `tick SECONDS` commands. A tick is
at most 0.25 seconds. `pick U V` uses normalized coordinates inside the displayed
gallery viewport. Rejected commands fail the run with their source line.

```sh
./build/gallery-port/paths_gallery --offscreen \
  --script tests/gallery_workshop.script \
  --capture build/gallery-port/workshop.png \
  --report build/gallery-port/workshop.json

./build/gallery-port/paths_gallery --offscreen \
  --script tests/gallery_depth.script \
  --capture build/gallery-port/depth.png \
  --report build/gallery-port/depth.json
```

`gallery_workshop.script` exercises the route controls, pause/resume/scrub,
three primitive kinds, and all camera navigation operations. Its final report
must have five objects, selected ID 5, 30 active ticks, and paused=true.

`gallery_depth.script` puts a red box in front of a blue box that is submitted
later, then picks the middle of the viewport. At 1440x900, pixel (525,489)
must remain red (selected tint is RGBA 255,89,89,255), and selected_id must be
2. This tests rendered depth ordering and pointer agreement independently of
the CPU-only scene tests.

## Verification and remaining boundaries

Verification on 2026-09-06:

- Release delivery build produced `build/gallery-port/paths_gallery` and
  `build/gallery-port/paths` inside this project.
- Focused `paths_scene_tests` / `paths_input_tests`: 2/2 passed. Scene coverage
  includes camera/picking agreement, exact frame/ramp geometry, occlusion,
  fixed-tick behavior, pause, invalid edits, and capacity boundaries.
- Copied Paths alone to `/private/tmp/paths-engine-port-isolated/source`.
  With `PATHS_BUILD_NATIVE=OFF`, the scene tests built and passed, including
  the added waypoint arrival, dwell, reverse speed, and loop closure cases.
- Enabled native targets in that copy and built both executables. The compile
  graph contains no `/Users/kogaryu/iggy3d` source/include paths; compiled
  source bytes were checked against the delivered files.
- From outside the original checkout, the copied Gallery Workshop script
  completed with five objects, selected ID 5, 30 ticks, and paused=true.
  The copied Guided Questions startup also rendered successfully with the
  scene path disabled.
- Native depth regression: centre RGBA was `(255,89,89,255)`, and the pointer
  selected front object 2 despite the rear blue object being drawn later.
- Inspected native offscreen captures at 1440x900 and 800x600. The small layout
  keeps the controls scrollable and the scene outside the panel.
- `git diff --check` and all 25 P005 provenance entries passed validation.

Local artifacts:

- [Standalone workshop capture](/Users/kogaryu/.codex/visualizations/2026/09/06/01a075b7-6a99-7852-8463-6a43d75cffc6/paths-engine-port/isolated-workshop.png)
- [Workshop report](/Users/kogaryu/.codex/visualizations/2026/09/06/01a075b7-6a99-7852-8463-6a43d75cffc6/paths-engine-port/isolated-workshop.json)
- [Depth regression capture](/Users/kogaryu/.codex/visualizations/2026/09/06/01a075b7-6a99-7852-8463-6a43d75cffc6/paths-engine-port/depth.png)
- [Minimum-size capture](/Users/kogaryu/.codex/visualizations/2026/09/06/01a075b7-6a99-7852-8463-6a43d75cffc6/paths-engine-port/workshop-800.png)

Production C++/GLSL delta: 21 additional files, +2734/-13 lines (net +2721),
including copied engine source. Build files, tests, and documentation are
counted separately. This is a requested feature port, not a cleanup reduction.

The graphics driver required an unsandboxed headless process on this Mac;
no visible window was opened. This proves offscreen rendering, not interactive
swapchain resizing, drag ergonomics, or human pointer acceptance.

The port is a scene workshop. Answer billboards, the source-card catalog,
equation progression, scoring, durable scores/presets, and twelve analytical
patrol presets remain game work. Scene edits are currently session-only.
The next capability is Equation Gallery on this ported foundation.
