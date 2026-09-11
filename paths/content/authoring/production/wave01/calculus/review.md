# Calculus production review

Contract revision 2 has one maintained generator, one finite recipe, one test
entry point, and ordinary editable lesson/question Markdown for each family.
The placeholder wrapper and pipe-delimited prompt parser have been removed.
Both families contain one reading and eighteen questions in three six-role sets.
The final packages remain unpublished.

The coordinator corrected the full difference-quotient identity, restored the
visible evaluation point in independent questions, and rebuilt the first-error
attempt with a correct full expansion followed by an incorrect subtraction.
Cubic terms are retained. The first repair explanation identifies the changed
coefficient without reporting the next numerical answer. Feedback distinguishes
a valid simplified quotient from the requested factored form, and numeric
options represent distinct named computations from the original polynomial.

The question prompts, domains, explanations and individual option feedback are
maintained in the existing Markdown templates. Python supplies original-case
arithmetic, semantic option positions and shared compiler/export assembly.
There is no new parser, renderer, runtime answer policy or per-set source fork.

Verification from the Paths root:

```sh
PYTHONPATH=tools python3 -B content/authoring/production/wave01/calculus/certificate_tests.py
PYTHONPATH=tools python3 -B content/authoring/production/wave01/calculus/generate.py --target b/sorter --model b/paths_learning_document_tests
```

All four regression tests pass. They cover all 36 original cases, independently
expand shifted polynomials by Horner multiplication, compare full quotient
coefficients and exact rational evaluations, check the derivative by a separate
quadratic/cubic formula, and verify distinct numeric options. Actual compiled
key, working and missing-point mutations are rejected by the shared certificate
boundary. Editing a real prompt in a temporary Markdown template changes the
compiled prompt. These checks do not initialize a window, fonts or ImGui.

All six staging packages pass the existing assignment gate. Each final family
passes the compiler, provenance check, 18 solving routes, 42 wrong choices, saved
progress replay and the shared `--family-lessons` disclosure check. Every final
receipt records source and executable hashes compared before and after the run.
Exact current package and verification paths are selected by
`build/production/wave01/calculus/production.json`; earlier immutable outputs are
historical evidence only. The coordinator's wave review queue owns release
acceptance. No visual observation or measured learner outcome is claimed.
