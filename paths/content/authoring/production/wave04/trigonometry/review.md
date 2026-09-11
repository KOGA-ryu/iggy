# Wave 04 trigonometry source-assisted handoff

## Scope and delivery

The maintained family is `prod04_trig_identity_equations` version 1, one canonical lesson and twelve complete questions bound to topic_0033 / Identities. The direct Markdown draft has 75 meaningful decisions, 225 symbolic options and 150 individual wrong-choice corrections. The four introductory, four practice and four mixed groups follow the approved design. Each first correct position is used four times, with varied later ordering and stable semantic IDs.

Every problem is adapted from a distinct approved Section 7.1 seed in David Lippman and Melonie Rasmussen's Precalculus: An Investigation of Functions, Edition 2.3. The twelve selected prompts are Examples 1, 2, 3 and Exercises 13, 14, 17, 19, 20, 21, 22, 23, 40. cases.json records exact locators, source mathematical givens and every coefficient/condition/route change. The distinct worked example adapts Example 4. Credit and adaptation notices are included in each metadata source record and the public lesson summary. The adapted lesson and questions use CC BY-SA 4.0: https://creativecommons.org/licenses/by-sa/4.0/ . This makes no application-wide license or author-endorsement claim.

The source-reading PDF skill was used solely for a second text extraction. The user's image-free workflow superseded rendering guidance. Raw page-8 extraction resolves flattened square exponents when compared with the pinned layout text; its bytes are retained in source-page8-raw.txt. No NC solutions manual, graph, figure, photograph or externally credited puzzle was used. Approved frequency prompts were not selected; non-special-angle coefficients were explicitly adapted rather than approximated. Source PDF/text and license evidence are hash-checked against SOURCES.json.

## Independent mathematics

The certificate expands an untrusted two-linear-factor witness and verifies that it equals a nonzero clearing multiplier times the original equation's left-minus-right expression. This exact original-derived identity makes factorization reversible on the original domain. Algebraic roots are then derived from each factor's coefficients; authored keys and answer labels are not the oracle.

The frozen Wave 03 helper supplies exact rational polynomial reduction and denominator exclusions; the frozen Wave 01 helper supplies exact rotations and existing fraction/pi/set formatting. Unit-circle intersection counts prove complete coordinate branches, not a scan of finite angle candidates. Exact arithmetic in Q(sqrt(3)) substitutes every retained angle into both original sides. All branches are united, deduplicated and filtered through [0,2pi) and the original reciprocal domain. Every local equation, branch list, angle set, key and reached state is checked against this certificate, with requested form checked separately.

The no-solution case proves sine squared would have to equal 4. Zero-factor cases preserve roots lost by dividing by sine or cosine. Secant cases retain cosine exclusions after clearing denominators. In q12, cotangent is c/s, so only sine-zero angles are excluded: cosine-zero angles remain valid solutions. The distinct worked example has overlapping branches; its union counts zero once and excludes 2pi.

The lesson defines all notation and prerequisites, identity versus equation, substitution, factor versus term, zero product, principal inverse value versus full angle set, coordinate range, period and domain. It supplies a fully explained worked route with independently closed Hint, Answer and Solution, plus a closed general proof. Every question references the canonical reading; its practice links retain q01-q12 order. No learner-facing implementation or test-status prose is used.

## Reproducible checks

From /Users/kogaryu/iggy3d/paths:

```sh
b/sorter --inspect-documents --documents content/authoring/production/wave04/trigonometry/authoring/documents > build/production/wave04/trigonometry/inspection.json
b/paths_learning_document_tests --question-batch content/authoring/production/wave04/trigonometry/authoring/documents > build/production/wave04/trigonometry/routes.json
b/paths_learning_document_tests --family-lessons content/authoring/production/wave04/trigonometry/authoring/documents > build/production/wave04/trigonometry/lessons.json
PYTHONPATH=tools python3 -B content/authoring/production/wave04/trigonometry/certificate_tests.py --routes build/production/wave04/trigonometry/routes.json
```

All four prescribed commands exit 0. The inspector accepts the chapter and provenance; the model replays all twelve routes, 75 decisions and 150 wrong choices with headless in-memory save replay. The shared lesson gate reports one reading, twelve questions, four closed disclosures, three independent worked disclosures and zero windows. It checks structure, not prose truth or native appearance. No persistent store or save is written. Exact hashes and per-decision evidence are bound in production.json and mathematics.json.

Real scratch Markdown prompt and wrong-feedback edits are compiled by the actual model and compared field-for-field with the original payload. Both scratch chapters, their compiled replays and exact before/after field-change records are retained in content-addressed folders under build/production/wave04/trigonometry/markdown-edits/. Their six file hashes are included in production.json, not merely summarized by success flags. Each scratch replay changes exactly one intended field and leaves every mathematical field unchanged.

All 167 deliberate rejection probes and two valid alternative controls pass. Equivalent-factor and reordered-angle controls test equivalence-aware acceptance; their competing duplicates reject. Further negatives cover false keys and registered false working at each decision, missing zero branches, principal-only sets, lost zero, added 2pi, invalid reciprocal inputs/domains and true-but-not-factored forms. The first complete mathematical/compiled audit passed. A coordinator-requested evidence revision retained the scratch artifacts rather than deleting them. A subsequent bounded review corrected q05's prose arithmetic to 2-1=1 before negation, made all q01-q07 angle feedback numerical and case-specific, replaced undefined terminology, added genuine strategy choices in q09/q11 and removed unused prompt copies from cases.json. No original equation, final solution set or decision count changed. The final checker updates the observed checks-done timestamp before binding trial.json into the receipt.

The two genuine method-choice slots retain their existing semantic IDs and reached mathematical states. Q09 compares factoring the original difference with dividing by cosine and squaring both sides. Q11 compares reciprocal rewriting with dividing by sine and squaring an isolated equation. Exact witnesses show loss of pi/2 and 0 respectively under division, and spurious roots 7pi/6 and 2pi/3 respectively under squaring. Correct strategies are proved reversible; the other options are rejected for the precise method goal, with feedback explaining how squaring could still be used if additional roots were later removed. Both choices are carried through the existing complete solves.

## Trial timing and handoff

Actual observed timestamps and source difficulties are in build/production/wave04/trigonometry/trial.json. Work began at 2026-09-11 03:20:17 UTC; the design checkpoint was 03:24:39 UTC. The coordinator recorded a usage-limit interruption before a completed chapter existed. Work resumed at 04:28:50 UTC and the full draft was written at 04:31:00 UTC. The interruption is recorded, not silently counted as active writing. No token or weekly-usage savings are estimated and no reset credit was used.

Receipt: /Users/kogaryu/iggy3d/paths/build/production/wave04/trigonometry/production.json. Stage at successful handoff is ready_for_coordinator_review, unpublished. Source and evidence hashes, exact source snapshots, registry hash and reused-helper hashes bind the delivery. review.md is itself a hashed source; the receipt does not hash itself.

The coordinator owns independent final review, aggregate/save checks and any publication. No callbacks or new tasks are requested. Earlier sources, accepted algebra/linear deliveries, shared tools/runtime and 3D work remain unchanged. No screenshots, captures, images, windows, font/ImGui probes, clipboard, rebuilds, store activation, publication or commits occurred. The checker certifies only the declared finite equations and controlled options, not arbitrary symbolic input. Native appearance and learner outcomes remain unobserved.
