# Question family teaching brief

Copy this document beside a new family's authoring sources. Fill every section
before requesting a batch. This is an author/reviewer contract, not runtime
syntax and not permission to expand a writer's assigned scope. The completed
[linear example](../../content/authoring/learning/linear_family/DESIGN.md) shows
the expected detail. The [research note](../MATH_PROBLEM_DESIGN_RESEARCH.md)
explains the evidence and its limits.

## Learning claim and prerequisites

- Observable decision the learner should make:
- Previously understood notation and operations:
- What success on these choice questions establishes:
- What it does not establish (for example, writing an unaided proof):

## Mathematical contract

- Domain, original givens, existence/uniqueness and excluded cases:
- Operation rules, necessary conditions and inverse/check:
- Generator inputs and numerical bounds:
- Independent verification from original givens, without trusting the seed answer:
- Degeneracy, equivalent-option and accidental-correctness rejection:

## Variation plan

For each variation record a stable name, its purpose, what is held fixed, what
changes, and the question role that tests it. Introduce one feature at a time
where possible. Identify deliberate combinations in a fresh application.
Distinguish a new representation from a harder mathematical method. Record a
finite tested pool before expanding it; arbitrary random numbers are not a plan.

## Decisions and explanations

Use the existing six roles: read_notation, worked_check, choose_next_step,
explain_step, repair_error, independent. Keep response mode multiple choice.
For every decision specify the requested goal and one unambiguously best answer.
For every distractor record the operation/error that produces it, evidence that
it fails the stated goal, and the exact correction shown after selection.
A valid transformation can miss a goal; feedback must distinguish those cases.
An option suggests a possible mistake, not a proven diagnosis of the learner.

## Reading and disclosure

Use the existing lesson.v2 blocks and choices.v1 questions. Define each symbol
before using it. Explain: what we have, what we want, which rule applies and why,
the arithmetic on this exact example, and how to check the reached result.
Provide a worked example separate from the final fresh problem. Keep Hint,
Answer, Solution and Proof in their existing independently closed disclosures.
Do not put the answer to an active question in its domain or prompt. Compact
choices do not require compact explanations. Do not prescribe new page geometry.

## Repetitions, fresh checks and identity

List teaching, further practice and reserved fresh sets. State what transfers
between them and what remains untested. A manually reserved fresh set is not an
implemented spaced-review schedule. Retrying the same question is not new
evidence of transfer. Give new mathematical content a new stable identity;
preserve old published questions and saves. Keep choice ordering deterministic.

## Verification and review receipt

Record separate outcomes for original-given mathematics, wrong-option checks,
actual compiler/model replay, template/prose review, native visual acceptance and
learner evidence. Include exact commands, source hashes and artifact paths.
Test at least a false accepted key, a changed working line and a degenerate case.
Describe what automation cannot check. Stop if the family needs unsupported
mathematics or new runtime behavior; send that need to the coordinator.

## Writer handoff

Name exact writable files, immutable inputs, current target/version, source
attribution rules, generation/check commands, stop conditions and returned
evidence. One pilot first. A checked candidate is not publication or acceptance.
