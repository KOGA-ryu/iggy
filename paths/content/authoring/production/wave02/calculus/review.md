# Wave 02 calculus — ready for coordinator review

Delivered one canonical lesson and twelve original complete derivative-to-tangent problems in ordinary native Markdown. Package: `prod02_calc_derivative_tangent`, version 1. Reading: `prod02_calc_derivative_tangent_r`. Chapter: `topic_0060` / Differentiation. The source is unpublished and has not been coordinator-accepted.

## Content and coverage

Four introductory, four practice and four mixed problems occur in the lesson's @practice order. Four chain-only routes have four meaningful decisions each; four product-only and four mixed routes have five each: **12 problems, 56 decisions, 112 wrong-choice corrections**. Eight cases use the chain rule, eight the product rule, and four both. All four mixed cases begin by selecting the combined identity before calculating. There are negative inner derivatives, negative second-factor derivatives, nonzero evaluation points, different powers and two deliberately horizontal tangents.

Each route finds the derivative function before its value, evaluates the original height and slope separately, constructs a tangent and performs incidence and slope checks in its final explanation. Factored derivatives are simplified in the reached state, and the checker verifies every equality in that state coefficient by coefficient. This avoids adding an artificial extra decision that simply repeats the final check's already-known numbers. q07 has f(-1)=0 and f'(-1)=0 without a zero derivative function; q11 has height 4 and zero slope through cancellation of two nonzero derivative contributions. Both cases retain mathematically distinct options.

The worked reading example `(x+2)^2(x-1)` at zero differs from all twelve inputs. It shows factor derivatives, both product contributions, expansion, original evaluation, derivative evaluation, tangent construction and both final checks. Only its original givens are public. Hint, Answer and Solution remain independently closed; the general coefficient-equality and tangent-uniqueness argument is under a separate Proof disclosure. No public block repeats the worked result. The lesson defines every symbol and rule condition used, including the derivative function versus its value, and the shorthand prime notation in method identities.

Wrong feedback is tied to semantic option IDs, not displayed positions. Each wrong option has its own correction. Missing factors, missing terms, signed evaluation errors, swapped height/slope and failed tangent incidence/slope receive different explanations. No domain or question title supplies a numerical answer or labels a particular problem as horizontal. Ordinary prose is used in history explanations; extended symbolic expressions are in native given/choice/after or reading displays.

## Commands and results

Run from `/Users/kogaryu/iggy3d/paths`:

```sh
b/sorter --inspect-documents --documents content/authoring/production/wave02/calculus/authoring/documents
b/paths_learning_document_tests --question-batch content/authoring/production/wave02/calculus/authoring/documents
b/paths_learning_document_tests --family-lessons content/authoring/production/wave02/calculus/authoring/documents
PYTHONPATH=tools python3 -B content/authoring/production/wave02/calculus/certificate_tests.py --routes build/production/wave02/calculus/routes.json
```

All final commands exit 0. Evidence is under `/Users/kogaryu/iggy3d/paths/build/production/wave02/calculus/`:

- `inspection.json`: ordinary importer accepts the document and its identities.
- `routes.json`: 12 routes, 112 wrong choices, save_replay=true, windows=0.
- `lessons.json`: one reading, twelve questions, four closed disclosures, three independent worked disclosures, windows=0.
- `mathematical.json`: all twelve original inputs, every choice/key/after-state, expanded coefficient certificates and final tangent checks accepted.
- `provenance.json`: generated using existing export_learning.Target.inspect() and export_learning.provenance(); all twelve questions and the reading covered by original-authoring metadata.
- `commands.json`: exact native command invocations, exit statuses and output paths.
- `production.json`: authoritative ready-for-coordinator-review index, all input and executable hashes, counts, evidence links and limits.

Target and model hashes were verified against `build/production/wave01/build-ready.json` before final commands and again in the subject suite:

- sorter: `242a5a6fc0c8f3e04ae7c79bc1a02d5e0fe32663dd9969c9ea72f00391033776`
- paths_learning_document_tests: `ec98ec35bcf980527fd2e80d3b8e49e24b0e012bf9c7046420312d66c9bbd8d2`

## Exact checks and mutation evidence

The checker expands original coefficient arrays using exact arithmetic and differentiates the expansion coefficient by coefficient. Authored derivative expressions are independently normalized from the compiler's structured fields and compared to those coefficients. Point evaluation alone is never used to certify equality of functions. Formal method identities use independent polynomial symbols for u, v, u' and v'; evaluated scalar and tangent decisions have their own local goals.

The suite rejects **173 deliberate corruptions**:

- 56 false keys, one at every actual step;
- 56 false intermediate reached states, including wrong arithmetic inside ordered pairs;
- 56 mathematically equivalent duplicate options with changed spelling;
- three invalid compiled domain or original-input cases;
- two adversarial polynomial changes invisible to the single evaluation point: adding (x-1)^2 to q01's original preserves its value and derivative at 1 but is rejected as a changed function, and adding (x-1) to its derivative preserves that derivative value at 1 but is rejected coefficient by coefficient.

Two real Markdown edits run through the existing model/compiler in temporary evidence-only folders: a prompt edit and one individual wrong-feedback edit. Each changes exactly its intended compiled field, leaves all other question fields and all other questions unchanged, and still passes mathematical checking. Canonical source hashes match before and after. Temporary edited copies are discarded after testing; the outcomes remain recorded in mathematical.json.

An initial test run stopped because the scratch-edit locator expected a repeated method prompt to occur only once in the whole chapter. The locator was corrected to scope the edit to q01. This was a test-fixture failure, not an arithmetic failure; the final rerun passes. It did not alter source outside this assignment or weaken mathematical rejection checks.

## Provenance, ownership and remaining limits

Source references and access date are in DESIGN.md and authoring.json: OpenStax Calculus Volume 1 sections 3.1, 3.3 and 3.6, accessed 2026-09-10. The numerical cases, lesson and explanations are original. No textbook problem text was copied.

There is one chapter.paths.md, one ordinary authoring.json, one finite cases.json and one mathematical/test entry point. No maintained document generator, alternate document parser, copied six-role gate, new runtime support level or new UI was introduced. The math-only expression evaluator consumes existing compiler output; it is not an authoring format or runtime solver.

All source changes are within this Wave 02 calculus folder, excluding its coordinator-owned BRIEF.md; all evidence is within its Wave 02 calculus build folder. Published Wave 01 content and review evidence, earlier sources, shared tools/binaries, other writers and 3D assets were not changed. No commits, publication, store activation, personal saves, windows, screenshots, captures, fonts/ImGui or delegation.

Remaining work belongs to the coordinator: independent content/test audit, immutable candidate capture, aggregate import/save verification and serial integration. Native visual appearance and learner outcomes were not observed; no mastery or retention claim is made. These prepared-choice routes do not provide the linear/matrix four-level written-support projection.

The input hash map covers DESIGN.md, cases.json, certificate_tests.py, authoring.json and the chapter document. This handoff review is excluded to avoid an index/review hash cycle. At the completion callback those generation inputs are frozen until a correction is assigned. No writer blocker remains.
