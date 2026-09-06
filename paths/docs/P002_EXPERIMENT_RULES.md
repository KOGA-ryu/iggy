# P002 — objectives, statistics, and scoring experiments

Status: user-authorized next capability after P001. The concrete implementation
and UI contract is [P002_BUILD_PACKET.md](P002_BUILD_PACKET.md). P001's
standalone build finishes before this Workshop implementation starts.

## Product behavior

The title screen gains `WORKSHOP`. Select a mode, an objective, and a
scoring rule. A small explanation says exactly what the selected rule rewards.
`START NEW RUN` freezes those choices for a run. `RESUME CURRENT RUN` restores
that run's original choices. Changing the setup never retroactively changes
the original result of work already done.

The first five presets are fully specified in
`experiments/P002_PRESETS.json`: calm Guided practice, Guided progress points,
classic Hunt banking, even Hunt banking, and a three-layer Guided objective.
These are configurations of two working modes, not five separately implemented
games. Keep the two title cards; show the selected preset below the relevant
mode when applicable.

No timer, speed bonus, lives, negative score, lockout, durable profile, random
problem generation, or free-response algebra judge is required here. Those
are additional behavioral capabilities if a later experiment needs them.

## 1. Canonical types and placement

Add `src/runtime/experiments/RunRules.hpp/.cpp` to `paths_model`. This single
module owns the rule registry, compatibility, config validation, objective
evaluation, score evaluation, and statistical projections for this initial
finite set. It has no SDL, ImGui, Vulkan, filesystem, or clock dependencies.
Do not create one file or one subclass per preset.

The principal value types are:

```text
ModeId                 = guided | hunt
ObjectiveId            = finish_question_v1 | resolve_layers_v1 | finish_hunt_pack_v1
ScoreRuleId            = practice_none_v1 | guided_completion_v1 | hunt_bank_v1
RunConfig {
  mode_id, mode_version,
  content_id, content_version,
  objective_id, objective_version, objective_parameters,
  score_rule_id, score_rule_version, score_parameters
}
ObjectiveResult { progress, target, reached, unit }
ScoreResult { enabled, optional<int64_t> points, label, error }
RunStats { mode-scoped factual counts; explicit denominator fields }
```

An objective ID includes its algorithm version. Store the separate version
field too for an explicit report contract; inconsistent pairs reject. Preset
IDs name UI conveniences, not evaluation algorithms. The full frozen parameter
values are retained so later edits to a preset cannot change a historical run.
Use integer points and bounded integer parameters; no floating rounding policy
is needed for these rules.

The registry is a constexpr/declarative table keyed by IDs with compatible
modes, parameter bounds, player description, and an exhaustive evaluator
selection. Invalid mode/objective/score combinations reject in the canonical
config validator, whether requested by pointer, CLI, or a later file loader.
The UI consumes validation results and never invents a fallback rule.

## 2. Evidence additions and ownership repair

Guided step records already preserve the needed choice/check/retry/reveal facts.
Its score and statistics are derived from those records. Do not create a
second answer-event history beside them.

Hunt's existing numeric score loses the grouping information needed to try a
different banking rule. Make one necessary evidence addition:

```text
HuntBankEvent {
  sequence: uint32,       // 1..N within this run
  cleared_row_mask: uint8 // six available row bits; at least one
}
HuntRunRecord {
  ... existing row records,
  frozen_config,
  vector<HuntBankEvent> banks
  // remove the independently maintained numeric score
}
```

Only an accepted `HuntSession::dispatch(ClearReady)` emits a bank event. Before
changing row states it captures exactly the Ready rows that will be cleared.
An event contains no points: it records what the player banked. The same row
cannot appear in two bank events; reviewed/released rows cannot enter one.
No Ready rows means rejected/no event. A second Clear after the first finds
none and cannot award or append again. Restart/ending a run freezes the events
inside that run's archive, rather than clearing a global banking ledger.

In the same change, delete the point schedule/calculation and mutable score
field from the Paths copy of HuntSession. Replace each live UI/report consumer
with the single `evaluateScore` result from RunRules. Preserve the report's
legacy `score` value for classic Hunt through that projection. Do not retain
the old accumulator as a cache or comparison authority.

The root parent engine copy is outside this workstream. It can remain an
attributed historical baseline; there is only one production scoring owner
inside the Paths product.

## 3. Exact scoring rules

| Rule | Compatibility | Parameters | Evaluation |
| --- | --- | --- | --- |
| practice_none_v1 | Guided, Hunt | none | enabled=false, points=null |
| guided_completion_v1 | Guided | points_per_layer in 0..1000, default 1 | completed/resolved layer count × points_per_layer |
| hunt_bank_v1 | Hunt | six nonnegative rewards, each 0..1000000 | sum rewards[popcount(bank.mask)-1] over valid bank events |

For Guided completion points, a layer is resolved by either a correct Check or
Show Me. A wrong checked answer alone earns no completion point. A revealed
layer earns exactly the same completion point as a player-resolved layer. The
visible description must say `Points for completing layers, including answers
you choose to see.` This is a progress experiment, not an accuracy grade.

The classic Hunt reward array is `[100,240,420,520,620,720]`. Even banking is
`[100,200,300,400,500,600]`. The algorithm is the same; the parameter arrays
are frozen separately. Six rows banked together earn 720 in classic Hunt;
two groups of three earn 840. That is the existing rule's actual behavior.
Do not describe 720 as the maximum for an entire run; it is the largest single
bank reward in this preset. The even preset gives 600 for either grouping.

Score evaluation is a pure projection. Calling it twice cannot award points
twice. Invalid bank masks, duplicate row membership, bad sequence order, or
events containing a row whose first check was incorrect return an evidence
error; they do not silently produce a plausible score. The live model should
prevent such states, and the evaluator protects the report/import boundary.

An explicitly zero-points rule still has `enabled=true, points=0`. Practice
with scoring disabled has `enabled=false, points=null`. Present those as
`0 points` and `Scoring off`, respectively. Missing information is not zero.

## 4. Exact objective rules

| Objective | Compatibility | Evaluation |
| --- | --- | --- |
| finish_question_v1 | Guided | target=step_count; progress=resolved steps; reached only when the model's completed flag is true after final Continue |
| resolve_layers_v1 | Guided | target=selected count in 1..step_count; progress=min(resolved steps,target); reached when enough steps resolve |
| finish_hunt_pack_v1 | Hunt | target=6; progress=Cleared or Released rows; reached when all six are terminal |

An objective is a goal indicator, not an alternate model transition route.
Meeting a small layer goal displays `GOAL REACHED` with `Continue practicing`
and `Back to modes`. It does not mark the mathematical question complete or
automatically replace its current phase. A final-step answer that has resolved
but has not yet been advanced to the summary can meet resolve_layers, while
finish_question still waits for the final Continue. Reports preserve both
`objective_reached` and the original model's `completed`/`finished` field.

With a three-layer objective, work beyond that goal is allowed. Score and
statistics continue to reflect all actual resolved layers; only the displayed
goal progress clamps to its target. This prevents the goal indicator from
becoming a second count of learner work.

## 5. Starting, resuming, and ending runs

Maintain a setup draft per mode in app state. Draft edits are reversible
presentation choices and create no attempt. Validating a draft does not start
a run. `START NEW RUN` is the explicit semantic command that copies the
validated config into a new run. Rejected validation leaves both draft and
active run evidence unchanged.

If a mode has an untouched initial record that has never been opened, attach
the initial validated config without fabricating an archived practice run.
If an existing run has been opened or has evidence, starting another run
archives it once with an explicit `ended_by_player` marker and its existing
completed/unfinished status. This extends the model with an explicit new-run
command; it does not weaken the existing completion-only Restart command.
Two invocations carrying the same new-run request ID cannot archive twice.

Track accepted mode/question opening as content exposure in the model/app
boundary, not from an idle draw call. Prior exposure is true for a later run
of a content revision that had already been opened. A run configured but never
opened is not evidence that the learner saw the question. Do not equate a
previously configured session with actual completion or prior success.

Resuming restores the frozen config, selected choice, recovery state, and
current step. The title may show the draft differs and that the draft applies
to a new run. There is no permission dialog: Resume and Start New Run are
separate explicit actions with their consequences written on the screen.

## 6. Statistics and comparison views

Statistics derive from the authoritative run records, across current and
archived runs as selected by the player. Label the selection `Current run` or
`This session`. Do not silently mix repeated content with first exposure.

Guided statistics:

- checked attempts; incorrect checked attempts;
- first-try correct layers / layers with at least one checked answer;
- corrected-after-retry layers, shown layers, resolved layers / total layers;
- completed questions, unfinished archived runs, objective reached;
- prior exposure and rule/config identity.

Hunt statistics:

- checked rows and initial correctness / checked rows;
- explained rows, released rows, banked rows, bank batch sizes;
- finished packs, unfinished archived runs, objective reached;
- prior exposure and rule/config identity.

A zero denominator displays `No checked answers yet`, not 0% accuracy. Do not
sum layers and rows into an overall success percentage. A revealed Guided
layer is not a correct checked attempt. A released Hunt row is not a banked
row. These facts stay true under every scoring preset.

A completed/ended run can show `Compare scoring rules`. Evaluate a compatible
alternate config against the same immutable facts and label the display
`Comparison — original run unchanged`. Show both rule IDs/versions/parameters.
Do not compare an incompatible Guided score against Hunt evidence, or imply
that changing points tells us how the player would have behaved under the
alternate incentives. This is a replay of facts, not a counterfactual model.

## 7. UI, report, and script contract

Workshop presents mode, objective, goal count where applicable, score
preset, and the exact rule description. Default to calm Guided practice or
classic Hunt. Parameter editing is bounded by the canonical validator; the UI
shows its precise rejection next to the setting. Use semantic IDs everywhere.

Add script commands `preset <id>`, `start_new_run <request_id>`, and
`resume_mode <mode_id>`. Register valid IDs and arities in the existing parser.
Script syntax rejects before native startup; incompatible but known settings
produce a contextual rejection through the app/domain boundary. Record rejected
commands separately from learning attempts. Never treat a failed preset lookup
as a request for defaults.

Report schema version 2 adds the full frozen config, raw Hunt banks, run-end
status, objective result, score result, and scoped statistics. An optional
comparison result sits under its own field and cannot replace the original
score. Preserve existing learner fields and content versions. The supported
history is still process-local; on-disk learner persistence is a later packet.

## 8. Required proof and ownership checkpoint

The golden examples in `experiments/P002_GOLDENS.json` define exact independent
expected values. They are specifications, not engine execution evidence.

The focused tests must establish:

1. The same immutable facts produce different expected points under classic
   and even banking, while every learning statistic remains equal.
2. Two evaluations do not mutate records or double points. Repeated Clear and
   duplicate new-run request IDs cannot duplicate facts or archives.
3. The live old Hunt score accumulator and schedule are removed; UI and JSON
   both read RunRules and preserve classic values.
4. Guided wrong/retry/reveal distinctions survive all presets, with disabled
   score null, explicit zero score 0, and zero denominators handled explicitly.
5. Resume keeps original config; new setup affects only the next run;
   incomplete archives retain incomplete status. Goal reached is distinct from
   question completed.
6. One end-to-end pointer/keyboard/script run produces the same frozen config,
   raw evidence, objective, score, and stats. One small-scale capture makes
   setup controls and the rule description readable.

No broad parent tests or renderer repairs are part of this workstream.
Report canonical owner, removed score route, production delta, actual tests,
and remaining manual acceptance. Stop at the rule-experiment capability.
