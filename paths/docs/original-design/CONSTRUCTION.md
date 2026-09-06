# Building First Move

Game extension to the [application construction plan](../app-plan/CONSTRUCTION_CHAPTERS.md) · Proposed sequence

This extension can begin after the app's Chapter 03 supplies a reader, attempts, help history, and setup comparison. A standalone design sketch can test the game mechanic sooner.

It uses the same content and learner evidence. It does not require completing the corpus importer or interactive tutor before something is playable.

## G00 — The rules on a small board

**Build:** One reviewed linearity pack and a paper or interactive sketch of marking, committing, banking, clearing, and recovering rows.

**Use:** Try the core interaction with no timer.

**Learn:** State transitions, interaction design, and separating a mathematical judgment from a game consequence.

**Proof:** Each example has a clear answer and reason. Unmarked cells are explicitly judged false when committed. A wrong first attempt cannot be overwritten by a repair. Observe whether banking and clearing offers a decision worth keeping.

**Stops at:** A chosen core mechanic. The supplied [rules sketch](FIRST_MOVE.html) illustrates this chapter's idea; the chapter is not claimed accepted or user-tested.

## G01 — A responsive Hunt board

**Build:** Original board visuals, readable mathematics, pointer and keyboard control, selection states, a fixed finite board, and adjustable motion.

**Use:** Finish a calm board and restart immediately.

**Learn:** Rendering, focus, input handling, animation state, and preserving state during layout changes.

**Proof:** Keyboard and pointer actions produce the same mathematical submissions. Held keys cannot submit a row twice. Text remains readable at enlarged sizes. Resizing or losing focus does not lose work.

**Depends on:** G00 and the app's reader and attempt contracts.

## G02 — Ready rows, combined clears, and recovery

**Build:** Scoring, ready rows, multirow clears, a review state that preserves the original judgment, and an explanation panel.

**Use:** Choose whether to bank or clear, study an error, and recover board space.

**Learn:** Deterministic game state, score rules, evidence records, and replay.

**Proof:** Verify the proposed 100 / 240 / 420 scoring cases and capped bonus. Repeated submission and repeated repair do not farm points. An explanation records assistance. A recovered row earns no clear score and no independent-success credit.

**Depends on:** G01.

**Milestone:** A small complete Hunt game.

## G03 — Incoming rows and a finite Cascade wave

**Build:** A predictable incoming queue, preview, arrival clock, pause, capacity limit, end-of-wave summary, and chosen pace.

**Use:** Play a short pressure scenario with a clear finish.

**Learn:** Time-based simulation, event ordering, pause semantics, and reproducible sessions.

**Proof:** Pausing freezes arrival time. A simultaneous clear and arrival follows one declared order. Overflow ends the run before erasing a row. The same recorded inputs and schedule reproduce the result. Backgrounding the app pauses appropriately and records the interruption.

**Depends on:** G02.

**Milestone:** The main arcade loop.

## G04 — The Lab and comparable personal records

**Build:** A small scenario catalog, fixed-item and optional timed runs, input settings, per-skill results, and personal comparisons tied to the actual scenario version.

Begin with unknown identification, linearity, a method-choice scenario, and one supported setup-production task.

**Use:** Train one weak component and see which part of the task changed.

**Learn:** Experiment conditions, latency measurement, exposure tracking, and honest summaries.

**Proof:** Recognition and production remain separate. Prior answer exposure, assistance, control changes, or changed rules prevent misleading comparisons. Correct-response timing is shown with errors and omissions.

**Depends on:** G02 and the app's supported checks where free input is assessed.

## G05 — Corpus packs and return to study

**Build:** An adapter from reviewed study records to bounded game challenges, source links, error-review handoff, and a fresh full-question follow-up.

**Use:** Play material connected to the book being studied and return to the exact source or explanation.

**Learn:** Shared data boundaries, content validation, and several activities built from one source.

**Proof:** An unresolved essential assumption excludes the challenge from scored play. A changed source invalidates affected game records. A generated variant receives its own mathematical review. A game error opens the corresponding study material without changing the initial attempt.

**Depends on:** The app's Chapters 06–08 and G04.

**Milestone:** A game that belongs to the corpus-based study system.

## G06 — Tune through play and assess transfer

**Build:** Refined pacing, audio, accessibility, playlists, stable comparisons, and small sets of unseen follow-up questions.

**Use:** Keep a sustainable practice routine with an arcade component that feels good to return to.

**Learn:** Playtesting, separating hypotheses, and interpreting evidence from actual use.

**Proof:** Observe fun and readability directly. Check the mathematical keys independently. Compare later unaided setups and selected full solutions with prior work, accounting for question difficulty and exposure. Do not use arcade scores as the evidence of transfer.

**Depends on:** G03–G05.

**Stops at:** A deliberately scoped first release. More subjects, endless modes, optional enemies, and deeper campaigns follow the same acceptance process.

## How this fits the existing manual

Keep the main application's chapter numbering intact. The G-series is an optional construction branch that reuses its foundations.

For a chosen chapter, write and review the complete listing before cutting typing cards. Preserve the distinction between a design sketch, an accepted rule set, a reviewed listing, typed code, and a tested game.

The current deliverables are the influence study, game design, construction extension, and a small interactive rules sketch. They do not imply that this extension has been implemented in the future C++ application.
