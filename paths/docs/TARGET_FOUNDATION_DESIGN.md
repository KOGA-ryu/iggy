# Target foundation: C++ methods and file structure

Status: implementation design, 2026-09-06. This records the methods and file
boundaries proposed after the user settled the gameplay shape. No production
files are created by this document.

Implementation update: the first target/question checkpoint is implemented in
[P006_TARGET_FOUNDATION.md](P006_TARGET_FOUNDATION.md). Its four new production
files and extended owners have targeted model and offscreen evidence; human
pointer acceptance and the later scoring/persistence work remain outstanding.
The design below records the agreed contracts; P006 records their concrete
signatures, delivered scope and limits.

## Settled interaction

- Equal-sized coloured balls correspond to stationary answer rows. A short
  letter/marker can reinforce each colour without putting equations on balls.
- A correct hit pops its ball. When the next question appears, the popped
  colour resets and the answer-to-colour assignment is shuffled again.
- Equality Sweep collects the accepted set under one prompt. Question Relay
  advances to a fresh question. Equation Chain advances through an authored
  problem. All three use the same physical targets and input route.
- The colour mapping is fixed within a question. Correctness belongs to the
  question model; colours, scene objects, and routes carry no answer key.

## Recommended file map

All paths below are relative to `/Users/kogaryu/iggy3d/paths/`. A pair means
one declaration header and one implementation source, not a folder of classes.

| Files | Status | Responsibility |
| --- | --- | --- |
| `src/scene/TargetMotion.hpp` and `.cpp` | Proposed new pair | Route specifications, preparation, bounded movement state, fixed-tick evaluation, and preview seeking |
| `src/runtime/gallery/GallerySession.hpp` and `.cpp` | Proposed new pair | Gallery commands, question/target bindings, seeded assignment, round progression, and composition of the existing scene and question owners |
| `src/scene/GalleryScene.hpp` and `.cpp` | Extend existing pair | Physical scene objects, Sphere primitive, visual spawn/pop phase, published geometry, and one common picking implementation |
| `src/runtime/first_move/LayeredQuestionSession.hpp` and `.cpp` | Extend existing pair | Shared immutable content catalog, variable step/choice lists, accepted sets, judged attempts, collection, and mathematical progression |
| `app/gallery_main.cpp` | Extend existing file | Native input/CLI translation, fixed answer board, workshop widgets, and presentation of the gallery session view |
| `src/platform/NativeVulkanHost.hpp` and `.cpp` | Reuse existing pair | Native/GPU resource lifetime and drawing the prepared scene/UI |
| `src/runtime/experiments/RunRules.hpp` and `.cpp` | Already proposed by P002; absent in inspected code | Canonical scoring/statistics projection when that capability lands; this design does not add another point authority |

The target foundation therefore proposes **two new production pairs, four
files**. The pre-existing P002 scoring work, later persistence, and tests have
their own scope. There is no `Ball.cpp`, file per patrol, class per colour,
or separate renderer for each game variation in this design.

Headers contain value types and small public contracts. Implementation files
contain validation, algorithms, and private helpers. Keep SDL, ImGui, Vulkan,
and filesystem access out of both proposed modules. Use explicit ownership
and ordinary functions; helpers that do not need private state need not become
member functions. See [C++ Core Guidelines C.4](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-member).

## Data and mutation ownership

Keep the physical ball in `GalleryScene`'s existing object collection. Extend
`GalleryObject` with Sphere support and a small visual lifecycle. Its size,
pose, colour, and movement state remain scene-owned. Do not copy those mutable
values into a second target collection owned by the game session.

The gallery session only needs a binding between that body and its question:

```cpp
// Interface sketch; these names are proposed, not compiled declarations.
struct TargetBinding {
    SceneObjectId object;
    OptionId option;
    DisplayToken token;
};

struct ChallengeBindings {
    ChallengeId challenge;
    std::array<TargetBinding, 8> entries;
    std::uint8_t count;
};
```

Use stable, distinct identity types for a scene object, question instance,
option, and display token. An option's complete evidence identity includes
question/content version and step ID. Never serialize an array address or
interpret an RGB value as an option ID.

Four choices ship first; a proposed bounded capacity of eight supports the
multi-answer formats without a fixed four-choice assumption in every caller.
The session validates the actual count. Reuse the scene's existing reserved
object storage. Fixed arrays plus counts or reserved vectors provide compact
iteration without a per-ball heap allocation on every reset. Read-only views
use `std::span<const T>` and remain valid only until the next mutation.

The shared question session owns accepted and collected option sets. A small
bit mask can represent membership by validated local choice index; exported
records retain stable option IDs. The gallery session consumes the verdict
and requests visual effects, rather than comparing answer text itself.

## Methods and their contracts

These are the operations to implement, with signatures finalized against the
code at the capability checkpoint. Public mutation remains through dispatch.

| Owner / operation | Input -> result | Contract |
| --- | --- | --- |
| `TargetMotion::prepareRoute` | Route specification -> prepared route or diagnostic | Validate once when configured; finite dimensions, speed, dwell and capacity; failure leaves the previous route intact |
| `TargetMotion::advanceMotion` | Prepared route, motion state, next fixed tick -> next state/pose | One common movement entry for all patterns; no question or colour access |
| `TargetMotion::seekMotion` | Prepared route, requested tick, preview checkpoint -> pose/state | Preview uses the same movement semantics; a seek cannot consume the live run's random stream |
| `GalleryScene::hitTestPresentedFrame` | Frame identity and viewport pointer -> closest hit/miss | Read-only geometric query against the published mesh; returns object identity and distance, never correctness |
| `GalleryScene::dispatch` | Scene action -> accepted/rejected result | Sole mutation of geometry, motion configuration and visual phase; adds Sphere and target activation/pop operations |
| `GalleryScene::publishFrame` | Current scene state -> immutable frame snapshot | Publishes frame identity, geometry and the exact camera/viewport used for picking |
| `GallerySession::dispatch` | Typed game command -> accepted/rejected result | Sole gameplay entry for pointer, keyboard and script actions; preflights the current challenge/context |
| `GallerySession::view` | Current state -> read-only UI view | Fixed question/answer rows, score projection, progress and feedback; reading it cannot advance play |
| private `prepareChallenge` / `commitChallenge` | Next content/config -> staged bindings and committed challenge | Stage the mapping, target reset and question together; publish one consistent new context |
| private `makeColourAssignment` | Option IDs, seed/state, assignment rule -> token permutation | Complete bijection with no missing/duplicate option; records the actual mapping |
| private `submitHit` | Validated hit and binding -> question verdict plus presentation effects | Submits once to the question owner; only an accepted result can request a pop |
| shared question `dispatch(SubmitOption)` | Stable option ID and interaction policy -> judged outcome | Supports immediate arcade submission and collection while preserving Guided's select/check/recovery semantics |

Use typed command payloads, for example `Shoot { frameId, challengeId,
pointer }`, carried by a small `std::variant` or equivalent closed command
type. One exhaustive dispatcher handles that finite set. Keep payloads
specific instead of growing a business `if/else` ladder around untyped values.

The existing scene Pick action currently changes editor selection. Refactor
its geometric calculation into `hitTestPresentedFrame` and have both editor
selection and gallery shooting consume that same result. Remove the displaced
private picking calculation; do not leave two competing geometry tests.

## Motion implementation

Keep one module for the twelve patterns. Use a closed `RouteKind` and matching
parameter data; no derived `CircleBall`, `ZigzagBall`, or behaviour tree.
The ball's shape and mathematical role do not select the motion algorithm.

- Horizontal, vertical, box, zigzag and stop-and-go presets can produce
  waypoint data for the already copied patrol kernel.
- Circle, ellipse, sine, figure eight and breathing spiral use compact
  mathematical samplers selected within the same motion module.
- Diagonal rebound uses bounded reflected motion; seeded roam builds bounded
  destinations from its own recorded random stream.
- Stationary is the zero-motion case and does not require a separate actor.

Use the existing 60 Hz scene tick as the active simulation clock. A live
gallery session sends Tick through the scene owner and reads the accepted
tick count; it does not run a second time accumulator. Motion and pop age
use those ticks. Bonus duration is measured from the current challenge's
start tick, so changing a question need not restart every route. Pausing
freezes the common clock.

The copied waypoint kernel already owns dwell, loop/ping-pong and segment
speed behavior. `advanceMotion` delegates those cases to it. Keep private
curve evaluators together in `TargetMotion.cpp`, using the same tick input.

One concrete trap in the existing API: its `sample...Progress` function samples
normalized distance, not elapsed time including dwell. Do not label that
function a replay timeline. `seekMotion` for waypoints should replay the same
fixed-tick kernel from a cached preview checkpoint. Keep a bounded preview
window/checkpoint interval so seeking does not replay an entire endless run.
Analytical curves can seek directly by tick. No separate preview movement law.

Validation and geometry preparation may allocate at setup. Normal advancement
is bounded by active object count and route capacity. No filesystem work,
content parsing, route rebuilding, or new allocation per moving ball per frame.

## Colour assignment

Use `std::mt19937` with a stored run seed and `std::shuffle` over option IDs.
This supplies the standard permutation operation with an explicit random
engine. The standard specifies equal probability for permutations, not a
particular implementation's sequence of swaps. See [C++ shuffle specification](https://eel.is/c++draft/alg.random.shuffle).

Give assignment, deck order, and roaming separate seeded engines. Colour
assignment runs once per new challenge. Pop/animation frames and editor
preview do not draw numbers from that engine.

For the optional "change the correct colour" constraint on a single-answer
question, shuffle first. If the correct option occupies the previous successful
token, swap it with a randomly selected different token. This is a bounded
operation, not repeated shuffling until something happens to pass. Apply the
constraint only where the accepted set and token count support its meaning;
the multi-answer collect rule does not have one unique correct colour.

Store the actual option-to-token permutation alongside the seed and assignment
rule version. Exact historical replay reads that saved mapping. A seed alone
is not a promise that another standard-library implementation or later rule
revision will reproduce the same permutation. A colour token indexes both
the answer-row badge and the scene appearance, so they agree by construction.

## Sphere, pop, and hit implementation

Generate one unit-sphere topology and reuse it for every ball. The current CPU
scene builder can scale/translate its cached vertices and apply per-instance
colour. Keep the existing Vulkan pipeline; GPU instancing is a later measured
optimization, not required to give four balls a common shape.

Use `VisualPhase { Spawning, Active, Popping, Retired }` and a phase-start tick
in scene-owned state. A small private `visualScale(phase, ageTicks)` function
supplies spawn/pop scale. The first pop is a short shrink to zero, with no
particle system or new alpha-blending path required. Omit retired objects
from the draw packet.

The current ray/AABB broad phase and triangle narrow phase can pick a sphere
mesh through the same published-frame route. This keeps hits consistent with
the actual visible silhouette, rather than adding a second sphere-specific
answer picker. The returned phase/object flags let a popping-target hit be ignored
without submitting another answer. Visible popping geometry can still occlude
objects behind it; only Active target bodies are answerable.

Correctness and point events are accepted before visual popping starts.
Completing the visual effect retires its geometry and cannot independently
award points or advance the mathematics. On a new question, reset the colour
slots, commit the new permutation, and activate the newly published challenge.
For collection, removed colours stay collected until that question completes.

## Composition and build boundary

Proposed additions to the current build graph:

```text
paths_scene
  GalleryScene + TargetMotion + existing copied math/camera/waypoint code

paths_model
  shared Hunt/Guided question models + the existing P002 rule work when ready

paths_gallery_model  (new pure composition target)
  GallerySession; links paths_model and paths_scene

paths_gallery
  gallery_main; links paths_gallery_model and the existing native/UI dependencies
```

Neither `paths_scene` nor `paths_model` depends on `paths_gallery_model`.
The native host still consumes a `SceneFrame` and has no question dependency.
This lets the gameplay composition be tested without SDL/Vulkan and avoids
adding scene dependencies to the existing math-only model library.

`GallerySession` owns one scene and the active shared question session. It
is the active-game coordinator, while those children remain the sole owners
of their own semantics. The existing workshop may use the scene directly in
its editor context. A scored run routes its commands through GallerySession;
widgets cannot bypass it and mutate the active question or target mapping.

Preparing a challenge preflights content, choice capacity, scene capacity and
the complete assignment. Commit the staged question/bindings/scene reset only
when all can succeed. A Shoot command carries the frame and challenge IDs it
refers to. Reading a stale generation rejects before either child is mutated.
After a valid submission, consume the question verdict and append its evidence
once, then request the corresponding scene effect. Scoring reads those facts
through its canonical rule owner.

## Implementation checkpoint

First prove the target/question boundary using a small reviewed equality
fixture, a two-question relay, and a short authored equation sequence. This
is foundation evidence, not a replacement for integrating source card 002.

The focused proof should establish one shared pick-to-verdict route, correct
collection, exactly-once rewards, colour reset/shuffle, stale-hit rejection,
and pause-consistent motion/pop timing. Extend the current scene and question
tests; add a small gallery-composition test target for the new live boundary.
Reinspect CMake when implementation starts to choose the actual minimum gate.

Render one representative sphere/answer-board frame offscreen and inspect it.
Human pointer feel remains a separate acceptance step. No source changes,
build, gameplay tests, window, or acceptance are claimed by this design.
