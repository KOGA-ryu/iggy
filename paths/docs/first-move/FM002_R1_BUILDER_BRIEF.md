# FM002-R1 — complete the Guided Question contract

Status: authorized bounded follow-up within FM002. Keep changes uncommitted.
The planner received the final brief and reviewed its delivered JSON evidence,
enlarged capture, and the specific source routes below. The builder reported
the original build and five-test gate green. Those passing results stand for
that implementation; this repair addresses the following uncovered behavior
and a flaw in the planner's original recovery hints.

Canonical detailed contract: [FM002_ARCHITECTURE.md](FM002_ARCHITECTURE.md).
Model reference cases:
[FM002_REFERENCE_CASES.json](reference_cases/FM002_REFERENCE_CASES.json).
Read these two files; there is no need to rediscover the source-card project.

## Required changes

1. **Keep retry distinct from reveal.** The original packet's step-5 and
   step-7 wrong hints disclosed the requested answer. Replace all pre-reveal
   hints with the neutral text in architecture section 3. Use the neutral
   card subtitle and the precise step-4 prompt there. Option order and keys
   remain unchanged. This is a correction to the planner's content, not a
   claim that the builder failed to implement the supplied text.
2. **Show the current mathematical work.** Add the authored per-step working
   lines from section 3. The present `drawAnswering` displays only the original
   equation even when the question asks about the simplified equation.
   Content supplies the line; the view does not compute it.
3. **Make 150% enlarge the text the learner reads.** The delivered
   `/private/tmp/first_move_fm002/guided_show_1024x768_150.png` has a large title
   and prompt but small option text, explanation, and button labels.
   `drawAnswering` pops its explicit prompt font before rendering those
   bodies. Apply the explicit Guided typography in section 5 consistently to
   draw-list text, ordinary text, measurement, and button labels. Stack the
   Guided subtitle under the title.
4. **Keep context and controls stationary.** Put the source equation and
   progress header above the single center scroller, and action controls
   below it. Measure option/footer heights; wrap long options and footer
   buttons. Add the section-5 presentation-only reading keys. Auto-reveal on
   state changes must not undo manual scroll on idle frames. Use explicit
   incorrect/revealed text labels on options; an amber outline alone does
   not explain a previous wrong choice.
5. **Make queue and rejection behavior explicit.** Adopt the stale-context
   admission rule in section 4 inside the existing bounded input adapter.
   Apply UI focus changes only after an accepted domain command. Present code
   changes `guidedOptionFocus` before SelectOption/restart rejection and Down
   with no choice starts at B. Implement the explicit navigation table. Keep
   R as an explicit Practice Again binding, repeat suppression, focus-loss
   clearing, and the Hunt review switch guard.
6. **Keep the old Hunt capture a Hunt capture.** In
   `cmake/iggy3d_tests.cmake`, `first_move_capture_smoke` currently launches
   without an explicit mode and therefore exercises Guided after the new
   default. Add `--start-mode hunt`. The separate Guided smoke already
   specifies its mode. Do not alter the renderer.

The existing run model already appears to preserve first-attempt/retry/reveal
evidence correctly in the delivered reports. Keep it. Do not rewrite it to
introduce another stored state machine. Use the reference cases to cover
recovery resume, completed-card reopen, selection-without-attempt, and repeated
archive rejection as needed, retaining consistent option/step indexing.

## Verification and stop

Build the affected app/model/input targets from the current graph. Run the
layered-question and input tests, plus the explicitly selected Hunt and Guided
capture tests. The unchanged Hunt model test already passed; repeat it only
if a Hunt-model boundary changes. Add focused input cases for stale queued
actions, first navigation, rejected-action focus, and manual-scroll stability.

Capture a normal step with its current working line at 1440×900 / 100%, and an
enlarged recovery/reveal state at 1024×768 / 150%. Verify both text size and
reachability, not just external UI recording. Use the same bounded GPU access
as the completed FM002 gate, without opening a visible window.

Stay inside the original FM002 production file list. There are no new
production files required for this repair. Planner owns department/ownership
updates at the checkpoint. Do not begin card 002 or 013, corpus ingestion,
persistence, new game modes, or engine changes in this turn.

Send the finished brief back to planner task
`01a074ae-914f-7b23-bbc4-abaf7d14d533`, including the adopted corrections, files
and LOC, exact checks, capture/report paths, and any remaining mismatch. The
planner will wait for that delivered brief without status polling.
