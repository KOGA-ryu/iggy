# P024: reusable prepared bracket equations

Status: implemented; targeted automated checks passed; uncommitted.
On 2026-09-07 the user confirmed testing the whole loop and that all buttons
do what they should. Formatting and sequencing remain the next design work;
this does not establish acceptance of their final presentation.

The user authorized making the P023 interaction reusable for a small family of
equations `a(x + b) = c`. Six prepared recipes cover positive and negative
factors, addition and subtraction, a zero answer and exact fractions. Each
uses the explicit division-first method and the same four mathematical decisions.

The authoring generator owns expansion and mathematical validation. The JSON
loaders admit the generated cards; `LayeredQuestionSession` remains the sole
runtime judge and progression owner. `EquationSorterSession` retains the
individual solving sessions so selecting another example cannot erase progress.

This is an authorized feature addition. The existing authoring tool is extended;
no production C++ files are added. The independent practice pack retains the
sorter's 100-slot contract, placing the six prepared examples first and filling
the remaining slots with existing sorting content. The original algebra and
mixed sorter packs retain their equations and slots. Their existing P023 link
receives version 2 of that question from the shared recipe generator.

## Play

From the Paths root:

```sh
cmake --build b --target sorter -j4
./b/sorter --content content/sorter/bracket_practice_v1.json
```

The first six cards have an **amber/gold** background, border and **Solve**
label. The inspected card has a brighter, thicker gold border. Inspect one, then use its Solve
button. Other cards remain sortable. Back to groups and Escape pause the
selected equation; Resume restores that card's own progress. Replay resets
only that question and archives its prior run. State lasts until app close.

| Sorter ID | Equation | Answer |
| --- | --- | --- |
| 1012 | `-2(x + 1) = 22` | `-12` |
| 3001 | `3(x + 2) = 21` | `5` |
| 3002 | `4(x - 3) = 8` | `5` |
| 3003 | `5(x + 3) = 15` | `0` |
| 3004 | `4(x + 1) = 10` | `3/2` |
| 3005 | `-6(x + 2) = 9` | `-7/2` |

The new sorter IDs avoid the mixed pack's subject-card IDs. Shared IDs retain
the same displayed equation across all three packs. The division-first prompts
acknowledge expansion as valid mathematics without claiming to support that
alternative branch.

## Recipe contract

`content/authoring/linear_bracket_recipes.json` is the authoring source. It
declares `schema_version: 1`, `method: "divide_then_shift"` and 1–16 recipes.
Each recipe supplies these exact fields:

| Field | Contract |
| --- | --- |
| `id` | Stable question name; lowercase letters, digits and underscores; starts with `sorter_`; at most 64 characters |
| `content_version` | Positive 32-bit revision; increment before changing an existing prepared question |
| `sorter_id` | Positive 32-bit card identity; unique in the batch; cannot redefine an existing algebra card |
| `pack` | Stable pack filename stem starting with `bracket_`; `bracket_practice_v1` is reserved for the sorter pack |
| `coefficient` | Integer `a`, with `2 <= abs(a) <= 12` |
| `offset` | Integer `b`, with `1 <= abs(b) <= 12` |
| `rhs` | Integer `c`, with `abs(c) <= 144`; zero is supported |

The existing `sorter_linear_bracket` / `linear_bracket_pack` / 1012 identity is
retained as the original linked question. Numeric booleans/floats, unknown or
duplicate fields, unsupported methods, duplicate IDs/equations, unsafe names
and out-of-range inputs are refused with the source file and field. Input is
limited to 64 KiB. Factors 0 and ±1 and offsets 0 are intentionally excluded:
they need different or shorter operation sequences.

```sh
python3 tools/generate_sorter_fixture.py
python3 tools/generate_sorter_fixture.py --check
```

The existing tool expands every recipe into four decisions and five working
states, with highlights, optional hints, next moves and a final substitution
check. `Fraction` computes exact reduced results. Distractors are deduplicated
by numerical value; each arithmetic step has four distinct choices and one
accepted result. Output depends only on the prepared inputs and generator
version; recipe ordering does not change an individual question's content.

Generated cards record the method, generator version and canonical recipe
hash in preparation metadata. Existing question-version changes are checked
across the whole batch before any output is written. Each file is then replaced
atomically; a filesystem failure during publication can still require rerunning
generation. `--check` compares all generated files without writing. It does not
constitute human acceptance.

This is a bounded typed recipe expander. Arbitrary expression/prose parsing,
automatic method discovery, expansion-first branches, new maths subjects and
persistent save/load are outside this capability.

## Verification

The current CMake graph selected `paths_sorter_solve_tests`,
`paths_sorter_content_tests`, `paths_bracket_recipe_tests`,
`paths_sorter_input_tests` and the native `sorter` target. The model checks use
`build/sorter-model` with `PATHS_BUILD_NATIVE=OFF`; native builds use `b`.
All four targeted suites passed across their relevant final changes:

- Independent parsing of rendered equations and exact rational answer checks
  for the six examples and 96 generated cases spanning signs, zero and numeric bounds.
  Accepted and rejected choices, deduplication, deterministic regeneration,
  stable cross-pack identities and refusal before publication are covered.
- All six loaded questions complete through the existing operation and actual
  projected-sphere commands. Interleaved partial questions retain their owners,
  attempts, help and paused state. Replay affects only its selected equation;
  the original Auto sort remains undoable after completing the family.
- Actual ImGui input checks cover the retained solver at 1440×900, 800×600 and
  360×480, and multi-card selection/resume at 800×600 and 360×480. Queued help
  from another card is discarded even when local challenge IDs coincide.
- The original 100 algebra equations retain their independent mathematical
  checks. All three bundled sorter packs load successfully from outside the
  project folder, without entering graphics startup. Generation `--check` passes.

The repeatable native scenario is `tests/equation_sorter_bracket.script`.
Current text evidence and hashes are recorded under `build/bracket-recipe-evidence/final/`.
No visible window was opened by this task. At the user's direction, visual
confirmation is performed by the user after coding, building and automated
checks finish; no further screenshot capture is permitted.

Against the saved starting state, three existing production C++ files change
by **+17/-11 lines (net +6)**. The existing authoring tool changes by
**+174/-3 lines (net +171)**. No production code files are added. Twelve
content files are added: one recipe source, five additional question files and
six packs. Two existing C++ tests and one new Python test contribute
**+251/-2 lines (net +249)**; one native scenario and this checkpoint are added.
The earlier build-name changes and all unrelated files retain their saved state.

## Delivered user visual checklist

The completed-build brief supplied these inspection points:

- The first six cards stand out in amber/gold. Their Solve labels and equations
  are readable; selecting one makes its gold border brighter and thicker.
- `4(x + 1) = 10` flows through divide by 4, `5/2`, subtract 1 and `3/2`,
  with immediate working updates and a readable final substitution check.
- Hint / Show next move / Do this step remain reachable; switching cards and
  returning offers Resume at the previous decision.

The user's subsequent playtest confirms the loop and controls. Their next
direction is subject/chapter/question selection followed by a persistent
solving workspace. The problem should stay easy to locate, and the methods
and games should occupy one 3D activity area without switching screens.
The sequencing discussion in `WORKSTREAMS.md` supersedes the earlier candidate
of immediately adding another equation family or subject.

The colour follow-up uses existing prepared-content and inspection state in
`EquationSorterUi`. It adds no card-specific IDs, mathematical policy or new
production files. Build and existing input checks are required; the user
confirms colour appearance. `sorter` rebuilt and `paths_sorter_input_tests`
passed after the colour change. This follow-up changes one existing C++ file
by **+15/-5 lines (net +10)**. Changes remain uncommitted. No screenshots were
taken for this follow-up; text evidence is under `build/bracket-recipe-evidence/colour/`.
