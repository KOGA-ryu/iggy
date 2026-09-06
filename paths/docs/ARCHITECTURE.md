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
There is no nested Git initialization in this workstream.

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
  app/main.cpp                      CLI, script runner, lifecycle, report serialization
  src/runtime/first_move/           existing pure models
  src/ui/FirstMoveUi.*              shared app dispatcher, title/stats/mode presentation
  src/ui/LayeredQuestionUi.*        guided question view
  src/platform/NativeVulkanHost.*   standalone SDL/Vulkan/ImGui host
  vendor/iggy3d/src/                seven bounded allocator/capture/receipt helper files
  third_party/imgui/                exact pinned SDL3/Vulkan ImGui subset plus license
  content/                         reviewed authoring data and source snapshots
  tests/                           copied baseline plus migration-boundary tests
  docs/                            active packets and source provenance
```

`paths_model` contains pure C++ game models. `paths_ui`
depends on `paths_model` and privately on SDL/ImGui. `paths_native` contains
the host and copied helper implementation, with private SDK/ImGui dependencies.
`paths` composes those targets. `paths_imgui` is the local pinned dependency.
No target uses `../src`, `../apps`, `../build`, an absolute iggy3d source path,
or an absolute visualization path for a build input.

The ImGui Vulkan backend already embeds its default UI shaders. The current
math views do not use a 3D scene, so P001 needs a native UI render pass, not the
entire engine renderer and its document/room/shader closure. Future 3D views
can add a scene-rendering capability behind the native boundary when a mode
actually needs one. This plan preserves Vulkan; it does not claim to have
migrated the full 3D editor engine.

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

`content/cards/002_guided.json` has 13 authored layers and
`content/cards/013_guided.json` has 14. Their source pages, hashes, choices,
misconceptions, working lines, explanations, and mathematical checks are
already present. See the content architecture for exact future catalog and
evidence IDs. In that document, old FM003 means the source-card slice, now
P003; its source paths are relative to this standalone project when implemented.

P001 ships the carried starter question and Hunt pack. P003 adds the completed
002 adaptation to a multi-card Guided grid. The 013 derivation follows using
the same variable-length catalog. Keeping these content slices separate makes
the isolation/build result assessable before changing the entire curriculum.

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
