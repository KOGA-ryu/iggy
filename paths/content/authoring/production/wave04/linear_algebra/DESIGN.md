# Span, independence, basis and coordinates

## Finite contract fixed before the cases

Work over the real numbers in R2 or R3. Original ordered vector lists have
two to four distinct columns with integer entries of magnitude at most seven;
the zero vector is permitted. Targets have two or three integer coordinates
of magnitude at most seven. Original defining plane normals have integer
entries of magnitude at most three. Coefficient and null witnesses use exact
rationals with numerator magnitude and positive denominator at most twelve;
row-working values stay within magnitude 1000. No polynomial/function spaces,
complex vectors, approximate tests, arbitrary dimension or general CAS.

Teach whether a target is in a span, why a homogeneous relation establishes
dependence or independence, and why a basis requires independence AND spanning
of the named space. Coordinates are with respect to an ordered basis. A basis
for a proper plane inside R3 has two vectors, not three. Original pivot
columns, not columns of a row-reduced matrix, supply a column-space basis.
Prerequisites are signed rational arithmetic, vector coordinates and complete
reversible row operations. Successful symbolic decisions establish bounded
recognition/completion, not unaided proofs, retention or measured mastery.

## Primary source and adaptation

Jim Hefferon, Linear Algebra, fourth edition, 2020-Apr-26.
Primary source: https://jheffero.w3.uvm.edu/linearalgebra/book.pdf
Author license offer: https://hefferon.net/source.html
Selected license: CC BY-SA 3.0 US,
https://creativecommons.org/licenses/by-sa/3.0/us/
The source registry records access at 2026-09-11T03:16:52.020541+00:00.
The registry and local PDF/text/license snapshots are hash-verified; exact
digests are in production.json. These adapted lesson and questions are
released under that same selected content license. This is not a statement
about the surrounding application's license. Paths supplies the changed
numbers/tasks, teaching routes, symbolic choices and corrections; no author
endorsement is claimed.

Approved seeds are restricted to the registry. The twelve cases each name a
distinct seed and reproduce its original mathematical givens in cases.json:

| Question | Exact source locator | Adaptation |
| --- | --- | --- |
| q01 | Two.I.2.23(a), printed 103 / PDF 113 | Change two xz-plane generators and target; construct and verify coefficients. |
| q02 | Two.I.2.22, printed 103 / PDF 113 | Retain all original numbers; add row reduction and explicit annihilator proof of non-membership. |
| q03 | Two.I.2.26(c), printed 104 / PDF 114 | Retain generators; add target (2,1,0), exact coefficients and explain failure to span R3. |
| q04 | Two.I.2.28(a), printed 104 / PDF 114 | Choose explicit generators for the xz-plane and an outside target; prove non-membership. |
| q05 | Two.II.1.23(a), printed 117 / PDF 127 | Retain original columns; add homogeneous calculation and distinguish independence from ambient spanning. |
| q06 | Two.II.1.21(b), printed 117 / PDF 127 | Retain original columns; require a relation normalized to third coefficient 1 and verify it. |
| q07 | Two.II.1.21(c), printed 117 / PDF 127 | Retain both original vectors, append zero; exhibit the resulting dependence. |
| q08 | Two.II.1.23(b), printed 117 / PDF 127 | Write original row vectors as columns; change second coordinates 3,4,11 to 1,2,5, retaining the relation u3=u1+2u2. |
| q09 | Two.III.1.22(a), printed 127 / PDF 137 | Retain target and ordered basis; add method, basis proof and original reconstruction. |
| q10 | Two.I.2.26(e), printed 104 / PDF 114 | Retain all four vectors; select original pivot-column basis of their span, add target (-1,1,0) and coordinates. |
| q11 | Two.III.1.27(b), printed 127 / PDF 137 | Convert row-space description x+y=0 to column notation; construct ordered parameter basis and add target (2,-2,3). |
| q12 | Two.III.1.23, printed 127 / PDF 137 | Retain vector and both ordered bases; expand into both complete coordinate solves and checks. |

The distinct worked seed is Two.I.2.27(a), printed 104 / PDF 114:
the source asks to parametrize and span {(a,b,c) as row vectors: a-c=0}.
Adapt to column vectors with x=z, ordered parameter basis
((1,0,1),(0,1,0)), and target (2,3,2). The hidden solution reduces
[1,0|2;0,1|3;1,0|2] by R3-R1 and verifies
2(1,0,1)+3(0,1,0)=(2,3,2). The exercise's original task is preserved
as the spanning proof; the target/coordinates and teaching are added.

Read local definitions of subspace/span in Two.I.2, homogeneous independence
in Two.II.1 and ordered basis/representation in Two.III.1. The main PDF's
multi-column layout extraction mixes vector brackets and columns. A second
pdftotext -raw extraction of PDF pages 113-114, 127 and 137 makes each vector
consecutive and confirms every selected entry. No entries were guessed.
No image/rendering was used. The adjacent externally attributed Cleary
exercise Two.I.2.24 is excluded; polynomial/function-space subparts, externally
attributed puzzles and question-mark problems are not used. The answer PDF is
not the oracle and is not needed to derive these results.

## Progression and local goals

Four membership problems lead from coefficients to inconsistency witnesses.
Four homogeneous problems distinguish independence, dependence, zero vectors
and missing ambient directions. Four basis problems include method selection,
original pivot columns, a proper plane and two different ordered coordinate
systems. q09 and q10 select methods and carry them through.

Every displayed original vector list is a COLUMN matrix A; coefficient c
means A*c. Row operations act on all columns including the target/zero column.
Wrong options model partial rows, wrong signed multiples/divisors, misplaced
coordinates, a zero witness, missing homogeneous conditions and confusion
between a plane and R3. Their corrections use actual operands.

Witness normalization and original-column basis selection are explicit goals.
Different valid bases and nonzero multiples of a null relation are
mathematically valid, but may miss those local goals. Do not offer reordered
or column-rescaled equivalents as competing basis choices, nor scalar
multiples of the same dependence relation. In contrast, a genuinely different
basis can be a true-but-goal-missing option when original pivot columns in
increasing order were requested; explain precisely why.

## Verification boundary

Use exact original minors for rank and original-row Cramer systems for
coefficients, checked against every original row. These are independent of
the authored row route. Derive homogeneous witnesses from the original
columns with the declared normalization. Verify every whole-row transition
and its inverse using the shared two-row operation helper on the selected
pair of complete rows. For non-membership verify ell^T*A=0 and ell^T*w!=0.
For bases prove independence and express every original generator in the
selected basis; for a defining plane prove parametrization symbolically.
Verify coordinates by original reconstruction in their stated order.

The certificate consumes actual --routes output, never an authored answer
key as mathematical oracle. The bounded field recognizer handles only numeric
matrices/vectors and the finite classification/method forms used here, not
Markdown or general TeX. Semantics retain ordered coordinates and requested
column origin, while duplicate comparison normalizes nonzero relation scale
and unordered column directions when appropriate. Include negative faults and
valid alternative controls. Primary seed text is provenance, not the oracle.

## Source, reading, checks and handoff

Maintain only this DESIGN.md, cases.json, certificate_tests.py, review.md and
one ordinary authoring metadata/Markdown chapter. No generator, copied common
parser, runtime, renderer or shared-tool edits. Read earlier exact helpers
without modifying them and hash dependencies. The ordinary lesson has start,
terms, rule, condition, worked, errors, practice and summary blocks; the
public summary gives readable attribution, license URL and adaptation notice.
Keep worked Hint, Answer and Solution independently closed. All twelve
questions read this lesson and its practice links follow the reserved order.

Run existing inspection, model routes, family-lessons, Target/provenance and
exact certificates. In scratch Markdown change an actual prompt and feedback,
compile, and prove an exact two-field delta with unchanged mathematics.
Check Release hashes without rebuilding. Record observed start, case design,
draft ready and checks-done timestamps, source difficulties and correction
rounds. Do not estimate tokens or account-wide savings.

Only this Wave 04 subject folder and build/production/wave04/linear_algebra/
are writable. Earlier waves, registry, briefs, assignments, shared tools,
runtime, stores/saves and 3D work remain untouched. No screenshots, images,
captures, windows, font/ImGui initialization, clipboard, publication or commits.
Return production.json and a normal final response; no callbacks. Freeze at
handoff and stop after this one family. Coordinator review, visual appearance
and learner outcomes remain separate from these authoring checks.
