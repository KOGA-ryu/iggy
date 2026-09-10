# Algebra pilot review: Understand each balanced move

Coordinator update, 2026-09-10: this pilot has been corrected, jointly checked
and locally published. See the [integrated subject review](../../../../docs/SUBJECT_PILOT_REVIEW.md)
for current status and review overrides. Visual acceptance remains with the
user. The worker delivery record below describes the earlier candidate and
retains its original hashes and pending-review statements as history.

## Scope and status

- **Authored:** candidate source written for `pilot_algebra_balance` version 1: one `lesson.v2` reading, six `choices.v1` cards, seven decisions and fourteen diagnosed distractors.
- **Mathematically checked:** yes, by the recorded pure certificate suite below; coordinator inspection of the provider remains required.
- **Compiled:** yes, by the shared headless candidate gate.
- **Model-replayed:** yes, by that gate's six prepared-question routes, with 14 wrong-choice checks and save replay.
- **Teaching-reviewed:** self-reviewed against the packet; coordinator prose review remains required.
- **Published:** no. The worker must not export, install or activate this candidate.
- **Visually accepted:** no. The coordinator/user performs the specified native manual check after integration.

This is one bounded real-linear-equation pilot for `topic_0003`, not a claim of complete algebra coverage, written work, mastery, four prepared-choice support levels or acceptance.

## Instruction paths loaded

- `/Users/kogaryu/iggy3d/paths/AGENTS.md`
- `/Users/kogaryu/iggy3d/paths/docs/ARCHITECTURE.md`
- `/Users/kogaryu/iggy3d/paths/docs/WORKSTREAMS.md`
- `/Users/kogaryu/iggy3d/paths/docs/PARALLEL_QUESTION_AUTHORING.md`
- `/Users/kogaryu/iggy3d/paths/content/authoring/parallel/AGENTS.md`
- `/Users/kogaryu/iggy3d/paths/content/authoring/parallel/algebra/AGENTS.md`
- `/Users/kogaryu/iggy3d/paths/content/authoring/parallel/algebra/PILOT.md`
- `/Users/kogaryu/iggy3d/paths/content/authoring/parallel/algebra/sequence.json`
- `/Users/kogaryu/iggy3d/paths/docs/EXERCISE_ROLES.md`
- `/Users/kogaryu/iggy3d/paths/docs/QUESTION_PRACTICE_FORMAT.md` (shared textbook presentation standard)
- `/Users/kogaryu/iggy3d/paths/docs/LEARNING_DOCUMENTS.md` (native `lesson.v2` and `choices.v1` syntax)

## Teaching and notation review

The reading's block sequence is start, terms, balance rule, signed-constant example, division condition, separate worked example, deliberate-error example, practice orientation and summary. It defines equality, solution, unknown, coefficient, signed added constant and inverse operation before their use. The notation inventory is `a`, `b`, `c`, `x`, `u`, `v`, `d`, `r`, `x1`, `x2`, and the real-number domain; each is introduced with a role. Displayed equations have local labels when prose refers back to them.

The distinct worked example is `4x+3=11`, never the final exercise. Its Hint, Answer and Solution are independently closed. The solution shows `4x+3-3=11-3`, `4x=8`, division by 4, substitution `4(2)+3=11`, and a completeness argument. The final independent card uses `-6x+5=14`; its public title, given and prompt do not disclose an answer or a method.

The restriction `a != 0` is explained through undefined division and the loss of a restriction under zero multiplication. The related `0x=0` and `0x=1` cases are explanatory boundaries only. For any allowed `a`, if two values solve `ax+b=c`, subtracting the two equations gives `a(x1-x2)=0`; nonzero `a` implies `x1=x2`. This establishes uniqueness beyond one successful substitution.

## Question and distractor review

| Role | Certified original case and accepted decision | Distractor purposes |
| --- | --- | --- |
| `read_notation` | `-4x+7=19`; `(-4,7,19)` | drops the coefficient sign; swaps added and right-hand constants |
| `worked_check` | `5x-6=9`; `9+6=15` | leaves the right side unchanged; treats removal of `-6` as subtraction |
| `choose_next_step` | `-3x+4=13`; subtract 4 from both sides | reversible addition that misses the stated cancellation goal; one-sided subtraction that changes equivalence |
| `explain_step` | recover `-4x=12`; multiply both sides by `-4` | repeats division; zero multiplication erases the restriction |
| `repair_error` | `3x-5=7`; first error `L1`, then `7-(-5)=12` | identifies later consequence instead of first error; repeats the old/right-side values |
| `independent` | `-6x+5=14`; `x=-3/2` | loses the sign; substitutes a value with residual 45 |

The repair card explicitly labels L1–L3 as a deliberately incorrect attempt. It proves `x=2/3` has original residual `-10`; after repair, `x=4` has residual `0`. The independent choices have original residuals `-18`, `0`, `45`, respectively. The operation card distinguishes a valid but goal-missing operation from an invalid one-sided change.

## Sources and authorship

- OpenStax, *Elementary Algebra 2e*, [2.1 Solve Equations Using the Subtraction and Addition Properties of Equality](https://openstax.org/books/elementary-algebra-2e/pages/2-1-solve-equations-using-the-subtraction-and-addition-properties-of-equality), accessed 2026-09-10. Checked: definition of a solution by substitution; addition/subtraction properties preserve equality when applied to both sides.
- OpenStax, *Elementary Algebra 2e*, [2.2 Solve Equations using the Division and Multiplication Properties of Equality](https://openstax.org/books/elementary-algebra-2e/pages/2-2-solve-equations-using-the-division-and-multiplication-properties-of-equality), accessed 2026-09-10. Checked: multiplication/division equality rules and the nonzero condition for division.

All explanatory prose, equations, examples, distractors and questions are original Paths material. The sources were used only to verify definitions and conditions; no exercise wording or textbook prose was adapted.

## Certificate boundary and expected evidence

`certificates.py` derives every given, reached state, three labels, accepted label and evidence fact from the immutable role case. It does not read either Markdown template. It uses `Fraction` for solutions, substitutions and residuals. It rejects changed fixed givens, zero/non-bounded coefficients, an incorrect signed subtraction, wrong reached states through the shared compiler comparison, noninverse operations, duplicate/equivalent choices, wrong keys and invalid independent candidates. The pure tests also exercise zero coefficient, altered givens, negative-subtraction, one-sided operation, zero scaling, duplicate choices, the repair's wrong candidate and uniqueness logic.

Commands run from `/Users/kogaryu/iggy3d/paths`:

```sh
python3 -B tools/check_authoring_pilot.py --packet-only
PYTHONPATH=tools python3 -B content/authoring/parallel/algebra/certificate_tests.py
python3 -B tools/check_authoring_pilot.py --subject algebra
```

Results: packet-only accepted the four pinned subjects and 24 reserved question IDs; `certificate_tests.py` ran six tests successfully; the subject gate accepted six routes, 14 wrong-choice checks and save replay with zero windows. The gate wrote these immutable candidate/evidence paths:

- `/Users/kogaryu/iggy3d/paths/build/parallel-authoring/algebra/a2cd670d71f6f4eda7dcf1dd882e562d91811e0061ec057c67dd401fc4b8d4f2/authoring`
- `/Users/kogaryu/iggy3d/paths/build/parallel-authoring/algebra/a2cd670d71f6f4eda7dcf1dd882e562d91811e0061ec057c67dd401fc4b8d4f2/checks/6b03dd0e54177f76051feaad0aa12ee475c5984631b3511a6a0aaa1f78c2fc8c/verification.json`

Gate-recorded input hashes: `sequence.json` `2aaf7a118f43e6f97c8c728f55838b01bcedf89f491fae7e0a4d91d9afbbe569`; `lesson.md.in` `b11117869a904d5d844d00f20c714a042e08025058de60553b50f338f6f88098`; `questions.paths.md.in` `382c001ed19933ed87c91f2e9407c583400e09c007e7e15ede7932df67ee2978`; `certificates.py` `4621c123c52604ede1e5414715d2ad5f9f2069b41b86fe11bd593e0695018533`; `authoring.json` `025c5546441b8897382bc867c4127bf8efe716854723a7e973088d756feedab8`. The shared gate's read-only target and model hashes were `7fcea0db0b6a90468161b957c4bee81da08d88dd1e22a02bdf6fb7a0bb3ead3a` and `bd672dcdfe7a039d0932a68e4aca88bef3f45ff8e9601ab6087ccca4b77051a3`, respectively. No output should be called published or visually accepted.

## Future figure and remaining gaps

A future non-answer-revealing balance-scale figure could show equal containers before and after the same signed operation on both sides; it should expose only the operation, not an exercise answer. No figure binds to this text-only pilot.

Remaining limits: this pilot does not cover zero-coefficient no-solution/all-solution cases as playable content, variable terms on both sides, inequalities, quadratics, denominators, alternative answer forms, written proof grading, long-term retention or accessibility/visual layout acceptance. Coordinator review must inspect prose, certificate design, all-subject identity collisions and the post-integration gold/cyan/green manual checks.
