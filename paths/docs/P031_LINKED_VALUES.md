# P031: linked numerical substitution, graph and value table

Status: Automated Green; user visual confirmation pending; uncommitted.

After passing P030's visual check, the user authorized continuing with the
recommended linked equation/graph/value table. This is one shared capability
applied to both existing graph chapters. No screenshots, visible native windows,
image assets or delegated agents were used.

## Delivered capability

All eight prepared graph questions now have three compact sample rows beside
the plot and an amber row displaying the current input. Single-line tables
show x and y; simultaneous-equation tables show x, y1 and y2. Sample inputs
span the low, middle and high points of the visible guide range, keeping all
listed points inside the same graph bounds.

Click a sample, or Tab to its button and press Enter, to move the existing
guide to that exact input. Moving the slider or dragging the plot updates the
amber live row instead. Numerical substitutions above the plot show the same
input in amber, with the first result in teal and the second in violet. For a
scaled original equation, the substitution uses its normalized slope and
intercept, matching the plotted y-value. All displayed decimals are rounded
to two places; `~` marks approximation. Calculations retain their full precision.

The table and substitutions become available through the existing guide gate:
after the completed line in Straight lines, or once both lines are visible in
Simultaneous equations. Unreached rows remain empty and disabled. The original
problem and graded working retain their fixed positions; the compact table
and graph share one activity area throughout the question.

Exploration never submits an answer, changes help evidence, or advances a
question. Replay retains the selected x. Returning and resuming the same
question preserves it; Play again resets x and clears the numerical reveal.
Focus loss pauses the run and blocks sample input. Completion still waits
for explicit Next.

## Canonical owner and scope

`LayeredQuestionSession::coordinateGraph(x)` remains the graph-maths owner. Its
existing point evaluator supplies the new `GraphValueRow` samples as well as
the probe points. The optional table follows `probeAvailable` and is absent
before exploration is permitted. No JSON fields, authored equations, content
versions or accepted answers change.

`EquationSorterUi` owns the table's layout, number formatting, native strokes
and transient requested x. Sample buttons, plot dragging and the existing
slider update that same value before one refreshed projection is drawn. This
also removes the prior slider-to-drawing delay of one frame. The former
floating probe labels are replaced by the stable table and substitutions;
no competing graph evaluator, answer route, history or selection state is
introduced. Equal axis units and the existing outer workspace rectangles
are retained.

Production code: four existing files, +97/−29 lines (net +68); no new
production code files. Two existing test files change by +87/−1 (net +86).
No new content files or image assets are added. This is a new capability,
not an ownership-cleanup workstream.

## Verification

All three native Release targets (`sorter`, `gallery`, `paths`) build. The
current graph maps this change to `paths_sorter_solve_tests` and
`paths_sorter_input_tests`; both focused suites pass. No broader department
or CTest gate was run.

Model checks cover all eight questions, all reveal stages, sample ordering,
finite bounds, both line equations, stable sample inputs, exact correspondence
between selected rows and plotted points, and non-finite/out-of-range requested
x values. Existing answer, assistance, completion and replay tests also pass.

The actual ImGui controls exercise all eight questions at 1440×860, 800×600
and 360×480. Checks cover disabled unreached rows, all sample buttons, Tab and
Enter, same-frame slider/plot updates, Replay, return/resume, focus loss,
fresh attempts, equation/table/readout containment and separation of the plot
from the table. Existing wrong/correct answers, help, live resizing and explicit
Next scenarios pass in the same input suite. No graphics capture is involved.

The default bundled pack validates from `/private/tmp` before graphics
initialization. All 42 JSON files in the bundled cards, packs and sorter
directories match source. Every prior JSON and source reference is unchanged
against the turn baseline. Evidence is recorded in
`build/linked-values-evidence/`, including baseline hashes, scoped changes,
build/test logs and the verification receipt.

## User visual check

Relaunch Sorter and try Straight lines and Simultaneous equations. Once the
guide unlocks, look for the amber bottom row and matching amber x in the
substitution above the graph. Click a sample row, then move the x slider:
the teal point/value, and violet point/value for systems, should move together.
Resize once and check that the table stays beside the plot, choices remain
readable and the gold original problem stays fixed.

Next candidate: bounded parameter controls for exploring how coefficients
change a graph. This remains a separate scope decision.
