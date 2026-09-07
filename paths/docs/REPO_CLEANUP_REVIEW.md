# Paths cleanup review — 2026-09-07

Authority: the user requested a repository review and a decision about what to
clean next, then approved publication cleanup, history-growth repair and the
subsequent history investigations with “gogo”. The initial review made no
production edits. Completed workstreams are recorded below.

## Decision

The selected workstream was **question-publication assembly**. The review found a
confirmed output-identity collision and three copies of catalogue composition
that disagreed with the newer matrix assembler about capacity. Both belong to
the same boundary: combining generated cards, packs and catalogue entries before
publishing files. The cleanup stayed in the existing authoring files, with the
question model and saved-practice format retaining their current owners.

## Findings

These findings and source line numbers describe the code at the initial review.

### P2 — Generated question files can silently replace another family's output

**Classification:** Contract Risk. **Disposition:** resolved by publication cleanup.

The route is custom bracket recipes → `bracket_outputs()` → the successive
`generated.update(...)` calls in `tools/generate_sorter_fixture.py:421` → written
question/pack/catalogue JSON → `loadSorterContent()`. Recipe validation checks
identities within each batch, but publication does not check output ownership
across batches.

A valid bracket recipe using question ID `sorter_graph_positive`, its existing
version, a distinct card ID `3501` and pack `bracket_collision_probe` passes both
the bracket and graph generators. The graph batch overwrites the bracket's
generated card at the same path. The resulting catalogue displays
`-2(x + 1) = 22`, but its linked question contains `y = 2x + 1`.

The reproduction used an isolated generated tree. The real `sorter
--check-content` loader rejected it with “solution must be valid and match the
displayed equation” before native-host startup. The runtime check protects
gameplay, but publication can still produce an unusable catalogue.

**Replacement:** the publication assembler must reject duplicate card/pack
output identities before writing. Catalogue replacements should be explicit
assembly operations, rather than unrestricted dictionary replacement.

Evidence: `build/repo-cleanup-review-evidence/publication-collision-probe.json`
and `collision-loader-result.json`.

### P3 — Repeated catalogue assembly silently omits additions at capacity

**Classification:** Duplicate Implementation. **Disposition:** resolved by
publication cleanup.

The straight-line, system and authored-matrix assemblers in
`tools/generate_sorter_fixture.py:270`, `:367` and `:386` independently append
records, truncate to 100, check uniqueness and reassign homes. In contrast,
`tools/generate_matrix_practice.py:153` has an existing `assemble_catalogue()`
that explicitly refuses to discard playable questions.

With a synthetic full catalogue of 100 study records, the first three functions
return success while omitting all four new line questions, all four new system
questions and the new authored matrix question respectively. The matrix-practice
assembler refuses the same capacity condition. This is a growth-boundary defect;
the current bundled catalogue contains 27 playable questions and 73 fillers.

**Replacement:** generalize the existing assembler's retained-playable and
capacity checks and delete the three competing append/truncate/reindex blocks.
Keep intentional chapter replacement explicit. The 100-card sorting contract
can remain in this cleanup; removing that product limit is separate work.

Evidence: `build/repo-cleanup-review-evidence/catalogue-capacity-probe.json`.

### P2 — Every mathematical attempt grows event storage by one allocation

**Classification:** Contract Risk for sustained play. **Disposition:** resolved
by the separate history-growth cleanup.

`LayeredQuestionSession::applyMathMove()` reserves `size()+1` for events and
successful working nodes at `src/runtime/first_move/LayeredQuestionSession.cpp:699`.
This preserves allocation-before-publication ordering but defeats amortized
vector growth on the current standard library.

A pure-model probe against the existing Release libraries submitted the allowed
1,024 wrong attempts. Event storage reallocated 1,024 times and relocated 523,776
previous event records. All attempts remained recorded and the question stayed
unfinished. These are storage-growth measurements, not a frame-rate benchmark.

**Replacement:** give the existing append boundary bounded growth while keeping
all potentially failing allocations before publishing a node/event. Removing the
reserve calls without preserving that ordering is insufficient. Keep current
limits, Undo branches, journals and completion semantics.

Evidence: `build/repo-cleanup-review-evidence/history_capacity_probe.cpp` and
`history-capacity-probe.json`. The current build graph supplies
`paths_math_moves_tests` and `paths_practice_save_tests` for that later work.

## Scope of the selected cleanup

Use the two existing authoring generators and their existing recipe tests.
Preserve bundled question bytes, IDs, versions and catalogue order. Add focused
checks for cross-family output collisions and full-catalogue refusal before any
publication. Verify both generators with `--check` and the resulting catalogue
with the existing headless content-loader entry point. The current CMake graph
registers `paths_bracket_recipe_tests` and `paths_matrix_practice_recipe_tests`.
Aim to remove the repeated assembly blocks without adding production files.

## Other boundaries and review limits

The traced math submission, progress and save-replay routes already retain their
canonical model owners. Hint/Reveal availability does not expose a comparable
conflict: the UI adds pause/input guards to the model projection, and the question
owner still validates assistance. Repeated calls to the same content validator
protect distinct JSON and direct-construction entry points. The `paths` and
`gallery` applications remain live targets with separate consumers.

The review traced selection, solving, help, content loading, publication and
saved practice, and inspected their current build dependencies. It did not run
graphics, take screenshots/captures, or repeat the existing passing test suite.
Only the isolated diagnostics above were executed.

Scene/build work changed concurrently in `CMakeLists.txt`, `README.md`,
`docs/ARCHITECTURE.md`, `docs/WORKSTREAMS.md` and `src/scene/GalleryScene.*`.
Those edits were left alone. Hash checks confirm that all sources cited by the
findings remained unchanged during the review. `scope.json` in the evidence
directory records that boundary. Existing work remains uncommitted; visual
acceptance retains its earlier deferred status.

## Publication cleanup completed

`assemble_catalogue()` in `tools/generate_matrix_practice.py` now owns the
chapter capacity, retained-question and home-assignment rules for all chapter
appenders. A question with a solve pack remains playable even without study
metadata. Replacing an existing chapter is explicit and requires the same pack
identity. The three competing append/truncate/reindex blocks were deleted.

`publish_outputs()` in the same existing module owns publication path identity,
question-version checks and file replacement for both generators. It checks all
output paths against one another and against authored inputs before writing.
Intermediate catalogue snapshots are assembled explicitly; only the final study
catalogue is published. Unrestricted cross-family dictionary replacement,
duplicate JSON-field parsing and duplicate publication loops were removed.

Production scope: two existing authoring files, **62 lines added / 89 removed
(27 fewer lines)**, no new production files and no runtime C++ edits. Two existing
test files gained 48 lines covering cross-family collisions, authored-input
protection, capacity refusal, retained order and explicit chapter replacement.

Verification completed:

- The two targeted recipe CTest entries passed.
- Both generators passed `--check`; question bytes, IDs, versions and catalogue
  order remain unchanged.
- Release `sorter` and `gallery` builds passed in an isolated build directory.
- The rebuilt headless loader validated both 100-card catalogues from outside
  the repository. All 79 deployed JSON files match their unchanged sources.

The evidence is in `build/publication-cleanup-evidence/verification.json`, with
logs, source hashes and a baseline-relative patch alongside it. Concurrent edits
to `docs/P039_MATH_OBJECTS.md` and `docs/WORKSTREAMS.md` were preserved. Changes
remain uncommitted; no windows or captures were used and visual review remains
deferred.

Publication still replaces files individually, as before; this cleanup does not
provide rollback of a partially written batch after an I/O failure. At this
checkpoint the next candidate was the separate history-allocation finding above.
No history storage, Undo, save or runtime behaviour changed in publication cleanup.

## History-growth cleanup completed

The live route remains symbolic answer/Undo controls → `MathematicalMove` →
`GallerySession::dispatch()` → `LayeredQuestionSession::dispatch()` →
`applyMathMove()`. The question session remains the owner of checked working,
attempts and their journal. Saved practice replays that journal through the same
owner; storage capacities are not part of the save format.

The two exact-one-element reserve requests in `applyMathMove()` now round up to
the next power of two with the already available `std::bit_ceil`, capped at the
existing 1,024-event and 128-node limits. Both reserves still precede either
append, so a reserve failure cannot publish half a checked move. No helper,
header, second route or production file was added. Production change is
**3 lines added / 3 removed (net zero)** in one existing file, including its
comment. The old exact-growth policy is gone.

A fresh Release baseline and the revised build produced these measurements:

| Measurement | Before | After |
| --- | ---: | ---: |
| Event reallocations over 1,024 wrong attempts | 1,024 | 11 |
| Existing event records relocated | 523,776 | 1,023 |
| Node reallocations while reaching 128 working nodes | 127 | 7 |
| Existing working nodes relocated | 8,128 | 127 |

The existing long-session test now guards allocation and relocation growth,
retained counts, journal preservation and capacity limits. The four focused
CTest entries passed: `paths_math_moves_tests`, `paths_practice_save_tests`,
`paths_practice_save_write` and `paths_practice_save_read`. They cover correct
and wrong moves, Undo branches, completion, limits and saved practice. A mixed
practice save generated before the source change is byte-identical to the same
scenario generated afterwards; the revised executable also resumed, completed
and reopened a copy of that earlier save.

Release `sorter`, `gallery` and `paths` builds passed in the isolated verification
directory and were rebuilt in the usual `b/` directory. Evidence, before/after
measurements, saved files, logs and a baseline-relative patch are under
`build/history-growth-evidence/`, with the result in `verification.json`.

The tradeoff is spare allocated slots between growth points, bounded by the
existing limits. This establishes reduced storage churn, not a measured frame
rate improvement. No other production source, content or saved-data schema was
edited. Changes remain uncommitted. No windows, screenshots or captures were
used; earlier visual acceptance remains deferred.

At this checkpoint the next candidate was a focused check of whether idle frames
repeatedly rebuild history summaries. That investigation and its resulting
cleanup are recorded below.

## History-totals cleanup completed

**Finding:** P3, Duplicate Implementation. Each solver frame calls
`GallerySession::view()`, which recounted every prepared attempt and mathematical
Submit event in the current and archived runs. Prepared runs already maintain
wrong-attempt counts, while mathematical working nodes already record every
correct move. A headless Release probe with 16 runs, 4,128 attempts and 16 Undo
events confirmed repeated totals reads without any new input.

The question session retains ownership of checked facts. `MathMoveRun` now has
the same `incorrectCheckedAttempts` count as prepared steps; `applyMathMove()`
increments it only after recording a wrong Submit event. Correct moves and Undo
leave it unchanged. New runs initialize it to zero, archived runs copy it, and
saved practice reconstructs it through the existing journal replay. The save
format contains no new field.

`GallerySession::view()` now adds these per-run/per-step facts: mathematical
correct moves are the working-node count minus the original node; prepared
correct answers are the attempt count minus wrong attempts. Both repeated
attempt scans were deleted. Mathematical `QuestionReview` reads the same
owner-maintained wrong count, deleting its separate event scan. No view cache,
revision key or invalidation route was introduced.

Production scope is **8 lines added / 8 removed (net zero)** across three
existing files, with no new production files. Two existing test files gained
29 lines comparing the projected totals with independent recounts of raw
verdicts through ordinary moves, partial collection, Undo, replay and archives.

Verification completed:

- Five targeted CTest entries passed: gallery, mathematical moves, practice
  persistence, separate-process save writing and separate-process save reading.
- The pre-change and revised executables produce byte-identical mixed-practice
  saves. The revised executable resumed, completed and reopened the earlier save.
- Release `sorter`, `gallery` and `paths` were rebuilt in the usual `b/` directory.
- The probe returned identical totals, journal length and checksums. Across five
  trials of 20,000 reads, median `GallerySession::view()` time fell from **3.5486
  microseconds to 0.022225 microseconds** on this host. This measures the totals
  view in an isolated synthetic history, not whole-frame or rendering speed.

Evidence is in `build/history-totals-evidence/verification.json`, alongside the
probe, measurements, saved files, logs, hashes and baseline-relative patch.

The tradeoff is eight extra bytes per mathematical run on this build. Totals
still visit retained runs and prepared steps; individual attempt records are no
longer revisited for those totals. Full first-try/retry classification and
expanded history drawing retain their existing on-demand traversal. Content,
input, UI layout and save rules are unchanged. Changes remain uncommitted; no
windows, screenshots or captures were used and prior visual acceptance remains
deferred.

At this checkpoint the next candidate was a focused check of long-history
scrolling and expanded step details, recorded below.

## History-drawing cleanup completed

The existing `drawMathSolving()` UI adapter emitted connector and checkmark
geometry for every retained node, including strokes wholly outside the history
panel. ImGui's renderer would clip that geometry later. The panel's existing
clip rectangle is now checked before those raw draw-list calls, using
`ImGui::IsRectVisible()` and bounds that include the strokes' antialiasing margin.
Connectors crossing the visible area still draw even if their endpoints belong
to offscreen rows.

This removes unconditional tessellation of invisible decorations. Row layout,
labels, IDs, hit rectangles, branch colours, expanded details and automatic
following remain on their existing routes. No row virtualization, cache or
additional model state was introduced. Production change is **3 lines added /
3 removed (net zero)** in one existing UI file, with no new production files.

The existing input test now builds long scalar and matrix histories with 127
nodes, retained Undo branches and retries at a branch point. It checks the top,
middle, bottom and partially clipped edges at **1440×860, 800×600 and 360×480**;
selects an earlier node with the actual pointer; scrolls with wheel input; and
checks following a new node and using the actual Undo button at the 128-node
limit. The existing ImGui tree state is opened directly to check expanded
attempt layout at wide and narrow sizes. Inspection preserves all run evidence
and the fixed workspace rectangles. The test file changes by +67/-2 lines.

A numeric probe reuses those checks and compares **54 states** before and after.
It hashes triangle vertices, colours, texture coordinates and clip rectangles
for every triangle whose bounds intersect its clip rectangle, independently of
draw-buffer indices. Those hashes, visible-triangle counts, presented control
rectangles and solving evidence all match. Total emitted vertices across the
states fell from **259,700 to 104,092** (about 60%); indices fell from **525,948
to 214,572**. Every sampled state emits less geometry. These are draw-data
measurements, not a frame-rate benchmark or a pixel-based visual review.

The targeted `paths_sorter_input_tests` CTest entry passed, including its
existing input, focus, references, saved Resume and explicit Next checks. The
affected Release `sorter` target was rebuilt in `b/`. The current build graph
does not link this UI library into `gallery` or `paths`, so those applications
did not require another build.

Evidence is in `build/history-drawing-evidence/verification.json`, with the
numeric probe, comparisons, logs, hashes and baseline-relative patch alongside
it. Source content and mathematical, persistence and input-routing owners remain
unchanged. No windows, screenshots, captures or image files were produced;
earlier visual acceptance remains deferred. Changes remain uncommitted.

The remaining cost is laying out rows and expanded text to establish their
scrolling positions. The next candidate is keyboard navigation through long
histories; no additional implementation is included in this checkpoint.
