# P011: Three points, one formula

Status: automated checks passed; native offscreen boards reviewed; uncommitted.

Card 002 now runs as a complete 13-decision Equation Chain through the P010
loader and existing gallery executable. The player identifies coefficient and
input roles, builds the equations, justifies elimination, recovers the function,
and distinguishes checking observations from uniqueness within this model.

```sh
./build/gallery-port/paths_gallery --start-mode equation_chain \
  --content-pack content/packs/source_002.json
```

Add `--seed 19 --motion stationary` to match the verified stationary layout.
The existing build target also deploys the pack beside the executable.

## Files and provenance

- Prepared question: [source_002_quadratic_three_points.json](../content/cards/source_002_quadratic_three_points.json).
- Separate single-question deck: [source_002.json](../content/packs/source_002.json),
  using the existing `equation_chain` mode. The foundation pack is unchanged.
- Retained authoring source: [002_guided.json](../content/authoring/002_guided.json).
  SHA-256: `609315864f4badae17cb85a3a3670b067768fcfc2e520763d172dd8385e2ad81`.
- Pinned source page: [002_quadratic_through_three_points.md](../content/source_snapshots/math/002_quadratic_through_three_points.md),
  Meckes & Meckes, chapter 1, exercise 1.1.7. Its SHA-256 matches the authoring
  artifact: `8db455115202bf5cabc83b4053376670244037b73c839542d91dd91ea99f59ea`.

The runtime retains question ID `rb_math_002_quadratic_three_points_guided` and
content version **1**. This is its first prepared runtime release. Both files
use schema version 1 in their respective formats; their shapes differ. The
P010 loader consumes only the prepared file named in the pack. No general
converter for the older authoring shape or new metadata schema is introduced.

All prompts, layer labels, choices, accepted choices, explanations, recovery
text and before-workspaces are preserved. The description is unchanged. The
persistent equation display adds the title and visible source attribution,
and condenses the original problem without changing its polynomial, points or
requested work. Its manually broken lines fit the existing answer board.
Symbols, distractor notes, complete source wording and verification scope stay
in the original authoring artifact. Neither it nor the source page is modified.

## Explicit identity map

These are assigned identities, independent of future array positions. Step-local
option IDs map as `o1 → 101`, `o2 → 108`, `o3 → 115`, `o4 → 122`. Preserve IDs
when reordering choices; later semantic revisions require a new content version.

| Authored step | Runtime step | Before → after state | Purpose | Accepted option |
| --- | --- | --- | --- | --- |
| `identify_unknowns` | 10 | 100 → 200 | `answer_choice` | 108 |
| `identify_input` | 20 | 200 → 300 | `answer_choice` | 115 |
| `linear_in_coefficients` | 30 | 300 → 400 | `answer_choice` | 122 |
| `implicit_coefficient` | 40 | 400 → 500 | `answer_choice` | 101 |
| `substitute_zero` | 50 | 500 → 600 | `calculation` | 115 |
| `substitute_minus_one` | 60 | 600 → 700 | `calculation` | 108 |
| `substitute_one` | 70 | 700 → 800 | `calculation` | 122 |
| `license_elimination` | 80 | 800 → 900 | `verification` | 101 |
| `solve_a` | 90 | 900 → 1000 | `calculation` | 115 |
| `solve_b` | 100 | 1000 → 1100 | `calculation` | 108 |
| `state_function` | 110 | 1100 → 1200 | `calculation` | 122 |
| `check_extent` | 120 | 1200 → 1300 | `verification` | 101 |
| `uniqueness_condition` | 130 | 1300 → 1400 | `verification` | 115 |

Each step has four choices, one accepted ID and `any_accepted` completion.
The card also passes Guided structural validation. Its live P011 consumer is
the ArcadeCollect gallery, with existing immediate feedback. The explanation
and recovery strings remain content; this checkpoint adds no feedback UI.

## Working and answer exposure

States 100–1300 copy each authored `workspace` joined by newlines. They contain
no appended answer or explanation. A wrong choice retains that block and
assignment. A correct choice resolves the decision; the existing pop/Continue
boundary publishes the next block with its targets. The transitions preserve
the author's context changes between vocabulary and calculation decisions.

The critical calculation sequence is:

| Decision being answered | Working before answering | Working after advancing |
| --- | --- | --- |
| Explain elimination | `c = 0`, `a - b = 1`, `a + b = 2` | `2a = 3` |
| Solve for a | `2a = 3` | `3/2 + b = 2` |
| Solve for b | `3/2 + b = 2` | Known coefficients and original model |
| State f | Known coefficients and original model | Three evaluated observations |
| State the check's scope | Evaluated observations | Coefficient matrix and supplied determinant |
| Explain uniqueness | Matrix and determinant `-2` | Completed solution state 1400 |

The evaluated observations and determinant are intentionally supplied for the
last two interpretation decisions, matching the authored prompts. Those steps
do not ask the player to calculate the displayed values independently. Earlier
substitution blocks similarly guide simplification by supplying the input.

State 1400 retains the ordered `solution[].work` lines. The question owner
exposes it on completion; the gallery immediately starts the next endless run
at state 100. Completed attempts and their identities remain archived.

## Verification

The affected targets were derived from the current CMake graph:

```sh
cmake --build build/question-content --target paths_content_tests paths_gallery_tests -j 4
ctest --test-dir build/question-content -R '^paths_(content|gallery)_tests$' --output-on-failure
```

Both passed with `PATHS_BUILD_NATIVE=OFF`. Content checks compare all authored
decisions, choices, working blocks and accepted identities through the explicit
map, validate the purpose/transition rules, and check the final solution state.
Gallery checks shoot real presented meshes through all 13 decisions with a
wrong/correct pair per step. They verify retry/pop working visibility, stale
shots, archived evidence and clean endless restart. Existing foundation
regressions in those targets also pass.

An independent exact-rational solve confirmed `(a,b,c) = (3/2,1/2,0)`,
determinant `-2`, and outputs `(1,0,2)` at inputs `(-1,0,1)`, matching the prior
planning receipt. This checks this card and adds no mathematical solver.

`paths_gallery` built with content deployment only. Its executable SHA-256 is
unchanged from before P011:
`08b6af5378b03fc6c6f808d7c9468d59b1f2b2b8a3b6853d9d4a5399ced25133`.
From another working directory, it loaded the new pack and completed a
70-frame offscreen script: **13 correct, 13 wrong, zero misses, one completed
question**, followed by a ready endless restart. Archived IDs match the card.

Evidence is under `build/card-002-evidence/`: `complete.script`, `complete.json`,
`complete.png`, `verification.json`, `mathematics.json` and per-step scripts.
Native captures for steps 1, 3, 8, 11 and 13 were inspected at 1440×900; all four
choices, prompts, current working and attribution fit the board. MoltenVK used
the established unsandboxed offscreen route. No visible window was opened;
human pointer and interactive swapchain acceptance remain separate.

Production C++ LOC and file-count changes are **zero**. P011 adds one prepared
card, one pack and this record; it extends two existing tests and updates
navigation/workstream docs. P003's multi-card Guided grid, the 013 adaptation,
scoring/timing and saved profiles remain separate capabilities. No contract
decisions remain unresolved. Changes are uncommitted and unrelated files are
preserved.
