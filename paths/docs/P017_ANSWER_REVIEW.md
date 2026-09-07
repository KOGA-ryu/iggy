# P017: Answer review

Status: automated checks passed; offscreen screens reviewed; uncommitted.
Baseline: `6a9f1c8`, with completed, uncommitted P016 New game work retained.
The builder's partial P017 implementation was resumed in this checkout.

## Player flow

Pause → Answer review opens a scrollable panel for the current question.
The latest reached step starts expanded. Each reached step has a selectable
row with its number, outcome and name; the name remains visible when collapsed.
Expanded rows show the prompt, working shown at that step, and the player's
submitted answer text in order. Attempts carry written Correct/Incorrect
labels, so their meaning does not depend on colour.

The question selector also offers completed questions retained in this game.
The header shows wrong clicks and steps needing another try separately:
repeated wrong submissions count as separate clicks but only one affected
step. Partial answer sets show the number collected and the number required.
Outcomes are In progress, Correct first try, Correct after retry, or Answer
shown when the underlying question record contains that assistance outcome.
The gallery does not gain a new show-answer action.

For a step requiring all accepted answers, several successful submissions
without an incorrect answer still mean Correct first try. For a step accepting
any one valid answer, its required count remains one. These labels summarize
the existing record, not an independent accuracy or scoring calculation.

Only reached steps are listed. Review does not enumerate unsubmitted options,
reveal remaining accepted answers, or show future working states. It preserves
the prompt and working already shown during play, including any intentional
guidance in the prepared content. This checkpoint does not add explanations
or a separate answer key.

Opening review pauses the same game. Targets, unfinished pops, question
progress and attempts remain frozen. Back to game returns to that paused
session; Resume is a separate action. Reopening review selects the current
question again. The history belongs to the selected pack/practice game and
lasts only while that session exists. New game replaces its history; it does
not clear other started games. No history is written to disk.

## Ownership and files

- `src/runtime/first_move/LayeredQuestionSession.hpp` and `.cpp` define
  `QuestionReview` and `review(runIndex)`. Index zero selects the current run;
  subsequent indices address completed runs in retained order. Invalid indices
  return no review. The projection resolves stable question ID/version pairs
  against frozen content and reads existing step/attempt evidence. It borrows
  display text for the catalog/session lifetime and performs no mutation or
  new answer judgment.
- `src/ui/GalleryMenu.hpp` and `.cpp` own the Review screen, selected run and
  expanded step. `OpenReview`, `CloseReview`, `SelectReviewRun` and
  `ToggleReviewStep` enforce selection and pause boundaries. Other menu/launch
  actions cannot bypass review. They use the existing session owner and P016
  replacement boundary.
- `app/gallery_main.cpp` draws the panel, forwards actions between frames,
  reports review state and prevents gameplay input while it is open. The
  native scene remains available for rendering without advancing the game.
- `tests/first_move_layered_question_tests.cpp`, `tests/gallery_menu_tests.cpp`
  and `tests/gallery_answer_review.script` exercise the model/menu/native
  boundaries. Additional bounded native scripts accompany the evidence.

There is one question/attempt owner and one menu navigation route. No competing
judgment, content loader, history store or renderer was introduced. Production
C++ changes span five existing files, with zero new production files.
P016 and P017 together are **net +208 lines** against `6a9f1c8`; subtracting
P016's documented net +46 gives **net +162 for P017**. This resumed pass adds
one net production line to keep collapsed step names visible, and completes
the missing verification and checkpoint documentation.

## Verification

The affected targets came from the current CMake graph. Both focused test
targets and the native executable built successfully:

```sh
cmake --build build/question-content --target paths_guided_tests paths_gallery_menu_tests -j 4
ctest --test-dir build/question-content -R '^(paths_guided_tests|paths_gallery_menu_tests)$' --output-on-failure
cmake --build build/gallery-port --target paths_gallery -j 4
```

Both tests passed. Coverage includes repeated wrong attempts in order,
AllAccepted partial collection, AnyAccepted completion, first-try/retry/shown
outcomes, reached-step filtering, no uncollected answer disclosure, stable
archived content identity, read-only review and content surviving source-file
removal. Menu checks cover frozen clocks/pops/attempts, valid and invalid
review selections, blocked navigation, return to a still-paused game,
reopening at the current question and New game clearing replaced history.

Native scripts use `review`, `back_to_game`, `review_run N` and `review_step N`
through the same semantic actions as the UI. The retained minimal scenario is:

```sh
./build/gallery-port/paths_gallery --offscreen --seed 19 --motion stationary \
  --resolution 1440x900 --script tests/gallery_answer_review.script \
  --capture build/answer-review-evidence/partial.png \
  --report build/answer-review-evidence/partial.json
```

Fresh results are in `build/answer-review-evidence/resumed/`, pinned to the
rebuilt binary in `run-manifest.json`. They cover:

| Scenario | Observable result |
| --- | --- |
| Partial answer set | One incorrect and one correct submission; 1/2 collected; review remains at the same simulation tick |
| Completed question | Completed run selected independently of the fresh endless run; three ordered attempts and Correct after retry |
| Back to game | Still paused; targets, bindings, attempts and counters identical to the partial-review report |
| Small screen | Source 002 review fits at 800×600; only its first, unattempted step is shown |
| Multi-step question | Source 002 shows only its two reached steps, with the earlier retry expanded and the next step collapsed |
| Shooting during review | Script rejected at line 3 before gameplay input is dispatched |
| Resume during review | Script rejected at line 3; review must close before gameplay can resume |

All five successful scenarios exited zero; both deliberate rejection cases
exited one with `only_frame_or_tick_available_in_menu`. Report assertions pass
via `python3 build/answer-review-evidence/resumed/verify.py`; the resulting
`verification.json` records hashes and the verification boundary. The 1440×900
multi-step and 800×600 captures were visually reviewed for layout and readable
row details. No visible window was launched. Human pointer feel and interactive
swapchain acceptance remain separate from these offscreen results.

Changes remain uncommitted. The next candidate is prepared explanations for
resolved steps in review, after defining their disclosure rules for partial
and unfinished steps. Scoring and durable profiles remain separate work.
