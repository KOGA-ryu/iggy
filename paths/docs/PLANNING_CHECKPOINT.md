# Paths planning checkpoint

Recorded 2026-09-06 after the user asked to confirm the original plan before
expanding the ideas further. This is a discussion checkpoint, not a new build
packet or a replacement for the builder's agreed scope.

Current focus, corrected by the user: parse prepared maths cards into a
gamified solver. The player chooses an operation, then completes the
arithmetic that operation enables. Storage and catalogue organisation support
that conversion; they are not the current planning deliverable.

## 1. Confirmed gallery build

The agreed product baseline is the
[arcade gallery proposal](AIM_GALLERY_PROPOSAL.md). The concrete target methods
and ownership are in [Target foundation design](TARGET_FOUNDATION_DESIGN.md).

- Work in the standalone Paths project. The original maths cards remain
  read-only source material for reviewed game adaptations.
- Start with a visible mouse cursor, moving coloured spheres, stationary
  colour-linked answer rows, and a dedicated question area.
- Correct hits pop their corresponding balls. The next question resets the
  colours and assigns answers with a seeded shuffle. Assignments stay fixed
  during a question; collected answers cannot score repeatedly.
- Support Equality Sweep, Question Relay, and Equation Chain through the same
  target and question foundation.
- Keep physical objects/picking in GalleryScene, route behaviour in
  TargetMotion, target/question coordination in GallerySession, and
  mathematical correctness in the shared question model. Reuse the renderer.
- Keep mathematical difficulty, aiming difficulty, and time pressure
  independently adjustable. The gallery's priority remains continuous arcade
  play, with detailed explanations mainly in review or explicit help.

The complete gallery proposal also includes real source-card content,
scoring/bonus rules, developer routes and display profiles, visual controls,
saved results, and review. These remain capability-sized checkpoints; the
target foundation alone does not deliver the whole product.

The current [work list](WORKSTREAMS.md) reports P006 as Automated Green /
Manual Test Needed, with its completion record in
[P006_TARGET_FOUNDATION.md](P006_TARGET_FOUNDATION.md). That is a
builder-reported state, not an independent review performed for this planning
checkpoint. Consult the builder's current evidence before making a delivery
or acceptance claim.

## 2. Current focus: prepared card to playable solver

The user clarified the objective as an automatic parser/converter for prepared
maths cards. The initial flow is:

```text
Prepared card and reviewed solution recipe
  -> read source identity and structured mathematical input
  -> parse expressions and named operations
  -> validate the supported mathematical transitions
  -> expand them into operation-choice and arithmetic stages
  -> emit a reviewable game plan for the shared question model
```

The converter runs during content preparation. Gameplay consumes admitted
content and presents the current stage. The parser does not choose colours,
move balls, calculate scores, or operate the renderer.

### What the existing cards supply

The inspected source card 002 has TOML metadata, a Question section, Notation,
Method 1/2/3, a Setup key, and a Solution table with work and reasons. Those
are useful document boundaries, but the solution cells contain prose, LaTeX,
and sometimes several mathematical actions in one row. Reading that table
does not by itself identify every legal operation or generate answer choices.

The first recommended input contract is a prepared copy or companion record
in Paths, retaining the original source identity and hash. It supplies an
explicit unknown, a bounded machine-readable expression, and typed operation
entries for the reviewed solution route. The original source pages remain
read-only. Learner Attempt/setup/solve fields are not answer keys; Program
sections and linked scripts are not executed by conversion.

Existing authored cards in
[SOURCE_CARD_ARCHITECTURE.md](../content/SOURCE_CARD_ARCHITECTURE.md) remain
their versioned adaptations. This parser proposal does not silently change
their schema or relabel an old content integration as implemented.

### Small algebra example

Illustrative semantic fields; the exact input syntax is still to be specified:

```text
card: original_linear_3x_plus_5_eq_20
version: 1
solve_for: x
equation: 3*x + 5 = 20
prepared_operations:
  subtract_both_sides(amount=5, goal=remove_constant)
  divide_both_sides(divisor=3, goal=isolate_unknown)
check: substitute_solution_into_original
```

This is an original simple-algebra fixture, not a claim that source card 002
contains this problem. The author supplies the compact mathematical recipe;
the converter expands its supported operations into gameplay stages.

| Generated stage | Player task | Accepted response and transition |
| --- | --- | --- |
| Choose operation | Choose the operation that removes the +5 from both sides | Subtract 5; expose the pending calculation `20 - 5` |
| Arithmetic | Solve `20 - 5` in the gallery | 15; commit the next working state `3x = 15` |
| Choose operation | Choose the operation that isolates x in `3x = 15` | Divide both sides by 3; expose `15 / 3` |
| Arithmetic | Solve `15 / 3` | 5; commit `x = 5` |
| Arithmetic check | Evaluate `3*5 + 5` from the original equation | 20; show agreement with the original right side and finish |

Selecting an operation unlocks its arithmetic stage. The solved next equation
is committed after that arithmetic stage succeeds. Preview may show the whole
generated plan; the live view reveals only the current permitted working.

A prompt must specify its subgoal. A different valid solving order must not
be described as mathematically invalid merely because the prepared route
subtracts before dividing. First support the prepared route with precise
prompts; branching among complete alternative routes is a separate extension.

### Proposed technical method

- **Document reader:** extract the prepared fields and preserve source
  locations, so diagnostics identify the card and offending field or step.
- **Expression parser:** tokenize a small explicit expression grammar and
  build an expression tree using recursive descent with operator precedence.
  This is a conventional parsing approach described in
  [LLVM's parser tutorial](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html).
  It is a method reference, not a proposal to add LLVM as a dependency.
- **Operation validator:** dispatch typed operation IDs to bounded rules.
  For the first example, support subtracting a known constant from both sides
  and division by a known nonzero constant, with exact rational arithmetic.
  Check declared preconditions, intermediate states and final substitution.
- **Stage expander:** use reviewed templates to produce prompts, operation
  choices, arithmetic tasks, explanations, stable IDs and dependencies.
  Derive numeric keys from the validated trace. Check distractors against the
  precise task, remove duplicate/equivalent choices, and report an error if
  the requested valid choice set cannot be formed.
- **Content adapter:** translate the checked stages into the current question
  model's variable steps, option IDs and accepted sets. The existing model
  remains the one runtime answer judge. Preserve activity intent as metadata;
  the model currently has no operation-placement scene or activity router.

These are responsibilities to design before choosing production files or a
parser library. Initial supported mathematics should be one-unknown linear
equations in a declared normal form, with prepared operation recipes. The
numeric bounds, syntax, permitted normalization and stage limits belong in
the first input contract. Unrecognized notation, unsupported mathematics or
missing semantic preparation produces a diagnostic, never an invented key or
a partially playable card.

Given the same prepared input and converter version, produce the same stage
plan and keys. Runtime colour shuffling remains independent. Conversion
records source/prepared-content identity and converter/rule versions. Publish
a card only after the complete plan validates and is reviewed.

Automatically deriving a solution recipe from a bare equation can later use
a named, tested rule for a supported equation family. General inference from
arbitrary mathematical prose is outside the first proposed parser scope.

## 3. Ideas preserved for later exploration

These are brainstorming ideas, not additions to the current builder packet.

- **Mental arithmetic and strategies:** begin with accessible arithmetic;
  explain useful rearrangements such as `19 + 7 = 20 + 6`; let familiar
  skills use fewer guided steps and optional timed practice.
- **Physical operation puzzles:** the operation-to-arithmetic dependency is
  part of the planned parser output. A Portal-style room, physical blocks and
  door controls are a later presentation of those stages.
- **A small family of mathematical minigames:** reuse sorting, grouping,
  placing, balancing or ordering interactions where their actions help
  express the mathematical task. Aim practice is one activity among them.
- **Sort a mixed collection, then solve in groups:** give the player a large
  mixed set of equations. They identify similar problems, group them, and
  solve each group in a quick sequence. This is a player activity, distinct
  from automatic catalog filtering. The grouping rule, acceptable alternative
  groupings, and progression remain to be designed.
- **Base building and tower defence:** correct work and response time could
  contribute to improving or defending a base. Economy, progression and
  combat rules are later game design, with no parser dependency added here.
- **Applied trajectory problems:** arrow-distance and trebuchet-launching
  calculations are later content/gameplay ideas. Their physical models and
  mathematical assumptions require their own bounded design.

## 4. Where to resume

Resume by specifying one prepared algebra card's exact input syntax and its
expected generated stage plan. Set the supported expression grammar and
operation contracts, then the methods and file boundaries for conversion.

The first proof should convert that card into the operation/arithmetic
sequence above, show its complete preview, and reject malformed input,
division by zero, inconsistent steps and unsupported notation. Verify the
resulting keys and final substitution independently. A second small card
should vary the constants to establish that the converter is not hardcoded
to the example. Derive the affected build/model gate when implementation is
actually dispatched.

This checkpoint changes planning documentation only. It does not dispatch a
parser build, replace the target builder's packet, or claim that the parser
or operation-placement interaction already exists.
