# First Move

A math arcade inside the study workspace · Proposed game design · 6 September 2026

**Read the structure. Make the right move. Clear the board.**

First Move gives mathematical recognition a physical rhythm: move, select, commit, clear. It uses the same reviewed material and learner history as the [math study workspace](../app-plan/PRODUCT_VISION.md).

The [influence study](INFLUENCES.md) explains the contributions from Number Munchers, Aimlabs, and Tetris. The [interactive rules sketch](FIRST_MOVE.html) lets you try the initial classification-and-clearing mechanic. It is a small design prototype with original examples, not the finished game.

## 1. The experience

A dark, quiet playfield contains a few rows of mathematical objects. The rule is fixed above it in large, readable type. A small cursor moves between the cells.

You recognize two equations that are linear in the named unknowns and mark them. You leave the nonlinear ones unmarked, then commit the row. Every selection and rejection is checked together.

The row lights up: ready. You can clear it now, making space, or prepare another row first. A preview shows what kind of material is coming. You judge whether there is enough room to hold your completed work.

You commit the next row, clear both, hear a short ascending chord, and watch the occupied board settle into the space you made.

That is the central pleasure: reading something accurately creates order you can see and feel.

## 2. The three ways to play

| Mode | Purpose | Shape |
|---|---|---|
| **Hunt** | Learn the controls and practise one mathematical distinction. | A finite board, one stated rule, no automatic arrivals. |
| **Lab** | Train a particular component and compare like with like. | Short scenarios with controlled presentation, input, difficulty, and feedback. |
| **Cascade** | Apply familiar distinctions while managing limited space. | Incoming rows, ready rows that can be banked, combined clears, and a visible capacity limit. |

All three use the same challenge records where appropriate. Each run records its actual activity and assistance. The complete study desk remains available through “Explain this” and “Try a full question.”

The working title is provisional. The identity is a compact, tactile mathematical arcade rather than a narrative requiring a large world or cast.

## 3. Core rules

### The board

The starting design has four cells per row and room for six rows. These dimensions are prototype choices to evaluate for readability.

Rows enter at the bottom and push the existing stack upward. Mathematical objects stay stationary between arrivals and clears. The capacity boundary is always visible. A wave uses one precise rule; changing that rule happens between waves, with an explicit transition and an empty board.

Each row contains at least one qualifying and one nonqualifying object in the first content pack. Their counts vary. The generator does not always put two correct objects in the same positions.

The active row is visually distinct. Every cell uses the same typography, size, and neutral initial treatment. Color does not disclose the mathematical answer.

### Mark

Move to a cell and toggle whether it satisfies the rule. Marks are tentative and reversible before submission.

Unmarked means “does not satisfy” when the row is committed. The player is told this explicitly. Thus each commit supplies judgments about the whole row, including the rejected objects.

### Commit

Commit the active row. Check its selected set against the reviewed predicate or accepted response set.

If every judgment is correct, the row becomes **ready** and stops accepting edits. It still occupies space. If any judgment is wrong, the row becomes **needs review** and preserves the initial response.

A row that already has an assessment cannot be repeatedly submitted for new accuracy credit.

### Clear

Clear all ready rows with one action. A single clear is safe and useful. Preparing several rows before clearing earns a modest bonus while using more board space.

Initial score proposal:

| Ready rows cleared together | Points |
|---|---:|
| 1 | 100 |
| 2 | 240 |
| 3 | 420 |
| Each additional row | 100 more |

The bonus is capped after three rows. This rewards planning without making an enormous bank the only attractive tactic. These numbers are tunable playtest parameters.

Correctness determines whether a row becomes ready. Repeated clicking earns nothing. Time affects how much incoming work can be handled in Cascade; it does not turn a wrong classification into a correct one.

### Recover

A row needing review remains on the board. Opening its explanation pauses the action and records assistance. The explanation compares the original selection with the rule and supplies a reason for each object.

The player can study and release that row for space, receiving no clear points for it. A later new example tests the distinction independently. A run using explanation or repair is marked assisted and compared only with comparable assisted runs, not a clean benchmark.

Recovery is part of playing and learning. It does not rewrite the original error or require losing earlier progress.

### Finish

Hunt finishes when its finite board is cleared or reviewed. The first Cascade scenario uses a finite wave so completion is attainable; endless play can follow.

In Cascade, an arriving row that would exceed capacity ends the run before overwriting any existing row. The summary retains correct, incorrect, helped, and untouched work separately.

Pausing freezes the game. Help remains available. A benchmark's timing rules record pauses explicitly; the game never silently treats an interrupted run as directly comparable with an uninterrupted one.

## 4. The first mathematical pack

Rule:

> Select the equations that are linear in a and b. The real number x is known.

For this pack, “linear equation” means it can be written as A(x)a + B(x)b = C(x), with coefficients independent of a and b. All examples have a clear classification without a hidden domain exception.

| Object | Qualifies? | Reason |
|---|---|---|
| 2a + b = 7 | Yes | The unknowns occur to the first power with fixed coefficients. |
| a² + b = 7 | No | Squaring an unknown violates this linear form. |
| ax² + b = 1 | Yes | x is supplied; x² is a coefficient of a. |
| ab = 4 | No | The unknowns multiply each other. |
| a cos x + b sin x = 2 | Yes | The trigonometric values are coefficients of the unknowns. |
| sin a + b = 0 | No | An unknown appears inside a nonlinear function. |

The lesson this pack serves is the reversal identified in the existing math pages: x can be a supplied input while a and b are the unknowns.

A later wave asks a different question about carefully reviewed objects, such as linearity in x with fixed parameters under stated assumptions. It must not simply reuse the first wave's labels. Parameter values and zero coefficients can change the classification.

The first release's content is deliberately narrow. That lets us assess the interaction before building a large scenario catalog.

## 5. The Lab: what “Aimlabs for math” means here

| Scenario | Player action | What the result can support |
|---|---|---|
| Read the symbol | Match a short expression to its interpretation. | Recognition of the displayed notation and scope. |
| Find the unknown | Identify quantities to be found in a short question. | Correct role identification under these conditions. |
| Condition check | Select the supplied hypothesis that licenses a stated move, or identify the missing one. | Recognition of an applicability condition. |
| Method switch | Choose an applicable technique across introduced task families. | Method recognition among the offered choices. |
| First line | Enter a short setup without seeing candidate answers. | Production of that setup, where supported checking exists. |
| Dimension check | Decide whether a matrix or function operation is well-formed. | Local compatibility checking. |
| Next implication | Judge a proposed proof step using the displayed hypotheses. | Recognition of a valid local inference. |
| Counterexample | Choose a case that meets the hypotheses and violates a conclusion. | Recognition of a counterexample among these cases. |

Some scenarios use the Hunt grid. Others use one central prompt with fixed response controls. “Tracking” from an aim trainer is not mechanically translated into chasing mathematical symbols: each scenario needs an actual mathematical purpose.

Start with a fixed number of questions and optional timed presets. Measure the distribution of response times on correct initial responses alongside mistakes and omissions. Varying reading length, input method, and prior exposure affects comparisons.

A displayed hypothesis or list of methods supplies context. The learning record retains that context. Benchmarks do not equate choosing from options with producing a method and setup independently.

## 6. Cascade: pressure that creates a decision

The player must choose how much ready work to keep before making space. Upcoming-row previews reveal workload and wave context, with no answer locations or correctness cues.

Pressure rises through arrival rate and starting occupancy. Mathematical difficulty rises through distractor similarity, representational variation, prerequisite depth, and the work required by the rule. These controls are independent.

Choose a pace before the run. Adjust it between runs after observing readability and accuracy. An optional adaptive training session can change pace between waves and records those changes; it is not compared as the same fixed benchmark.

New mathematical material is introduced in Study or Hunt before being proposed for timed practice. The player can always choose a calm version.

The initial design does not require enemies. If a later chase mode is worthwhile, its movement must be predictable enough to preserve reading and be reported as an additional motor challenge.

## 7. What makes the game rewarding

**Selection:** a small cursor pulse, clear mark state, and immediate input response.

**Commit:** a compact snap into a ready state after a correct row, with no long celebration between decisions.

**Combined clear:** a brief, distinctive chord and an outward sweep across the prepared rows; the rest of the board settles.

**Recovery:** an intelligible explanation and regained room.

**Progress:** named scenario records and visible improvement under comparable conditions. A session can end with “Your method choices were accurate; the setup still needed help,” when the recorded evidence supports that statement.

The art direction is restrained: deep ink background, warm mathematical type, a mint cursor, pale gold for ready rows, and a clear amber review state. State also has text and shape cues. Motion, sound, and text size are adjustable.

Cosmetic changes can recognize milestones. They do not alter the mathematical answers or supply hidden scoring advantages. The design does not need spending systems, daily streak penalties, or compulsory competitive rankings.

## 8. From the whole corpus to a playable challenge

An exercise becomes game material only when a short, well-defined decision can be extracted without losing essential context.

Each challenge records:

- Source record and version, or an explicit original/variant origin.
- The mathematical skill and prerequisite material.
- The complete rule, variables, domains, quantifiers, and relevant assumptions.
- Objects, accepted judgments, and reasons.
- A supported validator and the review evidence behind it.
- Presentation constraints: readable length, symbol support, and any essential figure.
- Exposure history, assistance, and whether the task asks for recognition or production.

A reviewed theorem might support a hypothesis-matching challenge. A proof might support one local implication. A numerical exercise might support a residual or conditioning check. A long original proof remains in the full study workspace.

Do not automatically compress an ambiguous exercise into a forced true/false tile. A source issue, unsupported predicate, or contested judgment keeps it out of scored play.

The live action uses prepared data and supported deterministic judgments. AI can help author and review proposed challenges outside the run; it is not required to improvise truth judgments while a timer advances.

## 9. The learning loop surrounding the game

1. Study the distinction with a definition and examples if needed.
2. Try a calm board and explain an uncertain choice.
3. Play a short Lab scenario or Cascade wave.
4. Review the most useful error, with its actual reason.
5. Set up a fresh related problem without candidate answers.
6. Return later for another independent problem and occasional full solution.

The earlier [tutoring research review](../TUTOR_METHODS_RESEARCH.md) supports the choice of activities such as examples, explanations, comparisons, and later independent assessment. It does not establish an optimal game duration or validate this exact sequence.

## 10. What we measure

| Game result | Learning evidence kept beside it |
|---|---|
| Rows cleared and combo points | Correct and incorrect initial classifications, including false selections and missed valid objects. |
| Pace and occupancy reached | Scenario difficulty, reading load, pauses, and control scheme. |
| Recovered rows | What help was shown and what was changed after it. |
| A personal best | Exact rules, content version, exposure policy, assistance, and timing conditions. |
| A completed wave | Which mathematical components were tested and which were not. |

A per-skill view is more informative than one “math level.” Classification fluency, free setup production, and complete proof construction remain separate.

## 11. First playable scope and acceptance

Build one original linearity pack, four-cell rows, six-row capacity, marking, one-shot commit, ready states, combined clear, recovery, and a finite incoming queue.

First test the untimed mechanic. Then test arrivals at several chosen rates. Watch for:

- Whether the player can explain the classification rule after the introduction.
- Whether banking a row creates an interesting choice.
- Whether the board remains readable as it fills.
- Whether input mistakes are distinguishable from mathematical mistakes.
- Whether clearing and recovery make the player want another run.
- Whether a later fresh question can be set up without the game's options.

Fun, interface correctness, mathematical correctness, and transfer are separate findings. A small successful playtest justifies the next construction step; it does not establish effectiveness across the corpus.

## 12. The broader destination

The mature game has a compact library of strong scenarios, topic packs from the corpus, configurable playlists, replayable personal benchmarks, useful error review, and a direct path back to the source and study desk.

Its defining promise stays concrete: **the moves you get better at are mathematical distinctions you can name and use.**
