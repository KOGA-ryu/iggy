# Wave 01 Linear Algebra final closeout

Contract revision 2: 36 questions, two canonical readings and two consolidated
family packages. This handoff supersedes the production receipt with SHA-256
62a692ed6bbfb148a3a1f52cb47be38a19b7a9d7318899407b957dec9490d8e0.
Each family contains 18 questions, 21 decisions and 42 wrong-choice explanations.
Six shared staging receipts are evidence, not separate publishable deliverables.

## Exact final evidence

Production receipt: /Users/kogaryu/iggy3d/paths/build/production/wave01/linear_algebra/production.json

Production SHA-256: 1b858e1310ae060a064ce5131c349cd800b3d53f4c2e167dbf789c5db566c859

Immutable snapshot: /Users/kogaryu/iggy3d/paths/build/production/wave01/linear_algebra/closeout/1b858e1310ae060a064ce5131c349cd800b3d53f4c2e167dbf789c5db566c859/production.json

Comparison and regression receipt: /Users/kogaryu/iggy3d/paths/build/production/wave01/linear_algebra/closeout/1b858e1310ae060a064ce5131c349cd800b3d53f4c2e167dbf789c5db566c859/comparison.json

Comparison receipt SHA-256: 522a540d6bfd1c4b58eb0d1bb0e87ae7f8755ddfa570bc3d3eee108e0c41d6da

The baseline production snapshot is retained beside the final snapshot; its
bytes reproduce the prior production SHA-256 above. The comparison lists every
changed compiled field with its exact old/new text and both document hashes.

### row_operations

Authoring: /Users/kogaryu/iggy3d/paths/build/production/wave01/linear_algebra/families/row_operations/845df07ecfaa561dbbaf3eabbb6e59b2b204f869c2b8da0e5a9f6d8238dbc60d/authoring

Verification: /Users/kogaryu/iggy3d/paths/build/production/wave01/linear_algebra/families/row_operations/845df07ecfaa561dbbaf3eabbb6e59b2b204f869c2b8da0e5a9f6d8238dbc60d/checks/7028a89b34b61c57193b65dab4949846e75d4f47e3ae8130c3477614bf1a875e/verification.json

Verification SHA-256: 7028a89b34b61c57193b65dab4949846e75d4f47e3ae8130c3477614bf1a875e

Source manifest SHA-256: 845df07ecfaa561dbbaf3eabbb6e59b2b204f869c2b8da0e5a9f6d8238dbc60d

Package inventory SHA-256: fb7cfaa4558512394a7393bb7748f209715643771e95d725ffed69c6f0e4f4c4

Document SHA-256: 09bd9b18a5167233dbc6d8ca776647134aec07f9088fddd100b365ff113c2604

Family lesson gate: accepted; one reading, 18 questions, three independently
closed worked-example disclosures, zero windows.

### determinants_2x2

Authoring: /Users/kogaryu/iggy3d/paths/build/production/wave01/linear_algebra/families/determinants_2x2/1dfcf4fdf1e13c342bec719e123818cfc71e6fd76b1c08305c883569fa684e73/authoring

Verification: /Users/kogaryu/iggy3d/paths/build/production/wave01/linear_algebra/families/determinants_2x2/1dfcf4fdf1e13c342bec719e123818cfc71e6fd76b1c08305c883569fa684e73/checks/337fa4d0153d650f392cf1ee8f63a209f4360925bbbb916e8e7d2c2203512201/verification.json

Verification SHA-256: 337fa4d0153d650f392cf1ee8f63a209f4360925bbbb916e8e7d2c2203512201

Source manifest SHA-256: 1dfcf4fdf1e13c342bec719e123818cfc71e6fd76b1c08305c883569fa684e73

Package inventory SHA-256: 0f725ebba9206ee70aaa92d21479a4a8a3ee30430ae2f2b80019ffbdb0a4dc6f

Document SHA-256: bc77d2c47009d5ef0f32fdb3febbda4d18d8ff6e6fe03a0b2b9dc4ca3a2bed7c

Family lesson gate: accepted; one reading, 18 questions, three independently
closed worked-example disclosures, zero windows.

## Source changes and byte comparison

Both document and package bytes changed intentionally. All 36 complete
mathematical certificates are exactly equal to the previous delivery. Every
question ID, original given, option label and order, answer key, reached working
state and evidence record is unchanged. Recipe SHA-256 is unchanged:
fc375812ace81500e4c9bb15446080f48a0c99804f3f47bab56e6517044e9f4a.
Both authoring.json files are byte-identical to their previous versions.

Only these compiled question fields changed:

- Row operations: nine explanations now show all three destination-entry
  calculations for worked, method and inverse roles, including the complete
  resulting/restored matrix; three wrong-option messages now correctly describe
  the actual wrong y coefficient and right-hand constant versus their required values.
- Determinants: three row-swap explanations now calculate ad-bc for A and cb-da
  for B before comparing signs; nine wrong-option messages use the actual
  selected misconception formula and exact evaluated expression. This allows
  the existing role Markdown to describe either reversal or omission accurately
  without a Python prose branch.
- Each canonical reading changes exactly one introduction line to a direct
  learning objective. No internal question counts or unmeasured-mastery claims
  remain in those introductions.

No other compiled question fields changed. The replacement of @feedback
serialization by actual template directives can also reorder source directive
lines without changing compiled option attachment.

The twelve existing role templates now own all explanations and wrong-option
feedback. Python retains arithmetic and semantic selection. The shared
choices_text() serializes choices/keys and fill_template() renders the Markdown;
row_feedback(), determinant_option_feedback(), determinant_feedback() and the
prose-producing calculation routes were removed, with no alternative parser
or generator. The arbitrary numeric answer+1 filler was removed. Cases without
two distinct computed misconceptions are refused; DESIGN.md records role
exclusions. Zero-first-row and all-zero matrices retain mathematical null-vector
evidence but are excluded from degenerate numeric-choice roles.

Final generate.py SHA-256: c0abfd49d56d8cf48c44c93f4528ef83ce4d188c9d3149ba3fad079e891bf90d

Final certificate_tests.py SHA-256: 69cdba00f4252d729e0528a7fc77b99c7dadd0d64772d020b7da25ddd972194a

Each family verification records every individual template/source hash. Source
manifest hashes cover the canonical JSON map of relative source filenames to
SHA-256. Package hashes cover the canonical JSON inventory of relative path,
byte count and file SHA-256, including authoring.json and the document.

## Checks performed

Both required commands passed on 2026-09-10:

```sh
PYTHONPATH=tools python3 -B content/authoring/production/wave01/linear_algebra/certificate_tests.py
PYTHONPATH=tools python3 -B content/authoring/production/wave01/linear_algebra/generate.py --target b/sorter --model b/paths_learning_document_tests
```

Exact regression stdout:

```text
production linear algebra certificate tests: passed (36 questions, two canonical families; 12 scratch Markdown templates: prompt and wrong-feedback edits verified in 36 compiled questions; complete row and swapped determinant explanations; numeric-collision refusal; isolated provider replay; compiled false-key and false-working refusal)
```

The scratch edit test copies all twelve role Markdown sources, edits one prompt
and one wrong-feedback message per template, renders both full families and
uses the real compiler. It verifies both corresponding fields changed in all
36 compiled questions and no other compiled field changed; live source bytes
are unchanged. Additional compiled assertions verify all three row calculations
and both displayed determinant calculations. These replace the old mere
string-presence assertion.

A freshly copied provider replays all 36 captured records with recipe and
filesystem reads disabled. Explicit family, set and original inputs determine
the mathematics; opaque replacement IDs leave mathematical evidence unchanged.
No mutable recipe lookup or ID parsing dispatches certificates.

The exact suite covers every augmented column, original-system substitution,
reversible row replacement, fractional elimination/solutions and determinant
sign. Every nonsingular case passes exact A*inverse and inverse*A identity
multiplication. Singular cases have explicit nonzero null-vector evidence;
zero-first-row and all-zero boundary facts are checked separately from their
numeric-role refusal. Equivalent row-swap choices are refused. Wrong-answer
feedback is attached to its actual semantic error, and repair decision one
does not disclose decision two's numerical answer. Compiled false keys and
false reached work are refused by the shared certificate verifier.

All six staging check_assignment() gates and both consolidated family gates
passed. Source, package inventory and executable hashes were rechecked, and
all eight exact verification directory names equal sha(encoded(receipt)).
The shared packet pins pass. No new runtime or shared tools were created.

Sorter SHA-256: 242a5a6fc0c8f3e04ae7c79bc1a02d5e0fe32663dd9969c9ea72f00391033776

Model SHA-256: ec98ec35bcf980527fd2e80d3b8e49e24b0e012bf9c7046420312d66c9bbd8d2

## Exact staging receipts

- row_operations / teaching: /Users/kogaryu/iggy3d/paths/build/parallel-authoring/linear_algebra/b3fe2ae3d08d1c54c70d518ad76122512612c2b250e41c9bf9864b41b34abc50/checks/2ebfcf922fa7a555434e366295b61004ef41ffafc183b2addb8ef4c6ca64093b/verification.json
- row_operations / practice: /Users/kogaryu/iggy3d/paths/build/parallel-authoring/linear_algebra/2b6657da9b0a81b0127b437f6c9eb68882da4afc91ea60eed8a86e7b197949c3/checks/f970caab0052d000c2b8e659c0ab4cd8d6494350d74365ca32a69099d7303ed5/verification.json
- row_operations / fresh_check: /Users/kogaryu/iggy3d/paths/build/parallel-authoring/linear_algebra/a72327379ed5a8735326e6c76677d87017c955e27e20aed2f845495b5f176f4b/checks/ca7e177313c7a8d06d9b5dc4e518bb2a1f751bd3e36fe82f8a2269ba2c0c0116/verification.json
- determinants_2x2 / teaching: /Users/kogaryu/iggy3d/paths/build/parallel-authoring/linear_algebra/de09e2f07aaec2feb602fa49f304d5167fae9b5f0927b7ef4cdcf7548ed719c6/checks/0ae9e60cf0cae6fd8860d2155d5c9385c6bfb5e2daf4f308f2fdff994f2ffd10/verification.json
- determinants_2x2 / practice: /Users/kogaryu/iggy3d/paths/build/parallel-authoring/linear_algebra/9cb4a197af99f6cb47c426f6856fbc6cc664b697fa096174971124ce461345e4/checks/a6ea07de52b521abdb904bf5c02561307765d0e421e8ae58f92908b0e44e1cc3/verification.json
- determinants_2x2 / fresh_check: /Users/kogaryu/iggy3d/paths/build/parallel-authoring/linear_algebra/3344d18f261d744840b25962bb6fe590ce02aa09746659be6da56ed9a16ce545/checks/c0ab6056933c89efb9dc510983649db3ea37ddbe228c4ebb116ff3e57124336f/verification.json

## Remaining concerns and scope

Writer checks pass. Coordinator acceptance remains pending; native visual
appearance and learner outcomes are unobserved. Automated selection does not
establish retention, transfer, mastery or unaided proof. Existing source
attribution and access date remain in DESIGN.md and package provenance.
No windows, captures, commits or publication occurred. All changes remain
within assigned subject sources and generated evidence; shared tools, runtime,
pilots, other subject writers and concurrent asset work were preserved.
