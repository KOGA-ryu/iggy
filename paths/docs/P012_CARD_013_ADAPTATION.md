# P012: The closest point on a line

Status: automated checks passed; native offscreen boards reviewed; uncommitted.

Card 013 now runs as a separate 14-decision Equation Chain through the P010
loader and existing gallery executable. The player identifies the parameters,
represents the line, checks projection conditions, assembles the supplied
point, and justifies closest and unique in Euclidean distance.

```sh
./build/gallery-port/paths_gallery --start-mode equation_chain \
  --content-pack content/packs/source_013.json
```

Add `--seed 19 --motion stationary` to match the verified layout. The normal
gallery build also deploys this pack beside the executable.

## Files and provenance

- Prepared question: [source_013_closest_point_line.json](../content/cards/source_013_closest_point_line.json).
- Separate deck: [source_013.json](../content/packs/source_013.json), selecting
  the existing `equation_chain` mode. The foundation and 002 packs are unchanged.
- Retained authoring source: [013_guided.json](../content/authoring/013_guided.json).
  SHA-256: `2b9b020d1c202703d9666ff53fd26d399716209320d9cc26df0dde5c01f1e10d`.
- Pinned source page: [013_closest_point_on_a_line.md](../content/source_snapshots/math/013_closest_point_on_a_line.md),
  Meckes & Meckes, chapter 4, exercise 4.3.9. Its hash matches the authoring
  artifact: `a341f24ee344a9974a6424990befba34aa56bb9cf50fec6df303df04cebac873`.

The prepared card retains question ID `rb_math_013_closest_point_line_guided`
and content version **1**, its first prepared runtime release. The original
authoring shape and P010 runtime shape both use schema version 1, but are
distinct formats. Only the prepared file is named in the new pack.

All authored prompts, layer labels, options, accepted choices, recovery text,
explanations, description and before-workspaces are preserved. The persistent
problem display states real parameters, the line through the origin, the given
point, Euclidean distance, uniqueness and the target formula. It labels the
exercise **Guided derivation - target supplied** and gives the book locator.
Manual line breaks fit the existing board without a UI change.

Complete source wording, symbols, distractor notes and verification scope stay
in the original authoring artifact. Neither that file nor the source snapshot
is changed. No general authoring converter, new metadata schema or mathematical
parser is introduced.

## Explicit identity map

Step-local option identities map as `o1 → 101`, `o2 → 108`, `o3 → 115`,
`o4 → 122`. These and the step/state IDs are assigned identities, independent
of future array positions. Preserve them when reordering presentation; later
semantic revisions require a new content version.

| Authored step | Runtime step | Before → after state | Purpose | Accepted option |
| --- | --- | --- | --- | --- |
| `identify_deliverable` | 10 | 100 → 200 | `answer_choice` | 122 |
| `identify_parameters` | 20 | 200 → 300 | `answer_choice` | 108 |
| `introduce_unknown` | 30 | 300 → 400 | `answer_choice` | 101 |
| `parameterize_line` | 40 | 400 → 500 | `operation_choice` | 115 |
| `projection_conditions` | 50 | 500 → 600 | `verification` | 122 |
| `nonzero_direction` | 60 | 600 → 700 | `verification` | 108 |
| `choose_projection` | 70 | 700 → 800 | `operation_choice` | 101 |
| `compute_numerator` | 80 | 800 → 900 | `calculation` | 122 |
| `compute_denominator` | 90 | 900 → 1000 | `calculation` | 115 |
| `assemble_point` | 100 | 1000 → 1100 | `calculation` | 108 |
| `perpendicular_residual` | 110 | 1100 → 1200 | `verification` | 101 |
| `distance_decomposition` | 120 | 1200 → 1300 | `verification` | 122 |
| `unique_minimum` | 130 | 1300 → 1400 | `verification` | 108 |
| `numerical_check_extent` | 140 | 1400 → 1500 | `verification` | 115 |

The two `operation_choice` steps select a representation and a projection
method. Purpose describes the decision; it does not determine correctness or
completion. All steps have four options, one accepted ID and `any_accepted`.
The card passes Guided validation as well as its live ArcadeCollect consumer.
The gallery keeps its existing immediate feedback; the retained explanation
and recovery strings do not introduce a new feedback interface.

## Working and what completion establishes

States 100–1400 copy the authored `workspace` lines exactly, joined by newlines.
A wrong target preserves the current working and assignment. Correct selection
resolves that decision while its ball pops; the existing Continue boundary
publishes the next working with the new targets. State 1500 holds the ordered
`solution[].work` lines. The question session exposes it at completion before
the gallery's normal endless restart opens state 100 in a fresh run.

The source problem prints the desired coordinates. Displaying them throughout
is intentional. The exercise asks why they work, including conditions and the
general distance comparison. Supplying the projection coefficient again when
assembling coordinates, or the positive distance gap when explaining uniqueness,
also matches the authored decision. No additional answer or explanation is
appended to an earlier working block.

Completed choices are evidence of guided derivation practice. They do not
establish independent discovery of the formula or independent proof writing.
The last decision explicitly distinguishes finite numerical checks from the
general justification. Archived question/version/step/option IDs and attempts
remain owned by the existing question session.

## Mathematical justification and checks

For every real `m`, let `v=(1,m)`, `p=(a,b)`, `d=1+m^2`, and
`t=(a+m*b)/d`. The vector `v` is nonzero because its first coordinate is 1,
and `d>0`, including at zero and negative slopes. Then `q=t*v` lies on the
line and the residual `r=p-t*v` satisfies `r dot v=a+m*b-t*d=0`.

For arbitrary real `s`, expand the squared Euclidean distance:

```text
p-s*v = r + (t-s)*v
||p-s*v||^2 = ||r||^2 + 2*(t-s)*(r dot v) + (t-s)^2*(v dot v)
           = ||p-t*v||^2 + (s-t)^2*(1+m^2).
```

The extra term is nonnegative and vanishes exactly when `s=t`. Distinct `s`
give distinct points because `v` is nonzero, establishing the unique closest
point for all stated real parameters. This argument does not divide by `m`
or `p dot v`. Vertical lines, affine offsets and non-Euclidean distances are
outside this particular exercise.

An independent exact-rational calculation checked the formula, on-line point,
perpendicular residual, stationary squared distance and positive second
derivative for the 12 instances in the prior planning receipt. These include
zero and negative slopes, the origin, and points already on the line. Five
comparison positions per instance checked the direct squared-distance gap,
including equality at `s=t`: **60 comparisons passed**. Those finite examples
support regression checking; the general argument above supplies the proof.

## Runtime verification and scope

The affected targets were derived from the current CMake graph:

```sh
cmake --build build/question-content --target paths_content_tests paths_gallery_tests -j 4
ctest --test-dir build/question-content -R '^paths_(content|gallery)_tests$' --output-on-failure
```

Both passed with `PATHS_BUILD_NATIVE=OFF`. The tests share the existing 002
adaptation checks while retaining explicit expectations for each card. They
compare every authored decision, answer and workspace, the stable identity
map, completion compatibility and the final solution state. Gallery tests
shoot the presented meshes through all 14 wrong/correct pairs and verify
retry/pop working visibility, stale-shot rejection, archive contents and a
fresh endless run. Existing 002 and foundation regressions also pass.

`paths_gallery` built and deployed the content. Its executable SHA-256 is
unchanged from before P012:
`08b6af5378b03fc6c6f808d7c9468d59b1f2b2b8a3b6853d9d4a5399ced25133`.
Launched from another working directory, it loaded the new pack and completed
a 75-frame offscreen script: **14 correct, 14 wrong, zero misses, one completed
question**, followed by a ready endless restart. All 15 target assignments and
both run records preserve the expected identities; the new run has no inherited
attempts or collected answers.

Evidence is in `build/card-013-evidence/`: `complete.script`, `complete.json`,
`verification.json`, `mathematics.json`, the binary hash and per-step scripts.
Captures at steps 1, 5, 7, 11, 12 and 14 were inspected at 1440×900. Prompts,
long formulas, current working, all four options and source attribution fit.
MoltenVK used the established unsandboxed offscreen route. No visible window
was opened; human pointer and interactive swapchain acceptance remain separate.

Changed files are the prepared card and pack, this record, the two existing
tests (`question_content_tests.cpp`, `gallery_session_tests.cpp`), `README.md`,
`docs/ARCHITECTURE.md`, `docs/QUESTION_CONTENT_FORMAT.md`, `docs/WORKSTREAMS.md`
and `content/SOURCE_CARD_ARCHITECTURE.md`. Production C++ delta is **+0/-0 lines,
zero files added or changed**. There is no new or conflicting gameplay route.
The loader, schema, build wiring and source artifacts are unchanged. Work is
uncommitted; no contract decisions remain unresolved for this checkpoint.

The next candidate is a gallery pack picker so these question sets can be
selected without launch arguments. P003's Guided grid, scoring/timing and
saved profiles remain separate capabilities.
