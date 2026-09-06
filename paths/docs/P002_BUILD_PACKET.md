# P002 — build the Paths Workshop

Status: user-authorized next capability. The user approved the proposed
Workshop with “make it so.” Finish P001 and its completion brief first.
The planner reviews that delivered checkpoint and releases this packet for
implementation; no user permission question is needed at that handoff.

Builder: `01a0747d-ab44-7e61-b673-3bd9a8ee29e4`.
Planner: `01a074ae-914f-7b23-bbc4-abaf7d14d533`.
Project root: `/Users/kogaryu/iggy3d/paths/`.

## Outcome and source of truth

From the title screen, open **WORKSHOP**, choose a mode, objective, and scoring
rule, then start or resume a run. Session Stats can compare an ended run's
points under another compatible rule while preserving its original result.

This packet supplies the concrete UI, integration, command, and verification
decisions for [P002_EXPERIMENT_RULES.md](P002_EXPERIMENT_RULES.md). That document
owns the exact mathematical/statistical semantics. The reviewed preset and
golden data are in `experiments/P002_PRESETS.json` and `P002_GOLDENS.json`.
Use **WORKSHOP** as the player-facing name everywhere; it replaces the older
`EXPERIMENT SETUP` working label. Work on the copied Paths code only.

The capability introduces one rule module and removes the existing mutable
Hunt score authority. This positive feature work is authorized by the user;
it is not a parent engine cleanup or a request to split unrelated files.

## 1. Ownership and header dependency

Add `src/runtime/experiments/RunRules.hpp/.cpp` to `paths_model`.

- `RunRules.hpp` owns mode/rule/objective identifiers, descriptors, `RunConfig`,
  validation results, objective/score/stat result types, and preset factories.
  Forward-declare the two mode run-record types; do not include their model
  headers here.
- The two mode headers may include RunRules for their frozen config values.
  RunRules.cpp includes those model headers and evaluates their const records.
  This prevents a model/rule-header include cycle.
- Promote the existing two-value app mode enum to this pure header as
  `paths::ModeId { Guided, Hunt }`. Migrate local consumers to it and remove
  the old enum. Do not add an alias plus a second conversion policy. Serialized
  mode IDs remain `guided` and `hunt`.
- Keep established summary functions as the owners of their current measures.
  RunRules can consume them and add goal, score, banking, and session views;
  do not give “first-try correct” a second definition in a UI loop.
- UI and report adapters invoke the same pure score/stat functions. Evaluating
  twice is safe and does not require a stored score cache.

RunConfig stores owned identity strings or values with a guaranteed static
catalog lifetime, explicit versions, and complete parameter values. Parameters
may be signed integers while in a draft so an invalid negative entry can be
reported accurately. Only a successful canonical validation can attach a
config to a run. Validate modes, content identities/versions, relevant
parameters, and bounds. Inactive parameter fields must be zero/empty in the
canonical accepted config; preset/score-type factories supply those defaults.

The five named presets remain conveniences. `hunt_even` and `hunt_classic`
both select the same `hunt_bank_v1` algorithm with different frozen arrays.
Custom parameter edits do not manufacture a new algorithm version or change
the global preset definition.

## 2. Model changes and run lifecycle

Both run records receive a frozen RunConfig and explicit `opened` and
`ended_by_player` facts. Keep their existing completed/finished semantics.
The session stores a `lastAcceptedStartRequestId` counter, initially zero.

`start_new_run <request_id>` uses a positive uint64 ID, strictly increasing
within that mode for this process session. Equal or older IDs reject as
`start_request_not_new` without any mutation. This also covers a delayed
replay after other requests were accepted, without an extra request-history
database. UI activations use that mode's next ID; overflow rejects clearly.

The start operation proceeds atomically:

1. Check request-ID freshness, validate the complete selected config, and
   preflight the existing navigation/review admission rules.
2. If the initial record has never been opened and has no evidence, configure
   it in place without an empty archive or false prior-exposure claim.
3. Otherwise archive the current record once. An incomplete run retains that
   status and gets `ended_by_player=true`; a completed run keeps completed
   status. Do not describe it as completed merely because another run starts.
4. Create the next run with the frozen config, fresh mode state, and the
   correct per-content prior-exposure flag. Increment run number only when a
   prior record is archived. Commit the accepted request-ID high-water mark.
5. Enter Playing in the chosen mode. Guided starts on its question grid;
   opening the question records exposure. Hunt enters its board and records
   opening at that accepted entry action.

Mark opening at an accepted app/model command, never from a draw loop or a
Stats read. Entering an already open run is idempotent. This is evidence that
the app opened the content, not a claim about biological attention.

The existing in-mode Practice Again/New Run actions reuse the finished run's
frozen config and remain completion-gated. Workshop Start New Run is the
explicit route for ending an incomplete run and starting with new settings.
Both use the same model initialization/archive implementation; there is no
parallel reset that fabricates a different history.

Draft selection and Workshop/Stats visits are allowed while a Hunt review is
open. The current guard still rejects opening/resuming/starting a different
mode until that explanation is closed or released. An explicit new Hunt run
may archive its own incomplete run; retain the old review/evidence in that
archive. Preflight this before changing any mode's run so a rejected navigation
cannot reset a hidden session.

Add the Hunt bank events exactly as specified in the rule architecture, and
delete its mutable score field and point calculation in the same change.
An accepted ClearReady captures its nonempty row mask before clearing. Every
bank has the next sequence number, and masks are disjoint. Released/incorrect
rows never enter a bank event.

## 3. Workshop screen, exact choices, and keyboard

Add `Workshop` to the existing Paths screen enum and extend the existing app
dispatcher. Keep one active gameplay mode. A separate `workshopDraftMode`
chooses which draft to edit; it does not switch the active run. Store a draft
per mode and preserve drafts when leaving this screen.

Title action: `WORKSHOP [W]`. Header: `PATHS / WORKSHOP`.
Subtitle: `Choose how this run works.`

Controls, in this order:

1. **Practice** — Guided Questions / Quick Hunt.
2. **Quick setup** — compatible preset choices. Selecting a preset replaces
   the complete draft with that exact preset. Selecting a mode restores its
   last draft, initialized to calm Guided or classic Hunt on first use.
3. **Goal** — Guided: Finish the question / Complete a number of layers;
   Hunt: Finish the pack. The layer-count control appears only for that goal
   and accepts 1..current question's step count. Default custom count is 3
   for the carried seven-step question, clamped by the factory if future
   content has fewer steps.
4. **Scoring** — Guided: Off / Points for completed layers; Hunt: Off /
   Points for banked rows. Changing score type loads that type's defaults.
   Choosing a named preset reloads it exactly, including parameters.
5. Guided progress parameters: **Points per layer**, 0..1000. Hunt banking
   parameters: six integer reward fields labelled **1 row together** through
   **6 rows together**, each 0..1000000. Classic/even presets populate these.
6. A wrapped rule explanation generated from the validated selected rule and
   its parameters. Do not keep a hardcoded classic reward sentence when the
   array changes. Guided completion text explicitly includes revealed answers.

Footer: `START NEW RUN`, `RESUME CURRENT RUN`, `BACK TO MODES`.
When a prior run is open/incomplete, show:
`Start New Run keeps the current work in session history and starts fresh.`
When the draft differs from the existing run, show:
`Resume uses that run's original settings. These settings apply to a new run.`

Start is unavailable while a draft has an invalid field. Show the validation
message by that field; do not silently clamp typed invalid values or start
using the previous valid value. Raw text-edit buffers may remain in UI state;
the canonical draft/config validator owns what the numbers mean.
Resume is available only after the mode's run has been opened. A completed
run resumes its summary, not a fresh attempt. Back discards no draft or run.

Use normal ImGui keyboard navigation within Workshop: Tab/Shift-Tab move
focus, arrows change a choice, Enter activates the focused action, and Escape
returns to Title unless first consumed by an active text edit. Enable
navigation for this screen while retaining the existing game input contexts.
Suppress app hotkeys while an input field consumes text. Pointer and script
changes use the same semantic actions and validation as keyboard changes.

At 1024×768 / 150%, use one central vertical scroller with the heading and
footer outside it. Wrap descriptive text and bank-reward rows; do not put
six tiny inputs on one line. Retain the corrected P001 font/option sizing.

## 4. Goal and score presentation during play

Near the mode header, show the current frozen goal, progress/target with its
unit, and `Scoring off` or the enabled score. Differentiate 0 points from
disabled scoring. Rule details are readable on request from the frozen config,
not from whichever draft was most recently edited.

Goal reached does not complete the question through a second route. A small
layer goal displays `GOAL REACHED`, `CONTINUE PRACTICING`, and `BACK TO MODES`.
Continuing dismisses the banner and preserves the current step; it does not
select an answer or bypass the usual Next Step action. The objective result
remains reached, but the dismissed banner must not reappear every frame.
Question/pack completion still follows its canonical mode model.

Keep question evidence labels and help choices intact. Changing points cannot
turn a revealed answer into a correct checked attempt or a released Hunt row
into a banked correct row.

## 5. Stats and score comparisons

Session Stats lets the user choose current or archived run by `(mode, run
number)`. Never store a raw pointer to a vector element across archive growth.
Display the frozen config, mode-specific counts, original objective result,
and original score for the selected record.

For a completed or explicitly ended run, offer **COMPARE SCORING**. Its
choices are distinct compatible scoring configurations from the initial
presets: Guided Off / Progress at 1 per layer; Hunt Off / Classic / Even.
Deduplicate presets with identical scoring parameters and different goals.
This first comparison view need not add a second custom-parameter editor.

Display `Original run` and `Comparison` with both rule descriptions and points.
Caption: `Same recorded work. The original result is unchanged.`
Only replace the score spec for this calculation; retain the source run's
content, evidence, and original objective. A comparison is not an estimate of
how the user would have played with different incentives.

The shared dispatcher stores only a comparison request containing mode, run
number, and alternate preset ID. Resolve the const record and evaluate when
needed. `CLEAR COMPARISON` clears that request, not a run. Unknown runs,
unfinished active runs, and incompatible scores reject with no learning-state
change. A scoring-disabled comparison returns null points, not zero.
Use `score_mode_mismatch` for a known alternate scoring preset that does not
apply to the selected source mode; the supplied script checks that reason.

## 6. CLI and report contract

Add these exact commands through the existing parser/dispatcher:

```text
open_workshop
preset <preset_id>
start_new_run <positive_uint64_request_id>
resume_mode guided|hunt
compare_score guided|hunt <positive_run_number> <preset_id>
clear_comparison
```

`preset` opens Workshop on that preset's draft mode and replaces that draft;
it does not open gameplay. Start uses the selected Workshop draft. Resume may
be requested from Title/Workshop and restores that mode's existing run.
Compare is available on Stats and only for an eligible selected record.
Pointer controls for individual parameters also dispatch typed app actions;
a general config-file language is not needed for this capability.

Malformed arity, unknown IDs, and integers outside the token type fail before
native startup. Known but incompatible combinations, stale request IDs, and
unavailable actions reject at runtime through the canonical owner. Existing
commands retain their behavior. Add the new syntax to `paths --help`.

Report schema 2 keeps all P001 learner fields and adds:

- frozen config and `opened`/`ended_by_player` per run;
- the raw ordered Hunt banks;
- per-run `objective`, `score_result`, and scoped `statistics`;
- optional top-level `score_comparison` containing source mode/run number,
  alternate score specification, original result, and comparison result.

For classic Hunt, the legacy numeric `score` field keeps its old value through
the new evaluator. For a scoring-disabled Hunt run it is null and the report
version makes that change explicit. There is no hidden classic score presented
as that run's selected scoring result. Learning fields keep their indices and
meanings. Reports and UI do not each implement their own score formula.

## 7. Files, tests, and exact reference runs

Expected new production files: only RunRules.hpp/.cpp. Authorized existing
files: the two mode models, the copied UI files, app/main.cpp, and Paths'
CMake/test wiring. Add `tests/paths_workshop_tests.cpp` linked to paths_model
for pure rules/lifecycle cases and extend existing input tests for app routes.
Do not change the native host, borrowed renderer helpers, third-party code,
parent project, or new source-card runtime content in this slice.

Derive the final target names from the completed P001 graph. The intended
focused gate builds Paths plus its Hunt, Guided, input, and Workshop tests;
runs those affected tests; and makes a bounded Workshop/Stats offscreen
capture. The three carried model/input targets must be rerun because their
score/config/navigation boundaries change. No broad parent CTest is relevant.

Required independent results:

- Classic Hunt banked in groups of three: **840** original, **600** under
  even-bank comparison; all six rows initially correct; two disjoint bank
  events; two comparisons/read-only renders leave the same evidence.
- Guided progress: one first-try layer, one corrected-after-retry, one shown
  layer gives **3 points**, **3 resolved layers**, **2 incorrect checks** in
  the supplied script, and **question not completed**. Those three layers
  satisfy the three-layer objective if that objective is selected.
- Ending that partial Guided run archives it as incomplete. A new calm run
  has scoring disabled, prior exposure true, and no borrowed attempts.
- Repeated Start request ID cannot duplicate that archive. Resume after
  changing a draft keeps the earlier run's frozen config.
- No-check statistics have a null rate and `No checked answers yet`.
- Goal reached before final Continue does not falsely set completed.
- Invalid/duplicate bank masks return evidence errors as in the golden file.

Ready-to-run script specifications are under `experiments/P002_SCRIPTS/`.
Each is intended for a fresh process with explicit `--start-mode title`.
Their expected reports are in `P002_SCRIPT_EXPECTATIONS.json`. They are not
runtime evidence until the built application has actually executed them.

Capture Workshop with Guided settings and with six Hunt reward fields at
1024×768 / 150%, plus the score comparison at an ordinary size. Check the
actual font, labels, rule explanation, and accessible footer. Preserve
external UI-recorded/nonuniform-pixel receipts. No visible launch is requested.

Update Paths README/WORKSTREAMS and send the completed brief to the planner,
with the canonical new score owner, removed accumulator, implementation LOC,
exact checks, JSON/capture paths, and remaining manual judgment. Stop at the
Workshop checkpoint. Card 002 is the following content capability.
