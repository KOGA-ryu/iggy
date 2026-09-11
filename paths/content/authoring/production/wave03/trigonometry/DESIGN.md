# Trigonometric identities with retained domains

## Learning claim and prerequisites

Twelve complete symbolic-choice simplifications establish algebraic identities on the full original domain. Prerequisites: signed arithmetic, fractions, factoring, difference of squares and unit-circle coordinates. The reading defines s=sin(theta), c=cos(theta), reciprocal and quotient functions, identity versus equation, original expression E, retained domain D, common denominator and conjugate.

A completed route establishes selected algebra/domain/form decisions with available reading. It does not establish written-proof fluency, general symbolic algebra competence, mastery, retention or empirical transfer. choices.v1 is the existing prepared workflow, not a new four-level typed solver.

## Bounded mathematical representation

The finite originals use coefficients 0, 1 and 2; test/distractor coefficients are integers of magnitude at most 3. Allowed leaves are s, c, sin(theta), cos(theta), tan(theta), cot(theta), sec(theta), csc(theta). Binary addition, subtraction, multiplication, division and squares form a bounded expression tree: depth at most 10, at most 96 nodes, and no arbitrary function arguments or exponents. This is a finite audit representation, not learner syntax or a runtime parser.

After reciprocal/quotient definitions, exact rational functions in s,c are checked by cross-multiplying their polynomial numerators and denominators, then reducing c squared to 1-s squared with rational coefficients. Polynomial intermediate degree is bounded at 16. A zero remainder is an exact proof on the retained nonzero-denominator domain, not evidence from sampled angles.

Allowed denominator-zero factors are s, c, 1+s, 1-s, 1+c and 1-c, including products/powers through four factors, and nonzero constants. Pythagorean-equivalent forms such as 1-s squared reduce to the same factors. The positive expression 1+cot squared is handled exactly by its numerator s squared+c squared=1, not by dropping its own sine-domain restriction. No arbitrary polynomial root finder is introduced.

These factors vanish only at the four axis directions modulo 2pi. The checker derives their complete zero sets algebraically: s=0 at 0/pi; c=0 at pi/2/3pi/2; 1-c=0 at 0; 1+c=0 at pi; 1-s=0 at pi/2; 1+s=0 at 3pi/2. Intermediate denominators may have no additional zeros on the original domain. Every reached state retains that original domain even when its displayed expression extends beyond it.

## Finite coverage and full routes

The twelve originals and candidate expression trees are explicit in cases.json. The candidate registry is a typed audit dictionary for matching the compiler's displayed mathematics; it contains no trusted accepted keys. The checker computes truth, safe domain and local-form validity for each decoded option independently of its position or authored key. Only this finite expression pool and its explicit regression controls are certified.

| Group | Cases and skills |
| --- | --- |
| Introductory q01-q04 | Pythagorean numerator replacement and cancellation; sine/cosine symmetry; tangent plus cotangent; reciprocal-square difference with its cosine restriction |
| Practice q05-q08 | Common denominators and expanded/factored numerators; a zero expression with nonempty exclusions; conjugate rationalization in both sine/cosine orientations |
| Mixed q09-q12 | Explicit strategy selection followed by nested reciprocal/quotient simplification, rationalization, Pythagorean reciprocal ratios and two conjugate denominators |

The final mixed routes choose and execute a strategy. Every question begins with E, determines or retains its full original domain, carries out the intermediate operations, and finishes by multiplying the original E by an explicitly nonzero polynomial multiplier. The resulting polynomial identity checks the simplified result without point samples.

First correct positions occur four times each; later presentation varies without detaching semantic IDs from their corrections. The maintained content is one direct chapter.paths.md, not a generator or template dialect.

## Truth, form and misleading alternatives

Local goals distinguish a common-denominator fraction, an expanded numerator, a factored numerator, a squared numerator, a cancelled single factor, a rationalized denominator or a reciprocal-square form. An expression can be mathematically equal to E while failing one of these requested forms. Tests explicitly reject such a true-but-goal-missing control while accepting a different valid form spelling/order when it meets the goal.

Authored options are pairwise mathematically distinct, not merely different TeX strings. The verifier rejects equivalent competing options, including extra spacing, reciprocal aliases and reordered polynomial terms. Domain options are compared by their full excluded direction sets, so redundant conditions do not masquerade as new choices. Valid equivalent controls are tested singly, not offered as competing answers.

Each @feedback explains that selected expression's algebraic or domain error. Original exclusions from sine, cosine, 1 plus/minus sine/cosine and nested quotient denominators are never cancelled away. Factoring/cancellation requires a nonzero complete factor, not cancellation of an added term.

## Reading and disclosure

The canonical lesson contains start, terms, rule, condition, worked, errors, practice and summary blocks. It defines all notation, demonstrates every rule condition and gives an original worked example different from all twelve questions. Only its original expression is public; Hint, Answer and Solution are independent closed disclosures, with an additional proof where useful. All twelve questions @read the same lesson, which links all twelve in order. No production-status or assessment claims appear in learner text.

## Verification and limits

The existing compiler, --question-batch, --family-lessons, Target.inspect and provenance own format/replay/metadata checks. certificate_tests.py --routes PATH consumes the actual compiled route JSON. Original givens/domain, every option/key and reached state are compared with exact mathematical certificates and explicit form predicates. A finite expression decoder maps only registered rendered trees and controlled equivalent spellings; it is not a general TeX, Markdown or symbolic parser.

Required negatives include false keys, false working, equivalent duplicate choices, invalid inputs/domains, cancelled-domain loss and true goal-missing expressions. Positive alternative controls avoid a literal-string-only checker. Real scratch Markdown prompt and individual-feedback edits must affect only those compiled fields. The final production receipt covers source (including this design and review.md), evidence and unchanged Release executable hashes. No self-hashing receipt cycle is used.

## Attribution and ownership

Original Paths exercises, explanations and finite certificates. Definitions and conditions browsed 2026-09-10: [OpenStax Precalculus 7.1, Solving Trigonometric Equations with Identities](https://openstax.org/books/precalculus/pages/7-1-solving-trigonometric-equations-with-identities). This provides the Pythagorean, reciprocal/quotient, common-denominator and identity-verification context. No external exercise prose is copied.

Only this Wave 03 subject's permitted files and build/production/wave03/trigonometry evidence are writable. All Wave 01/02, shared tooling/binaries, runtime, saves and 3D assets remain frozen. No new generator, general CAS, parser, runtime mode, screenshots/images/windows, fonts/ImGui, clipboard, commits, publication, task messaging or delegation. Return the production receipt normally; coordinator review/capture/integration is separate. Freeze after this one family. Native appearance and learner outcomes remain unobserved.
