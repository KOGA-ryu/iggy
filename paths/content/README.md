# Question content

[`authoring/learning/claude_002`](authoring/learning/claude_002) is the reviewed
source-lesson pilot: a pinned review manifest, explicit notation map and reusable
textbook template. Follow the [source lesson workflow](../docs/SOURCE_LESSON_WORKFLOW.md)
to prepare and publish it. Raw source pages and learner attempts are kept outside
runtime documents; unselected and stale drafts are not imported.

`authoring/learning/matrix_foundations/` is the first complete
[publishable learning package](../docs/LEARNING_EXPORTS.md): `authoring.json`
supplies permanent package identity and source records; `documents/` holds a
matrix lesson, all four solving modes and a shared prose include. Run
`python3 tools/export_learning.py publish content/authoring/learning/matrix_foundations`
from the Paths root. The next normal sorter launch uses the published library.
Existing document/question content remains available with unchanged save stamps.

`write/*.paths.md` files are editable source for the
[learning-document pipeline](../docs/LEARNING_DOCUMENTS.md). The executable reads
them directly with `--documents content/write`, or loads the bundled copy when
no published library is active. Includes, ToC records, lesson/question links, templates and registered
diagram names are validated as one import. These source documents are separate
from generated JSON packs and the read-only historical source snapshots.
`write/matrix.paths.md` adds a four-level two-variable row-reduction question.
Its numeric choices become symbolic row-operation tiles automatically. Use
`@template matrix.v1` and the named operations in the authoring guide.

The [four-level authoring workflow](../docs/QUESTION_AUTHORING_WORKFLOW.md)
uses `authoring/question_layers_v1.json` and `tools/question_workflow.py` for a
bounded, checked linear-equation family. Explicit publication produces
`corpus/linear_support.json`: 25 questions using the existing question owner's
Learn, Practice, Solve and Write modes. Authoring evidence remains separate
under `build/question-format-evidence/`. The document pipeline can now feed that
same runtime format with `@template linear.v1`.

[Related Library notes](../docs/LINKED_CORPUS_NOTES.md) are explicitly connected
by `reading_links` in `authoring/matrix_corpus_review.json`. Use permanent entry
IDs; both ends must have reviewed adaptations. Navigation metadata is published
separately from reviewed mathematics and does not revise the Symbols lessons.

[Four reviewed matrix notes](../docs/RANK_CORPUS_REVIEW.md) are now shared between
Library and matrix Symbols. Edit their reviewed adaptations in
`authoring/matrix_corpus_review.json`; `generate_corpus_toc.py` publishes both
the Library metadata and `references/matrix_notation.json`. Preserve the original
source snapshot. Other corpus entries still await mathematical review.

The [corpus library](../docs/MATH_CORPUS_TOC.md) is available through the violet
Library control in sorter Contents. `corpus/toc.json` contains 930 source-reading
entries across six subjects, explicitly **not yet fact-checked**. Its permanent
IDs live in `authoring/math_corpus_map.json`; the original seven files remain in
`source_snapshots/math_terms_v1/`. Reproduce the catalogue with
`python3 tools/generate_corpus_toc.py`, or verify it with `--check`.
Library entries are separate from playable question cards and checked notation
lessons; original Markdown/LaTeX is preserved for reading and later adaptation.

To change what the gallery plays, edit a question in **cards/** and select it
through a pack in **packs/**. The menu uses the three bundled packs below.

| Location | Purpose | What to edit here |
| --- | --- | --- |
| [write/](write/) | Automatically discovered learning documents | ToC declarations, lessons, questions, shared includes and registered diagrams |
| [cards/](cards/) | Playable questions in the current runtime JSON format | Prompts, working steps, choices and accepted answer IDs |
| [references/](references/) | Shared versioned definitions, rules and example inputs | Reusable concept entries linked by ID from questions |
| [packs/](packs/) | Explicit question-file lists and ordered practice decks | Which questions a practice type uses, and their order |
| [sorter/](sorter/) | Independent 100-card packs for the Equation Sorter | Displayed equations, IDs, home slots, subjects and hints; separate from the gallery schema |
| [authoring/](authoring/) | Retained 002/013 authoring cards and compact bracket recipes | Original authoring formats remain separate; bracket recipes generate the prepared solving cards |
| [source_snapshots/](source_snapshots/) | Read-only copies of source pages and supporting material | Preserve these; their byte hashes establish provenance |
| [reference_cases/](reference_cases/) | Planning scripts and reference expectations | Specifications used by the planning checks; not gallery command scripts |

## Playable sets

- [gallery_foundation.json](packs/gallery_foundation.json): the five starter
  questions and four practice types, including the substitution integral.
- [source_002.json](packs/source_002.json): the 13-decision quadratic adaptation.
- [source_013.json](packs/source_013.json): the 14-decision closest-point derivation.

The pack lists question files explicitly. Files in `authoring/` are not
automatically discovered or loaded. The retained cards are
[002_guided.json](authoring/002_guided.json) and
[013_guided.json](authoring/013_guided.json). Their current playable counterparts
are [source_002_quadratic_three_points.json](cards/source_002_quadratic_three_points.json)
and [source_013_closest_point_line.json](cards/source_013_closest_point_line.json).
Their shared question identity describes the adaptation relationship; the
authoring shape is not interchangeable with the playable JSON shape.

## Editing and trying questions

Follow the [question-file contract](../docs/QUESTION_CONTENT_FORMAT.md) for
required fields, stable IDs, versions and limits. Question paths inside a pack
are relative to that pack's folder. To try edited source content directly,
launch from the Paths project root:

```sh
./b/gallery --start-mode equation_chain \
  --content-pack content/packs/source_002.json
```

Without an explicit content pack, the menu uses the copy beside the executable.
Building `gallery` refreshes that copy. Close and relaunch to read edits;
an existing game keeps its original question content when resumed.

## Equation Sorter packs

[equations_v1.json](sorter/equations_v1.json) is the original 100-card algebra
pack. [mixed_foundations_v1.json](sorter/mixed_foundations_v1.json) retains 80
of its equations and adds five examples each of trig, calculus, linear algebra
and discrete maths. Each file uses schema version 1 and exactly 100 records
containing `id`, `home_index` and `text`; see the
[sorter contract](../docs/P020_EQUATION_SORTER_SPEC.md). The
[assistance extension](../docs/P022_SORTER_ASSISTANCE.md) adds optional `subject`
and `hint` fields. Both bundled packs now supply them for every card. Grouping
remains the player's choice; a subject suggests a group without grading moves
or supplying a sequence of accepted answers.

[P023 prepared solving](../docs/P023_PREPARED_SOLVING.md) adds an optional
`solve_pack` reference, relative to the sorter JSON. In both bundled packs,
card 1012 links to [linear_bracket_pack.json](sorter/linear_bracket_pack.json),
which loads [sorter_linear_bracket.json](cards/sorter_linear_bracket.json).
The linked question must match the card and supply its own working states,
accepted choices, hints, next moves and final check. Grouping remains unscored.
Editing the linked question and relaunching with `--content` reads it directly;
building `sorter` refreshes the bundled copies.

The original algebra and mixed packs link `-2(x + 1) = 22`. The separate
[bracket_practice_v1.json](sorter/bracket_practice_v1.json) places six complete
solving questions in its first six slots, followed by 94 existing sorting cards.
Cards with a linked solution have a **Solve** label. Each opened question keeps
its own session until closing the app; switching cards preserves its progress.
A statement
such as `sin(30 deg) = 1/2` remains sorting content. A later solving question
needs a separate learner prompt such as `Find sin(30 deg)` and an answer held
in its prepared choices; statement text is not interpreted as a solving task.

The default [study_practice_v1.json](sorter/study_practice_v1.json) combines
those six bracket questions with four straight-line graph questions and four
simultaneous equations and thirteen matrix row-reduction questions. The remaining
73 slots keep sorting content. The graph
chapters are **Straight lines → Slope and intercept** and **Simultaneous
equations → Two straight lines**. Prepared graph geometry lives in the linked
question, alongside its four accepted decisions and reveal states.
The runtime never extracts slope or answers from the displayed equation.

The [linked value table](../docs/P031_LINKED_VALUES.md) needs no additional
authored fields. The question owner samples the existing graph parameters at
the low, middle and high points of the guide's visible x range. The UI displays
those samples and the current input alongside its numerical substitution.
Values are withheld until the guide is available; original equations, accepted
choices and content versions are unchanged.

The six bracket questions now opt into [P032 mathematical moves](../docs/P032_MATHEMATICAL_MOVES.md)
with `working_model: "linear_moves"`. In the finite sorter workspace, the shared
question owner checks operations and selected equations with exact arithmetic,
then records the player's own solution path. The prepared sequence remains for
prepared arcade consumers. Content version 5 and generator version 3 record
this change. Graph questions keep their existing geometry/answer contract.

The authored [sorter_matrix_rows.json](cards/sorter_matrix_rows.json) adds
**Linear algebra → Matrices and systems → Row reduction** with
`working_model: "matrix_rows"`. It contains the original augmented matrix and
an independently checked prepared row sequence; runtime visual moves are
calculated from the current matrix by the shared mathematical owner. This card
is the editing source for this one question, rather than a generated recipe.
The existing publisher reads it to produce [matrix_rows_pack.json](sorter/matrix_rows_pack.json)
and its entry in the default study pack. Increase the card's content version
when editing it, republish, and run the matrix/content checks.

The generated **Linear algebra → Matrix practice** chapter adds twelve cards,
IDs 6101–6112 at content version 1, under **Integer foundations**, **Swaps and
negatives**, and **Exact fractions**. The separately loadable
[matrix_practice_v1.json](sorter/matrix_practice_v1.json) has the same 100-card
contract as the default catalogue. Both retain all fifteen earlier playable
questions and use the existing row-operation reference library. See the
[result sheet](../docs/P037_MATRIX_PRACTICE_RESULTS.md) for recipes, answers,
reproduction commands and independently tested routes. Publishing the chapter
replaces only sorting filler slots; existing question files keep their bytes.

Version 2 of this question links `row_swap`, `row_scaling` and `row_addition`
through `concept_ids`. Its generated pack selects the shared
[row_operations.json](references/row_operations.json) library. Each definition
has a stable ID, content version, title, explanation, rule and a separate
example input. The same exact row kernel generates the example calculations;
no answer matrices are authored for this reference panel. See the
[reference format](../docs/QUESTION_CONTENT_FORMAT.md#shared-row-references-p035).

Edit a definition once to update every linked question on the next launch.
Increase that entry's content version when editing it. Loaded sessions keep
immutable copies of the definitions they started with. Building `sorter` or
`gallery` copies the reference library beside the executable. The first library
covers the three existing row-operation families; other mathematical concepts
and the user's parser remain later integration work.

### Generating bracket and graph questions

Edit [linear_bracket_recipes.json](authoring/linear_bracket_recipes.json), then
run these commands from the Paths root:

```sh
python3 tools/generate_sorter_fixture.py
python3 tools/generate_sorter_fixture.py --check
./b/sorter --content content/sorter/bracket_practice_v1.json
```

The generator expands `a(x + b) = c` using the explicit `divide_then_shift`
method. It accepts bounded integer inputs, uses exact fractions for results,
and generates four decisions with distinct choices and prepared help.
The [recipe contract](../docs/P024_BRACKET_RECIPES.md) lists the fields and
limits. Change generated bracket cards through their recipes and increment
`content_version` before changing an existing question. The generator refuses
same-version content changes and validates the complete batch before writing.

The original P023 card is now generated as version 2, using the same stable
question ID and pack path. The two original sorter files retain their exact
bytes. No conversion or Python process runs during gameplay; building `sorter`
copies the prepared files beside the executable.

[line_graph_recipes.json](authoring/line_graph_recipes.json) supplies the graph
examples to that same generator. Each recipe declares a stable ID, sorter ID
(4000–4999), content version, integer `rise` (−8–8), positive `run` (1–8), and
integer `intercept` (−8–8). Rise/run must be in lowest terms. It produces four
steps and five graph states: grid, intercept, run, rise, line. Same-version
changes are refused before publishing; all older question files remain intact.
See [the graph contract](../docs/P029_COORDINATE_BOARD.md).

[system_graph_recipes.json](authoring/system_graph_recipes.json) adds two-line
systems through the same publisher. Each recipe has a stable `sorter_system_`
ID, sorter ID 5000–5999, a content version, and `first` and `second` objects.
Each line uses the same bounded `rise`, `run` and `intercept` fields as a
single-line recipe, plus integer `scale` 1–4 for the printed equation; for
example, scale 2 can print `2y = 2x + 2` while the graph remains `y = x + 1`.
Fractions are exact during preparation. Four decisions reveal the first line,
both lines, the solution count, and the checked result. Axes contain both
intercepts and any unique intersection; out-of-range intersections are refused.
The paired original equations use printable ASCII with `1:` and `2:` labels.
The infinity answer label is drawn with native strokes in the UI.
See [the systems checkpoint](../docs/P030_SIMULTANEOUS_EQUATIONS.md).

```json
{
  "id": 2001,
  "home_index": 4,
  "text": "sin(30 deg) = 1/2",
  "subject": "trig",
  "hint": "Use the 30-60-90 triangle: opposite / hypotenuse = 1/2."
}
```

Supported subjects are `algebra`, `trig`, `calculus`, `linear_algebra` and
`discrete_maths`, mapped to A–E respectively. Dump is reserved for manual use.
Hints allow up to 240 printable ASCII bytes. Unknown subjects and invalid hint
values report their exact source field. Omitting a subject keeps an older v1
card usable for manual grouping, but Auto sort leaves it unsorted and explains
why. An omitted or empty hint has no extra authored explanation. The runtime
never guesses a subject from an equation's text or ID.

The mixed pack uses these plain-text conventions:

| Notation | Meaning |
| --- | --- |
| `deg`, `rad`, `pi` | Explicit degree/radian angle units and the constant pi |
| `^`, `*` | Exponentiation and multiplication |
| `d/dx (f)` | Derivative of `f` with respect to `x` |
| `integral[a,b] f dx` | Definite integral of `f` from lower bound `a` to upper bound `b` |
| `[a,b]`, `[[a,b],[c,d]]` | Two-component vector and a matrix whose inner brackets are rows |
| `dot`, `det` | Dot product and determinant |
| `{a,b}`, `union`, `intersect` | Set, set union and set intersection |
| `choose(n,k)` | Number of ways to choose `k` distinct items from `n`, without regard to order |
| `NOT`, `AND`, `OR` | Boolean operations on `true` and `false` |

To read an edited source pack directly, launch from the Paths project root:

```sh
./b/sorter --content content/sorter/mixed_foundations_v1.json
```

Relaunch to read content edits; no C++ rebuild is needed for text changes.
Building `sorter` refreshes the sorter packs and their linked question files
beside the executable. The default is now the bracket practice pack and its
table of contents; pass `--content` to choose an older algebra or mixed pack.

Prepared sorter records may include a `study` object with `chapter`, `type`
and `form` strings. Each must contain 1–80 printable ASCII bytes and have
non-space text; a subject is required. Within a subject and chapter, one type
title must have one form. For example, the generated bracket pack declares
`{"chapter":"Linear equations","type":"Bracket equations","form":"a(x + b) = c"}`.
These are authored classifications, not values inferred from the equation.
Only records with both this metadata and a validated solving question enter
the selectable catalogue. Older unclassified packs retain their grouping flow.

`paths_sorter_mixed_tests` checks the actual new card strings with bounded,
test-only maths interpretations and rejects deliberately wrong results. It is
a fixture checker, not a general symbolic solver: new notation needs a
corresponding independent check before it joins a verified pack. The production
loader validates structure and declared metadata, not mathematical meaning. See the
[mixed-pack checkpoint](../docs/P021_MIXED_MATH_PACK.md) for the exact checks
and native readability evidence.

## Authoring and provenance checks

[SOURCE_CARD_ARCHITECTURE.md](SOURCE_CARD_ARCHITECTURE.md) explains the retained
authoring format. Its `source.snapshot` fields are relative to this `content/`
root, even though the cards now live in `authoring/`.
[source_manifest.json](source_manifest.json) records the snapshot hashes.

From the Paths project root, run:

```sh
python3 content/verify_planning_artifacts.py
```

This checks the retained authoring cards, source hashes, specified exact
arithmetic and reference cases, and refreshes
[planning_validation.json](planning_validation.json). It does not run the
game, validate the entire source corpus or establish mathematical correctness
for newly added questions. The separate `paths_content_tests` build target
checks loading and the authored-to-playable mappings for cards 002 and 013.

The migration manifest's destination entries follow the new authoring paths;
its original source paths and byte hashes are preserved. No question content
or source snapshots were rewritten by this folder change.
