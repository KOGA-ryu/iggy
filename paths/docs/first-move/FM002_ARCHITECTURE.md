# FM002 — Guided Question implementation contract

Status: planning addendum to the current FM002 capability. This document
resolves the named ambiguities in `FM002_GUIDED_QUESTION_BUILD_PACKET.md`;
it does not authorize another game mode or another question in FM002.

## 1. Ownership and the live route

```text
SDL keyboard ── bounded action queue ─┐
ImGui pointer ─ semantic action ──────┼─ dispatchFirstMoveAction
CLI script ─── parsed action ────────┘      │
                      ┌───────────────────┼─────────────────┐
                      ▼                   ▼                 ▼
                  app mode           HuntSession    LayeredQuestionSession
                      │                   │                 │
                      └───────────────────┴─────────────────┘
                                          │ const records
                          ┌───────────────┴───────────────┐
                          ▼                               ▼
                    ImGui presentation              JSON report
                          │
                    existing Vulkan UI pass
```

| Decision | Canonical owner | Other code may do |
| --- | --- | --- |
| Option key and educational explanation | reviewed `LayeredQuestionContent` | display them at the permitted stage |
| Correctness, attempts, reveal, step/run transitions | `LayeredQuestionSession::dispatch` | send a semantic command; read const records |
| Summary counts | `summarizeLayeredQuestionRun` | format the returned numbers |
| Mode switching and active-session admission | `dispatchFirstMoveAction` | request a switch |
| Key interpretation and queue admission | First Move input adapter | produce app actions, never a success record |
| Wrapping, scrolling, focus, scale | First Move UI | change presentation state |
| JSON serialization | `main.cpp` report adapter | serialize evidence without recalculating correctness |
| GPU presentation | existing renderer | draw supplied UI without knowing the question |

Stay inside the original production file list. Keep SDL, ImGui, Vulkan, clocks,
files, and learner-profile policy out of the question model. Add no Creative
document or persistence route. Department registration remains the planner's
checkpoint responsibility.

## 2. Minimal state, exact transitions

Keep the three stored phases `Grid`, `Answering`, and `Complete`. Derive the
interaction state from the current step; do not store a second independent
phase machine:

```text
Choose   = Answering, unresolved, !awaitingRecoveryChoice
Recovery = Answering, unresolved, awaitingRecoveryChoice
Resolved = Answering, resolvedByPlayer || answerShown
```

| Command | Accepted from | Result | Must preserve |
| --- | --- | --- | --- |
| OpenQuestion | Grid | Answering, or Complete if this run already completed | current step, selection, recovery state, all evidence |
| SelectOption(i) | Choose, i in 0..3 | replace tentative choice | attempts and first-attempt fields |
| CheckAnswer | Choose with a selection | append one checked attempt; enter Recovery or Resolved | previous attempts; initial fields after their first write |
| TryAgain | Recovery | Choose with no selection | wrong first answer and all checked attempts |
| ShowAnswer | Recovery | Resolved with `answerShown=true`; display correct option | wrong first answer; no synthetic correct attempt |
| Continue | Resolved | next unresolved step; after step 7, Complete | completed step and earlier evidence |
| BackToGrid | Answering or Complete | Grid | all run state, including completed flag |
| RestartQuestion | Complete with completed flag | archive once; next run starts Answering at step 1 | immutable archived run |

Every other combination rejects without changing the model. Selection of the
already selected option may be accepted with `changed=false`; it never creates
an attempt. The dispatch result is authoritative; rejected actions must not
move focus, clear a selection, or scroll as if they succeeded.

The completed card is still openable: Complete → BackToGrid → OpenQuestion
returns to the existing summary. It does not create another run. Only Practice
Again creates another run. Leaving an incorrect-answer recovery screen and
returning must show the same Try Again / Show Me choice.

## 3. Content corrections and equation context

These are corrections to the original packet, not extra features:

1. Use `Vocabulary and solving · 7 short steps` as the card description. Do not
   identify the equation type in the card subtitle immediately before asking
   the learner to identify its type.
2. Replace every pre-reveal wrong hint with this recovery copy:
   `That choice does not fit this step. You can try again or see the answer and why.`
   The previous step-5 and step-7 hints supplied the requested numeric answer
   before the learner chose Show Me. All seven steps now use the same neutral
   copy. Correct explanations stay after a correct check or explicit reveal.
3. Step 4 asks:
   `Which operation removes +5 from the left side while keeping both sides equal?`
   Adding 5 or subtracting 3 from both sides preserves equality too; the prompt
   must specify the operation's purpose rather than calling those operations
   invalid algebra.

Keep the source equation visible. The working line is authored content indexed
by the current step, not something the UI solves:

| Step | Original equation | Working line before answering |
| --- | --- | --- |
| 1–4 | `3a + 5 = 20` | omit duplicate working line |
| 5 | `3a + 5 = 20` | `3a + 5 - 5 = 20 - 5` |
| 6 | `3a + 5 = 20` | `3a = 15` |
| 7 | `3a + 5 = 20` | `3a / 3 = 15 / 3` |

Label the extra line `CURRENT WORK`. Never show `a = 5` before step 7 resolves.
Advancing or resuming selects the authored working line for that step. A wrong
selection cannot alter it. In feedback, distinguish `CORRECT` from
`ANSWER SHOWN`; both may display the explanation, but only the former records
a checked correct answer.

## 4. Input ordering without accidental step skipping

The UI must not reinterpret an old Enter as a command for a newly displayed
state. Examples: two Enter events cannot check and then skip the explanation;
H followed by Enter in one queued batch cannot submit an answer to hidden
Guided mode.

Use a small presentation-context stamp on queued actions: mode, run number,
phase, step, and derived interaction state. This can be an epoch counter plus
those values at enqueue time. On drain, reject a state-dependent action whose
context is stale. A changed selection alone does not invalidate subsequent
Check in the same context. Open, Back, mode switch, Check, Retry, Reveal,
Continue, and Restart invalidate actions captured for the previous context
when they change that context. Implement this inside the existing adapter;
there is no new input framework or model clock.

Explicit CLI actions are executed sequentially against their current context.
They do not pretend to be physical key events. Their model preconditions still
apply, and contextual rejection remains visible in the script report.

| Context | Enter | Up / Down | T / S | Escape |
| --- | --- | --- | --- | --- |
| Grid | open the one question | no question mutation | unavailable | no question mutation |
| Choose, no choice | reject Check, show `Choose an answer first` | Up selects D; Down selects A | unavailable | back |
| Choose, selected | check | previous/next, wrapping A↔D | unavailable | back |
| Recovery | no implicit choice of recovery | no change | Try Again / Show Me | back |
| Resolved | next step / see summary | no change | unavailable | back |
| Complete | no implicit new run | no change | unavailable | back |

Practice Again stays an explicit action (button or R); Enter does not start a
new run. G/H use the mode dispatcher.
Hunt's open explanation blocks a switch until it is closed/released. Guiding
state cannot mutate while Hunt is active. SDL repeat events do not enqueue
game actions. Focus loss clears the queue. Pointer buttons invoke the same
semantic actions, with no correctness checks in the draw function.

## 5. Layout behavior that the builder can measure

Use the existing text scales and one scrollable content region. At 1024×768 and
150%, the header with the source equation remains outside that scroller. Put
the available actions in a footer outside it. This is the concrete meaning of
the original packet's pinned equation and reachable action requirements.

The center region contains the working line, prompt, four options, and
feedback. There is no per-option scroller. Each option's height is at least
its wrapped text height plus two vertical paddings; do not use a fixed height
that clips a two- or three-line choice. Reserve room for letter and state
labels when measuring wrap width. The full option rectangle is clickable.

Footer actions depend on interaction state:

- Choose: `CHECK ANSWER [ENTER]`, disabled until selected; `BACK TO QUESTIONS`.
- Recovery: `TRY AGAIN [T]`, `SHOW ME [S]`, `BACK TO QUESTIONS`.
- Resolved: `NEXT STEP [ENTER]` or `SEE SUMMARY [ENTER]`; `BACK TO QUESTIONS`.
- Complete: `BACK TO QUESTIONS`, `PRACTICE AGAIN`.

Wrap footer buttons to another line when their measured widths do not fit.
Subtract the actual footer height when sizing the center region. Do not rely
on SameLine calls fitting every scale. With pointer focus in Guided mode,
PageUp/PageDown and Home/End may scroll the content region as presentation
commands; they never select, check, or advance. This supplies keyboard access
to long explanations after the option-selection keys become inactive.

Reveal the selected option after a navigation action. Reveal the start of the
feedback after Check or Show Me. Reveal the prompt after Continue. Resize and
scale changes make the active target visible once. Idle frames preserve the
user's manual scroll position. State labels carry meaning without color:
`SELECTED`, `YOUR INCORRECT CHOICE`, `CORRECT`, `ANSWER SHOWN`.

At 1440×900 / 100%, the entire current FM002 step and footer should fit. At
1024×768 / 150%, vertical scrolling is expected; clipped text, horizontal
scrolling, an unreachable control, or a moving formula while selecting is not.
Summary transformations put operation descriptions on their own wrapped lines.

Set an explicit Guided body font around options, recovery text, explanations,
and action labels: 16 logical pixels at 100%, 20 at 125%, and 24 at 150%.
Use the active font size for both measurement and draw-list text. The delivered
enlarged capture shows these bodies remaining small while headings enlarge;
padding growth alone is not text scaling. Use a 28-pixel base title, a
28-pixel base source equation, a 19-pixel prompt, and 14-pixel status labels,
all multiplied by the selected scale. Put the subtitle below the title in
Guided mode so it does not compete for a narrow remainder of the same row.
Keep the existing Hunt typography outside this repair.

## 6. Evidence invariants and reference examples

An attempt exists only because CheckAnswer was accepted. Merely focusing,
selecting, reading, or revealing an option never creates an attempt.

For each step:

```text
firstCheckedOption = attempts[0].optionIndex, or null when empty
firstCorrect       = attempts[0].correct, or null when empty
incorrectChecks    = count(attempt.correct == false)
resolved           = resolvedByPlayer || answerShown
resolvedByPlayer && answerShown == false
```

For each run, derive counts from step records:

```text
firstTry     = count(firstCorrect == true)
afterRetry   = count(resolvedByPlayer && firstCorrect == false)
shown        = count(answerShown)
incorrect    = sum(incorrectChecks)
assisted     = shown > 0
resolvedStepCount = firstTry + afterRetry + shown
```

The three resolution counts partition resolved steps, including a partially
finished run. `completed=true` requires all seven resolved and the final
Continue. Seven resolved steps before final Continue are still Answering.
`assisted=false` means no answer reveal; it does not establish unaided mastery,
because prior explanations, error feedback, and prior exposure still exist.

| Checked options on one step | Result | firstTry | afterRetry | shown | incorrect |
| --- | --- | --- | --- | --- | --- |
| correct | player resolved | 1 | 0 | 0 | 0 |
| wrong, retry, correct | player resolved | 0 | 1 | 0 | 1 |
| wrong, retry, wrong, reveal | shown | 0 | 0 | 1 | 2 |
| selected correct, never checked | unresolved | 0 | 0 | 0 | 0 |

Reports preserve nulls, checked-option order, exposure, and all archived runs.
Code and report option indices are zero-based; report step numbers and CLI
choices 1..4 are one-based. Labels A..D are adapters. These are the indexing
conventions in the delivered FM002 reports and must remain stable. Existing Hunt
fields retain their meanings and values. Diagnostic rejection records must not
be counted as learner attempts.

## 7. Acceptance scripts and checkpoint

`reference_cases/FM002_REFERENCE_CASES.json` specifies model-action sequences
and expected outcomes independently of the UI. Map each action to the existing
enum; do not replace these expected answers by calling the production key.

The integration checks add only the boundary cases above: stale queued input,
recovery resume, completed-card reopen, and measured layout at the small/large
scales. Keep the original targeted gate; do not add a broad department run.

The existing `first_move_capture_smoke` must explicitly pass
`--start-mode hunt`: an unscripted launch now defaults to Guided. The new
`first_move_guided_capture_smoke` explicitly passes `--start-mode guided`.
Both must prove external UI recording and a nonuniform image. A screenshot is
visual evidence, not proof of correct learner records.

The builder's completion brief should identify adopted content corrections,
canonical owner, files changed, production LOC, exact gate results, artifacts,
and any unmet contract. Do not claim user acceptance from a green test result.
The planner reconciles that brief once, updates ownership/departments, and
prepares the next capability. FM003 below remains a separate content slice.
