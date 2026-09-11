# Trigonometry Wave 01 — revision 2 teaching contract

## Learning claim and prerequisites

Both families assess decisions about real radian angles in [0,2pi): interpreting an equation's required coordinate, finding another branch, isolating the trigonometric value, distinguishing reflections, repairing an interval error, and selecting a complete solution set. Prerequisites are signed rational arithmetic, equality, interval inequalities, angle orientation and unit-circle coordinates. The lessons define theta, alpha, pi, radians, x/y coordinates, A/B/C, integer k, set braces, the empty set, and square roots.

Success establishes selection of the requested symbolic choices with available reading and corrections. It does not establish unaided written proof, retention, mastery, general inverse-trigonometric fluency or measured transfer. These six authoring roles are not six support levels; choices.v1 retains the existing prepared-choice owner.

## Mathematical contract

- sine_turn: chapter topic_0032, Unit Circle Framework; sine is the vertical coordinate. Its preserving reflection is pi-alpha across the vertical axis, reduced modulo a full turn.
- cosine_turn: same chapter and domain; cosine is the horizontal coordinate. Its preserving reflection is 2pi-alpha across the horizontal axis. The sine reflection does not generally preserve cosine.
- Originals have A*f(theta)+B=C with integer 0<|A|<=2, |B|<=2, |C|<=4, and (C-B)/A in {0, -1/2, 1/2, -1, 1}. No zero divisor, approximate coordinate, degree-based domain or arbitrary trigonometric equation is playable.
- Subtract B on both complete sides; divide by nonzero A. Multiplication by A and addition of B reverse those moves.
- Each accepted angle must satisfy the original equation and 0<=theta<2pi. No duplicate or coterminal answers are allowed. Zero is retained exactly when it solves the equation; 2pi is always excluded.
- A fixed coordinate h in x²+y²=1 leaves the other coordinate squared equal to 1-h². Thus there are two circle intersections for |h|<1 and one for |h|=1. Membership, exact cardinality and the one-turn direction bijection establish completeness over real angles, not merely over sampled angles.

Recipe data supplies all 36 declared cases, including exact levels, coefficient signs, offsets, known-branch indices and six different balanced answer-position permutations. The producer constructs C from the level but passes only original coefficients and the supplied angle to certificates; no seed answer/level crosses that boundary. Cases outside this explicit finite recipe are not a tested random pool.

The production solver lists special-angle branches and adjacent periodic copies. Its independent membership check uses exact repeated pi/6 rotations in Q(sqrt(3)), never an inverse lookup in the branch table. The test oracle independently folds angles into a right triangle using quadrant symmetries; it derives h from A/B/C, enumerates the 12 sixth-pi directions and proves completeness using the circle count. The test compares all 61 lattice angles from -4pi through 6pi for both coordinate functions and tests every declared question, every decision and every wrong option.

## Variation plan

Teaching introduces ordinary half-coordinates and the first endpoint repair. Practice changes signs/offsets and includes zero or an extreme coordinate. Fresh_check deliberately includes concept changes, not only new constants:

| Feature | sine_turn | cosine_turn | Invariant |
| --- | --- | --- | --- |
| Fresh worked_check | Zero height, with the second axis intersection | Zero horizontal coordinate, with the other axis intersection | Find a distinct second solution |
| Fresh explain_step | Supplied alpha is the second positive-half branch | Supplied alpha is the second positive-half branch | Correct coordinate-preserving reflection, reduce by a full turn |
| Fresh repair_error | Negative half-height branches plus a spurious endpoint | Negative extreme coordinate; a single circle point | Find first invalid line before selecting a corrected set |
| Fresh independent | Extreme positive height collapses two reflected branches into one | Zero horizontal coordinate requires two axis points | All and only original-equation solutions in [0,2pi) |

The three sets use different original offsets; practice notation also changes sign. A known angle can lie beyond the first quadrant: alpha means a supplied angle, not necessarily an acute reference angle. The independent role has no hint or supplied method. These manually designated sets are not a spaced-review scheduler or empirical transfer result.

Positions are zero-based in recipe.json. Each six-question set uses positions 0, 1 and 2 twice; all six permutations differ. Only presentation order changes. Semantic option IDs stay attached to their key and their individual Markdown feedback.

## Decisions and wrong-option reasoning

| Role | Goal-correct decision | Wrong choice 1 | Wrong choice 2 |
| --- | --- | --- | --- |
| read_notation | Exact isolated coordinate | Opposite sign; original substitution has the wrong right side | Coordinate increased by 1/2; substitution fails |
| worked_check | Other permitted matching angle | Known angle repeated: valid membership but misses the request | Quarter-turn gives a different coordinate |
| choose_next_step | Equivalent equation with function coefficient 1 | Valid equation A*f(theta)=C-B, but its nonunit multiplier remains | Wrong isolated coordinate; substitution fails |
| explain_step | Coordinate-preserving axis reflection | Half-turn negates both coordinates | Other axis reflection negates the required coordinate |
| repair_error, first | L2 | L1 is valid isolation, not a completed set | L3 merely endorses the already wrong list |
| repair_error, second | Complete filtered set | Missing the final valid angle, possibly an empty set | Retains excluded 2pi |
| independent | Complete original solution set | Angles at a different coordinate give a wrong original left side | Omits a valid branch or the only extreme solution |

The certificate and tests distinguish numerical equivalence from goal validity. The choose_next_step intermediate is valid, not called false. Repetition in worked_check is a genuine solution but not another solution. For explain_step the two non-preserving transforms give distinct directions with the opposite nonzero coordinate. No accidental correct or equivalent displayed answers are accepted.

Each wrong feedback line is separately authored in the family's questions.paths.md.in. It contains the actual arithmetic or transformation, not a generic retry message. Complete @why explanations likewise live in Markdown. Python supplies numeric/symbolic substitutions and semantic choices only. The repair's L1 is an isolated coordinate equation; its first reached state and explanation identify only L2, without supplying the corrected set.

## Reading, disclosure and identity

There is one maintained lesson and one six-role question template per family, reused by all three sets. The single canonical reading IDs are prod01_trigonometry_sine_turn_r and prod01_trigonometry_cosine_turn_r. Each consolidated family package contains one lesson.v2 reading and eighteen choices.v1 questions.

Each lesson has start, terms, rule, condition, worked, errors, practice and summary blocks. The separate worked equations are 2sin(theta)+2=4 and 2cos(theta)+3=5; each differs from every active original in its family. Only its original given is public. Hint, Answer and Solution are independently closed. The solution contains each arithmetic operation, the circle-point count, endpoint/periodicity filtering and substitution into the original. General special-angle justification is in the separate closed Proof, not a public duplicate of the worked answer.

Question IDs are stable hashes of original cases, prefixed by family/set/role. Changed originals receive changed IDs. No old published/pilot IDs or payloads are replaced. Final package IDs are prod01_trigonometry_sine_turn and prod01_trigonometry_cosine_turn, version 1. The six set packages are staging evidence only and must not be published individually.

## Verification and review receipt

Run from the Paths root:

~~~sh
PYTHONPATH=tools python3 -B content/authoring/production/wave01/trigonometry/generate.py --target b/sorter --model b/paths_learning_document_tests
PYTHONPATH=tools python3 -B content/authoring/production/wave01/trigonometry/certificate_tests.py
~~~

Generation uses the existing pinned packet(), reasoning_certificate(), reasoning_documents(), chapter_text(), check_assignment(), verify_role_content(), target inspection and provenance, and pure model replay. Consolidation passes eighteen questions through the same chapter wrapper, compares actual compiled question objects against their staging results, checks 21 decisions/42 wrong choices and save replay, and records the shared --family-lessons output. It does not add a document parser or runtime solver.

Tests mutate real staged Markdown prompts for all twelve family/role combinations and confirm only the intended actual compiled prompt changes. They also edit a real individual feedback sentence per role. Fourteen false keys and fourteen false reached states are deliberately compiled and rejected by the certificate/content comparison. Equivalent compiled labels, bad input types, unsupported heights, corrupted producer branches, missing branches, duplicate/coterminal answers and endpoint/domain errors are negative cases, not claimed passes.

Source/binary hashes and immutable verification paths live in build/production/wave01/trigonometry/production.json and tests.json; review.md records the final current receipt paths and commands. Old immutable artifacts remain historical evidence, not maintained sources. The former prose_question route, embedded step prose, hard-coded variation pool, dead assembled document and unused partial read_notation templates are removed.

## Attribution and writer handoff

Definitions and conditions were browsed on 2026-09-10 in OpenStax Precalculus 2e: [5.1 Angles](https://openstax.org/books/precalculus-2e/pages/5-1-angles), [5.2 Unit Circle: Sine and Cosine Functions](https://openstax.org/books/precalculus-2e/pages/5-2-unit-circle-sine-and-cosine-functions), and [7.5 Solving Trigonometric Equations](https://openstax.org/books/precalculus-2e/pages/7-5-solving-trigonometric-equations). Exercises, explanations and finite-case construction are original Paths authoring, not extracted source problems.

Writable source is this subject folder except coordinator-owned BRIEF.md. Outputs are confined to the assigned production tree and existing shared trig candidate staging. Shared tools, reviewed pilots, published content, runtime and other workers remain untouched. No screenshots, windows, publication, commits or new workers are authorized. Ordinary content failures are repaired locally; unsupported mathematical/runtime requirements are reported as precise blockers.

Return one completed revision-2 handoff to coordinator task 01a07b52-505e-7e10-822b-17f39b7f2fd1 on local, requesting coordinator review/integration. Automated acceptance here means authoring/math/model gates passed; coordinator prose review, aggregate integration and publication remain separate. Native visual appearance and learner outcomes are unobserved, not claimed.
