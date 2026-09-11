# Trigonometry Wave 01 — completion review, 2026-09-10

Contract revision 2 is complete at the automated authoring boundary: **36 questions, two canonical readings and two consolidated family packages**, unpublished. Each family has 18 questions, 21 decisions and 42 individual wrong-choice corrections. The six six-question packages are staging evidence only.

## Final current immutable evidence

- Current index: /Users/kogaryu/iggy3d/paths/build/production/wave01/trigonometry/production.json
- Immutable production receipt: /Users/kogaryu/iggy3d/paths/build/production/wave01/trigonometry/receipts/4ca433efdf35e0fe4520bbc425aeaaa7166827d472bab2acd7fd962c22aed33f/production.json
- Immutable certificate/compiler-edit test receipt: /Users/kogaryu/iggy3d/paths/build/production/wave01/trigonometry/tests/c33456a8f10dcedbb0e4c71b6a570080993de398cac399c0e37eb90bf4fd7d18/verification.json

### sine_turn

- Authoring: /Users/kogaryu/iggy3d/paths/build/production/wave01/trigonometry/families/sine_turn/9b27257aa0f314315537e4399f1f7e41bc5f4475491ba19295a5d7d8679c9d6d/authoring
- Verification: /Users/kogaryu/iggy3d/paths/build/production/wave01/trigonometry/families/sine_turn/9b27257aa0f314315537e4399f1f7e41bc5f4475491ba19295a5d7d8679c9d6d/checks/5db4372f9c9bf49922409411c31ce3f043363b869e0308c912305ac35a396d4f/verification.json

### cosine_turn

- Authoring: /Users/kogaryu/iggy3d/paths/build/production/wave01/trigonometry/families/cosine_turn/329db64954849b85d25f94165196ae349c17ebbbc28159e6365dac04ef9e34b3/authoring
- Verification: /Users/kogaryu/iggy3d/paths/build/production/wave01/trigonometry/families/cosine_turn/329db64954849b85d25f94165196ae349c17ebbbc28159e6365dac04ef9e34b3/checks/63eb973126cfbd2be47677f09ec61e73b9f1cd49b5ebf14361c55dafc23a379e/verification.json


Both family receipts record identical before/after hashes of every maintained generation input. The test receipt confirms those hashes still match the current source. review.md is a handoff document, not an input to generated learner content; it is intentionally outside that input manifest to avoid a receipt/self-hash cycle.

## Commands and observed results

Run from /Users/kogaryu/iggy3d/paths:

~~~sh
PYTHONPATH=tools python3 -B content/authoring/production/wave01/trigonometry/generate.py --target b/sorter --model b/paths_learning_document_tests
PYTHONPATH=tools python3 -B content/authoring/production/wave01/trigonometry/certificate_tests.py
~~~

Both commands exit 0. The generator ran all six existing check_assignment gates and both consolidated-family compiler/provenance/model gates. Each family replayed 18 routes, 21 decisions, 42 wrong choices and save replay. Exact compiled questions compare unchanged with the concatenated staging sets. All six first-choice permutations are distinct and each uses all three positions twice.

The generator also invoked the ONE shared disclosure gate on each final family's exact document bytes:

~~~sh
b/paths_learning_document_tests --family-lessons /Users/kogaryu/iggy3d/paths/build/production/wave01/trigonometry/families/sine_turn/9b27257aa0f314315537e4399f1f7e41bc5f4475491ba19295a5d7d8679c9d6d/authoring/documents
b/paths_learning_document_tests --family-lessons /Users/kogaryu/iggy3d/paths/build/production/wave01/trigonometry/families/cosine_turn/329db64954849b85d25f94165196ae349c17ebbbc28159e6365dac04ef9e34b3/authoring/documents
~~~

Each result is accepted=true, readings=1, questions=18, closed_disclosures=4, independent_worked_disclosures=3, windows=0. This is structural disclosure evidence, not visual acceptance or a prose-truth oracle.

The independent suite reports:

- All 36 finite original cases, 42 decisions and 84 wrong options checked.
- 412 deliberate rejections covering invalid coefficients/types, unsupported coordinates, changed domain/units, corrupted producer branches, missing branches, duplicate/coterminal answers, wrong quadrants, excluded 2pi and loss of valid zero.
- 54 compiler mutations: 12 real Markdown prompt edits and 12 real individual-feedback edits reach compiled content; 14 false keys and 14 false reached working lines are compiled then rejected by verify_role_content; two equivalent-option corruptions also reject.
- Prompt mutations change only the intended compiled prompt, with mathematics and other fields unchanged.
- Exact original-equation residuals, circle intersection counts, rational/radical rotations and a separately implemented triangle/quadrant oracle agree.
- Both lessons explicitly define alpha as a known solution angle, not necessarily acute. The compiled negative-sine worked explanation is regression-checked for “pi-7pi/6=-pi/6; adding 2pi gives 11pi/6”.

The mutation fixtures operate on temporary real Markdown inputs through reasoning_documents and the real compiler/model. They do not change canonical source or historical immutable outputs. The test no longer merely checks that a corrupted expected key differs from itself.

## Source ownership and consolidation

Each family now has one actual six-role questions.paths.md.in and one lesson.md.in. Every prompt, @why, fallback and individual @feedback sentence is editable in Markdown; Python supplies numeric/symbolic substitutions and choice ordering only. The finite variations, coefficient signs, offsets, known branch indices and balanced permutations are in recipe.json.

Removed superseded routes: prose_question and its Python teaching/feedback dictionaries, the hard-coded LEVELS pool, dead alternative document assembly, and both unused partial read_notation.md.in files. Historical immutable build receipts are retained for comparison; they are not live authoring routes.

The repair first decision identifies L2 without showing the corrected set. Its second decision requires a complete endpoint-filtered answer. Valid intermediate equations and repeated valid angles are described as missing the goal, not false mathematics. Separate worked lessons differ from all eighteen family originals; Hint, Answer and Solution stay independently closed.

Reused the shared chapter wrapper, reasoning_documents, reasoning_certificate, verify_role_content, check_assignment, compiler, provenance and model gates. No new parser, runtime solver or publication route was added. The only source edits are in this subject folder, excluding BRIEF.md. Published content, runtime, shared tools, reviewed pilots and other workers' files were not changed. No screenshots, windows, commits or publication occurred.

## Final input hashes

- DESIGN.md: edfab1e58461b6bf6000021754a559b71b3180ee83a8fd959bbe52922b5bf1be
- certificate_tests.py: 04e9ce3caa56d581c0059ef8d21c518b4316b28b3fab35aee656f72564462140
- families/cosine_turn/lesson.md.in: a04b116c3ccf589a002d85b1d11a46068674a2f1ba00caab259259c6cd1d80c3
- families/cosine_turn/questions.paths.md.in: 4fcc31bcec5d2069c3ec80c7530ae08841c883da9c25b31a5d6e42b66ce3e7e0
- families/sine_turn/lesson.md.in: 91f704873eba21acb514208fa5b931605f51ddfe21aa2d0d20004523b30290e7
- families/sine_turn/questions.paths.md.in: 9abe8d37b31c832125436248c8314c5fd3a4c11b4aae20349b61b878db066c6e
- generate.py: 2672223ee38be86e46467d77a124707b8038ad7fcc6b8eac61bd3bb7e6df67b5
- recipe.json: 801d9b5d9e8684e290dedf9794dc5f2e6af6a50a03a4ba096ddafd668693dc23

Checked executable hashes from the ready shared build:

- b/sorter: 242a5a6fc0c8f3e04ae7c79bc1a02d5e0fe32663dd9969c9ea72f00391033776
- b/paths_learning_document_tests: ec98ec35bcf980527fd2e80d3b8e49e24b0e012bf9c7046420312d66c9bbd8d2

## Coordinator handoff and remaining limits

Return these exact receipts to coordinator task 01a07b52-505e-7e10-822b-17f39b7f2fd1 on local for final delta review and integration under the authorized Wave 01 scope. Hold generation sources stable after handoff. There is no remaining writer blocker. Automated acceptance does not replace coordinator mathematical/prose review or authorize publication.

No native visual observation or learner study was performed. No claims about measured learning, retention or mastery are made. Source attribution and conceptual variation are documented in DESIGN.md; definitions were checked against the linked OpenStax textbook sections on 2026-09-10.
