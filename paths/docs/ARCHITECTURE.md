# Paths architecture

Authority: the user's request to isolate the math game in its own `paths/`
folder and give it a title screen, game variations, objectives, statistics,
and scoring experiments. This supersedes plans to extend First Move inside
the parent engine. Historical First Move documents are reference material.

## Product and project boundaries

Paths is its own CMake project and executable, physically under
`/Users/kogaryu/iggy3d/paths/` for now. It must configure, build, and run after
this folder alone is copied elsewhere. It does not invoke the parent CMake,
link the parent `iggy3d` target, load a Creative document, or require parent
shaders/assets. System SDL3 and Vulkan remain declared external dependencies.
The user separately connected this folder's own Git repository to
`git@github.com:KOGA-ryu/paths.git`. Local commit `2308995` records the
foundation through P013. Movement setup and content organization are recorded
together in the subsequent local checkpoint commit.

P001 carries the existing two playable modes into that boundary. Guided
Questions teaches one equation in layers. Quick Hunt classifies a row of
equations and banks correct rows. They are choices under Paths, not two
separate applications. New mode ideas become later playable additions.

The first title screen says `PATHS` and `One problem. More than one way
through.` It offers those two working modes, `SESSION STATS`, text size,
and `QUIT`. It does not display a gallery of disabled future features.

## Owners and dependency direction

```text
                    native input / pointer / script
                                |
                     semantic app dispatcher
                       /                 \
              title/navigation       active mode model
                                           |
                                  immutable run evidence
                                    /             \
                              statistics       scoring rule
                                    \             /
                                      presentation
                                           |
                                  native Vulkan host
```

| Owner | Decisions |
| --- | --- |
| Content catalog | source identity/revision, givens, roles, prompts, option keys, explanations |
| Mode model | legal actions, checked attempts, help exposure, completion, game-specific facts |
| App dispatcher | title/stats/mode navigation, active input context, forwarding to the canonical model |
| Objective evaluator | whether a frozen run objective has been met |
| Statistics projection | counts and denominators from actual evidence |
| Score evaluator | a named, versioned game's points from evidence and frozen rules |
| UI | layout, focus, presentation text, animation; no answer keys or point calculations |
| Native host | SDL events, Vulkan resources, ImGui frame lifecycle, capture and presentation |

P001 retains the already working Hunt score in `HuntSession`; its stat view
reads that value. P002 establishes the separate score owner and removes the
old point-calculation route in the same bounded change. Do not have two live
point authorities during the migration or claim that P001 already implements
the future objective/scoring interfaces.

The copied model namespace may remain `iggy3d::first_move` during P001. It is
a source namespace, not a link to the parent project. New app/host types use
`paths`. Rename old namespaces only when it materially improves ownership;
do not turn migration into a cosmetic rewrite.

## Folder and target map

```text
paths/
  app/main.cpp                     First Move startup, CLI, scripts and reports
  app/gallery_main.cpp             gallery menu, gameplay/workshop UI, CLI, scripts and reports
  src/runtime/first_move/           existing pure models
  src/ui/FirstMoveUi.*              shared app dispatcher, title/stats/mode presentation
  src/ui/LayeredQuestionUi.*        guided question view
  src/ui/GalleryMenu.*             gallery selection, pending setup and session lifetime; no SDL/Vulkan
  src/platform/NativeVulkanHost.*   standalone SDL/Vulkan/ImGui host
  src/scene/GalleryScene.*          pure scene actions, geometry, camera, and picking
  src/scene/TargetMotion.*          route preparation, common tick advancement and time preview
  src/runtime/gallery/GallerySession.*  target/question composition and challenge transactions
  src/content/QuestionContentIO.*  playable JSON decoding and shared validation
  vendor/iggy3d/src/                attributed math, camera, motion, allocator/capture helpers
  shaders/                         copied first-room vertex-colour shaders
  third_party/imgui/                exact pinned SDL3/Vulkan ImGui subset plus license
  content/cards/                   playable question JSON in the runtime schema
  content/packs/                   question file lists and ordered practice decks
  content/authoring/               retained 002/013 authoring JSON; separate format
  content/source_snapshots/        read-only source copies and provenance
  content/reference_cases/         planning/reference specifications
  tests/                           copied baseline plus migration-boundary tests
  docs/                            active packets and source provenance
```

The [content folder guide](../content/README.md) identifies the live editing
path. The gallery loads files explicitly listed by a pack; it does not discover
all JSON files under `content/`. Authoring cards retain their original bytes
and resolve their source snapshot fields from the `content/` root through the
planning validator. They are not inputs to the gallery JSON loader.

`paths_model` contains pure C++ game models. `paths_ui`
depends on `paths_model` and privately on SDL/ImGui. `paths_native` contains
the host and copied helper implementation, with private SDK/ImGui dependencies.
`paths` composes those targets. `paths_imgui` is the local pinned dependency.
No target uses `../src`, `../apps`, `../build`, an absolute iggy3d source path,
or an absolute visualization path for a build input.

The ImGui Vulkan backend embeds its default UI shaders. The existing math
startup remains UI-only. The user-authorized P005 engine port adds a separate
`gallery` startup with a 3D Gallery Workshop. `paths_scene` owns its pure
model and copied math/camera/waypoint kernels. `GalleryScene::dispatch` is the
sole mutation route for pointer, keyboard, developer UI, and script commands;
it does not add another active-mode variable to First Move.

`NativeVulkanHost` remains the sole native/GPU owner. Its optional scene path
ports the parent's primitive triangle pipeline to the host's existing render
pass, adds a D32 depth attachment, and draws indexed vertex-colour geometry
before ImGui. The host transposes the parent's row-major CPU matrix for GLSL.
The copied shaders are compiled with glslangValidator and embedded into the
executable at build time. Paths requires no parent shader directory at runtime.
P005 ports the bounded gallery foundation, not the entire Creative editor.
See [P005_ENGINE_PORT.md](P005_ENGINE_PORT.md) for controls, scope, and evidence.

P006 adds `paths_gallery_model`, linking `paths_scene` and `paths_model` without
SDL/Vulkan. Its `GallerySession` owns one scene and one shared question session.
Typed Tick, Pause, Viewport and Shoot commands enter through its dispatcher.
Shoot carries the exact presented frame and challenge identities. The scene's
read-only `hitTestPresentedFrame` is the one geometry query used by gameplay
and workshop selection. Question correctness remains in the question model's
single `judgeOption` route, reached by Guided CheckAnswer or arcade SubmitOption
under a frozen interaction policy.

The shared question model now owns immutable content, variable steps/choices,
stable evidence IDs and accepted/collected sets. The five foundation questions
load from the P010 prepared JSON pack. P011 and P012 add separate packs for
cards 002 and 013 using that same contract and runtime owners.
The original Guided startup retains its select/check/recovery behavior.

`TargetMotion` owns all route evaluation and seeking; every scene object goes
through its fixed-tick advance operation. Waypoints delegate to the local
ported kernel. A pause checkpoint bounds preview to the following 60 seconds,
including dwell and reversal. `GalleryScene` owns the shared sphere mesh and
Spawning/Active/Popping/Retired phases. Target reset, new question state and
the complete seeded colour assignment are prepared together before commit.
The UI processes input and simulation before drawing the current board, so a
transition cannot display an old answer mapping beside new targets.

The gallery header shows correct targets, wrong answers, aim misses and
completed questions only while stopped. It does not create a competing point rule;
P002 remains the owner of future score/bonus projections. See
[P006_TARGET_FOUNDATION.md](P006_TARGET_FOUNDATION.md) for current scope/evidence.

P019 establishes the gallery's continuous arcade loop. A wrong answer retains
the current prompt, targets and collected set while motion and input continue.
The existing question owner still advances after its required correct answer
or answer set; targets pop and the next step/question starts automatically.
There is no mistake-triggered pause, review, retry round or game-over route.

`GallerySession::submitAnswer()` forwards button choices and target-hit options
to the question owner and consumes its recorded verdict in one place. It sets
a typed `GalleryFeedback` and an expiry 30 scene ticks later. `view()` exposes
it for half a second of active time;
the indicator does not gate input or advance a question. A later accepted shot
replaces it and a new challenge clears it. Misses remain distinct from wrong
mathematical answers. The UI renders the short signal without interpreting
answer labels. The old persistent coaching strings and their active-time
display/unused per-challenge clock fields are removed.

The dispatcher retains input-specific guards, hit validation and aiming misses.
The shared answer route advances resolved button choices and finite practice
immediately; arcade targets retain their pop interval. It updates target colour
history only for an accepted correct target hit. See the
[prepared-answer cleanup](CLEANUP_SORTER_INPUT.md#follow-up-prepared-answer-feedback).

Stop/Esc uses the existing `GalleryPause` route. While paused, the UI displays
the existing totals and offers Resume, optional Answer review, question
selection and Quit. Resume hides totals and continues the same game. This
does not create a completed-run record or new scoring policy. The current mode
is endless; a whole-run countdown is a later capability. These rules supersede
the proposed automatic missed-question retry direction after P018. See
[P019_CONTINUOUS_PLAY.md](P019_CONTINUOUS_PLAY.md).

`GallerySession::view()` assembles shared identity, working, pause state,
completion and accumulated attempt totals once, then fills the mode-specific
fields. `GalleryScene` still owns pause state and `LayeredQuestionSession`
still owns question evidence. Mathematical Submit events and prepared answer
attempts retain their existing counting rules; Undo and rejected input do not
become additional answers. See the [summary cleanup](CLEANUP_SORTER_INPUT.md#follow-up-shared-game-summary).

The [P020 Equation Sorter](P020_EQUATION_SORTER_SPEC.md) is a separate native
`sorter` startup sharing the host and ImGui. `EquationSorterSession` owns
inspection, assignments, reserved inventory slots and transaction Undo.
`EquationSorterContentIO` decodes its independent 100-card JSON and calls the
model's structural validator before native startup. `EquationSorterUi` sends
one semantic activation per ordinary click/fresh key press, applying queued
input after host events and before taking the next rendering snapshot.
Focus loss discards queued activation; Escape has a sorter-owned shortcut route.
Its freely classified cards do not use the judged-question schema or change
gallery behavior. The first pack is generated and independently checked algebra.
[P021](P021_MIXED_MATH_PACK.md) adds a separate prepared mixed pack: 80 retained
algebra cards and five examples each of trig, calculus, linear algebra and
discrete maths. The loader and session do not interpret mathematical expressions.
Bounded test-only interpretations check the displayed expressions; no maths
evaluator enters the runtime. The UI reserves vertical scrollbar
space from the first frame to keep card widths stable when an inventory opens.
Broader subject coverage, richer notation and gallery integration remain future work.

[P022 sorting assistance](P022_SORTER_ASSISTANCE.md) extends that same owner.
Prepared optional `subject` and `hint` metadata enter through the existing
loader and structural validator. A shared subject table maps the five subjects
to A–E; Dump stays manual. `AutoSort` collects only unassigned, classified cards
in home order and calls the existing atomic transaction method. Manual moves,
the current view and reserved inventory slots survive; one Undo reverses the
whole batch. Repeated empty batches do not create history, and stale automatic
actions use the same revision guard as a card commit.

`view()` supplies readiness, next-action guidance and prepared hint text.
`ShowHint`/`CloseHint` change help visibility without assigning or inspecting a
card. The UI presents the projection in a scrollable panel with a fixed Close
control and routes Escape to help before an underlying confirmation or inspection.
It does not classify equations or advance a mathematical solution. After all
cards are assigned, selecting any group opens its inventory in one action.

[P023 prepared solving](P023_PREPARED_SOLVING.md) connects one linked algebra
card to the existing question/gallery owners inside `sorter`.
`EquationSorterSession` owns the selected solving session and return/resume;
`LayeredQuestionSession` owns accepted decisions, the current prepared working,
and separate hint/next-move/applied-step evidence. `GallerySession` forwards
operation buttons and real sphere hits to that same question dispatcher.
Its finite prepared flow advances immediately, while the standalone continuous
gallery retains its original lifecycle. `GalleryScene::FrameTargets` uses the
existing camera-fit kernel to keep the prepared arithmetic targets visible.

The optional sorter `solve_pack` is resolved and validated by the content loader
before native startup. Optional step help and bounded working-state highlights
are frozen question content. The UI presents those spans without parsing maths.
Solving does not mutate groups, home slots or grouping Undo, and opening another
subject's unprepared card does not silently choose the algebra question. Only
the linked algebra example is a solving question in this checkpoint; the other
mixed-pack statements remain sorting content.

[P024 bracket recipes](P024_BRACKET_RECIPES.md) extends the existing authoring
tool to expand bounded `a(x + b) = c` recipes into the P023 question format.
This is the sole producer for the six prepared bracket questions, including
version 2 of the original P023 card. Exact arithmetic, distinct choices and
source/version checks happen before publication. Neither runtime loader nor UI
derives mathematical answers. The native content-copy target includes all
prepared sorter packs and their linked question files.

`EquationSorterSession` now lazily retains one `GallerySession` per opened
content home slot, bounded by the existing 100-card contract. Its one active
slot selects the session; the others remain paused. Opening, returning and
replaying retain the existing dispatchers and grouping Undo. The UI consumes
queued game input when switching context and resets the displayed challenge so
two questions with equal local challenge numbers cannot share stale controls.
There is no second progression, help or answer-history owner.

[P025](P025_FIXED_SOLVING_WORKSPACE.md) replaces the step-dependent solve UI
placement with one presentation layout derived only from window dimensions.
An ImGui root with fixed child panels keeps keyboard navigation continuous;
operation buttons, bound sphere labels and completion controls share the scene
viewport. The original equation and current working remain separately visible.
History reads the question owner's existing review projection.

`EquationSorterSession::NextSolve` requires completion and a current revision,
then uses the same preparation route as OpenSolve for the next prepared home
slot. It preserves prior per-card sessions and refuses to wrap at the end.
No chapter-selection queue is introduced. UI context changes discard queued game
input; a shot's held press is consumed until release so it cannot activate a
new operation appearing in the same space. Existing judging and scene owners
remain unchanged.

[P026](P026_COMPACT_SYMBOL_CHOICES.md) keeps that ownership while centring a
compact problem/working/choice group and bounding the activity height. The
existing recipe generator emits operation symbols and a shared both-sides
instruction into ordinary option labels and prompts. No runtime schema or
English-to-symbol UI conversion is added. Generated questions and decks use
content version 4 with generator version 2; the existing font supports all
delivered symbols. Accepted IDs, mathematical working and help are unchanged.

[P027](P027_RESPONSIVE_SOLVING_WORKSPACE.md) supersedes P026's fixed activity
cap. The same UI layout computes a scale and fitted workspace from window
dimensions; problem text, working and choices grow together. The activity
rectangle remains shared with the existing scene viewport, and resizing uses
the existing gallery framing route. No content or runtime ownership changes.

[P028](P028_STUDY_SELECTION.md) adds a table of contents using optional authored
`study.chapter`, `study.type` and `study.form` fields on sorter records. The
loader and shared sorter validator check those titles; no equation parser or
UI classifier infers them. Only linked, validated solving questions enter the
session's immutable catalogue.

`EquationSorterSession` owns subject/chapter/type inclusion, all/random/specific
selection, stable random previews and a frozen study queue. UI chapter browsing
is presentation state. Start, Resume and Next share existing solving preparation;
selected-set Next traverses the frozen queue, while individual-card Next retains
home order. Draft edits cannot change a running set.

Starting a new set prepares copies of all selected sessions before replacing
retained owners. The existing gallery replay route forwards an explicit
`archiveUnfinished` option to the question owner's `RestartQuestion` command.
Only this new-set use opts in; ordinary replay still requires completion.
The original attempt is archived with its actual completion state and evidence.
No second history, judging or help owner is added.

[P029](P029_COORDINATE_BOARD.md) adds prepared straight-line graphs to that
catalogue. Optional `LineGraph` parameters and each working state's `GraphStage`
are validated by `LayeredQuestionSession`; its read-only `coordinateGraph(x)`
projection owns intercept, run/rise geometry, clipped line endpoints and probe
coordinates. Equation strings are never parsed during play. `GraphChoice`
steps enter the existing submission route, generalized from `ChooseOperation`
to `ChooseAnswer`; there is no parallel graph judge or attempt store.

`EquationSorterUi` draws the coordinate board with native lines, circles,
triangles and text. It owns screen fitting, short reveal animations, replaying
motion, and the transient requested probe position. Probe movement reads the
question projection without modifying evidence. The graph fills the same fixed
activity region through solving, help, completion and explicit Next. The native
sorter omits the sphere render snapshot for graph questions. Existing sphere
questions retain their viewport, camera and input route. No image assets or
screenshots are involved.

[P030](P030_SIMULTANEOUS_EQUATIONS.md) extends `LineGraph` with an optional second
line and a four-decision system reveal chain. The same question owner validates
both lines, clips their endpoints, and projects two points at one shared x.
Its single system-solution helper classifies intersecting, parallel and
coincident lines and calculates any unique intersection; validation also uses
that helper to reject a crossing outside the axes. The projection makes the
guide available after both lines are revealed, exposes the relation after the
solution-count decision, and marks a unique intersection only after the final
decision. The guide never submits an answer or changes evidence.

The existing UI renderer draws a solid teal first line, a dashed violet second
line, one vertical guide and two coordinate readouts. A completed unique point
or coincident solution line is mint. The infinity choice uses native Bezier
strokes independent of font coverage. Layout reserves space for the paired
original equations without moving the problem or board between steps. Existing
selection, `ChooseAnswer`, help, replay and explicit Next routes remain the
only routes for those actions. The recipe publisher computes exact rational
answers and expands four systems into the existing default study pack; no
runtime equation parser or new production code file is added.

[P031](P031_LINKED_VALUES.md) adds an optional three-row value table to the
existing `CoordinateGraphView`. `LayeredQuestionSession::coordinateGraph(x)`
uses its existing point evaluator to sample the shared visible x range at its
ends and midpoint. The table is absent until the existing probe-availability
gate opens. Its rows are independent of the current guide input; no question
content, answer rule or reveal chain changes.

The existing graph renderer reserves a compact table beside the plot and
numerical substitution strips above it. Three sample buttons, plot dragging
and the slider all change the same transient `graphProbeX`; their inputs are
handled before the shared projection is refreshed and drawn. The amber live
row, coloured graph points and numerical substitutions therefore use one
projection in the same frame. The UI formats provided parameters and values,
with two-decimal approximations, without evaluating the equations itself.
These strips replace the prior floating probe-coordinate labels. The original
problem, graded working, help, attempts and explicit Next keep their owners.

[P032](P032_MATHEMATICAL_MOVES.md) adds the `MathMoves` interaction to the same
question owner. Bracket content explicitly opts in with
`working_model: "linear_moves"`; the sorter selects this interaction through
its existing GallerySession. Its `MathematicalMove` command forwards the frozen
question/version/run/revision and the player's operation, number and equation.
The pure `LinearEquation` kernel parses bounded affine expressions with exact
rational arithmetic and checks the selected transformation separately on each
side. A matching solution alone cannot pass an unrelated operation. The UI
never parses or checks mathematics.

`LayeredQuestionRunRecord::math` owns append-only working nodes and events. A
correct submit appends a node with its parent; a wrong submit appends an event
without changing working. Undo records a return to the parent and keeps both
branches. Completion is exact substitution of the isolated numerical answer
into the original equation; the existing completion flag gates Next. Review,
summary, restart and per-question retention read this same evidence. Prepared
commands and answer targets are unavailable in this interaction; the prepared
interaction remains for its existing arcade and other authored consumers.

`EquationSorterUi` presents a fixed original/active/input area and a scrollable
blueprint using native text and strokes. Inspection is read-only. Input is
queued before the next view, guarded against focus loss and stale identities.
The native app omits the scene for mathematical-move questions. There is no
second progression or attempt store. The six generated bracket cards advance
to version 5; graph content and graph judging retain their existing format.
The P032 packet specifies parser, numeric and history limits. Session retention
continues to be in memory; durable persistence is not implemented.

[P033](P033_VISUAL_MATH_MOVES.md) replaces the equation/number fields and Check
button with concrete operation buttons and four result tiles. The shared
question owner's `mathMoveChoices()` projects the current equation through
`availableMathMoves()`. Generation and submission call the same
transformation rules; only submission records an attempt or advances working.
The bounded palette includes expansion, coefficient division, reciprocal
multiplication, and signed constant moves. Result choices contain one valid
transformation and distinct mistakes, including missed distribution and
one-sided operations. Their order is deterministic per equation and operation.

The UI caches this read-only projection for the current question/run/revision
and retains only a selected move index. Clicking a result forwards its operation,
operand and equation through the existing `MathematicalMove` route. A successful
move or Undo clears selection; an incorrect result preserves the offered order.
The displaced text buffers and public operation-menu API are removed. Content,
correctness, attempt history, graph controls and explicit Next retain their owners.

[P034](P034_MATRIX_ROW_MOVES.md) extends that same interaction to a two-row,
three-column augmented matrix. `MathWorkingModel` selects scalar linear working
or row reduction from declared content. `prepareMathWorking()` owns initial
validation and construction. Each existing working node contains a typed
`MathWorkingValue`; `LayeredQuestionSession` visits that value to request
choices, check a move and verify completion. There is one attempt/history
route, with no matrix-specific session or progression state.

The existing pure mathematical kernel shares exact rational arithmetic between
both forms. Matrix generation and judging use the same row transformation;
all six cells must match. Identity-form answers are substituted into both
original rows before completion. The UI uses the same operation/result/history
panels, with extra height for two-line matrices. The new authored card is linked
through the existing content publisher and study catalogue. No parent project,
native renderer, build graph or additional production file is involved.

[P035](P035_ROW_REFERENCES.md) adds an optional shared reference library to the
existing question-pack loader. Cards resolve `concept_ids` into immutable
`MathReference` copies, including versions, before the existing constructor
validation. A single declared row-operation table supplies file keys, operation
families and move-palette concept IDs. It replaces the former private
declaration; transformation, example projection and decoding use that table.

`LayeredQuestionSession::mathReference()` exposes only linked reference content.
`mathReferenceExample()` uses the existing exact row transformation to project
the authored example's before/after cells and per-column arithmetic. Neither
question-answer choices nor the player's working are used as example inputs.
The UI owns the open reference ID and a bounded column cursor, just as it owns
working inspection. These presentation actions create no attempt, completion,
hint or answer-reveal records. Correct result submissions still use the sole
`MathematicalMove` dispatcher; no reference-specific solver or progression
route is added.

The reference replaces inspection in the right support pane at wide sizes,
or occupies the existing lower support area at narrow sizes. The problem,
current working and answer tiles keep their rectangles. Its content scrolls
inside fixed Close/previous/next controls. Example stepping follows the game's
pause state, and Escape closes a reference before returning to Contents.
Question/run changes clear the reference; same-question return/resume preserves
it. CMake deploys the shared library with both native content consumers.

[P036](P036_SAVED_PRACTICE.md) adds device-local practice persistence to the
sorter. Finite `GallerySession` instances opt into the question owner's accepted
command journal. `LayeredQuestionSession::dispatch()` records only accepted
changes before publishing their evidence; unchanged help requests and rejected
or stale input do not enter the journal. Other gameplay modes do not allocate
this journal. It covers current and archived runs, including mathematical Undo.

`EquationSorterSession::studyProgress()` publishes stable card IDs, selected
titles, the exact random/specific draft, the frozen queue and its position,
plus each started question's ID/version and command journal.
`restoreStudyProgress()` stages a complete replacement and constructs every
saved finite game through the existing question checker. It publishes only if
all titles, IDs, versions, selections and journals validate. Startup returns to
contents with each game paused; Resume/Next keep their existing action routes.
The gallery regenerates scene bindings from checked collection facts, including
partly collected answer sets. It does not restore a physics clock or aim misses.

`StudyProgressFile` is the filesystem/JSON adapter. Its versioned format contains
inputs and a content stamp; no saved result, correctness flag or completion flag
is trusted. Reopening replays inputs through the same domain dispatch and exact
math kernel. The content stamp catches edited judging material even without a
version bump. Restoring an incompatible file leaves the live session and original
file untouched, pauses saving for that launch, and reports the problem. Writes
use a complete temporary file in the same directory followed by replacement.
Before writing, the adapter checks that the destination still matches its last
read version. The revision check excludes idle frame updates, and identical
snapshots do not rewrite the file.

The native sorter loads before creating its host and saves after applying frame
commands. It chooses SDL's user-data folder unless the user supplies a path.
Checks/scripts/bounded runs have no implicit personal save. The UI presents a
green save status, a blue Resume button and amber failure details; it owns no
persistence or recovery policy. Transient graph probes, reference-page cursors,
camera state and grouping history remain outside this practice capability.

[P037](P037_GROWING_MATRIX_PRACTICE.md) extends the catalogue with twelve generated
matrix problems while preserving P036 saves. The existing publisher composes
the new chapter with the default 100-card catalogue; its standalone catalogue
uses the same cards and shared row references. The live mathematical palette,
question judge and UI retain their existing owners.

Practice format 2 records the pool at the last selection edit and the exact
selected order. `EquationSorterSession` now owns an ordered selected-home vector;
the UI's selection mask is its read-only projection. Restoration maps saved
card IDs to current homes and validates All/Random counts against the saved pool.
The current title pool can grow while the saved draft and active queue remain
frozen. Deliberate selection edits refresh the pool; Start set freezes the chosen
order, and Resume/Next continue to use the existing queue.

`StudyProgressFile` checks teaching stamps by stable card identity for the saved
draft, queue and retained runs. Catalogue order and unrelated additions do not
affect compatibility. Version 1's full stamp supplies the old selection pool;
version 2 saves it explicitly so repeated reloads remain stable after growth.
No saved command is reinterpreted against changed teaching content. An
incompatible dependency identifies its card, preserves the file and live state,
and pauses saving for that launch. Existing atomic writes and external-change
checks remain the filesystem adapter's responsibility.

[P038](P038_CONTENTS_PROGRESS.md) adds current-attempt progress to Contents.
`LayeredQuestionSession::progress()` derives NotStarted/InProgress/Completed from
current completion, move events and prepared answer/help evidence. Navigation,
reference browsing and archived runs do not start a fresh attempt. Undo and
Replay therefore change the projection without deleting earlier evidence.
`EquationSorterSession::view()` projects these states per question and aggregates
completed/total across every type in each chapter, independently of selection.
The UI draws small circles/ticks and counts; it stores no progress policy.
Save restoration rebuilds the same evidence through the existing checker, so
these marks require no new persisted fields, revisions or migration.

## Modes and navigation

Use `PathsScreen { Title, Playing, Stats }` and retain the existing
`FirstMoveMode { GuidedQuestion, QuickHunt }` as the mode identity in P001.
The existing shared app dispatcher owns the screen and selected/resumable mode
in `FirstMoveUiState`, while each pure mode model continues to own its run.
Do not introduce a second active-mode variable in a new shell class.
Mode descriptors form a declarative table
with ID, title, description, and bindings. Title cards and script mode names
read that table; there is no growing chain of business-condition branches.

Title → OpenMode validates the descriptor and opens/resumes that mode. Playing
→ ReturnToTitle preserves both runs, selections, help state, and archives.
Title → Stats reads both sessions; Stats → Back returns to Title. A new run is
an explicit in-mode action. Enter on Title opens the focused mode, never starts
a new run behind the learner's back. F1 returns to Title from a mode. Escape
continues to do the existing local back/close action within each mode.

An open Hunt review may be paused by returning to Title. Its modal state must
remain intact. Until that review is closed or released, opening Guided is
rejected with `Finish or close the Quick Hunt explanation first.` Opening Hunt
resumes the review. This preserves the current model boundary while allowing
the title screen to remain reachable.

All pointer, keyboard, and scripted navigation uses the carried app semantic
dispatcher, extended with title/stats actions in the copied FirstMove UI
files. It either changes Paths navigation or forwards one model command.
Do not wrap it in a second dispatcher that independently checks mode policy. Ignore
state-dependent input queued for a previous screen/context. Neither drawing a
title card nor generating a report may advance a game.

## Truth, experiment rules, and statistics

An experiment changes the experience or interpretation of work. It must not
rewrite what the learner did. The authoritative facts include question and
content version, run identity, checked choices in order, first response,
retries, reveals, completion, and prior exposure.

P002's run configuration is a value copied at run start:

```text
RunConfig {
  modeId, modeVersion,
  contentPackId, contentPackVersion,
  objectiveId, objectiveVersion,
  scoreRuleId, scoreRuleVersion,
  parameters
}
```

Changing an experiment setting affects the next run. Resuming an unfinished
run retains its original configuration. A comparison score may be computed
from a finished run under another scoring rule, but it is labelled a
comparison and does not overwrite that run's original score/rule identity.

Start with three precisely named score rules in P002's design: `practice_none`
(no points), `hunt_bank_v1` (the existing 100/240/420/520/620/720 bank schedule),
and `guided_completion_v1` (one point per completed layer, equally whether
answered or shown). The last measures progress through practice, not
correctness. A different accuracy/retry incentive gets a new explicit rule.

Keep objective and score separate. `finish_question` completes after the final
Continue, regardless of assistance. `finish_hunt_pack` completes when each row
is cleared or reviewed/released. Neither equates to a threshold on points.

Statistics expose their denominator and scope. Guided counts distinguish first
try, after retry, and shown answers. Hunt counts distinguish checked rows,
initially correct rows, and explained/released rows. Never combine those two
units into a single accuracy percentage. Time measures and durable profiles
are future capabilities; do not invent a timer or persistence schema in P001.

## Content progression

P010 loads the five gallery examples from `content/cards/` through the pack at
`content/packs/gallery_foundation.json`. `QuestionContentIO` owns file reading,
JSON decoding, accepted-option-ID conversion and question-ID/version deck
resolution. It produces the existing `LayeredQuestionContent`, calls shared
structural validation, and reports errors with source paths and JSON pointers.
The [schema contract](QUESTION_CONTENT_FORMAT.md) versions the file shape
separately from each question's content.

P013 makes the question menu the default `gallery` startup.
`GalleryMenu` owns selection, navigation and the lifetime of started gallery
sessions. It offers three named bundled packs; available practice types come
from the validated pack decks. Its `launch()` is the shared preparation boundary
for the Play action and direct CLI gameplay startup. The former construction
block in `gallery_main.cpp` is removed. `paths_gallery_menu` depends on the pure
gallery model and content loader and can be tested without SDL/Vulkan.

Selecting a different set does not modify an existing game. Returning to the
menu pauses it; Play resumes the session for that pack/practice type or prepares
a new one. The last practice type is remembered per pack. Started sessions and
the workshop scene live until process exit; no saved profiles are introduced.
Load failures leave the menu usable and preserve existing sessions. Direct CLI
content errors still stop before graphics startup. An explicitly supplied pack
outside the bundled paths gets a Custom questions entry for later resume.

`gallery_main.cpp` draws the menu and forwards pointer, keyboard and script
actions. Navigation changes are applied between native frames so the host's
scene pointer remains valid and matches that frame's answer board. Gameplay
input still uses the existing GallerySession dispatch route. See
[P013_QUESTION_MENU.md](P013_QUESTION_MENU.md) for the controls and evidence.

P014 adds pending movement setup to `GalleryMenu`. `SelectMotion` and `SetPace`
use the same dispatcher from widgets and native scripts. `selectedConfig()`
returns the selected session's frozen config when it exists, otherwise the
next-game draft with the selected practice type. Editing a started selection
is rejected by the dispatcher as well as disabled in the UI. Visiting an old
game does not overwrite the draft for an unstarted set/type.

`validateGalleryConfig()` in the existing GallerySession module serves both
menu edits and game construction. It checks the practice type and gameplay
preset requirement, then delegates route validity to TargetMotion’s `prepareRoute()`
using the same compact route definition used to create targets. Numeric limits
remain in TargetMotion; custom waypoints remain in Workshop. The UI's slider
range is an editing convenience and does not narrow the existing CLI contract
of finite positive speeds through 100. Question content, judging, assignments,
progression, renderer and movement calculations are unchanged. See
[P014_MOVEMENT_SETUP.md](P014_MOVEMENT_SETUP.md).

P016 adds an explicit replacement draft to `GalleryMenu`. `NewGame` copies
the selected session's frozen config; `SelectMotion` and `SetPace` edit only
that draft. Selection/navigation and ordinary launch are rejected until
`CancelNewGame` or `StartNewGame`. Cancelling discards the draft and restores
the saved setup without changing the run or the ordinary next-game defaults.

The private `start()` boundary prepares initial games and replacements through
the same loader and GallerySession constructor. Replacement reloads the current
pack, resolves the selected deck and constructs the fresh session before
releasing the old one. Only that pack/practice slot is replaced. On success,
the draft becomes the next-game defaults and is cleared; on failure, both
the old paused session and editable draft remain. Run history is not archived
by the menu: the explicit Start new game control replaces it, as stated in
the UI. New-game input is applied between native frames, preserving the scene
pointer lifetime. See [P016_NEW_GAME.md](P016_NEW_GAME.md).

P017 adds a read-only answer-review projection to `LayeredQuestionSession`.
`review(runIndex)` selects the current run at index zero or a retained completed
run, then resolves its question ID/version against the session's frozen catalog.
It exposes only reached steps, recorded attempts in order, existing outcomes,
collection counts and the working shown before that step. Text views borrow
the catalog for the session's lifetime. It does not rejudge answers, expose
unsubmitted option lists or reveal later working states.

`GalleryMenu` owns the Review screen, selected run and expanded row through
`OpenReview`, `CloseReview`, `SelectReviewRun` and `ToggleReviewStep`. Opening
pauses the existing game and selects its current question. Closing preserves
that pause. Other navigation and launch actions cannot bypass the open review.
The startup draws the projection and routes widgets/scripts through those
actions between frames. It retains the scene for rendering while disabling
gameplay input and keeping motion/pop time frozen. The question session remains
the sole owner of attempts, progression and completed history. New game clears
history by replacing that one session through P016's existing route. See
[P017_ANSWER_REVIEW.md](P017_ANSWER_REVIEW.md).

P018 extends that projection with an explanation view. The question owner
returns the frozen step's existing `explanation` only when
`layeredQuestionStepResolved(record)` is true. The shared predicate already
covers player resolution and a Guided answer shown; partial AllAccepted sets
remain unresolved. No second completion rule is introduced. Text stays empty
for unfinished steps or empty authored explanations. The UI renders a nonempty
value below the attempts in an expanded row; it never looks up answer keys or
checks collection masks. The native report exposes `explanation_available`
from the same projection. Catalog, loader and schema remain unchanged. See
[P018_REVIEW_EXPLANATIONS.md](P018_REVIEW_EXPLANATIONS.md).

`GallerySession(config, catalog, resolvedDeck)` freezes the selected deck;
`LayeredQuestionSession` validates and freezes the catalog. Only the question
session judges answers, records attempts and advances prepared working states.
The runtime owners have no filesystem or JSON dependency. The former compiled
gallery catalog and fixed mode indices are removed. The original Guided
starter question remains compiled in its existing question owner.

Content files contain prepared steps; loading never derives a solution or
interprets display strings. General mathematical verification and automatic
content preparation remain separate capabilities.

P011 provides `content/cards/source_002_quadratic_three_points.json` and the
single-card `content/packs/source_002.json` deck for `equation_chain`. It maps
the retained 002 authoring artifact into 13 prepared decisions, with fixed
numeric identities and before/after state references. The original file and
source snapshot remain unchanged. See [the adaptation record](P011_CARD_002_ADAPTATION.md)
for the source hashes, identity map, answer-exposure review and native proof.
No runtime C++ or schema changes are needed for this content checkpoint.

P012 adds `content/cards/source_013_closest_point_line.json` and the separate
`content/packs/source_013.json` deck using the same startup. Its 14 prepared
decisions retain the supplied target formula, Euclidean conditions, residual
argument and uniqueness justification. The question owner still judges all
choices and keeps the attempts. Completion records guided derivation practice;
it does not establish independent proof writing. See
[the 013 adaptation record](P012_CARD_013_ADAPTATION.md) for its explicit ID
map, preserved working, mathematical argument and complete native run.

`content/authoring/002_guided.json` has 13 authored layers and
`content/authoring/013_guided.json` has 14. Their source pages, hashes, choices,
misconceptions, working lines, explanations, and mathematical checks are
already present. See the content architecture for exact future catalog and
evidence IDs. In that document, old FM003 means the source-card slice, now
P003; its source paths are relative to this standalone project when implemented.

P001 ships the carried starter question and Hunt pack. P003 adds the completed
002 adaptation to a multi-card Guided grid, with the 013 derivation following
there using the same variable-length catalog. Both cards already play as
separate gallery packs. Guided grid navigation remains its own capability.

## Milestones and acceptance

The migration's proof is a successful build and offscreen run of a copy of
`paths/` located outside the parent checkout. A build from the subdirectory
that silently reaches back into the parent is not isolated. Record build,
model/input tests, offscreen capture, interactive status, and user acceptance
separately. The user has requested a title screen to implement, not a visible
window to launch during this workstream.

The planner works in docs/content; Sol builds the scoped source changes and
pushes a completion brief. No repeated worker observation is needed. Each
brief advances the active packet in `WORKSTREAMS.md` and identifies the next
capability rather than growing an unbounded refactor.

## Native mathematical objects (P039)

`MathObjects` owns the mathematical parameters, derived primitive placements,
readouts, graph adjacency and route, and one challenge per object. Its semantic
dispatch validates changes before rebuilding a bounded, immutable snapshot.
It has no SDL, Vulkan, ImGui, content loading, or study-save dependency.

`MathObjectScene` tessellates the snapshot into the existing `SceneFrame` format:
position / colour / UV vertices and 16-bit indices, within the existing fixed
GPU capacities. Unit geometry is cached and frame buffers are reserved. Camera
motion uses the existing viewport-navigation kernel; both scene producers use
`publishSceneCamera` for the same Vulkan projection convention. The native host
continues to own buffers, shaders, synchronization and command recording.

`app/math_lab_main.cpp` is the live UI/CLI consumer. It forwards parameter, route
and Check actions to the model and camera input to the scene adapter. It does
not calculate mathematical answers, alter study attempts or write personal
saves. This object library and exploration startup are a separate capability
from scored question integration and persistent lessons.
