# Algebra production review

Contract revision 2 has one generator, one exact recipe pool, one certificate
provider, one test entry point, and editable lesson/question Markdown for each
of two families. Each family contains one canonical reading and eighteen
questions. Three six-question candidate sets are staging evidence only.

The coordinator completed the final intake corrections: the signed worked
check names its required operation; each wrong choice has specific feedback
maintained in Markdown; first-error teaching explains the operation without
reporting the second answer; and notation domains do not give away the key.
The existing fill_template, choices_text, chapter_text, certificate, compiler,
model replay and exporter remain the maintained route. The Python feedback
renderer was removed, and production.json references exact receipts instead of
duplicating their full route payloads.

Construction seeds retain exact fractions. A seed producing a nonintegral
right-hand side now fails explicitly instead of being rounded by int(). The
practice signed-equation coefficient is 6 and the fresh grouped-equation
coefficient is -6, preserving their intended 3/2 solutions with integer givens.
No previously published question was changed by these intake corrections.

Run from the Paths root:

```sh
PYTHONPATH=tools python3 -B content/authoring/production/wave01/algebra/certificate_tests.py
PYTHONPATH=tools python3 -B content/authoring/production/wave01/algebra/generate.py --target b/sorter --model b/paths_learning_document_tests
```

Six targeted tests pass: all 36 cases and their 84 wrong choices, malformed and
ambiguous cases, exact construction-seed preservation and refusal of rounding,
original-equation arithmetic, actual compiled key/working mutation rejection,
and a real Markdown prompt edit reaching the compiled prompt. The set keys
are balanced across all three positions. Both final families pass 18 routes,
42 wrong choices and the shared family-lessons gate. Each final receipt records
source and executable hashes compared before and after verification.

The exact current candidate paths are in
`build/production/wave01/algebra/production.json`. The coordinator's import,
repeat-import, invalid-import, save-upgrade and local publication records are
`build/production/wave01/algebra/release-rehearsal.json` and `release.json`.
The wave review queue owns acceptance; old immutable outputs are historical
evidence. All checks are headless. Native visual appearance and measured
learner outcomes are not asserted by these automated checks.
