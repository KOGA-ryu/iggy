# P006: Shared gallery target foundation

Status: Automated Green; Manual Test Needed; uncommitted. This implements the
first capability checkpoint in [TARGET_FOUNDATION_DESIGN.md](TARGET_FOUNDATION_DESIGN.md),
using the four proposed new production files. No visible window was opened.

## Delivered behavior

`paths_gallery` has three fixture startup modes. They use a fixed camera,
visible cursor, equal-sized coloured spheres, and stationary answer rows in
the right panel. Correct hits are recorded immediately and shrink the ball;
wrong hits leave the question available without a recovery modal. Equality
Sweep collects both expressions equal to 24, Question Relay alternates two
short questions, and Equation Chain solves `2x + 3 = 11` in three authored
steps. Completed fixtures repeat with prior-exposure evidence.

Every new challenge resets the colour slots and commits a fresh seeded
option-to-colour permutation. Collection keeps the assignment fixed until
the accepted set is complete. Single-answer steps use a bounded swap to avoid
repeating the last successful colour. The run report stores the actual
assignments, assignment rule version, seed, movement preset, and pace.

The header shows cleared targets, wrong answers, aim misses and completed
questions. These are evidence counts. P002's scoring/bonus rule owner is still
pending; this checkpoint deliberately adds no independent points calculation.
The elapsed-time readout uses the scene clock and freezes when paused.

## Canonical owners and removed routes

| Owner | Concrete responsibility |
| --- | --- |
| `src/scene/TargetMotion.hpp/.cpp` — new | Prepared route values, all motion evaluation, and bounded time-based preview seeking |
| `src/runtime/gallery/GallerySession.hpp/.cpp` — new | Typed gameplay commands, target/option/token bindings, assignment records, staged challenge commits, and consumption of judged attempts |
| `GalleryScene` — extended | Object identity, position/appearance, cached sphere topology, visual lifecycle, single simulation accumulator, and published-frame geometry query |
| `LayeredQuestionSession` — extended | Shared immutable content catalog, stable question/version/step/option evidence, accepted and collected sets, judgment, recovery policy and progression |
| Gallery startup/UI — extended | Native input and script translation, fixed answer board, independent Workshop context, report serialization |
| `NativeVulkanHost` — reused unchanged | Vulkan resource ownership and drawing of prepared mesh/UI |

The private selection-only pick calculation was removed. Both workshop Pick
and gameplay Shoot use `hitTestPresentedFrame(SceneFrameId, u, v)`, including
the ray/AABB broad phase and triangle narrow phase. It returns identity,
distance and visual phase without changing selection or judging an answer.
Room geometry and visible popping balls still occlude targets behind them.
Only Active balls can submit an option.

The scene's direct calls to the waypoint planner and distance-progress scrub
were removed. Every object uses `advanceMotion`; waypoint routes delegate to
the existing local port. `seekMotion` replays the same fixed-tick kernel from
a pause checkpoint, for at most the following 60 seconds. It preserves dwell
and physical segment speed on reversal. The old `scrub 0..1` script argument
now selects 0..60 seconds within that window, rather than normalized distance.
Workshop edits restart the selected route and its preview checkpoint.

`correctOption` was replaced by one accepted-set mask in question content.
Both Guided CheckAnswer and arcade SubmitOption reach `judgeOption` under a
frozen interaction policy. Guided retains selection/check/retry/reveal behavior;
arcade cannot invoke those Guided commands to change its collection policy.
Content supports 1..32 steps and 2..8 options per step; shipped fixtures use
four choices. Duplicate IDs, invalid masks and capacity overflow are rejected.

## Challenge and frame consistency

`Shoot` carries a distinct `SceneFrameId` and `ChallengeId`. A stale challenge,
unpublished/old frame, paused run, spawning ball or popping ball is rejected
before a new question attempt is appended. Replaying a successful click cannot
collect it again. Aim misses are separate from mathematical mistakes.

`prepareChallenge` copies the small active owners for staging and preflights
question progression, scene capacity, the target reset and the full mapping.
`commitChallenge` appends the prepared assignment before no-throw owner moves.
The published challenge context is then invalid until its next complete frame.
Existing slot motion continues across ordinary question resets; new slots
receive prepared routes. Spawn takes 9 ticks and pop takes 18 at 60 Hz.

The gallery has one persistent time accumulator, owned by the scene. The
session divides a long render delta into small submissions only to observe
transitions at the exact accepted scene tick. It starts elapsed time after the
complete set of Active balls is published. UI input and simulation happen
before drawing the answer board, preventing old answer rows from accompanying
a newly committed colour mapping.

## Motion and controls

The twelve moving presets are horizontal, vertical, diagonal rebound, circle,
ellipse, figure eight, sine wave, zigzag, box, stop-and-go, breathing spiral,
and seeded roam. Stationary and custom waypoints are additional options.
Roam prepares a bounded eight-point sequence from a separate derived seed;
frame drawing and preview seeking do not consume the assignment engine.

Workshop exposes Sphere creation, diameter, pattern, extent, pace, moving/loop
controls, route wait, roam seed, and custom point/dwell/segment-speed edits.
The custom route UI retains its four authored points; the route value supports
32. The preview slider is labelled in seconds. Curve pace is nominal speed at
the horizontal extent: noncircular curves can vary their instantaneous speed.
No per-ball allocation or route rebuilding occurs during normal advancement.

Gameplay only exposes pause/resume and quit; developer camera/object mutation
stays in the separate Workshop startup. Losing focus pauses either context.
The game board keeps all four fixture answers visible at 800x600; its lower
instructions can scroll at that size.

## Build and launch

From `/Users/kogaryu/iggy3d/paths`:

```sh
cmake -S . -B build/gallery-port -DCMAKE_BUILD_TYPE=Release
cmake --build build/gallery-port --target paths_gallery paths -j 6
./build/gallery-port/paths_gallery --start-mode equality_sweep
./build/gallery-port/paths_gallery --start-mode question_relay
./build/gallery-port/paths_gallery --start-mode equation_chain
```

Omit `--start-mode`, or use `workshop`, for the route editor. Optional gameplay
arguments are `--seed UINT32`, `--motion ROUTE_ID`, and `--pace NUMBER`.
Motion IDs are `stationary`, `horizontal`, `vertical`, `rebound`, `circle`,
`ellipse`, `figure_eight`, `sine`, `zigzag`, `box`, `stop_go`, `spiral`, `roam`.
The custom `waypoints` route requires Workshop authoring and cannot start an
unconfigured game. The default is gentle horizontal motion with seed 1.

The new pure `paths_gallery_model` library links `paths_model` and `paths_scene`.
Neither child depends on it, SDL, Vulkan or the native host. Configure with
`-DPATHS_BUILD_NATIVE=OFF` to build and exercise that composition without SDKs.

## Focused evidence

- Debug build: `build/target-foundation`. `paths_scene_tests`,
  `paths_guided_tests`, `paths_gallery_tests`, and `paths_input_tests` passed
  (4/4). The affected scene/gallery pair was rerun after final motion and input
  validation corrections and passed (2/2). No broad CTest loop was run.
- Scene evidence includes all route preparations, live/preview equivalence,
  a known arrival/dwell/reversal timeline, sphere silhouette vs AABB, visual
  phase timing, stale frame IDs and maximum sphere capacity. The cached sphere
  uses 114 vertices/672 indices; 64 spheres still fit the existing buffers.
- Question/gallery evidence includes variable choice capacity, stable IDs,
  Guided recovery preservation, wrong-then-correct collection, duplicate-hit
  rejection, old frame/challenge rejection, a two-question relay, complete
  three-step equation order, colour bijection/no-repeat, separate randomness,
  and pause/render-rate clock consistency.
- Native `tests/gallery_foundation.script`, at 1440x900 with seed 19 and
  stationary motion: one wrong hit, two correct hits, one completed question,
  two different recorded mappings, and four reset Active balls. It targets
  actual mesh coordinates; it never submits an answer ID directly. The seeded
  permutation in this script is pinned to the tested local C++ library.
- Inspected offscreen captures: initial Sweep, half-sized collected pop,
  shuffled reset, Equation Chain at 800x600, and the surviving workshop.
  The existing workshop script still produces 5 objects, selected object 5,
  and 30 common ticks under its new time-seek contract.
- Release delivery: `build/gallery-port/paths_gallery` and `paths` rebuilt.
  The release gallery repeated the native hit/reset script successfully;
  `release-sweep-reset.json` includes the variation, motion and pace along
  with the same verified ordered evidence and reset counts.
- Independent model closure: copied Paths alone to
  `/private/tmp/paths-target-foundation-isolated/source`, configured with
  `PATHS_BUILD_NATIVE=OFF`, built `paths_gallery_tests` in Release and passed
  its composition test (1/1). The compile database contains no reference to
  the original iggy3d checkout.

Artifacts are in
`/Users/kogaryu/.codex/visualizations/2026/09/06/01a075b7-6a99-7852-8463-6a43d75cffc6/paths-target-foundation/`:
`sweep.png`, `collected-pop.png`, `sweep-reset.png`, `chain-800.png`,
`workshop.png`, with JSON run reports and native capture metadata beside them.
`release-sweep-reset.png/.json` are the final release executable's capture/report.

Production change for this checkpoint: **4 new C++ files, +1146/-156 lines
(net +990)** across the new pairs and existing scene/question/UI startup.
This is the requested feature addition, not a cleanup. Tests, documentation
and CMake changes are excluded from that production count.

## Limits and next checkpoint

These are reviewed foundation fixtures, not source-card 002 integration or
the full requested game. Point/bonus rules, durable scoreboard/history,
save/load and exact assignment replay, setup/pause submenus, route handles,
saved display profiles and gameplay difficulty controls remain later work.
Assignments and attempts can be exported through `--report`; there is no
report-replay loader yet. Endless practice currently retains evidence in
memory and stages it on challenge changes; long-session storage/performance
needs the later persistence workstream.

No interactive swapchain, resize, pointer-feel or colour-accessibility acceptance
is claimed. The native renderer, shaders, copied vendor files, source cards
and parent engine are unchanged. Changes remain uncommitted.

The next content candidate is the complete 13-step card 002, using these
shared collection/judgment contracts. Its scoring/bonus projection should use
P002's canonical rules rather than adding arithmetic to GallerySession.
