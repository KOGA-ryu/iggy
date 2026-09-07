# Paths cleanup review — 2026-09-07

Authority: the user requested a repository review and a decision about what to
clean next, then approved the selected cleanup with “gogo”. The initial review
made no production edits. The completed publication cleanup is recorded below.

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

**Classification:** Contract Risk for sustained play. **Disposition:** second,
separate cleanup after publication.

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
provide rollback of a partially written batch after an I/O failure. The next
candidate is the separate history-allocation finding above. No history storage,
Undo, save or runtime behaviour was changed in this workstream.
