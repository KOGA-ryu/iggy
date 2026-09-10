# Trigonometry pilot review

Coordinator update, 2026-09-10: this pilot has been corrected, jointly checked
and locally published. See the [integrated subject review](../../../../docs/SUBJECT_PILOT_REVIEW.md)
for current status and review overrides. Visual acceptance remains with the
user. The worker delivery record below describes the earlier candidate and
retains its original hashes and pending-review statements as history.

## Scope and status

- Candidate package: `pilot_trigonometry_sine`, version 1; chapter `topic_0032` (Unit Circle Framework).
- Authored: one `lesson.v2` reading and six `choices.v1` questions in the fixed role order. The repair card has two decisions, for seven decisions total.
- Mathematically checked: passed; the pure provider uses exact `Fraction` values for theta/pi, algebraic isolation, special-angle membership, periodic branches, interval filtering, and a unit-circle completeness argument.
- Compiled: passed by the shared subject gate.
- Model-replayed: passed by the pure `paths_learning_document_tests` question-batch route (six of six questions, fourteen wrong choices, and no windows).
- Teaching-reviewed: author self-review complete; coordinator prose review remains required.
- Published: not performed.
- Visually accepted: pending user confirmation after coordinator integration.

## Instruction and format inputs loaded

- `AGENTS.md`
- `content/authoring/parallel/AGENTS.md`
- `content/authoring/parallel/trigonometry/AGENTS.md`
- `content/authoring/parallel/trigonometry/PILOT.md`
- `content/authoring/parallel/trigonometry/sequence.json`
- `docs/ARCHITECTURE.md` (Library/textbook and canonical question-owner sections)
- `docs/WORKSTREAMS.md` (Parallel subject authoring packets)
- `docs/PARALLEL_QUESTION_AUTHORING.md`
- `docs/EXERCISE_ROLES.md`
- `docs/LEARNING_DOCUMENTS.md` (lesson.v2, choices.v1, disclosure and feedback syntax)
- `docs/QUESTION_PRACTICE_FORMAT.md` and `docs/QUESTION_AUTHORING_WORKFLOW.md`
- `content/authoring/learning/matrix_reasoning/lesson.md.in`, `questions.paths.md.in`, and `content/authoring/learning/chapter.paths.md.in` as the checked six-role/textbook structure references.

## Source and authorship receipt

Primary teaching reference checked on 2026-09-10: OpenStax, *Precalculus 2e*, [7.5 Solving Trigonometric Equations](https://openstax.org/books/precalculus-2e/pages/7-5-solving-trigonometric-equations). Specifically checked: “Solving Linear Trigonometric Equations in Sine and Cosine,” its statement that sine has period 2pi and that all-turn solutions add `2kpi` for integer `k`; Example 2 (`sin(t)=1/2`, branches pi/6 and 5pi/6); and the interval form in the section’s linear sine exercise. The packet's own source receipt named the same URL/date.

All prose, the top-point worked example, questions, distractors, feedback, disclosures, and certificate code are original Paths material. No external exercise wording or figure was copied. `authoring.json` covers the reading plus every six fixed question identity with the portable original-material URI `paths:original/pilot_trigonometry_sine/v1`.

## Teaching and notation review

The reading defines radians, pi, theta, sine as vertical unit-circle coordinate, solution set, integer parameter, and excluded endpoint before their exercise use. It distinguishes an angle from its sine value and from the full set of solutions. It states that a principal inverse value would be only a starting angle and does not use inverse-function notation as assumed prior knowledge.

The conditions are explicit: all angles are radians; the interval is `[0,2pi)`; zero is included and `2pi` excluded; the algebraic divisor is the nonzero number 2; and reflection across the vertical axis, not horizontal-axis/opposite-point motion, preserves sine. The reading's separate worked example is `sin(theta)=1` on the stated interval, with independently closed Hint, Answer, and Solution disclosures. It explains the coincident reflected top point and checks periodic shifts with `k`.

## Decision and distractor review

| Role | Checked decision | Distractor diagnoses |
| --- | --- | --- |
| read_notation | Interpret `[0,2pi)` exactly. | Reverses both endpoint decisions; loses included zero. |
| worked_check | Supply `5pi/6` after the known `pi/6`. | Repeats the supplied angle; chooses a below-axis negative-sine angle. |
| choose_next_step | Isolate `sin(theta)=1/2`. | Drops coefficient 2; introduces an unsupported sign change. |
| explain_step | Select `pi-alpha`. | Opposite point changes height sign; horizontal-axis reflection changes height sign. |
| repair_error | First choose L2, then `{0,pi}`. | Mislabels correct all-real branch; identifies later consequence; loses zero; includes excluded `2pi`. |
| independent | Select `{7pi/6,11pi/6}`. | Uses positive-half angles; omits the second negative-half branch. |

There are fourteen specifically diagnosed wrong choices. Gold givens label L1-L3 deliberately incorrect. The independent prompt states neither an isolation route nor a quadrant; it remains a one-step multiple-choice response with no hint.

## Mathematical evidence and boundaries

`certificates.py` derives each case from `sequence.json`, not from the authored templates or answer IDs. It represents theta/pi as `Fraction`, derives the required sine level with rational algebra, validates exact special-angle membership separately from the enumerator, applies periodic branches, filters `[0,2pi)`, deduplicates, and records the analytic completeness fact: a horizontal line at each supported height has exactly the stated unit-circle intersections, while `2kpi` accounts for other turns. It rejects changed case fields, a mixed degree field, endpoint closure, a reflected-sign case, and an unsupported level. Its repair evidence preserves zero and excludes `2pi`; its fresh pair verifies `2(-1/2)+1=0` for both angles.

Bounded limitations: this is not a general trigonometric solver, inverse-trigonometry lesson, arbitrary-identity family, degrees workflow, or graphical interaction. It covers only sine levels `0`, `1/2`, `-1/2`, and the separate worked level `1`, all on `[0,2pi)`. A useful future figure is a unit circle with a selectable horizontal-height line and two marked intersections; it should display only in reading/help, before no fresh-answer response, and would not become a checker or new control owner.

## Verification receipt

Run only from `/Users/kogaryu/iggy3d/paths`:

```sh
python3 -B tools/check_authoring_pilot.py --packet-only
PYTHONPATH=tools python3 -B content/authoring/parallel/trigonometry/certificate_tests.py
python3 -B tools/check_authoring_pilot.py --subject trigonometry
```

Results: the packet-only command passed before authoring, reserving four readings and 24 IDs. The certificate command passed six certificates, seven decisions, fourteen wrong choices, and five rejection cases. The subject gate passed compilation, six model-replayed routes, all feedback/provenance checks, and wrote the immutable candidate at `build/parallel-authoring/trigonometry/1c80503b382eadb8865a363748814f03681e00acb149f50a8cdbc31c09e172dc/authoring` with receipt `build/parallel-authoring/trigonometry/1c80503b382eadb8865a363748814f03681e00acb149f50a8cdbc31c09e172dc/checks/c72709a7f652d75936bc67377342d70fcafe8614ac56301a6cd0b5ac0c638ddd/verification.json`. Publication, activation, app launch, screenshots, and visual acceptance are intentionally outside this authoring pilot.
