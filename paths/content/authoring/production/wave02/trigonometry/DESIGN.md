# Complete trigonometric solution sets

## Learning claim and prerequisites

Twelve complete prepared-choice problems carry the learner from an original equation to all requested angles and a final check. Prerequisites are signed rational arithmetic, reversible equality operations, radians, unit-circle coordinates and interval notation. The lesson defines argument u=k theta+phi, coefficient A, offset B, right side C, normalized coordinate h, frequency k, phase phi, period T, known solution angle alpha, integer n, branch, set, empty set, N (points per argument turn) and M (retained solutions or branches per theta period).

Successful choices establish the requested decisions with available reading and feedback; not independently written proofs, four-level typed support, mastery, retention or empirical transfer. No new runtime owner or document syntax is introduced.

## Mathematical contract and finite coverage

The supported authoring bounds are integer 0<|A|<=3, |B|,|C|<=4; k in {1,2}; phi/pi in {0,1/2,-1/2}; real radian theta. The normalized value h=(C-B)/A is in {0,+/-1/2,+/-1} or outside [-1,1]. Only the twelve explicit cases in cases.json are delivered and tested, not all combinations of these bounds.

There are six sine and six cosine problems. Ten ask for [0,2pi); two ask for all real solutions with an explicit integer n. Four introductory, four practice and four mixed/exceptional questions are ordered in the canonical lesson. The first key appears four times in each of three positions; later ordering changes deterministically while semantic IDs retain their feedback.

| Cases | Skills and variation |
| --- | --- |
| q01, q02 | Complete positive-half sine/cosine equations; distinct coordinate reflections, all periodic branches, interval filtering and original substitution |
| q03, q04 | Signed/nonunit coefficients, extreme coordinate, coincident reflected branches; cosine must include zero but exclude 2pi |
| q05, q06 | Frequency two expands the argument interval to four pi; zero sine needs four angles, extreme cosine two; divide the argument, not only a chosen branch |
| q07, q08 | Opposite phase shifts, negative sine coordinate or cosine zero; shift both argument bounds, retain negative argument representatives when valid |
| q09, q10 | Method selection before algebra; combine frequency and opposite shifts in a general real solution, carry both branches and divide the full periodic term |
| q11, q12 | Positive and negative normalized values outside the function range; end with a contradiction and the empty set, not invented angles |

The equations have nonzero A, so subtracting B and dividing by A are reversible. For a feasible h, x²+y²=1 gives two coordinate-line intersections for |h|<1 and one at |h|=1. Reflection preserves sine across the vertical axis and cosine across the horizontal axis. Periodic argument branches differ by 2n*pi. With k positive, 0<=theta<2pi maps exactly to phi<=u<2k*pi+phi. Solving theta=(u-phi)/k is bijective; the half-open interval retains k complete turns without duplicate endpoints. General solutions retain integer n and the branch period 2pi/k. Outside the coordinate range no real u, hence no theta, can solve the original equation.

## Decisions, distractors and explanation

Every local goal has three symbolic choices and one correct semantic ID. Isolation distractors normally change h by +/-1/2, with explicit substitution arithmetic. For impossible equations, they instead clip h to a range endpoint or replace it by zero; these produce distinct nonempty solution sets rather than three contradictory equations. Range distractors misclassify feasibility or intersection count. Branch distractors omit a branch or shift all branches by pi/6 to a different coordinate. Bound distractors shift only one end or use the wrong interval width. Filtering distractors lose a valid point or add the excluded upper argument. Back-substitution distractors leave the argument unchanged or offset the correct theta set by pi/12. General-method distractors use the other coordinate's reflection or period pi for the argument; general-theta distractors omit the phase or fail to divide the period. Final-check distractors give wrong arithmetic or count, distinguishing membership from completeness.

All normal problems have five or seven decisions; general problems six; genuinely impossible cases three. Final substitution and completeness are meaningful separate checks. Unshifted k=1 problems omit redundant bound/conversion steps. Wrong feedback is attached to the selected symbolic option, not an inferred diagnosis of the learner.

## Canonical reading and disclosure

One ordinary authoring/documents/chapter.paths.md maintains the lesson, all arithmetic/choices/keys, prompts, explanations and individual corrections. There is no generator, template dialect or copied six-role framework. One lesson.v2 has start, terms, rule, condition, worked, errors, practice and summary blocks. Every question links to that lesson, and it links to all twelve.

The distinct worked example is 3sin(2theta-pi/2)+1=-2. Its original given alone is public; Hint, Answer and Solution are separately closed. Solution includes isolation, range, branch construction, transformed bounds, integer filtering, solving for theta, endpoint removal and original substitution. The general proof is another closed disclosure. No public block reproduces the worked solution.

## Independent mathematical and compiled-content evidence

certificate_tests.py accepts --routes PATH from the existing model. It derives h from A/B/C, checks input types/domain and compares actual compiled givens/domain, all options/keys and every reached state with exact independently derived expectations. Unit-circle membership reuses the published Wave 01 exact rotation helper read-only, while branch enumeration and transformed bounds are derived directly from original inputs. The independent coordinate-line count proves completeness over real angles, not merely sampled points. Additional direct theta-lattice enumeration provides a second interval calculation; general formulas are checked algebraically for all integer n.

Negative tests reject false keys, false intermediate work, equivalent duplicate options and invalid cases/domains. Real scratch Markdown prompt and individual-feedback edits must reach compiled fields without changing mathematics. The shared importer, --question-batch, --family-lessons, Target.inspect and provenance remain authoritative for format, replay and attribution. Input and executable hashes are recorded before/after. Structural acceptance is not a proof of prose clarity or native visual correctness.

## Attribution, ownership and handoff

Original Paths problems, teaching, distractors and checks. Definitions/conditions browsed 2026-09-10: [OpenStax Precalculus 2e 5.1, Angles](https://openstax.org/books/precalculus-2e/pages/5-1-angles), [5.2, Unit Circle](https://openstax.org/books/precalculus-2e/pages/5-2-unit-circle-sine-and-cosine-functions), [6.1, Sine and Cosine Graphs](https://openstax.org/books/precalculus-2e/pages/6-1-graphs-of-the-sine-and-cosine-functions), and [7.5, Solving Trigonometric Equations](https://openstax.org/books/precalculus-2e/pages/7-5-solving-trigonometric-equations). No external exercise wording is extracted.

Write only the assigned wave02/trigonometry source and evidence; preserve coordinator BRIEF.md, Wave 01, shared tools/binaries, other writers, runtime, stores and 3D work. No screenshots, images, windows, fonts/ImGui, commits, publication or delegation. Ordinary repairs are local. Deliver production.json at stage ready_for_coordinator_review with hashes and actual checks, then one completion callback to coordinator 01a07b52-505e-7e10-822b-17f39b7f2fd1 on local and freeze source. Coordinator acceptance, aggregate/save review and publication are separate.
