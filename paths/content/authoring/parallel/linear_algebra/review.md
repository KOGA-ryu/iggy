# Linear Algebra pilot review

## Scope and instructions loaded

This is the one bounded `pilot_linear_algebra_rows` candidate: one `lesson.v2` reading and six `choices.v1` questions in fixed role order. I loaded these instruction sources in full before authoring:

- `/Users/kogaryu/iggy3d/paths/AGENTS.md`
- `/Users/kogaryu/iggy3d/paths/content/authoring/parallel/AGENTS.md`
- `/Users/kogaryu/iggy3d/paths/content/authoring/parallel/linear_algebra/AGENTS.md`
- `/Users/kogaryu/iggy3d/paths/content/authoring/parallel/linear_algebra/PILOT.md`
- `/Users/kogaryu/iggy3d/paths/content/authoring/parallel/linear_algebra/sequence.json`
- `/Users/kogaryu/iggy3d/paths/docs/PARALLEL_QUESTION_AUTHORING.md`
- `/Users/kogaryu/iggy3d/paths/docs/ARCHITECTURE.md`, `EXERCISE_ROLES.md`, `QUESTION_PRACTICE_FORMAT.md`, and `LEARNING_DOCUMENTS.md`

No runtime, UI, shared checker, assignment, source-package, asset, or publication file was changed.

## Source and original authorship

Primary teaching reference checked 2026-09-10: OpenStax, *College Algebra 2e*, [7.6 “Solving Systems with Gaussian Elimination”](https://openstax.org/books/college-algebra-2e/pages/7-6-solving-systems-with-gaussian-elimination), specifically “Writing the Augmented Matrix of a System of Equations” and “Performing Row Operations on a Matrix.” It was used to verify coefficient/constant column order, the augmented bar, translation between rows and equations, and elementary row-operation conventions. The zero-row and contradictory-row boundary descriptions were checked against the section’s dependent and inconsistent examples. The lesson prose, the separate [1,1|3], [2,3|8] worked example, all six cases’ wording, and every distractor explanation are original Paths material.

## Teaching and mathematical review

Notation is introduced at first use: system, ordered pair, coefficient, constant, augmented bar, row label, replacement arrow, and prime. The lesson keeps x, y, constant order; makes the old-row evaluation and constant-column operation explicit; proves row-addition reversibility in both directions; explains nonzero scaling; and distinguishes a valid reversible move from one that misses an elimination goal.

The mandatory separate worked example calculates all three entries for each operation, gives independent closed Hint, Answer, and Solution disclosures, identifies `(x,y)=(1,2)`, and substitutes into both original equations. It also frames `[0,0|0]` and `[0,0|1]` only as boundaries, without claiming that this unique-system pilot solves singular families.

The six roles contain seven decisions and fourteen diagnosed wrong choices. Their purposes are: sign/column-role reversal; skipped or sign-reversed constant arithmetic; a reversible but noncancelling operation and a wrong destination; repeated versus inverse operation and zero scaling; earliest omitted constant operation followed by its wrong consequence; and reversed or numerically wrong ordered pairs. The independent card remains one multiple-choice step and does not disclose an intermediate method or its answer before the response.

The certificate provider reuses `build_question_batch.MATRIX_ROLE_CHECKERS`, which independently obtains each unique-system result using exact `Fraction` arithmetic, determinant/Cramer solution, direct original-row substitution, row-addition inverse, exact reached display, and option uniqueness. `certificate_tests.py` independently checks the full-system solution list `(5,3)`, `(2,1)`, `(5,2)`, `(4,3)`, `(3/2,1/2)`, the final residual pairs `(-2,-2)`, `(0,0)`, `(9/2,3/2)`, and the separate reading example. It also exercises the prescribed malformed/altered cases.

## Verification and remaining boundaries

Run from `/Users/kogaryu/iggy3d/paths`:

```sh
python3 -B tools/check_authoring_pilot.py --packet-only
PYTHONPATH=tools python3 -B content/authoring/parallel/linear_algebra/certificate_tests.py
python3 -B tools/check_authoring_pilot.py --subject linear_algebra
```

Results: the packet-only command passed with 24 reserved questions and four readings; `certificate_tests.py` printed `linear algebra pilot certificate tests: passed`; and the subject gate passed. The gate compiled the reading and six questions, model-replayed six prepared routes with zero windows, checked seven decisions and fourteen wrong choices, and wrote this immutable candidate:

`/Users/kogaryu/iggy3d/paths/build/parallel-authoring/linear_algebra/5717e777c313f0ab5ffbebd20cb51e9da988d15448f5f69a7f851cc11042f449/authoring/`

Its immutable verification receipt is:

`/Users/kogaryu/iggy3d/paths/build/parallel-authoring/linear_algebra/5717e777c313f0ab5ffbebd20cb51e9da988d15448f5f69a7f851cc11042f449/checks/092bd5daa6d0dabc24d78189ed3af26ce8b1aeef8e43912614e9c85745c74184/verification.json`

Teaching review is authored and self-reviewed, but coordinator prose review remains required. Status: authored yes; mathematically checked yes; compiled yes; model-replayed yes; published not performed; visually accepted pending user confirmation. This candidate intentionally does not cover singular, inconsistent, free-variable, larger, or written-work systems, nor does it claim mastery.
