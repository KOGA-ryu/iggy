# Reviewed subject pilots

The four returned pilots are corrected, checked together and published to the
local Library on 2026-09-10. They add four readings and 24 multiple-choice
questions: six per subject, with seven decisions and fourteen wrong-choice
corrections in each subject. This is one reviewed sample family per subject.
Native visual acceptance and evidence of learning remain separate.

The review follows the [family teaching brief](templates/QUESTION_FAMILY.md).
The fixed assignment packet and original cases remain unchanged. Worker
delivery reports describe their earlier candidates; this document and the
[verification record](../build/subject-pilot-review/verification.json) describe
the integrated revision. Original worker bytes are retained under
`build/subject-pilot-review/before/`.

## Teaching claims and limits

| Subject and reading | Observable learning claim | Prerequisites and mathematical boundary |
| --- | --- | --- |
| Algebra → Worked linear practice → Understand each balanced move | Read signed coefficients, select an equivalent move that meets a goal, identify its inverse, repair a sign error and select an exact solution. | Signed arithmetic, fractions and equality. Real linear equations with nonzero coefficient; existence by substitution and uniqueness by cancellation. Zero-coefficient cases are explanatory boundaries, not playable additions. |
| Trigonometry → Unit Circle Framework → Find every sine solution in one turn | Translate an interval, isolate sine, use reflection and include every permitted angle exactly once. | Radian measure, interval notation, signed numbers and sine as unit-circle height. The fixed cases use [0,2π) and heights 0 or ±1/2; the separate worked example uses height 1. No general trigonometric solver is claimed. |
| Calculus → Differentiation → Build a derivative from a difference quotient | Distinguish function value from derivative, expand a difference quotient, justify nonzero cancellation, repair an expansion and select a derivative value. | Function evaluation, polynomial arithmetic and an introductory limit. Real dimensionless polynomials of degree at most two. Integer coefficients from -9 through 9 and evaluation points from -3 through 3 are the checked provider domain. |
| Linear Algebra → Worked matrix practice → Read and justify a complete row operation | Translate an augmented row, operate on its constant as well as its coefficients, choose and reverse a row replacement, repair it and check a pair in both original equations. | Signed arithmetic, fractions, ordered pairs and simultaneous equality. The fixed two-equation cases use the existing bounded exact matrix certificates; this is not arbitrary matrix reduction or singular-system coverage. |

Success shows the learner selected the requested decisions with the available
reading and feedback. It does not demonstrate independently written proofs,
retention, general transfer, mastery or a complete chapter. These six roles are
an authoring sequence, not six runtime support levels. The new cards retain
the existing prepared multiple-choice interaction.

## Deliberate variations and checked decisions

| Role | Algebra | Trigonometry | Calculus | Linear algebra |
| --- | --- | --- | --- | --- |
| Read notation | Preserve the coefficient sign: (-4, 7, 19). | Translate [0,2π) to 0 ≤ θ < 2π. | Recognize the defining quotient at input 3. | Read [-3, 2 ∣ 7] as -3x+2y=7. |
| Complete a calculation | Removing -6 gives 9+6=15. | Add the second positive-height angle, 5π/6. | The simplified quotient 4+h has limit 4. | The constant column is 7-2(2)=3. |
| Choose a move | Subtract 4 from both sides, giving -3x=9. A reversible addition can still miss the cancellation goal. | Isolate sin θ=1/2 in 2sin θ-1=0. | Factor the numerator as h(6+h). | Replace R2 with R2-3R1 to eliminate x. |
| Explain a condition or inverse | Multiplication by -4 reverses division by -4. | π-α preserves sine height for the stated α. | Cancelling h requires h ≠ 0; the limit approaches zero through permitted values. | Adding 2R1 reverses subtracting 2R1. |
| Repair the first error | Identify L1, then separately calculate 7-(-5)=12. | Identify L2's extra integer, then repair the set to {0,π}. | Identify L2's expansion error, then correct the derivative to 5. | Identify L1's constant-column error, then calculate y=3. |
| Fresh application | A negative coefficient and exact fractional answer: x=-3/2. | A negative sine height and a complete pair: {7π/6,11π/6}. | Include the linear term: derivative 10, whereas function value is 9. | Check (x,y)=(3/2,1/2) in both original equations. |

The invariant is each family's operation or definition. The sequence varies
sign, representation, missing calculation, necessary condition or error before
combining features in the final case. These fixed pilots are purposeful samples;
they are not a claim that every adjacent item changes only one feature, or that
an arbitrary random-number generator preserves the teaching plan.

Wrong options represent specific possible mistakes: omitted signs or columns,
one-sided operations, the wrong reflection, an excluded endpoint, a missing
solution branch, division at zero, incorrect expansion, or function value in
place of derivative. Each option is checked against the actual requested goal.
A wrong selection suggests a possible misconception; it does not establish a
diagnosis of the learner.

## Corrections made by the coordinator

- Removed the common correct-answer position sequence. Only displayed choice
  lines were reordered, deterministically by SHA-256 of the question, step and
  choice identities. Semantic choice IDs, answer keys and feedback remain
  attached. The four first-step position sequences are now distinct.
- Trigonometry, calculus and matrix worked-example blocks now present their
  original givens with the full calculation under Solution. Hint, Answer and
  Solution use the existing independent disclosures. All readings provide the
  shared start, terms, rule, condition, worked, errors, practice and summary
  blocks; algebra's existing rule/condition block IDs were aligned.
- Corrected algebra's reversed-operation certificate explanation and ambiguous
  recovery wording. Its first-error decision now stops before the arithmetic
  answer, leaving an actual second decision. Feedback checks the original
  equality without unexplained residual terminology.
- Trigonometry's notation card now asks for a translation instead of displaying
  the accepted inequality as its given. Defined radians, unit-circle radius,
  coordinate height, angle orientation and bracket meaning. Its provider now
  checks the assigned interval on every relevant route, exact input types,
  distinct angles, interval membership and complete branch count. Removed an
  unused interval helper.
- Calculus now compares an explicit expansion of (a+h)² by convolution with a
  separate derivative-by-degree calculation on the original coefficients.
  It verifies the constant, linear and quadratic coefficients of the difference.
  Tests cover all 48,013 coefficient/point combinations and inject faults into
  each of those expansion outputs. Defined capital A/B/C versus lowercase a,
  and removed answer cues from the cancellation and error-diagnosis domains.
- Corrected the matrix lesson's claim that a sign before the augmented bar
  belongs to y: in [-3,2∣7], -3 is the coefficient of x. Replaced raw LaTeX in
  plain prose and made wrong-pair feedback evaluate both original left sides.
  The existing matrix mathematical owner remains unchanged.

The immutable PILOT documents retain their original assigned presentation,
including choice order. The corrections above are explicit coordinator
overrides to those presentation details. Original sequence cases, identities,
correct mathematical decisions and all 30 pinned references are unchanged.

Definitions and conditions were checked against primary textbook sources:
[OpenStax equality properties](https://openstax.org/books/elementary-algebra-2e/pages/2-2-solve-equations-using-the-division-and-multiplication-properties-of-equality),
[radian measure](https://openstax.org/books/precalculus-2e/pages/5-1-angles),
[unit-circle functions](https://openstax.org/books/precalculus-2e/pages/5-2-unit-circle-sine-and-cosine-functions),
[trigonometric equations](https://openstax.org/books/precalculus-2e/pages/7-5-solving-trigonometric-equations),
[the derivative definition](https://openstax.org/books/calculus-volume-1/pages/3-1-defining-the-derivative),
and [augmented matrices and row operations](https://openstax.org/books/college-algebra-2e/pages/7-6-solving-systems-with-gaussian-elimination).
These checks support the mathematical conventions. The original Paths wording,
question choices and proposed teaching sequence are not claims of textbook
endorsement or measured learning effectiveness.

## Verification and publication

Release builds of `sorter`, `paths_learning_document_tests` and
`paths_learning_document_ui_tests` pass. The four certificate suites and final
candidate gates pass. The combined integration test checks all four packages,
12 independently opened worked disclosures, refusal of an added public worked
result, repeated publication, unchanged old question records, save replay and
preservation of active content after a malformed subsequent import.

The save-upgrade fixture now chooses a multi-step unfinished question and uses
each selected question's own controls. It previously assumed the first two
imported questions shared an interaction type. This test-only correction lets
the regression exercise a mixed Library without changing the runtime.

The actual local Library contains **413 questions, 959 readings, 190 chapters
and eight subjects**. All earlier 389 question records, 955 reading records and
published payload bytes compare unchanged. All four packages are version 1.
The final active generation is
`aac3fe81ef47fcae16a6c7d5246595ed7a5b3b44b03bc7532d083b0a34ef7a59`.
Actual-generation save replay passes using an isolated representative save;
it does not read or alter the user's personal progress.

The native textbook adapter also passes its data-only published-store checks
for copying, catalogue entries and reload behavior. It creates no ImGui context
and performs no font, clipboard, window or screenshot access. The accepted
layout, production runtime, parser, renderer, persistence schema and 3D assets
are unchanged. Three existing authoring certificate providers and their relevant
tests changed; no new production code file was added. Work remains uncommitted.

Final source/executable hashes, exact commands, candidate paths, publication
receipts, catalogue comparisons and test results are recorded in
`build/subject-pilot-review/verification.json` and its linked evidence files.
Automation checks these bounded mathematical and structural contracts. Prose
clarity and native appearance still need the user's judgement.

## Manual check and next writer boundary

Launch `b/sorter`, open Library and use the four paths in the first table.
Open each worked example's Hint, Answer and Solution separately. In a question,
the **gold Given** should stay identifiable beside **cyan Working**; a wrong
choice should retain the working and explain its mismatch. **Green completion**
should stay until Next. Check algebra's two-stage repair, trigonometry's interval
endpoints, calculus's 9-versus-10 distinction and the signed matrix coefficient.

The next candidate is one small practice and fresh-check set for an accepted
subject family, starting with the existing linear-family generator. Before any
trigonometry/calculus/matrix expansion, complete a family-specific brief naming
the exact parameter pool, strata, exclusions and independent checker. Return
one candidate through the existing gate; the coordinator reviews it before
serial publication. These pilot certificates do not authorize arbitrary new
cases, concurrent activation, a full-subject batch or a review scheduler.
