# Paths: arcade math gallery design

Status: expanded working design, 2026-09-06. The user confirmed **arcade
first: fast continuous play, with detailed explanations mainly in review**.
The original request establishes mouse-cursor targeting, moving mathematical
choices, a right-hand question panel, endless play, twelve patrol patterns,
developer placement and display controls, scene customization, results, and
separate startup shortcuts for the variations.

The subsequent target discussion establishes stationary answer rows linked
to equal-sized coloured balls. A correctly popped colour resets when the next
question appears, and a seeded assignment shuffles answers among colours.
The proposed C++ methods and file boundaries are recorded in
[TARGET_FOUNDATION_DESIGN.md](TARGET_FOUNDATION_DESIGN.md).

Implementation update: [P006](P006_TARGET_FOUNDATION.md) now proves this shared
target/question boundary with three fixture startup modes, coloured sphere
targets, twelve motion patterns, collection and shuffled question transitions.
The complete source-card game, point/bonus rules and saved workshop profiles
remain later checkpoints. P006 has automated/offscreen evidence; it has not
received human pointer acceptance.

Those are product requirements. Numerical defaults and the delivery order
below are recommendations for review and playtesting. Gallery behavior is
not implemented or accepted by this document. The active P001/P002 packets
retain their recorded status; this design does not silently supersede them.

## Product direction

Build standalone startup variations of a mouse-driven learning gallery in
`/Users/kogaryu/iggy3d/paths`. Each variation should start directly in its own
setup screen for now. Share the Paths runtime and content identities so a
later common launcher can open the same variations without merging separate
copies of the game logic. A separate shortcut per variation can select its
startup preset; names and new launch arguments remain proposed.

The player has a visible mouse cursor and a fixed view into a shallow 3D
gallery. Coloured balls float through authored routes; a stationary answer
board pairs each colour with its answer. The current question, required action,
working so far, and step progress stay in a dedicated right-hand panel.
A correct click earns points immediately and pops the corresponding ball.
When a new question opens, its colour slots reset with a fresh answer mapping.
A wrong click gives brief feedback while play continues. Resolving the step
quickly opens the next one. Detailed explanations live in review or explicit help.

The countdown is a bonus opportunity. Reaching zero never ends an endless
run or forces a mathematical answer. Completing a problem briefly shows its
result, then opens the next problem. There are no lives or mandatory recovery
dialogs in the default arcade preset.

Math content comes from the read-only source collection at
`/Users/kogaryu/devil/99-red-booleans/problems/math/`. A source question can
produce several short learning decisions: notation, symbol roles, setup,
method, calculation, justification, and verification. Equations and answer
text stay on the stationary board; balls carry only their colour and an
optional short marker.

The complete initial gallery experience includes the route workshop, all
twelve patrols, display profiles, customization, and durable local results.
The smaller first playable checkpoint proves the core interaction; it does
not constitute delivery of that whole request. The other named game variations
then expand the reviewed content and interaction rules one at a time.

## Recommended first-run settings

Open the Equation Gallery shortcut into its own setup screen. Show Start,
content selection, Math difficulty, Aiming difficulty, and Time pressure.
Keep the deeper controls in the pause menu. After the first run, restore this
variation's last valid setup; each variation has its own saved selection.

| Setting | Proposed arcade default | Reason / adjustable alternative |
| --- | --- | --- |
| Session | Endless, manual Finish | Continue through a seeded deck; repeated cards are identified |
| Starting content | The complete 13-step card 002 | A real equation sequence with notation, calculation, and reasoning |
| Visible choices | Four, all present together | Fits the existing reviewed card contract |
| Camera and input | Fixed camera, visible OS mouse cursor, click to answer | Route motion provides the aiming challenge |
| Aiming | Gentle moving routes initially; horizontal and circle available | Increase motion independently of the mathematics |
| Reading | Immediate start once the complete question and all choices are presented | Optional Ready mode provides a separate practice preset |
| Bonus | 20 seconds for calculation/setup/result steps; 30 for prose-heavy steps | Initial tuning values, authored or overridden explicitly per step |
| Wrong hit | Brief outline/sound, no modal or movement stop | The same mathematical step remains active |
| Success | Points immediately; about 0.30 seconds of transition feedback | Manual Continue is an optional practice setting |
| Problem boundary | About 1 second showing the result, then next problem | Pause or Finish remains available during the transition |
| Multi-answer arcade | Collect each correct target once | Exact-set confirmation is a separate practice rule |
| Score | Per-target rewards plus a declining bonus; no multiplier initially | Exact rules and examples below |

These are proposed feel settings, not measured difficulty levels. A first
playtest should tune target speed, text size, bonus duration, and transition
delay together. A short stationary preview in Setup shows the controls using
neutral labels; it does not expose the next scored answer in advance.

Changing the mathematical pack must not reset the user's preferred mouse,
text, or colour settings. Starting a run freezes a resolved copy of its
gameplay settings; later edits change the next run.

## Current foundation, checked locally

- The source folder contains 95 Markdown math cards. Their presence does not
  mean all 95 have game adaptations or verified game answer keys.
- `content/authoring/002_guided.json` contains 13 authored steps for a quadratic
  through three points. `013_guided.json` contains 14 for a closest-point
  derivation. Both currently specify four options and one correct option per
  step. Multiple-answer content needs an explicit extension and review.
- P006 extends `LayeredQuestionSession.hpp` with immutable content, variable
  steps, two through eight options, and accepted/collected sets. Guided still
  opens its seven-step starter; gallery modes use four reviewed fixture
  questions. The authored JSON cards are not loaded. Integrating card 002
  remains real work and cannot be counted as already playable content.
- Paths owns a separate CMake project, pure Hunt/Guided models, a semantic UI
  dispatcher, and an SDL3/Vulkan native host. Its current CLI offers startup
  choices `title`, `guided`, and `hunt`.
- The current code also has a `paths_gallery` startup, `paths_scene` library,
  and `GalleryScene` with Box/Ramp/Frame/Sphere primitives, camera controls,
  movement, and picking against the published triangle mesh. The engine
  foundation comes from [P005_ENGINE_PORT.md](P005_ENGINE_PORT.md); P006 adds
  TargetMotion and GallerySession without changing the native renderer.
- `docs/WORKSTREAMS.md` records P005's port in verification, independently of
  P001 through P004. This plan's code inspection is not a build result or
  interactive acceptance of that port.

## Reuse the existing iggy3d engine

The user explicitly authorized harvesting the existing engine for Paths in
the follow-up to this proposal. Use that working implementation as the source
for the gallery foundation. Keep Paths' independent build boundary and record
each actual copied file or extracted function in
`docs/MIGRATION_SEED_MANIFEST.json`, including its source revision, source byte
hash, destination, and any adaptation. P005 now contains the bounded port;
this documentation checkpoint performs no further extraction.

The following live routes were inspected in the parent checkout:

| Gallery need | Existing source, relative to iggy3d | Reuse boundary |
| --- | --- | --- |
| Coloured 3D meshes and depth | `src/render/vulkan/FirstRoomPipeline.*`, `PipelineLayout.*`, `ShaderModule.*`; `shaders/vulkan/src/first_room.vert.glsl` and `first_room.frag.glsl` | The existing vertex-colour pipeline and shaders supply the simple gallery draw path. `VulkanBackend.cpp` is their current live consumer. |
| Developer fly camera | `src/app/iggy3d/creative/camera/Fly.*` | `EditorFrame.cpp` routes semantic movement into `applyProductCreativeFlyInput`; the fly kernel depends on vector math, not a Creative document. Reuse its speed, sprint, normalized movement, and frame-bounds behavior. |
| Orbit, pan, zoom | `src/app/iggy3d/creative/camera/ViewportNavigation.*` | Reuse the pure viewport navigation kernel for setup controls when its UI is implemented. |
| Camera matrices | `src/app/iggy3d/creative/render/CreativeSceneFrame.cpp`; `src/core/math/Vec3.*` and `Mat4.*` | Carry the camera basis and Vulkan projection conventions into the gallery frame. Reuse the mathematical pieces without requiring the complete scene/document projection. |
| Boxes, ramps, and gallery frames | `src/render/vulkan/RoomMeshCpuPrimitives.cpp`; `src/content/assets/GeneratedGeometry.hpp` | `RoomMeshCpuGeometry.cpp` calls the existing bounded primitive builders. Extract the box/frame functions and necessary types for the initial gallery; add ramp/stair functions with a live gallery consumer. |
| Floor and boundary grids | `src/render/vulkan/RoomMeshCpuGeometry.cpp` | Existing grid lines are constructed through the same box builder. Reuse this construction for the gallery ground and surrounding frames. |
| Cursor picking | `src/core/math/Ray3.hpp`, `Aabb3.*` | Reuse the ray-box intersection kernel for axis-aligned gallery blocks. Map pointer coordinates through the actual gallery viewport and camera. More complex target shapes must use matching hit geometry. |
| Waypoint patrols | `src/app/iggy3d/creative/play/RuntimeMovingPlatforms.*` | Existing definition building and fixed-tick planning support loop/ping-pong routes, dwell times, and outgoing speed multipliers. Reuse the bounded local port for targets. Its progress sampler measures distance; time-based preview must retain dwell semantics. |

The source map above explains the port. Its concrete integration boundaries
are documented in P005:

- The parent primitive pipeline uses Vulkan dynamic rendering. P005 adapts
  the primitive pipeline to Paths' Vulkan 1.1 render pass, with a depth
  attachment and geometry recorded before ImGui. Retain that single native
  host for spheres; no second renderer/device is required.
- `RoomMeshCpuPrimitives.cpp` currently reaches scene/resource types through
  its internal header, and `RuntimeMovingPlatforms.cpp` also contains world,
  collision, and rider updates. P005 copies bounded primitive and route
  functionality with its small contracts. New target work reuses that local
  port rather than pulling additional world/collision/rider dependencies.

Developer setup can enable the reused fly camera and viewport navigation.
Entering a scored gallery run restores its configured camera and visible
cursor. Target clicks use the same projected frame that was displayed.
Once the Sphere primitive is added, its rendered mesh can use the existing
broad-phase bounds test and triangle narrow phase. Stationary answer text
remains UI and does not enlarge the ball's geometric hit area.

The first gallery implementation should prove a parent-derived block room,
correct depth ordering, developer camera movement, and a known target pick
before completing the question loop. Verify camera matrix packing, the scene
viewport beside the question panel, and pause/input-context transitions.
Derive the exact focused build/test targets from Paths' updated CMake graph.
An offscreen capture proves rendering separately from human pointer feel.

## Variations with their own startup presets

| Variation | What the player does | Initial content fit |
| --- | --- | --- |
| Equation Gallery | Click the result or working line that completes the current step; proceed through the entire problem | 002, quadratic through three points |
| Equality Sweep | Collect every expression that satisfies one key, such as equality to 24 | Small reviewed equality fixture, then source-card-derived criteria |
| Question Relay | Hit the colour of the right answer, reset that colour, and continue to the next short question | Reviewed short-question playlist |
| Notation Range | Choose what a symbol means, identify its role, or select an expression with the requested meaning | Role/vocabulary steps in 002 and 013; later reviewed syntax packs |
| Condition Sweep | Collect every correct statement in arcade play; optionally confirm an entire set in practice | Candidate adaptations of 010, maximum rank, and 046, basis criteria, requiring review |
| Error Hunt | Find the first invalid line in an authored worked example, then choose its repair | A new reviewed variant of 004, row reduction |
| Transform Range | Match a rotation, reflection, or projection to a matrix, vector, or diagram | Candidate adaptations of 006 and 013, requiring diagram/key review |
| Reason Relay | Choose a valid next line and then the reason that licenses it | 013 and other derivation/proof cards |
| Practice Again | Revisit selected concepts and steps from a previous run, mixed with other reviewed questions | Actual gallery attempt history |

Equality Sweep, Question Relay, and the Equation Chain behavior of Equation
Gallery establish the three progression cases for the shared foundation.
Small fixtures prove them before the complete card-002 playable checkpoint.
Other variations reuse targets, patrols, controls, and evidence.
Syntax/grammar is initially interpreted as mathematical notation and meaning.
Programming syntax or language grammar can later use separately authored packs
with explicit language/version or grammar conventions.

Each startup shortcut selects a variation descriptor and that variation's
saved preset. Use one executable and one descriptor registry, with stable
variation IDs such as `equation_gallery` and `condition_sweep`. Launch-argument
spelling is an implementation decision; existing `--start-mode` values must
not be represented as already supporting these new modes. An invalid or
unavailable preset opens Setup with the problem explained and no run started.
The future combined launcher calls the same entry action. It does not import
or combine separately forked game implementations.

| Variation | Additional rule that must be authored and proved |
| --- | --- |
| Notation Range | State the requested symbol role or representation; equivalent values are not automatically equivalent notation |
| Condition Sweep | State the mathematical universe and all conditions; the accepted set is reviewed explicitly |
| Error Hunt | Identify the earliest invalid line under the stated assumptions, then open its authored repair step; a later consequence of that error is not the first error |
| Transform Range | State axes, vector convention, transformation order, and diagram scale where relevant; diagrams and keys describe the same transformation |
| Reason Relay | A line and its reason are separate linked decisions; a correct reason cannot silently repair an incorrect preceding line |
| Practice Again | Supply the prerequisites needed for the selected step and retain its previous exposure; correctness in a replay is a new practice event |

Do not label all alternate methods wrong just because they differ from an
author's preferred solution. Ask a sufficiently specific question, accept
all valid displayed choices, or use a mode whose validator supports alternative
paths. Selecting a guided proof step is recorded as guided selection practice.

## Gallery layout and interaction

```text
+-----------------------------------------------+------------------------+
| Score   Bonus countdown   Streak   Pause       | Question               |
|                                               | Full givens            |
|       (A)                       (C)            | Current step / total   |
|                    (B)                        | Current instruction    |
|                                               | A / blue: answer text  |
|           (D)                                 | B / gold: answer text  |
|                                               | C, D: remaining answers|
|      ground grid / walls / target routes       | Feedback / help        |
+-----------------------------------------------+------------------------+
```

- Keep the right panel outside target spawn and patrol bounds. Reserve its
  width according to text size and the longest authored prompt.
- Start with a shallow 3D room, fixed camera, ground grid, side/back frames,
  and equal-sized coloured balls. Support rectangular, polar, and hex grid
  appearances as visual presets; these do not change answer correctness.
- Keep the full givens pinned above the current instruction. Show the latest
  accepted working lines below it, with earlier work available on expansion.
  Opening a larger working/review view pauses play. A notation aid must not
  reveal a requested symbol role without recording help exposure.
- Show the cursor. Mouse clicks select the visible target under the cursor.
  A click over the panel or a menu never passes through into the gallery.
  One button-down edge produces at most one selection; holding does not fire.
- A target is a ball identified by colour and optionally a short A-D marker.
  Its visible sphere silhouette is clickable. The hit region must match the
  drawn geometry; an oversized hit region is an explicit assistance setting.
  Hovering over a ball does not slow or freeze it. There is no projectile
  flight, weapon recoil, or camera aiming in the initial mouse-cursor game.
- Resolve picking against the same camera and target positions used by the
  displayed frame. Use visible surface/depth ordering if targets overlap.
  Default routes keep required balls visible and individually selectable.
- Measure and wrap full answers on the stationary board. Ball size is
  independent of answer length. Place the board below the question or in a
  fixed lower dock when the longer choices need more width. Validate the
  chosen layout before starting; every answer and colour binding must fit.
  Preserve all authored options and the distinctions that determine the key.
- Use a display token to obtain both the answer-row badge and ball colour.
  Keep assignments fixed within the question. On the next question, reset
  the popped colours and commit the new semi-random assignment together with
  the new prompt. Feedback changes an outline/marker, not the identifying hue.
- Use identical visual treatment for unjudged correct and incorrect options.
  Position, size, motion, and colour must not systematically reveal the key.
- After success, append the accepted working line, show the earned points,
  and transition automatically. Save the full explanation for review.
  Wrong hits receive a short neutral response, not a pre-answer explanation
  of the distractor. Consume queued old-step clicks before the next step.

The display target is 1440 x 900 at normal text size, with roughly one third
of the width available to the question panel. Also validate 1024 x 768 at
150% text size. These are proposed layout checks, not claims that the new
gallery already fits. Reserve HUD, panel, and margins before validating
motion. Test the longest actual choice; both authored cards currently contain
an 86-character option.

Use separate motion lanes that contain each projected ball throughout its
route, including its identification marker and turning extents. Validate the
stationary answer board separately from ball movement. If the full answer
text does not fit, offer the wider board layout or a larger window. A resize
or text change pauses and revalidates the layout before resuming.

## Card 002: one complete arcade round

Pinned problem: `f(x) = ax^2 + bx + c` passes through `(-1,1)`, `(0,0)`,
and `(1,2)`. Build the coefficient equations and find `f`.
Retain the adaptation label and source locator: Meckes & Meckes, 1.1.7.

| Step | Decision | Correct outcome / working retained |
| --- | --- | --- |
| 1 | Which symbols are unknown coefficients? | `a, b, c`; retain as a role note |
| 2 | What is the role of `x`? | Supplied function input, with values `-1, 0, 1` |
| 3 | Why is the system linear in the coefficients? | Each coefficient appears to the first power with known multipliers |
| 4 | What unwritten number multiplies the term `c`? | `1*c` |
| 5 | Use `(0,0)` | `c = 0` |
| 6 | Substitute `(-1,1)` | `a - b + c = 1` |
| 7 | Substitute `(1,2)` | `a + b + c = 2` |
| 8 | Why may the two reduced equations be added? | Equality is preserved; `-b+b=0`, giving `2a=3` |
| 9 | Solve `2a=3` | `a=3/2` |
| 10 | Substitute into `a+b=2` | `b=1/2` |
| 11 | Assemble the function | `f(x)=(3/2)x^2+(1/2)x` |
| 12 | What do the three substitutions verify? | Passage through the supplied points |
| 13 | What does determinant `-2` establish? | Uniqueness of the coefficient vector within the specified model |

For step 9, the existing four authored answers are `a=3`, `a=1/2`,
`a=3/2`, and `a=2`, each linked to a coloured ball. The panel retains `2a=3`.
Hitting the colour assigned to `a=2` records that option and leaves the balls
moving. Hitting the colour assigned to `a=3/2` next awards the recovery reward,
pops that ball, appends the line, and opens the step for `b`. The colour resets
with the next question's assignment. The wrong choice remains in the record.

Role and reason steps produce notes; calculation steps produce working.
Do not append explanatory prose as if it were an equation. Authored accepted
working must be explicit for every step; the renderer must not derive algebra
from answer text. No step may display its own answer in the working panel
before it is earned or intentionally revealed.

The full 13-step card is the first content slice. A later calculation-only
playlist needs an authored starting context and its own playlist identity;
unchecking conceptual steps cannot leave dependent steps without givens.

## Answer policies

Every step declares an accepted option set. Separately, its permitted
interaction declares whether to choose one, collect all, or confirm a set.
Correctness uses stable option IDs independently of display position. A preset
cannot invent extra accepted answers or offer collection for an unsuitable
question. The question session validates this compatibility.

| Policy | Completion rule | Player instruction |
| --- | --- | --- |
| One answer | The one accepted option resolves the step | Hit the correct answer |
| Any valid answer | Any one member of the reviewed accepted set resolves it | Hit one valid answer |
| Collect all, arcade | Every required option must be hit once | Hit all N correct answers |
| Exact set, practice | A submitted selection must equal the whole accepted set | Select all that apply, then confirm |

For One and Any, a click is immediately judged. A wrong target does not
advance, freeze the route, open a recovery dialog, remove a distractor, or
change the option-to-route assignment. The target returns to its ordinary
appearance after brief feedback. Each subsequent fresh click can be judged;
button repeats and queued clicks from a previous context cannot.

For Collect all, a correct hit locks that option, pops its ball, and marks
the stationary answer row collected. It cannot score again during that
question. Remaining balls continue moving. A wrong hit leaves collected
progress intact and reduces subsequent rewards as specified below. The final
required hit resolves the step.
Display `1 / N collected` and the required count by default. A hidden-count
variant has a distinct assistance/profile identity. A queued hit on an already
collected target is an ignored duplicate. Once the whole question completes,
its colour slots reset for the next question.
The first collection fixtures should use four choices with two or three
correct options, leaving at least one distractor. The author specifies that
set explicitly; the engine never randomly decides which statements are true.

Collection deliberately gives incremental correctness feedback. Report it as
guided collection accuracy/clean completion, not evidence that the learner
identified the entire set before receiving help. For example, with accepted
options `{a,b}`, clicks `a, c, b` record correct collection, a wrong answer,
then successful recovery; they are not one correct exact-set submission.

For Exact set, clicks toggle selection without disclosing correctness.
A fixed Confirm button or Space submits the selected IDs. Require a nonempty
selection in the initial content contract; a valid empty answer set needs an
explicitly authored "none apply" option. A proper subset is incomplete, and
any extra incorrect option makes the set incorrect. Both are judged attempts
and prevent advancement. Retrying clears the submitted selection without
deleting it from history. The full key appears only after success or reveal.

Any-valid options must lead to the same supported next-step context. Equivalent
forms such as `3/2` and `1.5` may converge on a canonical exact working line,
with that conversion explained in review. A different solution method that
requires different subsequent work needs an authored branch or a more precise
prompt; the first implementation does not infer proof paths.

Numeric equality and syntax are different tasks. If a question asks for a
value, two equivalent displayed expressions cannot silently be treated as
one right and one wrong answer. If it asks for a particular representation,
state that requirement. New answer sets and distractors need mathematical
review before entering an endless deck. Runtime answer checking uses reviewed
keys; it does not need a symbolic algebra engine or generated answer judge.

### Help, skipping, and unfinished problems

Help is explicit and pauses play. The first slice offers Reveal answer; richer
hints require authored content and exposure records. Reveal marks the step
assisted, supplies its accepted working, and gives zero further rewards for
that step. Already earned collection points remain historical facts.
Continue after an explicit reveal is manual so the user can read the help.

Skip problem is available from pause. It ends the open step/problem as
skipped and opens the next problem instance without inventing attempts for
untouched steps. With a one-card deck, that instance is a labelled repeat of
the same card. Avoid Skip step in the first slice: later calculations require
the skipped result. If introduced, it must supply that prerequisite and label
it as given assistance. Finish keeps the current problem partial; Restart
ends that run and begins a new one without erasing the old attempts.

## Twelve starting patrol patterns

| Pattern | Movement | Main controls |
| --- | --- | --- |
| Horizontal sweep | Left-right ping-pong | Endpoints, speed, turn dwell |
| Vertical lift | Up-down ping-pong | Height range, speed, easing |
| Diagonal rebound | Travel diagonally within a bounded rectangle | Bounds, initial heading, speed |
| Sine wave | Sweep sideways while smoothly rising and falling | Width, amplitude, period, travel speed |
| Circle | Orbit a centre | Centre, radius, direction, angular speed |
| Ellipse | Orbit with different horizontal and vertical radii | Centre, two radii, orientation, speed |
| Figure eight | Repeat a smooth crossing loop | Centre, width, height, period |
| Box patrol | Follow four corners | Corner positions, corner easing, speed |
| Zigzag | Alternate between offset waypoints | Points, turn radius, speed |
| Stop-and-go | Visit points and pause at each | Points, per-point dwell, travel speed |
| Breathing spiral | Orbit while radius smoothly grows and shrinks | Centre, min/max radius, radial period, turn speed |
| Seeded roam | Smoothly visit repeatable generated destinations | Bounds, seed, turn limit, speed |

Also provide a stationary preset for reading, calibration, and regression
checks. Start waypoint patterns from the existing moving-platform route
kernel described above. Add the analytical curves and seeded roaming as
target motion presets; the existing engine does not yet establish all twelve
gallery patterns. No navigation mesh or enemy decision AI is required.
Patterns use active simulation time/fixed ticks and a stored seed where needed.
The same preset can be replayed without depending on render frame rate.

Expose a single speed multiplier for ordinary play. The editor can also show
the underlying linear speed or lap period. A lap period is not an independent
second speed control: editing one updates the derived other value. State
whether a curve uses constant angular speed or approximately constant travel
speed; the same slider must not quietly change meaning between patterns.

For the first release, movement stays within a shallow plane facing the play
camera. Routes may share a pattern while using independent lanes and phase
offsets. Depth travel can be added as an explicit aiming profile after its
projected text size and picking are proven. The room supplies the 3D setting
without requiring required answers to travel behind one another.

Closed curves repeat; open paths explicitly stop or reverse at their endpoint.
Stop-and-go dwell is part of the route period. Seeded roam remains inside its
assigned safe region and has a stored generation version. Editor scrubbing
and replay must not consume new random numbers or alter later destinations.

Route editing should support place/drag handles, numeric coordinates, grid
snap, duplicate, delete, undo/redo, loop/ping-pong/open paths, preview, and
time scrubbing. Give each route an origin, extent, permitted depth, speed,
phase offset, and target assignments. Validate previewed screen bounds and
legibility for the selected camera, resolution, and text scale. Reject an
unplayable layout with a useful reason rather than hiding required answers.

A few sampled preview frames cannot prove that two moving cards never overlap.
Use conservative per-route screen-space envelopes and disjoint lanes as the
initial admission rule, including any curve overshoot. Sampled preview is
additional visual evidence. Free crossing routes remain Preview-only until
there is an explicit, proven policy for continuous visibility and picking.

## Developer controls and display profiles

Expose an editor through the pause menu with a clearly separate Preview action.
Preview has no scored attempts. Save named local presets that include content,
routes, targets, display order, difficulty, colours, and a repeatable seed.

| Control group | Settings |
| --- | --- |
| Targets | Sphere size, colour/marker tokens, hit area, count, route assignment, stationary answer-board text size |
| Routes | Twelve patterns, handles, speed, dwell, phase, depth, snap, preview |
| Display order | Which spawn slot/route receives each choice; simultaneous or staged appearances |
| Content | Card/pack, enabled step kinds, answer policy, explanation length, reviewed difficulty tier |
| Difficulty | Independent question, motion, and time settings; optional staged progression |
| Colours and scene | Target fill/outline/text, selected/feedback states, grid, walls, ground, background, sky preset |
| Input | Mouse bindings, cursor size/style, keyboard alternatives; later gamepad virtual cursor, sensitivity, deadzone |
| Results | Current run, local scoreboard, concept/step history, replay/export |

The workshop interaction is concrete:

1. Open Targets and routes from Pause, then Edit draft. Preserve the paused
   run and its configuration while editing a separate draft.
2. Choose a pattern and place its origin in the play area. Show its safe
   region, full motion envelope, and ball-size outline.
3. Drag handles or enter coordinates; optionally snap to a chosen grid spacing.
   Duplicate a route to another lane and assign a target slot to it.
4. Set speed, dwell, direction, phase, and ball style. Assignment is to
   display slots; option-to-colour mapping belongs to the current question.
5. Select a display-order profile and seed. Preview uses neutral test labels
   or a clearly identified content preview outside a scored run.
6. Play, pause, and scrub the preview timeline. Undo/redo edit operations;
   undo does not mean undoing scored game events.
7. Save a named preset after validation. Resume the existing run unchanged,
   or use Start new run to apply the draft. Cancel discards the draft edits.

Developer camera controls operate only in this editor/preview context. A
scored run restores the preset's fixed camera and mouse-cursor behavior.
Previewing a real question records that exposure for a later run; neutral
preview labels avoid that issue for ordinary route editing.

Use separate sequence settings for question selection and target display.
Shuffling target slots must never reorder the mathematical steps of a card.

Initial target display profiles: fixed slot assignment, seeded shuffled slots,
rotating slot assignments between steps, alternating patrol groups, and exact
replay. A later staggered-wave profile controls spawn delays and dwell windows.
Within a default timed step, publish every stationary answer and matching
active ball before starting the bonus clock.

Keep separate random streams or derived seeds for deck order, answer-to-slot
assignment, and roaming routes. Changing the sky colour or previewing a route
must not change which question or answer position comes next. Fixed assignment
is useful for setup but allows position memorization; seeded shuffled slots
are the default scored profile. The bounded shuffle and optional rule that
avoids repeating the last correct colour are specified in
[TARGET_FOUNDATION_DESIGN.md](TARGET_FOUNDATION_DESIGN.md#colour-assignment).

Keep mathematical difficulty separate from target speed/size and countdown
pressure. A hard question can have stationary targets; an easy question can
have fast patrols. For a mouse using absolute screen coordinates, preserve
normal OS cursor motion; camera or virtual-cursor sensitivity settings should
only affect the corresponding input mode.

| Difficulty axis | What it changes | What it must preserve |
| --- | --- | --- |
| Mathematics | Reviewed pack, prerequisites, step kinds, distractor closeness | Valid questions, complete context, correct accepted sets |
| Aiming | Motion speed/extent, path complexity, phase, sphere radius | Identifiable balls, every choice reachable, matching hit geometry |
| Time pressure | Bonus off/on, duration, immediate or Ready start | A question remains playable after expiry |
| Assistance | Reveals, authored hints, known answer count, keyboard answers, manual advancement | Exposure and configuration recorded honestly |

Suggested aiming presets are Stationary, Gentle, Standard, and Fast. Choose
concrete world/pixel speeds only with a fixed camera and measured target size;
a route's name alone does not establish difficulty. Start with a uniform
speed scale and four targets. Faster play does not require adding extra
distractors or shrinking the mathematical text.

No automatic escalation is enabled initially. A later adaptive preset may
change one declared axis between problems, record each change, and show why
it changed. It must not infer that an empty-space miss means weak mathematics.

Scene controls include target fill, outline, text, selected and judged states,
ground, walls, grid spacing/colour/opacity, background, and sky preset. Start
with Flat, Dark gallery, and Daylight colour environments; textured skyboxes
are a later asset option. Rectangular, polar, and hex grids are independent
visual appearances. Changing a visual grid does not silently change route
coordinates or snap spacing. Correctness uses symbols/text as well as colour.

Gameplay edits take effect on a new run in the initial implementation.
Freeze the gameplay preset at run start. A later live-override feature must
mark the run modified and retain its configuration-change history.
Text/colour preferences can change while paused; revalidate the layout and
record any resulting hit-area change that affects comparability. Colour-only
changes need not split score groups. One registry owns preset validation
across all entry points.

## Endless progression, time, and points

Endless initially means cycling a seeded shuffled deck of reviewed adaptations,
with an immediate-repeat guard when the deck has alternatives. Preserve order
inside each problem. Record prior exposures rather than treating repeat cards
as previously unseen work. Procedural numeric variants are a later capability
requiring validated answer generation and distinct instance identities.

The default lifecycle is Setup -> Preparing -> Active -> Success feedback ->
Next step. Reading and explicit help are optional branches. Pause is an
overlay over the current state; it does not restart a question or its timer.

| State / event | Motion and clocks | Input and next state |
| --- | --- | --- |
| Preparing | No question bonus or answer-active time; scene may continue through a transition | Measure text, validate slots, resolve content and present all choices; start only from a valid presented frame |
| Reading, optional | Stationary choices; reading time only | Ready starts Active; no target can score during reading |
| Active | Motion, bonus, and active duration advance together | Judge fresh target clicks; wrong hits stay Active; accepted completion enters Success feedback |
| Bonus expires | Motion and active duration continue | Emit one expiry event; bonus is zero; the answer remains playable |
| Success feedback | Correct ball pops; route motion and visual feedback use the scene clock; resolved question earns no further time bonus | Show points and accepted working; advance after the configured delay |
| Problem complete | Show the final result and problem count briefly | Open the next card; optional manual progression is a different preset |
| Pause / focus loss | Freeze movement, bonus, feedback transition, and active time | Block gallery input; Resume restores the exact previous state |
| Explicit help | Pause the step and record aid exposure | Return without reveal, or Reveal then manual Continue |
| Finish / Quit / Restart | Stop the old run once | Retain complete/partial status and its evidence; Restart creates a new run |

Store active, reading, feedback, help, and paused durations separately using
exclusive categories, plus wall duration. None alone establishes thought time.
Nested help inside Pause must not double-count elapsed time. An ordinary pause
menu obscures current answer choices and requires an explicit Resume after
focus returns. Pause remains a convenience, not an anti-cheat guarantee.

The same active clock drives motion and bonus decay. A step starts only when
its prompt and all choices are present, with an input context referring to
that content and layout. A layout/loading failure leaves it in Preparing.
On a renderer interruption, pause instead of silently spending bonus time or
fast-forwarding invisible targets. Opening, closing, or redrawing UI must not
create another start or expiry event.

Sample deterministic motion at a fixed simulation rate with presentation
interpolation as needed. A click is resolved against the most recent presented
frame snapshot, including camera, viewport, target bounds, and interpolation
phase, rather than a newer simulation pose. Store the frame timestamp and the
input timestamp separately; use the input event's active timestamp for both
judgment and bonus evaluation. If the frame belongs to a stale step/context,
reject the click. Each prepared step receives a new context generation.

### Score rule: `gallery_arcade_v1` (proposed)

For One, Any, and Collect all, award each newly accepted required target once:

```text
base = 100, if no wrong mathematical hit and no aid preceded this hit
       25, if a wrong mathematical hit or authored hint preceded this hit

bonus = floor(50 * clamp((T - t) / T, 0, 1)), if eligible
        0, otherwise

hit points = base + bonus
```

`T` is the step's frozen positive bonus duration; `t` is cumulative active
time since that step began. With bonus disabled, award zero bonus without
dividing by a duration. Eligibility lasts until the first wrong mathematical
hit, empty-space miss, hint, or reveal in that step. It never resets on retry.
Prior earned points remain unchanged when a later mistake occurs.

Reveal stops further target rewards for that step. Skip/Finish awards no
completion reward. Earlier legitimate collection points remain recorded even
when a set or problem is unfinished. There is no separate completion payout,
negative score, life loss, or streak multiplier in this initial rule.

For Exact set, tentative toggles earn nothing. An exact successful submission
earns one `base + bonus` award, using the same first-error/help conditions.
An incomplete or wrong submitted set disables its bonus and reduces a later
successful submission to 25. This is a separate comparison group from Collect
all, where several target rewards can be earned.

| Example, with a 20-second bonus | Recorded outcome | Points |
| --- | --- | --- |
| Correct single hit at 4 seconds | First correct response, 16 seconds remain | `100 + 40 = 140` |
| Empty-space miss, then correct at 4 seconds | First mathematical response is still correct; aiming miss removed bonus | `100` |
| Wrong target, then correct at 4 seconds | Incorrect first response, recovered step | `25` |
| Correct first hit after 20 seconds | Correct response after bonus expiry | `100` |
| Two required targets at 4 and 10 seconds, no errors | Two distinct collected answers | `140 + 125 = 265` |
| Correct at 4 seconds, wrong target, second correct at 10 seconds | Partial clean collection, then recovery | `140 + 25 = 165` |
| Click an already collected target again | Ignored duplicate | `0` additional |
| Reveal before selecting an answer | Assisted resolution | `0` |

The HUD shows total points, current bonus opportunity, clean-hit combo, and
step/problem progress. "Bonus +40" describes the next eligible correct hit,
not a pot already awarded. Each fresh accepted correct target increments the
clean-hit combo; wrong targets and empty-space misses reset it. Ignored clicks
do not change it. Reveal and Skip also reset it; Pause and ordinary successful
step transitions preserve it. A new run starts at zero. Keep any mathematical
clean-step streak and geometric hit streak as separately named statistics,
not synonyms for the combo.

Collection steps can award more than single-answer steps. Compare scores only
within compatible content and policies. Repeated cards can earn points in a
new problem instance during endless play; repeats are visible in the record.
The score encourages speed and recovery but is not a mastery estimate.

Ready mode exposes content before the clock starts, so its bonus describes
post-Ready execution time. It must not be presented as mathematical response
speed or compared directly with immediate-start runs.

Score evaluation consumes recorded facts under a named, versioned rule.
The UI only displays its result. This is a new gallery rule, not an implicit
change to Hunt banking or the planned Guided completion rule. If P002's
canonical evaluator has landed by implementation, add this rule there rather
than making a second score authority.

## Results, mistakes, and local persistence

Record both kinds of "where": the question/step/concept and the position in
the gallery. A useful attempt contains run ID, source/content version, card
and step IDs, displayed option mapping, selected option(s), attempt ordinal,
judged result, active timestamp, input mode, help exposure, and preset identity.
Pointer events additionally preserve click coordinates, target identity and
displayed bounds/position, and route/time reference for reconstruction.

The record has three levels: one endless run, each presented problem instance,
and its ordered step instances. Repeating card 002 in the same run creates a
new problem instance; it cannot overwrite the first pass or be mistaken for a
duplicate click. A minimal event envelope contains:

| Record group | Required values |
| --- | --- |
| Identity | Run ID, monotonic event sequence, problem instance, question/content version, step ID, event kind |
| Configuration | Variation, answer interaction, resolved preset/hash, score-rule version, input/assistance mode |
| Timing | Active timestamp, motion tick/interpolation, presented frame/context generation; phase-duration events |
| Display | Stable option-to-slot mapping, camera/viewport/layout revision, route references and seeds |
| Choice | Clicked option or submitted set, judged outcome, first/retry ordinal, newly collected IDs, aid exposure |
| Pointer | Window and normalized gallery coordinates, matched target, its displayed sphere geometry/bounds, ignored/miss reason |
| Source and review | Immutable question snapshot/reference with prompt, choices, accepted set, explanation, source locator/hash |

Save reusable layout/content snapshots once and reference them from events.
An event need not copy the entire question or frame image. A movement seed
alone is insufficient for an exact pointer reconstruction after algorithms,
viewport, or text measurement change. Keep the actual click-frame geometry
needed for the review overlay. Never require the mutable source-card directory
to reopen an old local result.

Distinguish wrong target, incomplete submitted set, empty-space miss, duplicate
or ignored click, timeout, retry, skip, and reveal. An option's misconception
tag describes the selected distractor; it does not prove the cause of a
player's mistake or let us infer what they intended to click.

The run board should show points, completed steps/questions, first-attempt
results, retry/assistance counts, and active duration. Review should offer:

- A step-by-step history with the actual question, choice, correct answer,
  explanation, and chronological attempts.
- Separate first-response correctness for One/Any, clean collection completion,
  and exact-set accuracy, each with its denominator. Never count tentative
  selections as submitted sets or combine these units into one percentage.
- Pointer hit accuracy with explicit hit/miss counts, response duration, and
  screen-position heatmaps for correct, wrong, and empty-space clicks. A wrong
  mathematical target is still a geometric target hit.
- Concept summaries and Practice Again selection for weak or requested steps.
- Local personal bests grouped by compatible content, gameplay preset, score
  rule, and input/assistance settings. An endless cumulative score reflects
  play length; also offer fixed-duration slices or points per active minute
  with the actual duration and attempt count visible.

Define the displayed measures before implementation:

| Measure | Numerator / denominator and exclusions |
| --- | --- |
| First-response correctness, One/Any | Steps whose first judged answer was correct / steps with a first judged answer; show unassisted and assisted subsets |
| Clean collection completion | Collect steps fully completed without wrong hits or help / ended collect steps; show revealed, skipped, and unfinished outcomes in the denominator breakdown |
| Exact-set first-submission accuracy | Steps whose first submitted set exactly matched / steps with a submitted set; separate assistance subsets |
| Pointer hit accuracy | Eligible target-hit clicks, correct or wrong / eligible target-hit clicks plus empty-space misses |
| Clean-hit combo | Consecutive newly accepted correct-target hits; resets on wrong target, empty-space miss, Reveal, Skip, or a new run |
| Completion | Player-resolved and revealed steps shown separately; partially collected or skipped steps are not completed |

Menu clicks, Reading clicks, transition clicks, duplicate locked-target clicks,
and stale input are excluded from pointer accuracy. Keep their diagnostic
counts separately. Keyboard answers do not fabricate pointer coordinates and
are excluded from aim statistics. Zero denominator displays an em dash with
"No attempts", never 0% accuracy. An ended partial step is distinct from a
not-yet-encountered step.

Review opens with a run summary and a chronological list of its problems.
Selecting a step shows the original prompt, all choices, the actual attempt
sequence, correct answer, explanation, and any hints/reveals. A click overlay
can then show where each input landed. Default review filters are Wrong
answers, Aiming misses, Help used, and All; an entry may appear in more than
one filter. A distractor tag describes the selected answer's issue and must
not claim to diagnose the learner's intention.

Practice Again offers Selected steps or Whole problems. Whole problems always
retain the authored step order. A selected step uses an authored context
snapshot with its prerequisites clearly supplied; it does not resume from an
arbitrary later equation with missing work. Store the parent review selection
and prior exposure on the new run. Automatic mastery scores and spaced review
scheduling are separate future designs.

Durable scores and mistakes need explicit local storage. Save versioned preset
files and an append-only run/event journal, with summaries derived from that
journal. Use the platform's local app-data location, with a test/output-path
override; an installed app must not depend on the repository being writable.
Do not write game results into the original mathematics pages.

Flush completed step records and run/pause boundaries, with periodic durable
checkpoints during long active steps. Track the last durably saved sequence;
the UI must not claim an event is saved before storage acknowledges it.
Recover the valid ordered prefix after interruption and identify any truncated
tail. Preserve an interrupted run as interrupted. Continuing creates a linked
recovery segment from the last saved state, with the same content/configuration
and acknowledged points, and is identified in score comparisons. Do not
fabricate unsaved hits or silently call a partial run completed.

Preset saves use replacement of a fully validated file. Unknown versions or
invalid values produce a useful load error while retaining the last valid
data. Failed history writes mark the session as not fully saved and offer a
local export; play must not erase earlier durable results. JSON export carries
the full event/configuration record; CSV offers one row per judged attempt
and a separate pointer-event table, so collection and misses remain distinct.

Bound in-memory event batches and stream older events to disk. Do not retain
screenshots or rebuild a whole-session heatmap every frame. Reconstruct reports
on demand or incrementally from the journal. Persistence is its own checkpoint
with save/load, interruption, and duplicate-sequence protection checks.

Personal bests are local. Group by variation/policy, content and version,
question-order/seed policy, actual aiming/time settings, effective target
geometry, score rule, input mode, assistance policy, and recovery/modification
status. Keep intended configured assistance separate from aid actually used.
A fixed-seed challenge and an ordinary shuffled endless run are different
groups. Cosmetic preferences do not need new groups unless they affect the
hit/readability contract. Ordinary shuffled runs can share a group under the
same shuffle algorithm/seed policy while retaining their actual seed for
replay. Fixed-seed challenge groups include the exact seed. Always show run
duration and repeated exposures.

## Pause menu

| Menu | Contents |
| --- | --- |
| Resume | Return to the exact paused step and route phase |
| Controls | Mouse button, cursor visibility/size, pause binding, keyboard navigation, later gamepad virtual cursor |
| Targets and routes | Pattern, placement editor, speed, display profiles, target dimensions, font, preset save/load |
| Difficulty and content | Mathematical pack/tier, aiming, bonus duration, answer interaction, endless deck settings |
| Colours and scene | Target states, text, ground, boundary grids, walls, background and sky |
| Audio and accessibility | Cue volumes, mute, reduced motion, large labels, contrast, manual progression |
| Help and review | Explicit reveal or authored hints, current-run history, completed-step explanations |
| Developer preview | Draft editing, camera controls, deterministic timeline, validation diagnostics |
| Finish / Restart / Skip problem / Quit | Preserve the old outcome and apply the requested lifecycle transition once |

Escape toggles Pause/Resume when no deeper dialog owns it; in a submenu it
goes back one level. Space confirms a set only in Exact-set Active mode and
acts as Ready/Continue only in those named states. Do not overload Space as
a second mouse shot in arcade play. A held key or the pointer release that
closes a menu cannot answer the next question.

Mouse and keyboard navigation ship first. Direct keyboard answer shortcuts
are an explicit accessible profile and do not count as aiming trials.
Gamepad virtual cursor requires its own tuning for sensitivity, acceleration,
deadzone, and optional assistance before it becomes a selectable gameplay
profile. Reduced motion, large labels, optional sound cues, and manual
advancement remain playable settings. Correctness never relies only on colour.

## Content admission and shared ownership

Keep the original source pages read-only. The current 95-card collection is a
source library, not a 95-card playable deck. Each admitted game adaptation
needs its full givens, source locator and pinned bytes, stable IDs/version,
ordered steps, reviewed keys/distractors, working lines, and explanations.
Test calculations independently where possible and review the scope of proof
claims. Learner setup/solve fields are not acceptance receipts.

The current authoring schema supports four choices and one key. Multi-answer
steps need a versioned accepted-set/interaction extension with compatible
progression data. Do not edit the two source adaptations merely to make a
prototype appear to support that extension. Preserve old content identities
and translate the existing single key at the one content admission boundary.

Reuse the compiled immutable catalog approach already proposed in
[SOURCE_CARD_ARCHITECTURE.md](../content/SOURCE_CARD_ARCHITECTURE.md). The
gameplay runtime does not parse the external Markdown collection or depend
on Python. A general content importer is a later capability with a live
authoring need.

| Canonical owner | Owns | Boundary to preserve |
| --- | --- | --- |
| Shared content catalog | Prompts, roles, option IDs, accepted keys, working, explanations, source/version | Both Guided and Gallery reference the same admitted content |
| Shared question session | Legal choices, correctness, ordered attempts, accepted working, help, resolution and next mathematical step | Extend the existing `LayeredQuestionSession` semantics; do not create a second gallery answer checker |
| Gallery run state | Active problem instance, arcade presentation phase, timing anchors, config and option-to-target bindings | Consumes the scene's fixed clock and question outcomes; cannot independently advance algebra or change correctness |
| Patrol model | Target poses from route configuration, active time and seed | No access to answer correctness when assigning or moving unjudged targets |
| Gallery command entry | Semantic gameplay actions, active context, admission/forwarding | Mouse, keyboard and scripts converge at `GallerySession::dispatch`; editor scene actions and existing Guided/Hunt entry routes retain their owners |
| Presented-frame picking | Pointer-to-target/empty-space resolution from visible geometry | Produces a target identity and frame reference, never a correct/incorrect answer |
| Score/statistics evaluator | Rewards, denominators, summaries from authoritative facts | One score authority shared with P002 where available; no points in draw code |
| Native host / renderer | Vulkan resources, gallery drawing, text, capture | Displays state; cannot mutate a question or journal an attempt during drawing |
| Storage | Ordered durable events, versioned presets, reconstruction | Cannot rewrite old attempts after retries or content/rule updates |

The live route to prove is:

```text
Mouse button-down in a presented gallery viewport
  -> input context + presented-frame pick
  -> shared semantic answer action with stable option/step identity
  -> question session judges and records the attempt exactly once
  -> gallery timing/transition consumes the accepted outcome
  -> score/statistics consume the resulting ordered facts
  -> renderer presents points, feedback, and the next valid working line
  -> journal persists that same evidence when durable history is implemented
```

The current Guided session requires Try Again after a wrong answer, only
offers Show Answer from recovery, and uses select-then-check. Arcade needs
continuous retry, explicit reveal, and a click that submits immediately.
These are explicit interaction-policy extensions at the shared session
boundary. Preserve Guided's existing recovery behavior. One atomic semantic
submission must validate the option and record its judgment; do not have a
widget call unrelated public mutators and locally reconstruct the result.

The gallery phase reflects whether a resolved step is being shown during its
short transition. Only the shared question session can accept advancing to
the next mathematical step. The next-step event must not be emitted once by
the session and again independently by the animation callback.

The concrete method and file map is in
[TARGET_FOUNDATION_DESIGN.md](TARGET_FOUNDATION_DESIGN.md). It proposes the
`TargetMotion` and `GallerySession` header/source pairs and extends the existing
scene and question owners. Those four proposed production files are new
feature work, not a cleanup claim. This documentation change creates none of
them and moves no Creative production code.

## Delivery checkpoints and observable proof

Before implementation, reconcile the current P001 completion evidence and
the recorded P002/P003 order. Card 002 requires the catalog work described
for P003. Gallery scoring must fit P002's canonical rule owner if it exists.
Implementing every Workshop screen is not an intrinsic requirement for a
target to move. This proposal records those dependencies without dispatching
or silently reordering the existing workstreams.

Take one capability per checkpoint. These labels describe proposed gallery
deliverables; they do not replace existing packet IDs.

| Checkpoint | User-visible result | Focused proof / completion boundary |
| --- | --- | --- |
| Shared card 002 catalog, existing P003 concept | The complete authored card is available to the shared question model | All 13 steps, stable source/content identity, correct solution, old starter behavior preserved; prerequisite to the gallery content loop |
| G1: Equation Gallery | Direct startup, shallow 3D room, fixed cursor play, four moving choices, arcade progression, bonus, Pause/Finish and in-session review | Complete and repeat card 002; wrong hit followed by recovery; empty-space miss; expiry; pause; exactly-once rewards; stale-click rejection; stationary, horizontal, circle layouts |
| G2: Route Workshop | Place/drag/duplicate/assign routes, speed/phase/dwell, grid snap, undo/redo, draft preview, named presets | Edit and reload one useful four-target setup; no practice attempts from neutral preview; draft cannot mutate a paused run |
| G3: Complete patrol and display set | All twelve patterns plus Stationary; fixed/shuffled/rotating/alternating/replay profiles; ground, boundary-grid, target and sky colours | Versioned route data and recorded assignment reproduce poses and mapping; ball/marker envelopes stay admissible and stationary answers remain readable; paused/resumed phases and a preset round trip agree |
| G4: Condition Sweep | Its own startup, reviewed multi-answer question, Collect all arcade, Any-valid where authored, optional Exact-set practice | Distinct accepted IDs score once; wrong hit preserves collected progress; incomplete/wrong sets do not advance; policies retain different evidence |
| G5: Saved results and scoreboard | Durable local runs, partial runs, ordered attempts, review filters, click overlay and JSON/CSV export | Save/load, truncated-tail recovery, acknowledged event sequence, correct comparison groups, zero-denominator display; points can be reconstructed |
| G6: Practice Again | Replay selected mistakes or whole problems from review | Correct context supplied, original evidence unchanged, new instance/exposure identity, preserved whole-problem order |
| Following variations | Notation Range, Error Hunt, Transform Range, Reason Relay, each separately launchable | One reviewed card slice and final observable outcome per variation; no duplicate target, question, or scoring implementation |
| Common launcher | Choose ready variations from one location | Same variation/preset entry actions as the direct shortcuts; saved settings and run identities preserved |

G1 is the first playable checkpoint. G1 through G5 deliver the initial gallery
feature bundle described in the original request, after their prerequisite
content integration. G6 and the other variations add breadth. All twelve
patrols are part of that initial bundle, even though only three are needed to
prove the first interaction. Each row still needs its own review; this plan
does not authorize one unbounded implementation pass through the entire table.

For G1, derive actual targets from the then-current CMake graph. The inspected
graph has `paths_scene`, `paths_model`, `paths_ui`, `paths_native`, `paths`,
and `paths_gallery`, with test executables `paths_scene_tests`,
`paths_guided_tests`, `paths_hunt_tests`, and `paths_input_tests`. The target
foundation design proposes a separate `paths_gallery_model` composition
library and its focused test. This is a dependency map, not a blanket gate.
Extend the affected headless model/scene/input boundaries and use one
representative native offscreen capture case for the gallery.

G1's acceptance cases must cover:

- The complete live route for card 002 through all 13 steps, correct final
  working, and a new instance on the endless repeat.
- A wrong answer and a geometric miss produce different history and the
  score outcomes in the table; neither creates a recovery modal in arcade.
- Pause/focus loss preserves route and timer state; bonus expiry happens
  once; an old frame or held click cannot answer a fresh step.
- All four full answers remain readable in the stationary board and their
  corresponding balls remain selectable. Use the longest actual explanation
  and option content. Explicit compact layout may be necessary.
- Native room geometry has correct depth and the target pick agrees with
  the displayed sphere. Verify Vulkan camera conventions, resize, and viewport
  offset; a scene capture and a pointer input case prove different boundaries.

Automated checks establish state, arithmetic, input boundaries, and rendering
evidence. Offscreen captures require visual inspection. Human playtesting then
judges cursor feel, identification of moving balls, stationary answer
readability, feedback pacing, and whether the timer makes the mathematics
enjoyable. Record those acceptance states separately. Build and verify
headlessly until a visible window is requested.

## Decisions still to tune

The arcade priority is settled. The following are proposed defaults or future
choices, not blockers to completing this design:

- Tune `100 / 25`, the 50-point bonus, step durations, and the 0.30-second
  transition after observing a complete card. First establish understandable
  rewards; add combo multipliers only for a demonstrated gameplay benefit.
- Choose measured speeds and sphere sizes for Gentle/Standard/Fast from a
  common viewport. Preserve the minimum readable mathematical text.
- Confirm the implementation queue relative to the existing P002/P003 work
  before issuing a gallery build packet.
- Mathematical notation is the initial syntax content. Programming or natural
  language grammar requires a named subject and convention-specific pack.
- Advanced depth patrols, staggered waves, procedural numbers, full proof-path
  branching, gamepad aiming, textured skyboxes, online boards, and automatic
  adaptation are later capabilities with their own live need and proof.

## Proposal validation

The original proposal inspection covered the brief, arcade-first preference,
authored cards and parent-engine reuse routes. Its recorded checks found 95
source cards, matching source hashes for both adaptations, ordered 13-step
identities for card 002, the exact substitution/determinant, and consistent
score-example arithmetic. These are retained inspection results, not new
gameplay or build evidence.

This method-design revision incorporates the settled coloured-ball interface
and checks the current Paths CMake graph, scene, native startup, question
model, and copied movement kernel. The live question starter still has seven
fixed steps; Sphere and gallery/question composition remain proposed.

The two design documents were reviewed for consistent ownership, method
contracts, and the aggregate documentation diff. Local Markdown links exist;
code fences are balanced and no trailing whitespace was found. All 63 input
files included in the source/content snapshot retained their starting hashes.
Changes are confined to this proposal and `TARGET_FOUNDATION_DESIGN.md`.
Production LOC/file count is unchanged by this revision. No game build,
gameplay test run, window, or interactive acceptance is implied. Both
documents remain uncommitted alongside the pre-existing worktree changes.
