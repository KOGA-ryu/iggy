# P018: explanations in Answer review

Status: automated checks passed; offscreen screens reviewed; uncommitted.
Baseline: completed P017 working state, including retained uncommitted P016.
The starting files and hashes are in `build/answer-explanation-evidence/baseline/`
and `baseline.json`.

## Player contract

Pause → Answer review → expand a resolved step. Its **Explanation** appears
below the submitted attempts, using the text already authored in the card.
All 39 steps in the seven playable cards have nonempty explanations; no
question content or JSON schema change is needed.

- For AnyAccepted, one valid answer resolves the step and unlocks its text.
- For AllAccepted, every accepted answer must be collected first. Wrong answers
  and partial sets keep the explanation hidden, even after a correct click.
- A resolved step can be reviewed before Continue or while later steps remain
  unfinished. Unreached steps are still excluded by P017's projection.
- Completed-question history retains the explanation from that run's frozen
  question ID/version. A later attempt at the same question starts hidden again.
- In the shared Guided model, an explicitly shown answer is already resolved
  and may expose its explanation. Its Answer shown outcome remains intact;
  this feature does not introduce an assistance action in the gallery.
- An empty authored explanation is valid; the UI omits the entire section.

After resolution, the authored explanation may discuss correct alternatives
and distractors. This is a step-level explanation, not per-option feedback or
an automatically derived proof. Unfinished-step explanations are never copied
into the review view. Existing prompt/working guidance remains as authored.
The loader does not parse or verify mathematical display text.

Opening, browsing or closing review does not change attempts, completion,
working, targets or timing. Back to game retains the pause. New game replaces
the selected game's history and resets explanation eligibility through the
existing P016 boundary. Text remains in the session's frozen catalog; changing
or removing source files does not change explanations for that game. No disk
history or separate unlock state is introduced.

## Methods and files

- `LayeredQuestionSession.hpp`: adds the borrowed `explanation` string view to
  `QuestionReviewStep` and documents its resolution gate.
- `LayeredQuestionSession.cpp`: `review(runIndex)` calls the existing
  `layeredQuestionStepResolved(record)` predicate before exposing that frozen
  step's authored explanation. This is the only new disclosure decision.
- `app/gallery_main.cpp`: the expanded review row renders a nonempty view with
  the existing wrapped-text helper. The report adds an `explanation_available`
  boolean derived from the same view. Availability describes eligible text;
  the existing `expanded` field describes whether its row is open.

The UI does not re-evaluate accepted masks, search the catalog or load files.
There is no new dispatcher, completion policy or judgment route. The existing
scrollable panel contains long text while Back and the question selector stay
above it. No new production files are needed.

Production C++ delta against the saved P017 state: **+5/-2 lines, net +3**
across three existing files. C++ tests add **+32/-3 lines, net +29** across the
existing question and menu tests. `tests/gallery_answer_explanation.script`
adds the repeatable native multi-step scenario. Prior P016/P017 changes remain
uncommitted and preserved.

## Verification

The current CMake graph selected the model/menu tests and native executable:

```sh
cmake --build build/question-content --target paths_guided_tests paths_gallery_menu_tests -j 4
ctest --test-dir build/question-content -R '^(paths_guided_tests|paths_gallery_menu_tests)$' --output-on-failure
cmake --build build/gallery-port --target paths_gallery -j 4
```

Both test targets passed. The regression coverage proves unattempted, wrong,
partial and newly reached steps keep their explanation hidden; full and
AnyAccepted resolution expose the exact authored string; earlier/archived
steps use their own frozen content version; empty text stays valid; and Guided
assistance retains its recorded outcome. The menu scenario proves loaded
explanations survive source-file removal, archived selection works, and both
endless progression and New game reset eligibility for the new run.

The native script uses the existing shooting and review actions:

```sh
./build/gallery-port/paths_gallery --offscreen --seed 19 --motion stationary \
  --resolution 1440x900 --script tests/gallery_answer_explanation.script \
  --capture build/answer-explanation-evidence/chain.png \
  --report build/answer-explanation-evidence/chain.json
```

Four offscreen scenarios exited zero: a partial set with no explanation, a
completed archived question with its explanation, and a source-card run with
one resolved and one unfinished step at 1440×900 and 800×600. Both layouts were
visually reviewed. The small view wraps the explanation and scrolls remaining
rows while retaining the navigation controls. No visible window was launched.

`build/answer-explanation-evidence/` contains the binary hash, scripts, reports,
captures and logs. `verify.py` checks the saved reports, including identical
question evidence, target state and clocks against the matching P017 reports,
then writes `verification.json`. These comparisons are evidence checks, not
reruns of the earlier implementation gate. Human pointer feel and interactive
swapchain acceptance remain pending.

The next candidate is a focused retry round for missed questions, preserving
the original completed runs and recording new attempts separately.
