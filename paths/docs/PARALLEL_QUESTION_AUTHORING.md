# Parallel question authoring: recipe 1

Status: **instruction packets prepared; subject workers have not started**.
This first wave reserves four readings and 24 multiple-choice questions. It
does not assign the completion of four entire disciplines. The user's approval
of the current recipe establishes the presentation reference; each new lesson
still needs mathematical, teaching and eventual visual review.

## Start here

| Subject | Worker brief | Exact pilot | Existing chapter |
| --- | --- | --- | --- |
| Algebra | [AGENTS.md](../content/authoring/parallel/algebra/AGENTS.md) | [PILOT.md](../content/authoring/parallel/algebra/PILOT.md) | Worked linear practice |
| Trigonometry | [AGENTS.md](../content/authoring/parallel/trigonometry/AGENTS.md) | [PILOT.md](../content/authoring/parallel/trigonometry/PILOT.md) | Unit Circle Framework |
| Calculus | [AGENTS.md](../content/authoring/parallel/calculus/AGENTS.md) | [PILOT.md](../content/authoring/parallel/calculus/PILOT.md) | Differentiation |
| Linear algebra | [AGENTS.md](../content/authoring/parallel/linear_algebra/AGENTS.md) | [PILOT.md](../content/authoring/parallel/linear_algebra/PILOT.md) | Worked matrix practice |

The [assignment register](../content/authoring/parallel/assignments.json) owns
reserved identities, folder boundaries, chapter bindings and reference hashes.
Each subject's sequence.json owns its six fixed cases, objectives and
prerequisites. Those supplied cases are assignment inputs, not completed cards.
PILOT.md supplies the decisions, answer expectations and teaching requirements.
If the inputs disagree, report the exact conflict to the coordinator; do not
quietly choose a different equation or weaken a mathematical check.

The recommended initial authoring setting is Terra High for each subject.
Evaluate these bounded pilots before changing model settings or batch size.
An instruction file does not start a worker or schedule ongoing work. Dispatch
requires a separate user instruction. Four subject tasks are the intended team;
actual concurrent task capacity must be checked when dispatching.

## Authority and ownership

Read [Paths AGENTS.md](../AGENTS.md), this contract, the subject brief and its
pilot. Use the current [exercise roles](EXERCISE_ROLES.md) and the
[shared textbook presentation standard](QUESTION_PRACTICE_FORMAT.md#shared-textbook-presentation-standard).
For exact syntax use [Learning documents](LEARNING_DOCUMENTS.md).
The broader [lesson task template](templates/LESSON_TASK.md) remains for lessons
that require app/model integration; these text-only assignments use the filled
pilot packets instead of inheriting its 3D integration requirements.

Every worker owns only these new files in its assigned subject folder:

- **lesson.md.in**: the complete structured reading.
- **questions.paths.md.in**: six complete prepared questions in the assigned order.
- **certificates.py**: this family's original-input mathematical certificates.
- **certificate_tests.py**: independent checks and mathematical rejection cases.
- **authoring.json**: original authorship and exact reference attribution.
- **review.md**: teaching review, evidence paths and unresolved issues.

The supplied AGENTS.md, PILOT.md, sequence.json, shared contract and register
are read-only to subject workers. Generated evidence belongs under
build/parallel-authoring/SUBJECT/; temporary files may use that subject's
scratch/ directory. Workers do not edit another subject, shared tools/tests,
the root build, active packages, progress, corpus IDs, renderer or 3D sources.
Source problem pages outside the project and bundled source snapshots stay
read-only. Preserve concurrent changes. Leave work uncommitted.

The coordinator owns shared contract changes, checker review, existing batch
registry integration, builds, aggregate coverage and serial publication.
The 3D worker owns assets and model controls. These pilots bind no new figure;
record a useful future figure in review.md with its mathematical purpose and
pre-answer visibility. A missing figure cannot block these text pilots, and an
existing model is not evidence that it can judge an exercise.

## The teaching recipe

Use exactly the native lesson.v2 textbook blocks and choices.v1 question
controls. No HTML, custom page geometry, screenshots, font probes or substitute
renderer. Gold Given and cyan Working remain adjacent to the response. Wrong
choices retain working; a correct decision advances one step; green completion
stays until Next. Existing question owners provide those behaviours.

Every reading must include these stable local blocks, numbered 1.1 onward
where useful: start (introduction), terms (definition), rule (proposition),
condition (example or proposition), worked (example), errors (example),
practice (exercise), summary (summary). More blocks are allowed when they
explain a missing step. Number a displayed equation when referring to it;
do not introduce decorative numbering or enlarge controls to fit more prose.

Define each unfamiliar symbol at first use in words: its role, domain, order,
units if applicable, and how to read it. Explain the rule, its conditions, why
it preserves the relevant meaning, the intermediate arithmetic and the check
against the original problem. A nonzero restriction needs an explanation of
what fails at zero. A complete-solution claim needs an argument that no branch
was missed. Compact controls do not justify compressed reasoning.

The separate worked example in each pilot is mandatory. Put its Hint, Answer
and Solution in independent closed disclosures; use a separate Proof disclosure
where justified. Definitions and essential givens remain available without
revealing the exercise's answer. Public titles, goals, prerequisite text,
captions and the last question's prompt must not expose its solution or method.
Review what appears before a response, after a wrong response, after
each accepted step and after opening each help disclosure.

All six roles are multiple choice, including independent:

| Role | Evidence required | What the learner does |
| --- | --- | --- |
| read_notation | notation_interpretation | Interprets the symbols and their roles. |
| worked_check | worked_example | Supplies one missing result in a shown calculation. |
| choose_next_step | method_condition | Selects a move that achieves the stated goal under its conditions. |
| explain_step | justification | Selects a reason, inverse or condition, with an actual argument behind the key. |
| repair_error | first_error | Identifies the earliest invalid line, then corrects its consequence. |
| independent | fresh_context | Solves a different problem without a prescribed intermediate step. |

The last role means fewer cues with optional reading. It is not written work,
an unassisted-exposure guarantee or a mastery judgment. Four support levels
already exist for bounded linear.v1 and matrix.v1 tasks; that capability is
separate from these six roles. These pilots neither require typing nor claim
four written support levels for prepared questions.

Use three short symbolic choices per decision. Each pilot specifies the choices
and key positions; do not sort them into an answer-position pattern. All wrong
options need specific @feedback, plus a general @wrong and a complete @why.
Exactly one option meets the stated question. A valid operation can still miss
the requested goal; explain that distinction rather than calling it invalid.
Incorrect sample work must be labelled deliberately incorrect in @domain
and shown in the givens, never inserted into accepted working before selection.

## Authoring and mathematical checks

Follow these exact editable references; the register pins their bytes:

- [Matrix role reading](../content/authoring/learning/matrix_reasoning/lesson.md.in)
  and [questions](../content/authoring/learning/matrix_reasoning/questions.paths.md.in):
  the current six-role structure, feedback and disclosure example.
- [Linear teaching template](../content/authoring/learning/linear_reference/question.paths.md.in):
  the accepted depth for definitions, balanced operations and substitution.
- [Shared chapter wrapper](../content/authoring/learning/chapter.paths.md.in):
  the sole chapter/reading/practice-link assembly route.

Templates use literal authored mathematics plus the existing fields
{{reading_id}} and {{ROLE_id}}, {{ROLE_title}}, {{ROLE_objective}} for each
role's full name. The shared tool supplies them from the register and sequence;
do not invent extra interpolation syntax or maintain another chapter wrapper.
Copy the matrix example's @goal, @given, @domain, @read, @step, @choice, @answer,
@feedback, @after, @wrong, @why and @end structure. References demonstrate format;
use the actual assigned cases.

**certificates.py** exports CHECKERS, a dict with the six existing role names.
Each value is a pure function accepting that role's sequence record and returning
a four-tuple: (given_latex, reached_latex_states, decisions, evidence_facts).
Each decision is (three_choice_labels, accepted_label). Strings must match the
intended rendered mathematical strings exactly, including LaTeX.

Use build_question_batch.require for mathematical rejection. The shared
reasoning_certificate() supplies IDs and evidence kinds; validate_role_sequence()
supplies metadata checks; verify_role_content() compares actual compiled givens,
reached working, options and accepted labels. Do not reproduce those functions.
Each provider must derive the mathematics from the original case independently
of the authored template and key. It must not read those templates or return a
truth flag based only on matching the assignment's expected answer. Reuse the
existing matrix certificate table where it already covers the assigned cases.

The case contract, separate proof/calculation and counterexamples belong in
certificate_tests.py and review.md. Reject an altered key, an altered reached
equation, equivalent options, a missing valid branch, and the subject-specific
boundary failures in the pilot. Numerical spot checks alone cannot prove a
symbolic identity or completeness. The shared gate checks certificate agreement;
it does not fact-check prose or prove a newly written provider is correct.
The coordinator must inspect that provider and its independent checks.

All examples and prose are original project material. Use the pilot's primary
textbook sections to verify definitions and conventions; record exact sections,
URLs, access date and what was checked in review.md. Correct or flag a hasty
corpus definition instead of copying it as authority. External exercise text
requires its own permission/attribution decision; these assignments do not need
it. authoring.json uses the existing export schema, the reserved package ID,
version 1, and source coverage for the reading plus all six question IDs.
Use a portable paths:original/PACKAGE/v1 URI for original material.

## One shared command per pilot

Run from /Users/kogaryu/iggy3d/paths. Replace SUBJECT with the exact assigned
folder name; linear_algebra uses an underscore. These commands are headless:

~~~sh
python3 -B tools/check_authoring_pilot.py --packet-only
PYTHONPATH=tools python3 -B content/authoring/parallel/SUBJECT/certificate_tests.py
python3 -B tools/check_authoring_pilot.py --subject SUBJECT
~~~

The first command checks the instruction/reference bytes and 24 reserved IDs.
It does not claim the unwritten cards passed. The other two become usable after
that worker authors the required files. Missing sources fail with
pilot.incomplete; changed references fail with pilot.reference_changed.
Compiler errors retain file/line diagnostics; certificate errors identify
the card, role and changed field. Do not edit the reference lock to clear a failure.

The shared gate assembles through reasoning_documents(), calls the real
document compiler, replays all six questions through the pure model, compares
the actual compiled data with the certificates, and checks provenance. It writes
an immutable candidate authoring folder and a verification receipt below the
subject's output directory only after those gates pass. Rechecking identical
content is safe. It never exports, installs, activates a library or reads personal
saves. Workers must not invoke publish or install, or edit generated output.

The coordinator prepares sorter and paths_learning_document_tests from the
current build graph before dispatch. Subject workers share the built executables
read-only; they do not run four competing CMake builds. No worker may launch the
app, take a screenshot/capture, initialize ImGui/fonts or run unfiltered CTest
or paths_native_math_tests. The user performs visual checks after integration.

## Dispatch and return

Start each authorized worker from its subject folder in this prepared checkout,
or an isolated copy of these exact current files. Much of the implementation is
uncommitted: a worktree from the default branch is not a valid baseline. Record
the actual instruction paths loaded. Reference hashes establish the selected
recipe/tool inputs; they are not a snapshot of every runtime source file. Use
the same built binaries during the wave, with the gate recording their hashes.
When shared code changes, the coordinator reconciles the references and rechecks
affected work. Do not silently switch baseline or commit the shared checkout.

Dispatch prompt (substitute the subject; read the full packet):

> Complete the one pilot in PILOT.md under your current subject directory. Read
> its AGENTS.md and the shared parallel authoring contract. Write only the six
> allowed subject files. Use the fixed sequence, reserved IDs, native textbook
> format and multiple-choice controls. Finish the subject certificate tests and
> shared headless gate. Report exact output/evidence paths and any remaining
> mathematical or teaching issue. Leave publication to the coordinator, take no
> images or windows, keep changes uncommitted and stop after this pilot.

The worker's review.md records the outcome, prerequisite explanations,
notation inventory, source checks, separate worked example, every distractor's
purpose, completeness/condition arguments, exact test commands and results,
input hashes and remaining gaps. State separately: authored, mathematically
checked, compiled, model-replayed, teaching-reviewed, published, visually accepted.
Return the shared command's candidate path; do not label it accepted or published.

The coordinator reviews prose and independent proofs, checks all four packages
together for identity/reference collisions, and tests additive installation,
repeat publication and old-save preservation through the existing publisher.
Only the coordinator registers reusable family producers in the existing batch
tool. Package review and publication use the same canonical compiler and store.
Then finish the app build and give the user short manual checks with gold/cyan/
green landmarks. Publication and visual acceptance remain distinct states.

After this wave, assign one reviewed leaf and mathematical family at a time.
Use live topic IDs and review prerequisites, duplicates and scope before
filling a queue. Track drafted, checked, integrated and accepted counts separately;
six number variants are not six task families. Do not use the historical 229-leaf
linear generator ledger as an up-to-date total for all published packages.
