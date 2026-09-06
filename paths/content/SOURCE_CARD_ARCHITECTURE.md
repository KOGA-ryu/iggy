# First Move — source-card architecture

Status: detailed plan and authored content, not an implemented importer.
FM002 remains the current implementation slice. FM003 adds source card 002
after that checkpoint; card 013 is prepared for the following content slice.

## 1. Concrete product path

The player opens the question grid, chooses a problem, and stays inside that
one problem while naming symbols, choosing a representation, performing the
work, and explaining why the result is valid. Every layer uses the existing
Try Again / Show Me loop. Completion describes the work and help used.

The first three cards, in this order, are:

| Grid title | Source | Purpose | Delivery |
| --- | --- | --- | --- |
| `3a + 5 = 20` | original FM002 drill | learn the interaction and basic equation vocabulary | current slice |
| `Three points, one formula` | math page 002, Meckes & Meckes 1.1.7 | distinguish function inputs from unknown coefficients; build and solve a system | FM003 |
| `The closest point on a line` | math page 013, Meckes & Meckes 4.3.9 | parameters, projection, a derivation, and uniqueness | following content slice |

Use the exact authored layers in `cards/002_guided.json` and
`cards/013_guided.json`. The JSON is a reviewable authoring artifact. It is not
a new runtime file format or a request to load Markdown in the game.

## 2. Source identity and what counts as evidence

Source root: `/Users/kogaryu/devil/99-red-booleans/problems/`.

The user reports that answers on 001–040 were computed and verified, including
ten corrections; 041 onward had not caught up at the time of the message.
Keep that report as provenance. Do not turn a numeric cutoff into a perpetual
verification rule as the writing pipeline continues.

In the inspected local files, 002 and 013 have `setup = "not started"`,
`solve = "not attempted"`, and empty Run output. Their referenced `work/002.py`
and `work/013.py` are absent. Current `pp.py status` reports learner setup/solve
fields, not a compute-stage verification ledger. This does not contradict the
user's separate verification history, but that history is not reproducible
from those fields alone.

`source_manifest.json` records byte hashes and snapshots of the two selected
pages and supporting format documents. It separately records:

- the user's verification report;
- the observed local learner fields and presence/absence of run receipts;
- the source locator and exact source bytes used by this plan;
- the planner's independent checks of the two adapted mathematical answers.

The planner's checks establish only the claims listed in
`planning_validation.json`. They do not validate the other 46 completed pages,
the active writing pipeline, or the engine implementation. The theorem-level
claim in 013 rests on the derivation in section 8, not finite numerical tests.

No source page is edited. In particular, `Attempt`, `opened`, `setup`, `solve`,
and `What I got wrong` remain the learner's/source app's data. Do not run
`pp.py check` as part of ingestion: it writes source pages and runs arbitrary
per-card programs. Use read-only `status` when needed and attach explicit
verification receipts to a pinned revision. Ignore any `*_tmp.md`, not just
the two names that were temporary in the user's message.

## 3. One source page becomes one authored learning sequence

| Source material | Game use | Required authoring decision |
| --- | --- | --- |
| Question + locator | retained problem statement and source attribution | keep all subparts, domains, givens, and requested output |
| Notation | role and vocabulary questions; pronunciation text | distinguish unknown, parameter, function input, supplied value, index |
| Method 1 | structural classification | avoid giving away a later requested answer |
| Method 2 | method choice and conditions | name the exact goal so another valid method is not marked wrong |
| Method 3 | representation and first working line | stop the prompt before the calculation it asks the player to make |
| Setup key | reviewed answer and explanation for setup layers | preserve accepted equivalent reasoning in the authored option set |
| Solution | ordered mechanics and solution lines | one mathematical decision per layer |
| Which line licenses this? | reason/justification layers | ask why a specific step is valid |
| Check + Run | verification layer plus author evidence | explain both what the check establishes and what it cannot establish |
| Variant | future separately reviewed question | new identity; never a text substitution into an old answer key |

All displayed game questions are labelled `Guided adaptation` with the original
book/chapter/exercise locator. Retain the complete source Question separately
in the authored card data. The authoring JSON does not claim that its added
multiple-choice prompts are verbatim book exercises.

A proof whose answer is printed in its statement stays a derivation exercise.
It is legitimate to show the target result throughout. The report must then
describe selected derivation steps, not claim the learner independently
produced or discovered the formula. The supported objective is
`guided_derivation`, not free-response proof assessment.

## 4. Authoring contract, schema version 1

`cards/*.json` has the following shape. IDs are identity; visible letters and
positions are presentation. The order is fixed in this revision.

| Field | Type and rule |
| --- | --- |
| schema_version | integer 1 |
| question_id | stable ASCII string; distinct from source page ID |
| content_version | positive integer, immutable after release |
| source | page ID, relative snapshot path, SHA-256, book/chapter/exercise |
| adaptation | `guided_multiple_choice` or `guided_derivation` |
| title, description | neutral grid copy that does not answer the opening layer |
| source_question_verbatim | complete Question section from the pinned page |
| display_problem | authored plain-text problem with all required givens |
| symbols | symbol, spoken form, local role, meaning |
| steps | nonempty ordered list, at most 32 in the first catalog implementation |
| step.id | stable within the question |
| step.kind | vocabulary, role, structure, condition, setup, method, mechanics, result, justification, or verification |
| step.prompt | one decision, with its local conditions stated |
| step.workspace | authored lines visible before answering; no hidden next answer |
| step.options | exactly four `{id, text, misconception}` entries |
| step.correct_option_id | exactly one of those IDs |
| step.recovery_text | the same neutral Try Again / Show Me copy as FM002 |
| step.explanation | shown only after correct Check or Show Me |
| solution | ordered `{work, reason}` lines on completion |
| verification_scope | claims established, limitations, independent check recipe |

Option IDs `o1`..`o4` are stable *within a step*, not global and not player
letters. The record identity is `(question_id, content_version, step.id,
option.id)`. Letter A is the first displayed option in this revision. Do not
shuffle yet. A wording/key/condition/order/step-identity change after release
requires a new content version and a fresh review; evidence stays tied to its
old version. Layout-only changes need not create a new mathematical version.

`misconception` is author/reviewer metadata explaining why a distractor fails.
Do not display it on hover or selection. Current player feedback stays one
neutral recovery message, then the correct explanation when permitted. No
automatic authoring, language-model judge, symbolic-equivalence parser, or
procedural variation is needed for these cards.

The `symbols` list is also authored reference metadata, not a pre-answer
glossary automatically displayed while role questions are being answered.
This slice may show it after completion. A future pre-answer notation aid must
record its exposure explicitly. `comparison_variable` and `defined_object`
are additional descriptive roles used in 013; do not silently classify them
as unknowns or parameters merely to fit the source template's five common roles.

## 5. Exact FM003 C++ integration seam

FM003's one capability is opening and completing the source-derived 002 card
alongside the starter card, while preserving each card's progress independently.
Keep the pure `LayeredQuestionSession` as the correctness/evidence owner. Its
single-question semantics remain as specified in FM002.

Make these bounded changes in the existing files:

1. In `LayeredQuestionSession.hpp/.cpp`, replace the single hardcoded question
   accessor with `layeredQuestionCatalog()` returning a read-only span of
   statically owned `LayeredQuestionContent`. Catalog order is starter, 002.
   Add `findLayeredQuestion(id)` returning a const pointer or null. Avoid a
   second compatibility accessor once its live consumers are migrated.
2. Each content record owns its ordered step list (a vector initialized once
   inside the static catalog is sufficient). Each step still has exactly four
   options. Add stable step/option IDs and the authored working lines.
3. Construct `LayeredQuestionSession` with a const reference to one catalog
   record. The catalog is immutable and outlives every session. Expose
   `content()` as a const reference. Replace the fixed seven-step run array
   with a vector sized from that record. Derive bounds, progress, and summary
   denominator from `content().steps.size()`.
4. In `FirstMoveUi.hpp`, add a plain app-owned `GuidedQuestionState`: one
   session per catalog entry plus `optional<size_t> activeQuestionIndex`.
   This owns navigation and storage of those sessions, not their correctness.
   An absent active index means the question grid is shown. The grid-focus
   index is presentation state and may differ from the active session.
5. `dispatchFirstMoveAction` owns opening a card by stable question ID. It
   validates mode and identity, calls that session's OpenQuestion, then makes
   it active only on success. Back calls that session's BackToGrid and clears
   the active index only on success. All answer actions delegate to the active
   session; absence of an active question rejects. No UI widget mutates a run.
6. Restart archives only the active question's completed run. Switching modes
   preserves active question and both sessions. Returning to Guided restores
   the same screen. Opening a completed card returns to its summary.
7. `LayeredQuestionUi.cpp` iterates catalog data for grid cards, displays actual
   step count, wraps multi-line working blocks, and uses authored solution
   lines rather than an FM002 equation literal. Keep the pinned source area,
   one center scroller, footer, and neutral recovery behavior.
8. `main.cpp` adds `guided_open_question <question_id>` and optional
   `--question <question_id>` for a deterministic Guided entry. Validate IDs
   before native window creation; `--question` implies Guided unless an
   explicit conflicting `--start-mode hunt` is supplied, which is a CLI error.
   Preserve `guided_open` as the documented script action for the starter;
   this has verified FM002 consumers. Existing commands keep their semantics.
9. Extend reporting with an ordered `guided_questions` array containing each
   question's identity/version/current run/archives and the active question
   ID or null. Preserve FM002's existing top-level `guided_question_id`,
   `guided_question_version`, `guided_runs`, `guided_archived_runs`, and
   `guided_current_run` fields as the starter's records. Existing receipts
   use those fields. New consumers use `guided_questions`; never reinterpret
   the old fields as whichever card is currently active. Existing option
   indices remain zero-based and displayed/report step numbers one-based.

Compile reviewed strings directly into the existing model source for these
first two cards. The JSON files define the authored content for transcription
and review; there is no runtime path back to `devil/99-red-booleans` and no
dependency on Python, NumPy, a book corpus, or mutable Markdown after launch.
Use the existing CMake targets; expected new production file count is zero for
this slice. Positive content LOC is expected for a new user capability, not
claimed as an ownership cleanup. Do not split the runtime by card ID.

## 6. Grid navigation and learner data

The FM003 grid has two cards. At wide sizes use two columns; at the small test
size use one column if the measured minimum card width does not fit. Arrow
navigation follows the displayed row/column arrangement; Enter opens the
focused card. The entire card rectangle is a hit target. Display question
title, neutral description, step count, and current progress, not a score.

Run number and prior exposure are per question/version. Returning to an
unfinished question preserves its tentative choice and recovery state. Opening
another question does not archive or clear it. Practice Again affects only
that question. No cross-card mastery, scheduling, locked progression, durable
history, or answer migration is part of FM003.

Evidence uses semantic IDs in addition to display indices. Keep selected but
unchecked choices distinct from checked attempts, and error recovery distinct
from answer reveal. Store only values actually produced by those interactions.
The source-page setup/solve fields are never written back from game play.

At the later durable-history boundary, the storage key must include question
ID and content version. Record a new version as new content; never attach an
old option index to a changed answer list. Storage is a separate capability
requiring its own save/load integration test; it is not silently added here.

## 7. Card 002 mathematical decisions

Unknowns are `(a,b,c)`; the supplied function inputs are `-1,0,1`. Substitution
in `a*x^2 + b*x + c` gives:

```text
a - b + c = 1
        c = 0
a + b + c = 2
```

After substituting `c=0`, addition gives `2a=3`; hence `a=3/2` and `b=1/2`.
The function is `f(x)=(3/2)*x^2+(1/2)*x`.

The coefficient matrix, in that same row/unknown order, is
`[[1,-1,1],[0,0,1],[1,1,1]]`, whose determinant is `-2`. Nonzero determinant
establishes uniqueness within the specified polynomial model. Evaluating the
answer at the three supplied inputs verifies passage through the observations;
that check alone does not prove uniqueness among arbitrary functions.

The source setup-key wording mixes “exactly quadratic” and “degree at most 2.”
The adaptation states the supplied form and solves it; the recovered `a=3/2`
is nonzero, so the resulting polynomial is indeed quadratic. Do not generalize
this exercise into a rule that any three observations force a nonzero quadratic
coefficient. Other polynomial families require their own assumptions.

## 8. Card 013 mathematical decisions

The supplied `m,a,b` are fixed real parameters within an instance. Introduce
the unknown line position `t`. Let `v=(1,m)`, `p=(a,b)`, and `q=t*v`.
The first coordinate of `v` is 1, so it is nonzero for every real `m`.

Perpendicular residual gives the scalar equation:

```text
(p - t*v) dot v = a + m*b - t*(1 + m^2) = 0
t = (a + m*b)/(1 + m^2)
q = (t, m*t)
```

Here is the complete closest-point and uniqueness argument, so neither a
theorem number nor numerical sampling substitutes for the justification:

```text
For every real s,
p - s*v = (p - t*v) + (t-s)*v.
The cross term in the squared Euclidean norm is zero because (p-t*v) dot v = 0.
||p - s*v||^2 = ||p - t*v||^2 + (s-t)^2 * (1+m^2).
The extra term is nonnegative and is zero exactly when s=t, since 1+m^2>0.
Thus q is the closest point, and it is unique.
```

The alternate squared-distance calculation agrees:
`g(s)=(s-a)^2+(m*s-b)^2`; `g'(s)=2*((1+m^2)*s-(a+m*b))`;
`g''(s)=2*(1+m^2)>0`. The checker exercises exact rational instances and the
distance-gap identity. These are regression examples, not a proof for every
real parameter; the argument above supplies that proof.

Do not replace Euclidean distance with another norm, omit the through-origin
condition, divide by `m`, or claim this slope representation includes vertical
lines. An affine line `y=m*x+c` needs a translated construction and its own
reviewed card. Those are not interchangeable distractor answers here.

## 9. Validation and handoff boundaries

Before transcribing a card, run the artifact-only checker:

```bash
python3 verify_planning_artifacts.py
```

It checks pinned hashes, schema references, stable identities, answer sequences,
and exact-rational mathematical examples. It does not execute the game. The
builder then proves the live route with these targeted new cases:

1. Both FM003 cards open from pointer, keyboard, and script through the same
   dispatcher, with isolated records and correct content IDs.
2. Starter recovery → grid → 002 progress → grid → starter resumes that exact
   recovery; no lost attempts or false archive.
3. All 002 layers completed using the authored answer sequence produce the
   specified solution and per-step evidence with the correct denominator.
4. One wrong/retry/correct and one wrong/reveal in 002 retain distinct evidence;
   repeated Practice Again archives only one completed 002 run.
5. Long option and matrix-like text remain readable at 1024×768 / 150%; no
   clipping, hidden footer, or horizontal scrolling. Existing Hunt remains
   explicitly selected in its capture gate.

Derive target names from the then-current CMake graph. Reuse the existing
model/input targets and a single new source-card capture case; do not create
a second parallel test harness or repeat unrelated passing renderer gates.

After FM003's brief is reviewed, card 013 becomes a data/content slice using
the same catalog, routing, and evidence types. Its proof-heavy completion must
retain the distinction between choosing valid derivation steps and writing a
proof independently. Future cards enter one reviewed revision at a time until
the authoring contract is stable enough for a separate batch-content tool.
