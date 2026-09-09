# Representative starting questions

The Library now has **278 original starting questions**: a separate question
for each of its six subjects, 178 chapters, and 94 named subcategories. The
user selected representative starting difficulty. The first question in each
category introduces a central idea with a small, explicit example.

| Subject | Subject questions | Chapters | Subcategories | Total | Notation exercised |
| --- | ---: | ---: | ---: | ---: | --- |
| Algebra | 1 | 29 | 20 | 50 | Transformations, factors, sets, quotients, maps, homology |
| Trigonometry | 1 | 29 | 14 | 44 | Angles, ratios, identities, harmonics, kernels |
| Calculus | 1 | 30 | 18 | 49 | Limits, derivatives, integrals, variations, differential equations |
| Linear Algebra | 1 | 30 | 16 | 47 | Vectors, matrices, row operations, projections, decompositions |
| Discrete Math | 1 | 30 | 16 | 47 | Logic, sets, counting, graph notation, probability bounds |
| Probability and Statistics | 1 | 30 | 10 | 41 | Distributions, inference, transitions, estimation, stochastic models |

The [complete coverage sheet](CORPUS_STARTER_COVERAGE.md) lists every assignment,
task and result. Coverage is anchored to the published topic IDs and explicit
subcategory IDs in `content/authoring/corpus_starters/subcategories.json`. The
generator checks those subcategories against the pinned corpus heading paths.
Repeated Round/Part drafting labels do not become duplicate categories. The
930 individual term/theorem/example/proof entries remain reading material;
this pass does not create a separate exercise for every entry. The independent
`math_lab` textbook and its seven-part outline remain the other workstream.

## Play and formatting

In `sorter`, open **Contents → Library → Questions**. Subject and chapter
filters apply to this collection; search matches question titles, prompts and
IDs. The list is replaced by a compact chooser in narrow windows.

Each question has two decisions: **Setup → Result**. Tiles contain typeset
mathematics. The gold task and givens stay above the controls throughout both
decisions. Cyan working and symbolic choices share the workspace; green marks
completion. A correct choice advances one step immediately. A wrong choice
records the attempt and keeps the working. The final working and explanation
remain until **Next**. **Again** archives the completed run before restarting.
**Definitions** returns to the reader without losing the question.

NativeMath renders the notation through the existing ImGui/Vulkan route, at
16 px for tiles and 18 px for givens, with 13 px UI text. Authored separators
between givens can become multiple display lines. Wide expressions scroll
horizontally; tall content and history scroll within their own areas. This is
the common subject-format starting point. Bespoke graphs, diagrams, draggable
objects and longer proof construction are not implied by this coverage pass.

## Canonical owners and persistence

`CorpusPractice` loads bounded, ID-linked question metadata, selects independent
sessions and persists their journals. `QuestionContentIO` validates the same
question format used by existing play. `LayeredQuestionSession` remains the
sole owner of answer judgment, transitions, attempts and archived runs.
`CorpusPracticeUi` presents those facts. No additional mathematical checker
or alternative answer-policy route was added to production.

The collection does not consume the sorter's fixed 100 slots or increase the
64-question session catalogue limit: each independent session freezes one
question. Existing sorter cards and their save format are unchanged.

Normal play automatically saves starters to `corpus-starters-v1.json` beside
the existing practice save. Explicit `--no-progress`, scripts and bounded runs
keep the pre-existing no-implicit-save behavior. Stable IDs and exact question
stamps permit catalogue reordering and additions. On incompatible mathematics,
malformed commands or outside edits, the original file is retained and the
message names the problem. Replay through the question owner reconstructs
evidence; saved files do not dictate correctness flags. Atomic replacement uses
an exclusively created sibling temporary file for each write. Abandoned `.tmp`
files are left intact and are never promoted into learner evidence. They do not
prevent later saves, including the first successful save.

`CorpusPractice` owns write recovery. A persistent `.lock` sibling uses the OS
file lock on macOS/Linux while checking and replacing progress. Closing its
descriptor, including on process exit, releases the lock; the lock file stays
in place so app instances cannot lock different replacement files. Writers check
the original save bytes while holding the lock and again before replacement.
Another window's committed work is retained; automatic retries never merge or
adopt its evidence. Symbolic save and lock paths are rejected.

Transient write errors retain the latest in-memory working, show the actual
failure and retry automatically after two seconds. Closing the app makes one
immediate attempt, bypassing that delay. Invalid or incompatible loaded saves
remain protected even on close. If a failure remains visible, keep the window
open until saving succeeds; abandoned temporary bytes are not a recovered draft.
The save payload and v1/v2 replay formats are unchanged.

## Authoring and mathematical evidence

The six `.txt` files in `content/authoring/corpus_starters/` are explicit authored
rows: ID, task, givens, setup, setup distractor, answer, two answer distractors,
explanation and an independent calculation probe. They are developer-owned
content. The Python checker executes those probes as test code; the game never
evaluates them. The published JSON stores ordinary prepared question records.

The 276 calculation probes include exact rational arithmetic, polynomial
identities and derivatives, integration of polynomials, matrix operations,
enumeration, and explicit numerical tolerances for transcendental examples.
Numerical probes do not prove limit or convergence theorems. Two category-theory
questions apply stated rules; these are author-reviewed rule applications,
not numerical proofs. All prepared routes are also exercised through the
canonical runtime, including every wrong-answer branch.

The source corpus is not treated as a verified mathematical authority. These
questions are original examples, with prerequisites and conventions supplied
where necessary. This does not promote the original 930 reading entries to
fact-checked status. Primary references used to check the governing conventions:

- [Stacks: functors](https://stacks.math.columbia.edu/tag/001B),
  [quotients of triangulated categories](https://stacks.math.columbia.edu/tag/05RA),
  [long exact cohomology](https://stacks.math.columbia.edu/tag/0117), and
  [Koszul complexes](https://stacks.math.columbia.edu/tag/0621): algebraic rules
  in the small quotient, composition and homology examples.
- [NIST DLMF, trigonometric identities](https://dlmf.nist.gov/4.21) and
  [Fourier series](https://dlmf.nist.gov/1.8): angle identities, coefficient
  normalization, orthogonality and midpoint convergence at a jump.
- [MIT, multivariable calculus notes](https://math.mit.edu/~poonen/notes02.pdf)
  and [distributions and Sobolev spaces](https://math.mit.edu/~dyatlov/18.155-F21/):
  gradients, Jacobians, weak derivatives and the stated Sobolev norm convention.
- [MIT, Linear Algebra](https://ocw.mit.edu/courses/18-06-linear-algebra-spring-2010/)
  and [Cornell, eigenvalue conditioning](https://www.cs.cornell.edu/courses/cs6210/2022fa/lec/2022-10-18.pdf):
  matrix conventions and the supplied Bauer-Fike bound.
- [MIT, Mathematics for Computer Science](https://ocw.mit.edu/courses/6-042j-mathematics-for-computer-science-spring-2015/)
  and [CMU, deviation bounds](https://www.stat.cmu.edu/~cshalizi/sml/21/lectures/05/lecture-05.html):
  counting, logic and the concentration-bound examples.
- [MIT, Introduction to Probability and Statistics](https://ocw.mit.edu/courses/18-05-introduction-to-probability-and-statistics-spring-2022/):
  probability, expectation and inference conventions. Advanced model questions
  state the update formula they ask the learner to apply.

## Verification and visual review

The Release sorter build and all four targeted CTest entries passed. The route
gate completed all 278 questions and checked all 834 wrong tiles. Native checks
typeset all 2,502 equation/working/tile instances without fallback and exercised
real pointer events for all six subject starters at each of three sizes. The
generator reproduces both the runtime bank and coverage sheet. All 342 protected
pre-existing content/model/typesetter files retain their initial hashes, and
all 71 deployed content files match their sources. The existing sorter input
regression passed. Concurrent textbook changes were preserved.

Production C++ changed by **+356 lines**, including **three new files** and 28
lines of existing wiring. The reconstructed pre-edit wiring matches the initial
hashes, so this delta excludes concurrent work. This is a new question capability;
no conflicting answer route was introduced or removed. The canonical question
owner is unchanged. Build files, authoring, generated data, tests and docs are
excluded from this C++ count.

Evidence is in `build/corpus-starters-evidence/`. Targeted gates cover publication
reproduction, arithmetic probes, all solving routes, saved history and failure
preservation, native notation, actual pointer input and the existing sorter
input boundary. Tested viewports: 1440×860, 800×600 and 360×480. Headless ImGui
frames and font atlases stay in memory. No windows, screenshots, captures or
image outputs are used. Changes remain uncommitted.

User visual acceptance of the previous Library typesetting is recorded by the
user's “heyyy it looks great” response. Visual acceptance of these new questions
remains pending. Try one question in each subject: the **gold** question should
stay put, **cyan** symbols should fit or scroll clearly, and the **green** result
should wait for **blue Next**. Close and reopen once to check saved progress.
The next subject-format candidate is a user-selected chapter from this bank.
