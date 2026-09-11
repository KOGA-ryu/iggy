# Wave 04 span/basis source handoff

Stage: ready for coordinator review, not published. One canonical lesson,
twelve complete questions, 72 decisions and 144 wrong choices. The source
contains four introductory membership problems, four homogeneous-system
problems and four mixed basis/coordinate problems. Decision counts in order:
5, 5, 5, 3, 5, 7, 6, 7, 7, 8, 6, 8. q09 and q10 select a method and carry
it through; q11 additionally selects a parametrization method.

Package prod04_linear_span_basis, version 1; reading
prod04_linear_span_basis_r; questions prod04_linear_span_basis_q01 through
prod04_linear_span_basis_q12; existing chapter topic_0090.

## Exact results and receipt

The final receipt is
/Users/kogaryu/iggy3d/paths/build/production/wave04/linear_algebra/production.json.
It binds all six maintained source files, evidence files, source snapshots,
read-only helper dependencies and both executable hashes. The receipt excludes
itself from its evidence hash map. Authoring metadata and the actual Markdown
remain the ordinary editable package inputs, without a generator.

Chapter SHA-256:
6ccc5c957b110dd9aa951f3bc442a9dea19f5d92a18241a63822d4cbb3bccca2

Headless commands, all exit 0:

    b/sorter --inspect-documents --documents content/authoring/production/wave04/linear_algebra/authoring/documents
    b/paths_learning_document_tests --question-batch content/authoring/production/wave04/linear_algebra/authoring/documents
    b/paths_learning_document_tests --family-lessons content/authoring/production/wave04/linear_algebra/authoring/documents
    PYTHONPATH=tools python3 -B content/authoring/production/wave04/linear_algebra/certificate_tests.py --routes build/production/wave04/linear_algebra/routes.json

Outputs are inspection.json, routes.json, lessons.json and mathematics.json
under the subject evidence directory. Target.inspect and export.provenance
also passed; the resulting provenance.json covers all thirteen content IDs.
The binaries match build/production/wave01/build-ready.json. They were not
rebuilt. The lesson gate reports one reading, twelve questions, four closed
disclosures, three independently closed worked-example disclosures and zero
windows.

The final inspection and lesson gate include the three terminology
clarifications requested by the coordinator. The question routes were not
changed by those reading edits. The scratch compiler proof also demonstrates
that the final source still produces the same complete twelve question records
apart from its two intentional temporary teaching edits.

## Independent mathematics and actual choices

Rank is computed by exact original minors of order at most three, independently
of the authored row sequence. Unique coefficients use Cramer's rule on an
original full-column-rank minor, then check every original row. Column-space
bases use rank increments of original column prefixes; every original generator
is reconstructed in the selected basis. A plane basis comes from the original
normal equation and free-parameter order, with a symbolic normal-times-basis
zero check and full-column-rank check.

Every actual compiled original given, domain, choice and reached state is
checked. All augmented columns participate in each row operation. The shared
row helper operates on the chosen pair of complete rows, and the exact inverse
must recover the prior matrix entry for entry. Original annihilator dot
products establish both non-membership conclusions; original nonzero
homogeneous witnesses establish all dependence conclusions. The independent
two-column example proves independence without claiming to span R3.

q10 selects original columns 1 and 2, proves independence and expresses u3
and u4 in them. Its different valid basis using columns 1 and 3 is acknowledged
as valid mathematics but misses the explicit original-pivot-column goal.
Reduced coordinate columns are not accepted as original basis vectors.
q11 spans the entire defining plane by (s,-s,t), not by sampled targets.
q12 independently checks coordinates (2,1) and (10,-7) against both original
bases, reconstructing the same (3,-1). Basis uniqueness follows from full
column rank; general homogeneous relations retain their free parameter.

All 144 wrong-choice feedback passages and all 72 explanations were reviewed
against their actual choices and operands. Reversible alternatives are
described as missing the requested operation, not as invalid mathematics.
First correct positions occur four times each; later positions rotate
deterministically. No reordered/rescaled-equivalent basis or proportional
relation competes with its equivalent answer.

The suite rejects false keys at every one of the 72 decisions, partial
augmented updates, changed original givens/domains, false rank, zero dependence
claims, swapped coordinates, wrong original-column bases, normalized-witness
goal failures and semantic duplicates. It accepts equivalent rational notation,
an equivalent unaugmented array wrapper, reversed swap notation, and the
mathematical validity of nonzero relation multiples and genuinely different
bases. Goal compliance is separately checked.

The numeric atom reader is reused read-only from the published Wave 03
certificate. Only that helper is used, not its 2x2 inverse solver or rectangle
recognizer. Shared question_workflow.exact, build_question_batch.operate,
the exporter and the existing native compiler/model remain unchanged.
The new finite rectangle recognizer handles only this family's bounded numeric
fields, not Markdown or arbitrary TeX. It does not claim to prove arbitrary
natural-language prose; prose review is separate.

## Real Markdown edit proof

Scratch source:
/Users/kogaryu/iggy3d/paths/build/production/wave04/linear_algebra/markdown-edit/d36237b06297ea6506960c4c1751d229a554ec6a15f7a3153c57285ab7b5c836/documents/chapter.paths.md

Exact receipt:
/Users/kogaryu/iggy3d/paths/build/production/wave04/linear_algebra/markdown-edit/d36237b06297ea6506960c4c1751d229a554ec6a15f7a3153c57285ab7b5c836/checks/edit.json

A real first-question prompt and one wrong-feedback passage were edited only
in the scratch copy. The actual compiler changed exactly those two fields.
All other compiled question fields and all mathematical certificates stayed
equal, and live source bytes stayed unchanged. Scratch source and replay are
content-addressed evidence, not another maintained authoring route.

## Source tracing, observed phases and corrections

All twelve questions map to twelve distinct approved Hefferon exercise
subparts; the lesson's numerical worked example maps to a separate thirteenth
seed. cases.json preserves each original seed's exact givens and changes;
DESIGN.md supplies the full mapping. The original PDF, text and author license
snapshot match the registry pins. The content is adapted under the
author-offered CC BY-SA 3.0 US option, credited in metadata and public summary.
No answer PDF was used as the oracle.

The layout text interleaves some matrix brackets. A second pdftotext -raw
extraction of PDF pages 113-114, 127 and 137 made the entries consecutive and
resolved those cases without guessing. Those three raw text files are retained.
No image or rendering was used. Externally attributed Two.I.2.24 (Cleary),
question-mark puzzles and polynomial/function-space subparts were excluded.
No chosen seed remains unreadable.

Observed UTC phases:

- Start: 2026-09-11 03:20:36.
- Design fixed: 2026-09-11 03:25:47.
- Cases ready: 2026-09-11 03:27:38.
- Complete twelve-question draft: 2026-09-11 03:54:05.
- Exact checks and final terminology lesson gate passed: 2026-09-11 04:00:29.

These observations span interrupted work and are not a continuous
authoring-throughput measurement. Final receipt records final capture time.
No token or weekly saving is estimated.

Corrections in this completion:

1. Preserved q01-q08 and wrote q09-q12 directly in the existing Markdown.
   Removed the unnecessary q04 normalization after a contradiction was already
   established; its meaningful three-decision route is the short exception.
2. Clarified U as the original vector list, distinguished the zero coefficient
   column from a dependence witness, and explicitly named the annihilator,
   following the coordinator's bounded terminology review. Question givens,
   choices, states and IDs were unchanged by these reading clarifications.
3. The first certificate run found a negative test modifying a distractor
   instead of the accepted relation. Corrected the test to target the answer;
   no question mathematics changed. Added all-decision false-key checks and
   numeric-wrapper/semantic-equivalence boundary controls. Final suite passes.

Two failed apply_patch attempts were atomic context mismatches and made no
partial changes. An inspection stdout was truncated when returned to the
tool; final full inspection is captured in inspection.json. These were
authoring/evidence-capture issues, not passing mathematical receipts.

## Remaining concerns and scope

No unresolved mathematical or source-reading concern is known within this
finite pool. Coordinator review, aggregate import, preservation and publication
are still pending. Native visual appearance and learner outcomes are
unobserved; passing symbolic-choice checks does not establish unaided proof,
retention, transfer or mastery.

The coordinator reports the current live baseline as 653 questions; this
writer did not mutate or inspect the store to establish another baseline.
No runtime/shared-tool changes, stores/saves, earlier sources, agents,
screenshots/captures/images, windows, fonts/ImGui, clipboard, commits or
publication were used for this Wave 04 completion. Source is frozen at handoff.
Only a normal final response is returned; no callback or further job follows.
