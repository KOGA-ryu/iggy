# P030: simultaneous equations on the coordinate board

Status: accepted by the user; automated checks passed; uncommitted.

The user confirmed that the visual check passed and authorized continuing
with the proposed linked value table. The original build evidence below is
retained as the automated checkpoint.

The user authorized the proposed simultaneous-equation set, then requested a
discussion of high-impact cross-section features after this build. This
checkpoint completes that set. No screenshots, image assets, visible native
windows, or delegated agents were used. The user performed the visual check.

## Delivered capability

The default study pack now contains 14 prepared questions: six bracket
equations, four single-line graphs, and four **Algebra → Simultaneous equations
→ Two straight lines** questions. The other 86 cards remain sorting content.

| Sorter ID | First equation | Second equation | Shared solutions |
| --- | --- | --- | --- |
| 5001 | y = 2x + 1 | y = -x + 4 | (1, 3) |
| 5002 | y = 2x | y = -2x + 3 | (3/4, 3/2) |
| 5003 | y = x + 1 | y = x - 1 | None; parallel with intercept difference 2 |
| 5004 | y = x + 1 | 2y = 2x + 2 | Every point on the shared line |

Four numerical decisions find the first slope, second slope, solution count,
and final check. The last check selects an intersection coordinate pair for a
unique solution or the intercept difference for parallel/coincident lines.
Solution-count options include a native drawn infinity symbol. Both labelled
original equations remain on the gold problem board for the entire question.

The first accepted answer draws a solid teal line; the second draws a dashed
violet line. A white vertical guide then compares two coloured coordinate
readouts at the same x. Drag on the plot, move the x slider, or use its existing
keyboard controls. The shared guide is available while deciding the solution
count and final check. The marked mint intersection appears only after the
final answer. A coincident solution receives a mint stroke along the entire
shared line; no point is invented for parallel or coincident cases.

Reveal motion lasts 0.28 seconds and never delays the next answer. Replay
repeats the latest motion without recording an attempt. Wrong answers and
Hint retain the active reveal stage. Show next move and Do this step use the
existing assistance/evidence route. Back to contents and Resume retain the
question and guide; Play again archives the completed attempt and starts a
blank grid. Finished questions remain until explicit Next, which follows the
selected queue and stops at its end.

## Canonical owner and scope

`LayeredQuestionSession` remains the canonical owner of graph parameters,
reveal stages, answer judging, help and immutable attempts. `LineGraph.second`
extends the existing model. A shared internal solution helper classifies two
lines and computes a unique intersection; both validation and the read-only
`coordinateGraph(x)` projection use it. The projection clips both lines to the
same axes and clamps the requested x to their common visible range. The UI
maps supplied points to pixels and does not solve equations or choose answers.

The existing single-line projection calculation is replaced by a helper used
for either line. `ChooseAnswer` remains the single submission route through
the gallery to the question owner. No conflicting judge, history store,
selection owner, window, or parallel UI action route is added. Existing bracket
and single-line questions retain their content and live routes.

The generator reads four bounded recipes from
`content/authoring/system_graph_recipes.json`. Exact fractions prepare choices,
explanations and solution checks. Its shared line formatter supports scaling
the printed equation, including the equivalent pair in question 5004. The
publisher preserves version checks and refuses malformed inputs before writing.
Both displayed equations use the sorter's printable-ASCII text contract.
New system cards and deck references are version 2.

## Verification

Production code: six existing files, +251/−46 lines (net +205), no new
production code files. Three existing test files change by +212/−3 (net +209).
Nine new JSON files comprise four questions, four packs and one recipe file.
Only the existing default study pack changes among prior runtime JSON files;
all prior question files, other packs and source references retain their bytes
against the turn baseline. No build-graph changes were required. This is a new
capability, not an ownership-cleanup workstream.

All three native Release targets (`sorter`, `gallery`, `paths`) build. Six
focused suites pass: sorter solving, sorter input, recipe arithmetic/publication,
question content, Guided Questions and gallery sessions. The recipe suite has
seven tests, including independent parsing of the printed equations, scaled
coefficients and every accepted/rejected mathematical option.

Model checks cover all four systems, graph reveal gates, shared x values,
line clipping, non-finite requests, wrong/stale/paused answers, hints, assisted
steps, immutable evidence, completion retention and fresh attempts. Invalid
second-line parameters, wrong reveal chains and out-of-bounds intersections
are rejected through the shared validator and JSON decoder.

The actual ImGui input harness runs all four questions at 1440×860, 800×600
and 360×480. It selects the new chapter, submits wrong and correct choices,
uses Hint and Replay, drags the guide, returns/resumes, advances with Next,
starts fresh, and resizes the finished workspace. Text-only measurements check
equation/control containment, finite draw vertices, equal axis scale, coloured
line reveal stages and the native infinity drawing. Existing single-line and
bracket input scenarios also pass. These checks do not establish visual or
human pointer acceptance.

Generator `--check` passes. The bundled default 100-card pack validates from
`/private/tmp` before graphics initialization. All 42 bundled runtime JSON
files match source. Scoped diffs, baseline/content hashes, build logs and a
verification receipt are in `build/simultaneous-equations-evidence/`.

## User visual check and next discussion

The user has passed this visual check. The supplied checklist is retained
below for reference.

Relaunch Sorter, select only **Simultaneous equations**, then Start set. Check
that both gold equations stay readable and fixed; teal and violet lines share
one movable guide. The first two problems finish with a mint point at (1, 3)
and (0.75, 1.50). The third stays parallel with unequal y-values; the fourth
overlaps with matching y-values and offers the infinity symbol. Resize once,
use Replay and confirm the completed question waits for Next.

The subsequent discussion recommended shared equation/graph/value tables,
parameter controls, revisiting earlier visual steps, and placing answers
directly. The user authorized continuing with the first recommendation;
P031 adds linked values to the existing graph chapters. Other candidates
remain future work.
