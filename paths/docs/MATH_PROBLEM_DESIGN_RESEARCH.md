# Designing mathematics problems for learning

Research date: 2026-09-10. Status: research and proposed authoring improvements.
This note does not change the four running pilots, their pinned instructions,
the document grammar, or the accepted native textbook layout. It is material for
the coordinator's review and the next assignment design.

The central recommendation is to design each mathematical family around a
specific learning claim, observable reasoning, anticipated errors, deliberate
variation, and a later check. A large collection of correct answers does not
by itself establish a coherent course or demonstrate that students learned it.

**Evidence and its limits**

The sources below serve different purposes. Professional guidance describes
defensible design practice; classroom experiments test particular interventions;
assessment-system documentation addresses engineering reliability. None directly
evaluates Paths or guarantees transfer to every advanced subject.

| Source inspected | Finding and scope |
| --- | --- |
| [MAA, Instructional Practices Guide (2018)](https://maa.org/wp-content/uploads/2024/06/InstructPracGuide_web.pdf), CP.2.1–2.5 and DP.4.1 | Undergraduate guidance connects task selection to objectives, prior knowledge, readiness, cognitive demand and delivery conditions. Backward design starts from learning goals. This is a research-informed professional guide, not an experiment proving one universal recipe. |
| [Cornell CALM, Writing Good Questions activity](https://e.math.cornell.edu/sites/activelearn/training-workshops/Writing-Good-Questions/activity.pdf), Part 2; [workshop slides](https://e.math.cornell.edu/sites/activelearn/training-workshops/Writing-Good-Questions/presentation.pdf), 2022 | Faculty development materials ask colleagues to solve and classify one another's questions by the knowledge and cognitive processes required. Use this as a review lens, not a validated numerical difficulty scale. |
| [Cornell, Good Questions calculus bank](https://pi.math.cornell.edu/~GoodQuestions/GoodQuestionSlides.pdf), updated 2003, opening Limits questions | A concrete university example of selected-response questions about concepts, domains and counterexamples, accompanied by instructor explanations. It demonstrates a format; it does not establish the effectiveness of our adaptation. No bank questions are imported by this note. |
| [IES/WWC, Organizing Instruction and Study to Improve Student Learning (2007)](https://ies.ed.gov/ncee/wwc/PracticeGuide/1), recommendations 1–5b and 7 | The guide rates spacing, alternating worked examples with solving, and connecting representations as moderate evidence; retrieval through quizzes and deep explanatory questions as strong evidence. These are the guide's ratings for its reviewed literature, not measured effects in Paths. |
| [IES/WWC, Teaching Strategies for Improving Algebra Knowledge (2015; revised 2019)](https://ies.ed.gov/ncee/WWC/PracticeGuide/20), recommendations 1–3 | For grades 6–12, the guide recommends analyzing solved work, attending to algebraic structure and intentionally choosing among strategies. It rates the first two recommendations minimal evidence and strategy choice moderate evidence. Minimal evidence is not evidence of no benefit. |
| [Rohrer and Taylor (2007), The Shuffling of Mathematics Practice Problems Boosts Learning](https://digitalcommons.usf.edu/psy_facpub/1767/), author institution's abstract | Two experiments with college students separately examined spacing and mixing problem types. Both favored those arrangements on tests one week later. This supports testing delayed mixed practice here; it does not supply an optimal schedule or a universal effect size. |
| [Durkin, Rittle-Johnson, Star and Loehr, Comparing and discussing multiple strategies](https://cdn.vanderbilt.edu/t2-my/my-prd/wp-content/uploads/sites/3147/2021/03/CEMS_Y3Topic1Paper_FullFinalVersion.pdf), DOI 10.1080/00220973.2021.1903377, abstract and discussion | The Algebra I classroom intervention combined teacher development, materials, comparison and discussion; posttest findings were promising, especially for flexibility. The authors also report implementation limitations in earlier work. Showing two solutions in an app is not equivalent to reproducing this intervention. |
| [University of Waterloo, Designing Multiple-Choice Questions](https://uwaterloo.ca/centre-for-teaching-excellence/catalogs/tip-sheets/designing-multiple-choice-questions), stems and alternatives | Recommends clear, self-contained prompts, plausible distractors reflecting mistakes, one best answer, and avoiding irrelevant wording or answer clues. Three options can be sufficient. Its general advice needs mathematical judgment: quantifiers and counterexamples can be the intended mathematical content. |
| [STACK, Deploying](https://docs.stack-assessment.org/en/STACK_question_admin/Deploying/) and [Testing, debugging and quality control](https://docs.stack-assessment.org/en/STACK_question_admin/Testing/) | Advises generating and testing variants before deployment, including correct and incorrect responses. Variant identity supports reproducibility and statistics. Technical tests do not establish equal difficulty; learner data are needed. This supports our existing candidate/compiler/checker boundary, without requiring STACK integration. |

All listed sources were accessed as text on the research date. No screenshots,
captures, PDF page images or native rendering were used. Historical publication
dates above come from the sources, rather than search-engine crawl dates.

**Proposed design procedure for Paths**

The procedure below is a project-specific synthesis. Its examples and suggested
review fields are original design proposals, not quoted institutional standards.

1. State a narrow learning claim before selecting numbers. For example: the
   learner can distinguish a reversible equation transformation from a move
   that actually isolates the variable term. Record the prior knowledge needed
   to make that distinction. A chapter title alone is too broad for this purpose.
2. Decide what evidence the response can provide. Selecting a correct result,
   recognizing its justification, repairing an error and constructing a proof
   are different observations. A selected explanation shows recognition of that
   explanation; it does not demonstrate that the learner can generate it.
3. Solve the intended task and plausible alternative routes before authoring its
   choices. Specify domains, quantifiers, units, endpoints and exceptional
   cases. Identify exactly why one option meets the stated goal.
4. Design each distractor as a reproducible error or a meaningful competing
   strategy. Record the mistaken intermediate work that produces it. Treat a
   learner's selection as evidence consistent with that mistake, not a certain
   diagnosis of their thinking. Guessing and reading errors remain possible.
5. Preserve a meaningful learner decision at each step. Let the learner choose
   an applicable rule, missing quantity, condition, representation or correction.
   A long chain of trivial clicks can hide the main mathematical decision.
6. Write support in layers: essential givens and notation; an orienting hint;
   a relevant rule or subgoal; a worked step; a complete explanation and check.
   Match this to capabilities the chosen template actually has. Keep assistance
   optional where the current contract requires it; measure exposure separately.
7. Vary one meaningful feature during initial comparisons. Change a sign, an
   endpoint, an ordering, a condition or a representation with an explicit
   purpose. Later combine features and remove method cues. Numerical repetition
   remains useful, but should have a named practice purpose and bounds.
8. Include examples and boundaries. A unique-solution equation can be contrasted
   in the reading with an identity or contradiction. A theorem should have an
   example and an explanation of why its hypotheses matter. A playable boundary
   case needs a supporting mathematical checker before being admitted.
9. Make feedback identify the issue, explain the consequence, and connect the
   correction to the original mathematics. Retain valid working after a wrong
   response. A retry teaches correction; a later fresh item checks whether the
   learner can apply it again.
10. Revisit the idea after other work and after a delay. A later mixed set should
    require recognizing the mathematical family rather than relying on the
    chapter heading. Timing and mix are design variables to evaluate, not fixed
    intervals supposedly prescribed by the research.
11. Review the prompt as a learner with the stated prerequisites. Check whether
    titles, preceding examples, labels, help and answer patterns provide an
    unintended shortcut. Verify both the concise response area and the complete
    optional explanation.
12. Validate learning claims against actual use. Keep assisted success, first
    attempts, retries, delayed performance and performance in changed contexts
    distinct. A slow response is not automatically weak knowledge; reading and
    accessibility needs also affect time.

**Three independent design dimensions**

| Dimension | What changes | What it should not silently imply |
| --- | --- | --- |
| Mathematical demand | Familiarity of the structure, number of concepts, choice of strategy, generalization or proof | Large numbers alone mean deep reasoning. |
| Available support | Definitions, subgoals, examples, hints and disclosures | The underlying equation, domain or correct result changes. |
| Evidence collected | Result selection, method choice, justification recognition, error repair or constructed work | All response types demonstrate the same proficiency. |

The existing [support contract](QUESTION_PRACTICE_FORMAT.md) already separates
complexity from assistance. Its four support levels apply to bounded linear and
matrix templates; the running prepared-choice pilots use the separate
[six-role contract](EXERCISE_ROLES.md). This research does not add four support
levels to choices.v1 or require the user to type mathematics.

**Original example: one equation, several useful decisions**

Given real x and the equation 6x-8=10, consider this prompt:

> Which equation removes the added constant while keeping the coefficient 6
> and preserving the original solution set?

| Choice | Mathematical status | Intended distinction |
| --- | --- | --- |
| 6x=18 | Add 8 to both sides; x=3. Meets the goal. | Correct balancing operation and goal. |
| 6x-16=2 | Subtract 8 from both sides; still x=3. Misses the stated isolation goal. | A valid operation can move away from the goal. |
| 6x=2 | Does not preserve the original solution; gives x=1/3. | Inconsistent treatment of the two sides. |

The full correction for the third choice can show the left addition
6x-8+8=6x and the right addition 10+8=18. If x=1/3 is substituted into the
original left side, it gives -6, which is not 10. For the second choice, the
feedback should acknowledge that equality was preserved, then explain that the
constant became -16 instead of cancelling. Do not call every unhelpful move
mathematically invalid.

Follow-up designs can reuse the concept without repeating the same decision:

- Ask which inverse returns 6x=18 to the original equation.
- Contrast 6x-8=10 with 6x+8=10, explicitly examining the changed sign.
- Compare adding 8 first with dividing both complete sides by 6 first; both
  can produce a valid solution route.
- Present 10=6x-8 in a later mixed set and ask for x without naming a method.
- Translate an appropriate verbal relationship into an equation, then solve it.
- In a separately supported family, contrast 0x=0 and 0x=2 and classify their
  solution sets. These are outside the current unique-solution pilot checker.

These are proposed future items, not additional assignments for the running
algebra writer. Repeating the same equation with more help does not create a
fresh retention test.

**Reusable author brief, before mass generation**

Initially place these answers in a family's design/review document. These are
review fields, not new parser directives or a proposed runtime schema.

| Field | Required answer |
| --- | --- |
| Learning claim | What specific mathematical ability is being taught or checked? |
| Prerequisites | What notation, arithmetic and prior concepts does it depend on? |
| Mathematical contract | Domain, parameters, restrictions, correct result, proof and independent verification. |
| Learner decision | What must the learner decide at each checkpoint? |
| Misconceptions | How each incorrect choice is produced, why it fails, and what feedback explains it. |
| Worked example | A separate example, every material step, conditions and original-problem check. |
| Variation plan | What remains invariant; what changes; numerical repetitions versus new structures. |
| Support and disclosure | What is visible before responding, after help, after an error and at completion? |
| Transfer and delayed check | Which fresh problem checks the same idea later or in another representation? |
| Review evidence | Mathematical tests, teaching review, source provenance and eventual learner observations. |

For more advanced subjects, the same brief can specify theorem-hypothesis
selection, proof-gap identification, counterexample selection, model assumptions,
existence versus uniqueness, or interpreting a computed result. Selecting among
three complete proofs can assess discrimination; it cannot certify independent
proof construction. The existing multiple-choice interaction can support the
former while later capabilities address the latter.

**Reliable generation and review**

For a prospective linear family, choose a bounded nonzero coefficient a, a
target solution s and a constant b, then derive c=a*s+b. This provides known
instances of ax+b=c; independently substitute and prove uniqueness. Choose
parameters deliberately to exercise signs and fractions. Reject cases where
distinct error procedures accidentally produce the same answer or a second
correct option. Generating from a known solution is an engineering convenience,
not independent verification of the generator.

For each family, review at least its ordinary cases, domain boundaries and
intended error mechanisms. Build the constraints into the existing certificate
provider and replay actual compiled questions. Tests can verify arithmetic,
option uniqueness, state transitions and contracts; people still review prose,
mathematical intent and the interpretation of learner behavior.

After enough real attempts, inspect first-response distributions for each
variant and each distractor. An option nobody chooses may be implausible or
simply target an already-understood distinction. An unexpectedly popular error
may indicate a misconception, ambiguous wording or missing prerequisites.
Compare variants within similar preparation and help conditions; do not infer
fairness or learning effectiveness from raw completion percentages alone.

**Application to the current wave**

The current packet already includes original-input certificates, exact domains,
specific wrong-choice feedback, separate worked examples, explanations, error
repair, symbolic responses and protected solution disclosures. These are useful
ingredients. Their implementation and teaching quality still need review in the
four returned candidates.

The next design pass should add an explicit family variation plan, a delayed
fresh check and planned comparisons between structures or strategies. The six
roles are a starting sequence, not a guarantee that every topic needs the same
number or order of questions.

All four fixed pilots deliberately share the same first-decision key-position
pattern. Balanced positions within one pilot do not prevent a repeated pattern
across pilots. Evaluate this before scaling; preserve the current pinned cases
during the wave. Any later ordering mechanism must preserve semantic choice
identity, feedback, replay and save behavior.

The separate 3D work can eventually supply mathematical evidence: predict an
effect, manipulate a relevant quantity, explain the observed invariant, then
connect it back to notation. The question owner must still judge the response.
This is a future interaction proposal, not a measured learning benefit or a
new assignment to the asset worker.

Recommended next authoring improvement: complete one reviewed family brief and
its variation plan before generating a large number of additional instances.
Keep the accepted renderer and current pilots stable while evaluating that brief.
