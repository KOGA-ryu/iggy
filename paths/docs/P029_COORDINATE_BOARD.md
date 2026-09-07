# P029: coordinate board and slope triangle

Status: accepted by the user; automated checks passed; uncommitted.

The user reports that the feature works throughout and believes they tested
all four graph questions. This records their interactive acceptance; the
automated verification receipt below remains the original build checkpoint.

The user authorized the proposed coordinate board with slope triangle. They
require completing code, builds and automated checks without screenshots, then
giving them a short visual checklist. This checkpoint used no screenshots,
captures, image assets, visible native windows, or delegated agents.

## Delivered capability

Sorter now bundles `content/sorter/study_practice_v1.json`: six existing bracket
questions plus four new **Algebra → Straight lines → Slope and intercept**
questions. Chapter selection, all/random/specific choices and the frozen study
queue remain with the existing selection owner.

| Sorter ID | Equation | Rise / run | Intercept | Second point |
| --- | --- | --- | --- | --- |
| 4001 | y = 2x + 1 | 2 / 1 | 1 | (1, 3) |
| 4002 | y = -x + 2 | -1 / 1 | 2 | (1, 1) |
| 4003 | y = (1/2)x - 1 | 1 / 2 | -1 | (2, 0) |
| 4004 | y = 1 | 0 / 1 | 1 | (1, 1) |

Four numerical decisions reveal the intercept, horizontal run, vertical rise,
and full line. Wrong answers and requested hints preserve the visible graph
stage. Do this step uses the existing assisted-answer route. A zero rise stays
horizontal. The original equation stays on a gold board; accepted geometry is
mint, and the slope triangle and movable point are teal. Compact numeric choices
remain beneath the same drawing throughout solving.

Graph reveals animate for 0.28 seconds without blocking input. Replay repeats
the latest movement without adding attempts. Completion retains the graph
until explicit Next; Play again archives the previous attempt and starts a
blank grid. A completed line can be explored by dragging on the plot or using
the x slider; its coordinates and dashed axis guides update. Tab, Space and
arrows adjust the slider; Enter supports typed values. Leaving focus pauses
the workspace and blocks input. Back to contents and Resume retain the current
question, graph stage and probe position.

The graph consumes the available activity height with compact equation,
working and prompt rows. The two axes use equal-sized units. Layout and control
bounds are checked at 1440×860, 800×600 and 360×480, including live resizing.

## Canonical owners and scope

`LayeredQuestionSession` owns validated line parameters, reveal stages, answer
judging, assistance and immutable attempts. Its read-only `coordinateGraph(x)`
calculates line endpoints clipped to the authored axes and the probe point.
It rejects zero runs, invalid bounds and broken reveal chains before gameplay.
The UI maps these coordinates into pixels; it does not infer slope, equations
or accepted answers from text.

The existing gallery button action is generalized from `ChooseOperation` to
`ChooseAnswer`. Both operation and graph choices reach the same question
submission route. The obsolete action name is removed from live code; no new
judge, history store or parallel submission route is added. Existing sphere
questions keep their live picking and camera route.

`EquationSorterUi` owns native line/circle/triangle/text drawing, fitting,
animation progress and the requested probe position. Exploring that point is
presentation, not an attempt. The native sorter omits the sphere render
snapshot while a graph occupies the activity area. This is the first flat
coordinate board in the existing solving workspace; curves, draggable answers,
and full 3D plots are future capabilities.

The existing authoring generator expands four bounded recipes from
`content/authoring/line_graph_recipes.json`. It prepares exact rational slopes,
distinct choices, explanations, graph states and the combined study pack. It
refuses changed questions without a version increase before writing. Every
pre-existing question, pack and source reference remains byte-identical to the
turn baseline.

## Verification

Production code: 10 existing files, +327/−43 lines (net +284), with no new
production code files. Three existing test files change by +196/−4 (net +192).
The build adds two test-fixture definitions. Ten new JSON files comprise four
questions, their four packs, one combined study pack, and one authoring recipe
file. This is a new capability, not an ownership-cleanup workstream.

All three native Release targets (`sorter`, `gallery`, `paths`) built. Six
focused suites passed: sorter solving, sorter input, recipe publication and
arithmetic, question content, Guided Questions, and gallery sessions. The
recipe suite contains five tests, including independent parsing of each graph's
printed equation and checking every accepted/rejected numerical option.

The actual ImGui input harness verifies chapter selection, wrong/correct
clicks, hints, assisted steps, animation replay, plot dragging, keyboard probe
control, focus loss, completion retention, explicit Next, fresh attempts,
return/resume and resizing. It checks finite drawing vertices, equal axis
scale and graph/control containment using text-only measurements. The shared
model tests verify clipped points satisfy their line equation and that probe
movement never changes attempts or completion.

The generator `--check` passed. The bundled default content validates from
`/private/tmp` before graphics initialization; bundled runtime JSON matches
source. Evidence, baseline hashes, scoped diffs and the verification receipt
are in `build/coordinate-board-evidence/`.

## User visual check

The user has confirmed the feature works. The checklist supplied for that
review is retained below for reference.

Relaunch Sorter. Untick Linear equations and tick Straight lines, then Start
set. Check that the gold problem stays fixed, the teal run/rise triangle grows
in place, and the mint line slopes up, down, gently up, then horizontally across
the four questions. After completion, move the teal point and use Replay;
Next should wait for you. Resize once and check graph labels and choices.

Next candidate: reuse this coordinate board for two lines and their
intersection. It has not been implemented in this checkpoint.
