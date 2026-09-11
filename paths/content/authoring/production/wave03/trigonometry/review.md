# Trigonometry identity family — writer handoff

## Delivery and scope

Package `prod03_trig_identities`, version 1, contains twelve complete original questions and one canonical lesson under `topic_0033` / Identities. Question suffixes are q01 through q12 in order; all reference `prod03_trig_identities_r`. The reading links those twelve questions in the same order.

Four introductory, four practice and four mixed routes contain **69 decisions, 207 symbolic options and 138 individual wrong-choice explanations**. Decision counts by question are 4, 4, 5, 6, 6, 5, 6, 6, 7, 7, 6, 7. The first accepted position occurs four times in each of the three positions; later ordering varies without moving keys or feedback away from their semantic IDs.

The single maintained learner source is `authoring/documents/chapter.paths.md`. Prompts, givens, choices, reached work, explanations, individual feedback and the lesson are ordinary editable Markdown. `cases.json` holds finite original inputs, candidate expression trees, original domains, local form contracts and regression controls. It has no trusted accepted-key field. There is no generator, template dialect, per-question script, runtime change or alternate maintained authoring route.

## Mathematics and teaching review

The introductory routes develop coordinate-square replacement, cancellation, a sum of quotient functions and a reciprocal-square difference. Practice adds unlike denominators, a distributed subtraction yielding zero, factoring, and both orientations of conjugate rationalization. All four mixed routes explicitly select and execute a coordinate, Pythagorean or common-denominator strategy. Each route ends by checking the original expression with a nonzero polynomial multiplier and states the final expression on its retained domain.

Original exclusions are computed before simplification. Sine, cosine and 1 plus/minus either coordinate have their complete unit-circle zero sets recorded. Nested quotients also require their divisors to be nonzero. The positive outer denominator 1+cotangent squared introduces no new zeros, but does not remove cotangent's own sine exclusion. Cancelled factors never restore original exclusions; newly introduced factors cannot remove originally allowed angles.

The independent finite audit converts the original and each actual compiled candidate to rational functions in s and c with exact rational coefficients. It computes cross-product differences and reduces c squared to 1-s squared. An empty remainder proves equality everywhere on the retained domain; point samples are not used to prove identities. Full per-option truth, domain, local-form and nonzero remainder evidence is retained in mathematics.json. The checker does not infer mathematical correctness from an authored key, option order, expected label or reached-state string.

Requested form is a separate predicate. For example, a true uncancelled fraction is not a single coordinate factor, and a true reciprocal-square denominator is not an explicitly requested unchanged conjugate product. Both are tested as rejected controls. Equivalent valid controls are tested singly: sin(theta) in place of s, an alternate fraction spelling, and a reordered expanded polynomial. No equivalent expressions compete as separate options. The checker detected an equivalent pair of distractors during development; the source was corrected without weakening that check. A distractor denominator outside the declared six-factor pool was replaced by a supported misconception rather than extending the checker into a general root finder.

The lesson defines theta, s, c, E, D, identities versus equations, reciprocal and quotient definitions, common domains, terms versus factors, conjugates and legal cancellation. Its distinct worked original is (2-2s squared)/c, with every domain restriction, factorization, cancellation and exact original-expression check explained. It is different from all twelve originals. Only its original given is public; Hint, Answer and Solution are independently closed. The shared lesson gate also finds the separately closed general proof. Function squares use conventional function-name exponents, and multiplication displays avoid redundant nested parentheses. No visual inspection is claimed.

Definitions and conditions were checked on 2026-09-10 against [OpenStax Precalculus, section 7.1](https://openstax.org/books/precalculus/pages/7-1-solving-trigonometric-equations-with-identities). Exercises and explanations are original; no external problem wording was copied. Attribution is also in DESIGN.md and portable authoring metadata.

## Reproducible headless gates

From `/Users/kogaryu/iggy3d/paths`:

```sh
b/sorter --inspect-documents --documents content/authoring/production/wave03/trigonometry/authoring/documents > build/production/wave03/trigonometry/inspection.json
b/paths_learning_document_tests --question-batch content/authoring/production/wave03/trigonometry/authoring/documents > build/production/wave03/trigonometry/routes.json
b/paths_learning_document_tests --family-lessons content/authoring/production/wave03/trigonometry/authoring/documents > build/production/wave03/trigonometry/lessons.json
PYTHONPATH=tools python3 -B content/authoring/production/wave03/trigonometry/certificate_tests.py --routes build/production/wave03/trigonometry/routes.json
```

The final production.json records the observed results and exact source/evidence hashes after all corrections. Its required source map includes this review, DESIGN.md, cases.json, certificate_tests.py, metadata and Markdown. This review deliberately does not contain its own hash or the final receipt hash. The evidence map covers actual inspection, route, lesson and mathematical outputs, excluding production.json itself. Source hashes and both executable hashes are checked again at the end of the audit.

All four commands exited 0 on the final source. The observed inventory is twelve successful routes, 69 decisions, 138 exercised wrong choices and headless in-memory save replay. The ONE shared disclosure gate reports one reading, twelve questions, four closed disclosures, three independently closed worked disclosures and zero windows. Target.inspect and export.provenance verify normal compiled metadata and complete attribution coverage. The checker also verifies chapter binding, canonical reading links, practice order, content versions, exact compiled originals/domains, every option, unique keys and the reached-state chain.

The suite includes 151 deliberate rejection probes: a false key and a false registered mathematical reached state at every decision, differently spelled and reordered equivalent duplicate options, invalid coefficients/types/powers/denominators, original/domain corruption, cancelled-domain loss and true-but-goal-missing expressions. Three positive controls prevent a literal-string-only interpretation of those tests. In two scratch Markdown copies, an actual prompt and an actual wrong-feedback passage are edited and compiled with the real model; exact payload comparisons show that only the intended compiled field changed. Scratch copies are removed; canonical source remains unchanged.

Reused shared helper: `tools/export_learning.py` for canonical evidence encoding, read/hash utilities, temporary document materialization, Target.inspect and provenance validation. Its exact current hash is recorded in production.json. No earlier trigonometry generator or solver was copied. The Release executable pins are read from `build/production/wave01/build-ready.json`; no rebuild occurred.

## Handoff

Receipt: `/Users/kogaryu/iggy3d/paths/build/production/wave03/trigonometry/production.json`.

Supporting evidence: `inspection.json`, `routes.json`, `lessons.json` and `mathematics.json` in that same subject build folder. Authoring: `/Users/kogaryu/iggy3d/paths/content/authoring/production/wave03/trigonometry/authoring/`.

The handoff stage is `ready_for_coordinator_review`, not accepted or published. Freeze the source after final verification. The coordinator owns independent mathematical/prose review, aggregate/save checks, capture and publication. No user QA, new workers, task callback or further family is requested. No screenshots, captures, images, windows, font/ImGui initialization, clipboard access, commits, publication, store activation or persistent save writes were performed. Earlier waves, shared tools/runtime, other workers and 3D assets were left unchanged.

There is no known writer blocker within the declared finite scope. The checker does not certify arbitrary TeX, arbitrary polynomial denominators or universal symbolic algebra. Native appearance and learner outcomes are unobserved; no mastery, retention or empirical transfer claim is made.
