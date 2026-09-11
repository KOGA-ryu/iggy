# Wave 03 inverse family: author handoff

Stage: ready for coordinator review, not published or independently accepted.
Source is frozen at the hashes in
build/production/wave03/linear_algebra/production.json. No subsequent family is
started. This report is the writer's review, not the coordinator's acceptance.

## Delivered

Package prod03_linear_matrix_inverse, version 1; chapter topic_0092 / Matrix
Algebra; one canonical lesson prod03_linear_matrix_inverse_r and twelve
complete choices.v1 questions ending _q01 through _q12. The one direct Markdown
file contains 64 decisions, 192 symbolic options, 64 reached states and 128
specific wrong-choice corrections. All twelve questions link to the lesson,
whose practice links follow the reserved order.

Four introductory, four practice and four mixed/exceptional problems are
complete. q01-q04 and q10 use full [A|I] row reduction. q05-q09 construct a
determinant, adjugate and evaluated inverse separately. q09 selects the
conditioned adjugate method and q10 selects/applies a complete-row swap under
an existing-row goal. q09-q10 finish by solving Ax=b and checking original A.
q11-q12 compute the zero determinant and prove no inverse exists using an
explicit nonzero vector mapped to zero. There are ten invertible and two
singular cases, with signed/nonunit determinants and exact fractional entries.

## Independent original-derived mathematical results

Matrices below use row-first order. Every listed inverse has been multiplied
with the original matrix in both orders, yielding exactly I. The two
application answers are also independently checked by original-column
Cramer's rule and original substitution.

| Case | A | det(A) | Inverse B or singular witness |
| --- | --- | --- | --- |
| q01 | [2,0;0,-3] | -6 | [1/2,0;0,-1/3] |
| q02 | [1,2;0,1] | 1 | [1,-2;0,1] |
| q03 | [1,0;-2,3] | 3 | [1,0;2/3,1/3] |
| q04 | [0,2;-1,0] | 2 | [0,-1;1/2,0] |
| q05 | [2,1;1,3] | 5 | [3/5,-1/5;-1/5,2/5] |
| q06 | [1,3;2,1] | -5 | [-1/5,3/5;2/5,-1/5] |
| q07 | [-2,1;3,2] | -7 | [-2/7,1/7;3/7,2/7] |
| q08 | [3,-2;1,2] | 8 | [1/4,1/4;-1/8,3/8] |
| q09 | [2,-1;1,1] | 3 | [1/3,1/3;-1/3,2/3] |
| q10 | [0,1;2,1] | -2 | [-1/2,1/2;1,0] |
| q11 | [1,2;2,4] | 0 | v=[-2,1], Av=[0,0], v nonzero |
| q12 | [0,0;-3,0] | 0 | v=[0,1], Av=[0,0], v nonzero |

q09 uses b=[1,4], giving x=[5/3,7/3] and Ax=[1,4].
q10 uses b=[3,-1], giving x=[-2,3] and Ax=[3,-1].
For each singular case, a hypothetical inverse C would imply
v=(CA)v=C(Av)=C0=0, contradicting the explicitly nonzero coordinate.
The conditional equality decision asks for this contradiction, not an
unconditionally true numeric equality. Its tautology distractor is correctly
described as true but insufficient for the requested consequence.

For each row operation, the certificate checks all four columns, reverses the
operation exactly and verifies E*A=L for current [L|E]. It derives the inverse
from original entries independently of the row route, never from an answer key
or reached label. Every actual compiled given and domain matches cases.json.
Every option is decoded to exact values and checked for requested shape and
semantic duplication before the authored key is compared. Original signs,
adjugate positions, both product orders and vector coordinate order were also
reviewed in the prose.

## Requested form versus truth

The finite numeric field reader is not a Markdown parser or general CAS.
It supports only the matrix, vector, determinant and finite method forms
documented in DESIGN.md. It distinguishes an outside scalar from a matrix
with all scalar multiplication evaluated, while comparing exact entry values
for duplicate detection. Unreduced but equivalent fractions are accepted:
the goal is evaluated matrix entries, not one preferred fractional spelling.
An existing-row swap goal separately excludes a reversible row combination.

All 21 negative probes reject:

- False accepted key; partial whole-row update; unchanged second augmented
  column; adjugate order and sign errors.
- A true unevaluated inverse in both fields, option only and reached state
  only; reversible addition missing the stated existing-row method.
- Equivalent fractional matrix duplicates and an equivalent outside-scalar
  duplicate.
- Out-of-bound integer and boolean original; zero row divisor; direct
  zero-determinant inverse division; a singular route attempting the formula.
- Changed compiled original right-hand side and real domain.
- The true zero null vector missing the nonzero/normalization goal; a true
  tautology missing the conditional consequence; false reverse product.

Five controls pass: equivalent fractional inverse entries; an alternate
two-column array wrapper with equivalent fractions; equivalent augmented
fractions; equivalent determinant fraction; reversed swap notation.
First answer positions are 1,2,3 repeated four times; later positions rotate
and are verified, while feedback remains bound to its choice ID.

## Lesson and teaching review

The eight native lesson blocks are start, terms, rule, condition, worked,
errors, practice and summary. The reading defines entry order, identity,
row-column products, inverse, determinant, adjugate, augmentation, pivots,
complete row operations and their inverse operations. It now explicitly
distinguishes the given right-hand-side vector b from the scalar matrix-entry
notation b. The worked original H=[1,-1;0,2] is distinct from every exercise.
Its inverse [1,1/2;0,1/2], three augmented matrices and both identity products
are checked exactly, with computed matrices bound to actual lesson source.

Hint, Answer and Solution are independently closed; the proof disclosure is
separate. The family-lessons receipt reports one reading, twelve questions,
three independent worked disclosures, four closed disclosures and zero windows.
The public worked block contains its given and task, not its answer. Learner
titles are neutral and public blocks omit wave/version or implementation
status. All 64 explanations and 128 selected-choice corrections were reread
against their actual option arithmetic. Valid but goal-missing choices are
not described as invalid row operations.

The repeated identity-product options are deliberate checks of the two factor
orders; the detailed histories calculate all four entries. Choosing a tile
does not demonstrate unaided multiplication or proof writing. This limitation
belongs in the evidence, not in learner-facing lesson prose.

Primary conditions were read in the Georgia Tech Interactive Linear Algebra
sections recorded with exact URLs and access date 2026-09-10 in DESIGN.md and
ordinary authoring provenance. Exercises and prose are original.

## Exact headless checks

All final commands ran from /Users/kogaryu/iggy3d/paths and exited 0:

    b/sorter --inspect-documents --documents content/authoring/production/wave03/linear_algebra/authoring/documents
    b/paths_learning_document_tests --question-batch content/authoring/production/wave03/linear_algebra/authoring/documents
    b/paths_learning_document_tests --family-lessons content/authoring/production/wave03/linear_algebra/authoring/documents
    PYTHONPATH=tools python3 -B content/authoring/production/wave03/linear_algebra/certificate_tests.py --routes build/production/wave03/linear_algebra/routes.json

Outputs are inspection.json, routes.json, lessons.json and mathematics.json
in build/production/wave03/linear_algebra/. The existing export_learning.Target
and provenance validator also pass; provenance.json covers all thirteen IDs.
No package export or publication was requested or performed.

The real Markdown-edit regression changes q01 step 10's prompt and option
12's feedback in a scratch source, compiles that actual file, reruns the exact
mathematics and proves the full routes JSON differs in exactly those two
fields. mathematics.json names its content-keyed scratch paths and hashes.
Earlier scratch output is retained as superseded evidence, not active source.

One intermediate certificate rerun failed with FileExistsError because the
shared exporter's write_tree deliberately uses exclusive creation and an
earlier scratch file already existed. No mathematical check failed. The test
now preserves content-keyed scratch files and checks existing bytes before
reuse. The final rerun passes. execution.json records the failure and repair.

The Release sorter, model and UI-test binary hashes match the frozen
build/production/wave01/build-ready.json. No rebuild or UI-test binary run was
performed. Source/evidence/helper/executable SHA-256 maps and exact command
receipts are in production.json.

## Remaining concerns and ownership

No known unresolved mathematical or source-structure defect remains in this
finite family. The certificate does not grade arbitrary proof prose, all TeX
spellings or a general inverse-solving curriculum. Author prose review is not
independent coordinator review. Native visual appearance and learner outcomes
are unobserved; no rendering or educational-effectiveness claim is made.

All edits are confined to this assigned source and evidence folder. Shared
tools, coordinator briefs/assignments, earlier published sources, runtime,
3D assets, stores and user saves were not edited. No windows, screenshots,
screen captures, images, font/ImGui initialization, clipboard access, commits,
publication, other-task messages or completion callback. The coordinator owns
capture, independent review, corrections, integration and serial publication.
