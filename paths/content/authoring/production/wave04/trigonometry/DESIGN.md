# Source-assisted trigonometric equations

## Claim and finite scope

Twelve complete symbolic-choice problems solve equations on the radian interval [0,2pi), including zero and excluding 2pi. Four introductory routes use factored/quadratic sine or cosine expressions; four practice routes use Pythagorean or reciprocal conversion, including impossible coordinate branches and an empty solution set; four mixed routes combine methods, retain zero-factor roots and preserve original reciprocal exclusions. In q09 and q11, existing decision slots genuinely choose among distinct strategies before executing them: a reversible factorization/reciprocal route, division that loses a zero-factor solution, and squaring that adds a non-solution.

Prerequisites are signed fractions, multiplication, factoring, squares and unit-circle coordinates. The lesson defines identity versus equation, factor versus term, substitution, zero product, original domain D, solution set S, period, principal inverse values versus all angles, and the final solution count N. Prepared choices establish these bounded decisions, not unaided proof fluency, mastery or learner outcomes.

The expression pool has integer coefficients with magnitude at most 12, rational constants made by division of such integers, leaves s=sin(theta), c=cos(theta), tan, cot, sec and csc, binary addition/subtraction/multiplication/division and squares. Adapted originals have argument theta only. Tree bounds are depth 10 and 96 nodes. Nonzero-denominator factors are confined to those already supported by the frozen Wave 03 exact helper; this family actually needs only sine and cosine exclusions. No arbitrary exponents, inverse-function approximation, frequency argument or general polynomial root finder is introduced.

The original source exercises may contain other coefficients, but those are provenance givens, not additional certified input cases. The twelve adapted original equations are the complete checked production pool. Exact branch values are rational; admissible coordinates belong to 0, +/-1/2 and +/-1. The deliberately rejected values include -3 and +/-2. Every retained angle is a rational sixth multiple of pi, with exact coordinates in Q(sqrt(3)).

## Seeds and planned adaptations

Source: David Lippman and Melonie Rasmussen, *Precalculus: An Investigation of Functions*, Edition 2.3, Section 7.1, pinned Chapter 7 PDF and text from SOURCES.json. Examples 1-4 and Exercises 13-32, 37-42 are the only approved pool. All selected exercises use explicit mathematical givens, not graphs, photos or externally credited puzzles.

| Question | Exact approved locator | Original mathematical prompt | Adaptation |
| --- | --- | --- | --- |
| q01 | Example 1, printed p.454 / PDF p.2 | 2sin^2(t)+sin(t)=0, 0<=t<2pi | Rename t to theta; retain equation and interval; add full decision route |
| q02 | Exercise 23, printed p.460 / PDF p.8 | 2sin^2(w)+3sin(w)+1=0 | Rename w; retain equation and interval; explicit branches and check |
| q03 | Exercise 19, printed p.460 / PDF p.8 | sin^2(x)=1/4 | Rename x; retain equation and interval; factor after clearing the constant denominator |
| q04 | Exercise 20, printed p.460 / PDF p.8 | cos^2(theta)=1/2 | Change right side to 1/4 for rational half-coordinate branches |
| q05 | Example 3, printed pp.456-457 / PDF pp.4-5 | 2sin^2(t)-cos(t)=1, 0<=t<2pi | Rename t; retain equation; make identity conversion and complete branches explicit |
| q06 | Example 2, printed p.455 / PDF p.3 | 3sec^2(t)-5sec(t)-2=0, 0<=t<2pi | Rename t; solve in cosine after reciprocal conversion rather than the source's secant substitution; retain cosine exclusions |
| q07 | Exercise 21, printed p.460 / PDF p.8 | sec^2(x)=7 | Change right side to 4 for exact half-coordinate angles; retain reciprocal domain |
| q08 | Exercise 22, printed p.460 / PDF p.8 | csc^2(t)=3 | Change right side to 1/4 to produce impossible sine values +/-2 and prove no solutions |
| q09 | Exercise 13, printed p.460 / PDF p.8 | 10sin(x)cos(x)=6cos(x) | Change coefficients to 2 and 1; preserve the cosine-zero factor branch |
| q10 | Exercise 14, printed p.460 / PDF p.8 | -3sin(t)=15cos(t)sin(t) | Change coefficients to -1 and 2 for exact cosine -1/2; preserve sine-zero roots |
| q11 | Exercise 17, printed p.460 / PDF p.8 | sec(x)sin(x)-2sin(x)=0 | Retain equation; expose reciprocal conversion, cosine exclusion and zero-factor branches |
| q12 | Exercise 40, printed p.460 / PDF p.8 | 3cos(x)=cot(x) | Change coefficient 3 to 2 for exact sine 1/2; retain original sine exclusion |

The distinct worked example adapts Example 4, printed pp.457-458 / PDF pp.5-6: tan(x)=3sin(x), 0<=x<2pi. Change 3 to 1 and x to theta, obtaining tan(theta)=sin(theta). Its zero-product branches overlap at theta=0, which is counted once; cosine zeros remain excluded. This is not one of the twelve question originals.

All twelve questions therefore have distinct approved seed locators, exceeding the required six. Every cases.json record includes source_id, exact locator, unambiguous original mathematical givens, adaptation_kind and changes. The adaptations preserve traceable source problems rather than merely citing general identities.

## License, attribution and source-reading record

Selected content license: CC BY-SA 4.0, https://creativecommons.org/licenses/by-sa/4.0/ . Credit David Lippman and Melonie Rasmussen, Edition 2.3, https://www.opentextbookstore.com/precalc/2.3/Chapter%207.pdf . The source registry snapshot was acquired at 2026-09-11T03:16:52.020541+00:00. The pinned front matter explicitly identifies the Attribution-ShareAlike 4.0 International license; its exact PDF/text hashes and chapter PDF/text hashes are checked against the registry. No NC detailed solutions manual is used. No [UW] exercise, externally credited puzzle, figure or photograph is adapted. The separate Chapter 3 Stitz/Zeager attribution is not part of these Chapter 7 seeds.

The adapted lesson and questions are distributed under CC BY-SA 4.0, with an explicit adaptation notice in metadata and the public lesson summary. This does not assert a license for the surrounding application or imply author endorsement. Our new teaching, choices and misconception feedback remain part of that adapted content. The source answer discussion is background, not the mathematical oracle.

Reading difficulty: the pinned layout text flattens superscripts. A second, raw text extraction of printed p.460 / PDF p.8 was inspected to distinguish square exponents in Exercises 19-23 from arguments such as 2x in Exercises 15-16. The two representations agree on all selected givens. No image or rendering was used. Exercises 15-16 are not selected because frequency transformations add an unnecessary second capability; Examples 5 and Try it Now prompts are outside the assigned seed pool and are not used. Non-special-angle coefficients in Exercises 20-22 and 13-14/40 are deliberately changed as recorded above, not silently approximated.

## Independent certificates and complete solution proof

Reuse frozen Wave 03 `certificate_tests.py` only for bounded exact expression evaluation, rational polynomial reduction, denominator zero sets, equality and finite display serialization. Reuse frozen Wave 01 `generate.py` only for exact unit-circle rotations and established fraction/pi/set formatting, not its generation entry point or keys. Record hashes of these and imported shared helpers; never edit them. New logic is confined to these twelve equations and their step certificates.

Each case supplies an untrusted two-linear-factor witness and an explicit clearing multiplier. Expand the factors with exact arithmetic and prove their product equals the multiplier times the original left-minus-right expression using circle reduction. Prove that the multiplier is nonzero on the original domain. This checked identity makes the factorization reversible; its roots are then derived as -b/a, not taken from authored answer keys. The witness is a proof to verify, not an answer oracle. Every proposed factorization/equation option is likewise tested by exact expansion against its local original-derived polynomial, with a separate form predicate for expanded versus factored or reciprocal/identity rewriting.

Zero product proves all algebraic branches. Coordinate range removes impossible values, then a coordinate line intersects the unit circle in exactly two points for absolute value below 1, one point at an extreme, and no points outside the range. Exact rotations verify the proposed representatives and that count proves each branch complete in the continuum. Unite every branch, remove duplicate angles, apply original exclusions and [0,2pi), and evaluate the original equation exactly at each retained angle. No finite-angle scan or graph establishes completeness. Endpoint zero is retained whenever valid; 2pi is excluded even when coterminal with a valid root.

Actual compiler givens and domain must match original inputs. Every actual option, key, reached state and local goal is checked. Finite typed option registries avoid a new Markdown or TeX parser. Equivalent root/angle orderings and coterminal spellings cannot compete as separate answers. Positive alternative controls demonstrate equivalence-aware checking; true-but-wrong-form probes demonstrate that truth alone is insufficient. Negatives include false keys/work, missing zero branches, principal-only roots, invalid reciprocal domains, added 2pi, lost zero and equivalent duplicates.

For the two method-choice slots, correct plans are proved reversible by exact identities and original denominator conditions. The division plans have explicit original-solution witnesses where the proposed divisor is zero. The squaring plans have exact witnesses satisfying the transformed equation but not the original. These counterexamples disprove reversibility; they are not point-sample proofs of an identity or completeness. Squaring can be used with later rejection of added roots, and the feedback says that it misses the requested reversible-strategy goal rather than declaring all squaring invalid. cases.json owns only mathematical/option/goal data; unused prompt copies are removed, leaving Markdown as the wording authority.

## Teaching, gates and ownership

The maintained content is one direct choices.v1/lesson.v2 chapter. Normally five to eight meaningful decisions cover domain, identity/method, factored equation, coordinate branches, allowed values, complete angles and original substitution/completeness. First key positions occur four times each; later order varies while IDs retain their feedback. Specific @why and individual corrections remain editable Markdown, with symbolic choices kept compact.

The canonical reading defines all notation and rules and has the required start, terms, rule, condition, worked, errors, practice and summary blocks. The distinct worked example has independently closed Hint, Answer and Solution; its answer is not repeated in public blocks. All twelve questions reference the single lesson, whose twelve practice links retain assignment order.

Run the existing inspector, question-batch, family-lessons and certificate_tests.py --routes gates. Validate metadata through Target.inspect and provenance, then record exact source/evidence/binary/dependency/snapshot hashes. Real scratch Markdown prompt and wrong-feedback edits must change only the intended compiled fields. Record observed start, case-design, draft-ready and checks-done timestamps, source difficulties and correction rounds, without invented token or weekly-usage savings.

Only this Wave 04 subject's authorized files and build evidence may change. Earlier waves, snapshots, shared tools/runtime, saves and 3D work remain frozen. No workers, callbacks, screenshots, captures, images, windows, font/ImGui probes, clipboard, rebuild, publication, store activation or commits. Return production.json normally at ready_for_coordinator_review, not accepted or published. Native appearance and learner effects remain unobserved.
