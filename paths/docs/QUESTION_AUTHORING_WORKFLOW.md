# Creating trustworthy practice in every area

Status: **general authoring workflow with a historical linear coverage ledger**.
Current checked batch families and six-role matrix/probability sequences are
listed in the [batch workflow](QUESTION_BATCHES.md). The
[parallel authoring contract](PARALLEL_QUESTION_AUTHORING.md) supplies current
instructions for four new subject pilots, all using multiple choice. The
linear coverage ledger below counts only its original 25 questions; it does
not aggregate published packages or establish current library totals.
This does not complete the question bank. Read
[QUESTION_PRACTICE_FORMAT.md](QUESTION_PRACTICE_FORMAT.md) for the learner-facing
contract. The user prioritizes this foundation before more interactive 3D work.

## Presentation reference and workstream boundary

Use the [shared textbook presentation standard](QUESTION_PRACTICE_FORMAT.md#shared-textbook-presentation-standard)
for every new equation-solving family and source-card adaptation. Inspect the
closest existing textbook lesson and any relevant figure provider before
designing its presentation. Under the [current work allocation](../AGENTS.md#current-work-allocation),
the textbook worker owns lessons, reference material, formatting, question
solving, saved work, the content pipeline and figure integration. The separate
worker builds 3D assets/models; teaching is part of this textbook workstream.

For the pilot, name the reused definition/block IDs, source revisions, equation
conventions and existing mathematical checker. Describe each help disclosure
and any figure's pre-answer visibility. A model's existence does not establish
that it accepts the question's exact state or can judge its response. Reuse the
existing presentation and mathematical owners through explicit bindings. Keep
unsupported source material recorded as an authoring gap, not silently dropped
or labelled playable. The first adapter must preserve this standard before a
bulk conversion of Claude's source cards.

## Coverage unit and quantity

The original source-map baseline has six subjects, 178 chapters and 94 named
subcategories. Forty-three chapters have named children; the other 135 do not.
That baseline has **229 practice leaves**: 94 named subcategories and 135
terminal chapters. Track every leaf, and roll totals up to its chapter and
subject without counting the same instance twice. Track conceptual skills
within each leaf. The hastily drafted taxonomy is provisional, not a verified
curriculum. Review titles, scope, duplicates and prerequisites before filling a
leaf; preserve its stable ID or record an explicit replacement mapping.

Initial target: **24 reviewed instances covering at least four distinct task
families per leaf**, normally six per family. Expansion target: **60 instances
covering at least six families**, normally ten per family. These are production
targets, not a claim that an exact count produces mastery. A proof-heavy leaf
may need fewer genuinely distinct authored tasks; record the reason rather
than manufacture near-duplicates to reach the count.

Four support levels do not multiply the count. Six sign/number variations do
not turn one mathematical task into six task families. A family represents a
different mathematical objective or route, such as solving a unique system,
characterizing a free-variable family, or recognizing inconsistency. Track
parameter variety separately from task variety and support level.

Each coverage row must include: taxonomy review; mapped skills; families;
drafted instances; mathematically checked instances; formatted instances;
integrated instances with their actual response template; and unresolved gaps.
Record four-level support only where implemented. Avoid a single green
"done" flag. `tools/question_workflow.py` emits `coverage.json` for all 229
leaves. It counts 25 integrated repetitions only when the runtime pack matches
the checked generator; all other leaves retain zero integrated counts. A pack
match is not a substitute for the runtime or visual acceptance gates.

The current family covers 24 varied repetitions across six parameter strata,
plus its golden example. Its arithmetic and four-level runtime are checked.
Its teaching still needs the user's format review. It does not complete its
leaf or provide four different task families.

## The repeatable authoring loop

1. **Choose one leaf and one skill.** Read the existing starter, its source
   heading and nearby definitions. Check the current worktree and ownership.
   Write a one-sentence objective, prerequisite list and intended response type.
   Do not derive a skill merely from an impressive-looking title.
2. **Specify one family.** Pin the domain, symbols, units, dimensions,
   assumptions, parameter ranges, exclusions and desired outcome classes.
   List the valid solution routes, common misconceptions and structural
   variations worth practicing. Separate numerical complexity from support.
3. **Author one golden problem fully.** Supply a precise prompt, exact answer
   or rubric, complete worked reasoning and an independent verification.
   Every step gets the teaching fields from the format contract. Explain
   definitions at first use and state side conditions where they matter.
4. **Use the assigned response contract.** New role sequences use multiple
   choice throughout, with optional reading and separate help disclosures.
   Their final role removes the prescribed intermediate step while retaining
   choices; it does not claim independent written work. Existing `linear.v1`
   and `matrix.v1` tasks can also expose their implemented four support levels.
   Use the same immutable instance when changing support. Prevent unintended
   clues in titles, status text, examples and figure captions.
5. **Check the golden problem independently.** Use arithmetic, a separately
   implemented algorithm, enumeration or a proof/counterexample review suitable
   for the domain. Test alternative correct forms/routes. List unsupported
   responses explicitly. Do not use the generating formula as its only oracle.
6. **Create a small batch.** Begin with 6–12 instances for a new family. Once
   its rules pass, expand deterministic variation within the validated bounds.
   Prefer constructing known-valid instances to unconstrained random prompts.
   Record the seed, family version, parameters and exact mathematical identity.
7. **Validate every instance.** Check structure, assumptions, all required
   solution outcomes, intermediate steps, final answers and distractors.
   Deduplicate equivalent choices. Do not change a correct distractor into an
   arbitrary wrong string just to get enough tiles; choose another instance or
   a diagnosed misconception that applies to it.
8. **Check teaching and layout separately.** Verify that symbols are defined,
   reasoning contains no unstated leap, the uncued role receives no future answer, and
   long equations and reading remain reachable at the supported window sizes.
   Native typesetting success is not mathematical validation.
9. **Integrate a bounded reviewed pack.** In a parallel assignment, return the
   verified candidate to the coordinator; only the coordinator publishes.
   Use the existing content loader and
   canonical question owner after that family's checker is available.
   Freeze math IDs/versions; retain source attribution and evidence. Do not
   overwrite current playable cards or infer that authoring JSON is loadable.
10. **Record results and stop.** Update coverage with the exact reached gates,
    source versions, checks and remaining gaps. Keep changes uncommitted.
    Carry one concrete family or response type into the next workstream.

Authoring and review are separate passes even when one builder performs both.
The review pass attempts to break the content: another valid answer, an excluded
domain value, a missing solution branch, a degenerate parameter, a misleading
hint, or an equivalent distractor. A model saying "verified" is not evidence;
attach the executable check or the explicit mathematical argument.

## Generation recipes by subject

| Area | Construct controlled instances | Independent check and deliberate variations |
| --- | --- | --- |
| Algebra | Choose a solution/root set, then construct an equation with bounded coefficients | Substitute in the original; prove completeness and domain restrictions. Vary signs, zero, fractions, brackets, variables on both sides and legitimate degenerate outcomes in separately declared families. |
| Trigonometry | Choose exact angles/identities and a finite interval, then formulate the task | Enumerate all periodic solutions in that interval; verify endpoints, repeated solutions, units and branch conventions. Do not silently substitute a principal value for all solutions. |
| Calculus | Choose a bounded polynomial/elementary family with an exact reference, or a stated integrand and interval | Differentiate candidate antiderivatives, check endpoint/domain conditions and integration constants. For a polynomial, use an independent coefficient calculation. A numerical spot check is supplemental. |
| Linear algebra | Construct a controlled rank/RREF and transform it with reversible operations; use a known vector/family to form b | Independently eliminate or inspect exact minors, then check residuals, rank, nullity and complete solution families. Separate consistent, underdetermined and inconsistent construction. |
| Discrete mathematics | Build a small explicit finite set, graph or sample space alongside the task | Compare enumeration with the counting argument. Vary order/replacement/identifications deliberately; test a counterexample to an overbroad claim. Proof tasks need checked arguments, not just a true/false key. |
| Probability/statistics | Start from bounded integer counts or an explicit finite model; normalize exactly | Enumerate the model or apply an independent formula; check mass sums, conditioning denominators, dependence and rounding. Distinguish population and sample definitions. |

Each recipe needs its own degeneracy rules and oracle. The original linear pilot checker covers
only real linear equations `ax+b=c` with nonzero bounded integer a and b. A new
subject cannot be enabled by changing its label or routing it through a scalar
answer matcher. Reuse the content fields and four projections; add the required
mathematical checker in its owning module when that family is implemented.

Parameter identity must survive generation order and batch size changes. Seeds
select instances, not mutable runtime surprises; freeze the selected parameters
before an attempt. Reject exact duplicates and scalar-multiple duplicates where
they add no intended practice. Tag near-duplicate structures so a short session
does not repeatedly ask the same calculation with recoloured labels.

Reserve some distinct instances for a later **fresh check**. The pilot selects
three practice instances and one fresh-check instance per stratum. This label
alone cannot establish unseen exposure: the runtime must consult the learner's
history before presenting it as fresh. Do not claim independent mastery after
showing that exact question's worked answer at another support level.

## Finding and using online questions

Use the web to check topic scope, definitions, assumptions and examples of good
task types. Start with primary textbook/course sources and the user's existing
read-only source cards. Prefer original, verified parameterized families for
large banks. A changed number or paraphrased sentence does not automatically
make an adapted exercise original.

For each external source record: exact URL; title/author; edition or date;
section/exercise locator; access date; source hash if a permitted local copy is
retained; usage (`reference_only`, `adapted`, `verbatim`); license/permission URL;
required attribution; and the resulting question IDs. Check the specific item
and any third-party exceptions, not merely the host site's name.

For example, current [OpenStax reuse guidance](https://help.openstax.org/s/article/Openstax-textbook-licensing-and-customization)
and [MIT OCW terms](https://ocw.mit.edu/pages/privacy-and-terms-of-use/) specify
noncommercial and share-alike conditions for their material. Record the exact
edition and reuse permission before bundling an adaptation; neither source is
a blanket permission to import exercises into any distribution. These pages
were checked on 2026-09-08. The OpenStax support article was available through
its indexed text; direct opening returned a loading page.

The pilot uses OpenStax's [addition/subtraction section](https://openstax.org/books/elementary-algebra-2e/pages/2-1-solve-equations-using-the-subtraction-and-addition-properties-of-equality)
and [multiplication/division section](https://openstax.org/books/elementary-algebra-2e/pages/2-2-solve-equations-using-the-division-and-multiplication-properties-of-equality)
to check general equality rules and the nonzero divisor condition. Its examples,
parameter construction and teaching prose are authored here; no exercise text
was imported. Unresolved reuse rights do not block original question authoring:
keep the source as a reference and use a checked original family.

## What the executable pilot checks

From `/Users/kogaryu/iggy3d/paths`:

```sh
python3 tools/question_workflow.py
python3 tests/question_workflow_tests.py
python3 tools/question_workflow.py --check-runtime
```

The first command writes only authoring/evidence artifacts in
`build/question-format-evidence/`: 24 instances, a golden solution, four
pre-answer views, the coverage ledger and a verification receipt. It does not
update `content/corpus/`, CMake, saves or game executables. `--per-stratum N`
allows 1–10 instances in each of six strata. Raising this number demonstrates
bounded generation, not added family coverage or pedagogical acceptance.

The second command checks golden results, independent exact substitution and
step invariants, wrong and equivalent choices, altered givens/answer keys,
broken working chains, zero/negative/fractional/boundary cases, deterministic
identity, definition references, all four disclosure projections and complete
leaf accounting, plus the runtime publication adapter. The third verifies the
runtime file reproduces exactly. Use `python3 tools/question_workflow.py --publish`
only when intentionally updating that pack, then run the C++ model and native
UI gates. It uses Python's standard library only. Do not use `eval` or
execute a downloaded exercise's content as part of validation.

The JSON file is a **closed authoring pilot**, not the eventual universal
runtime schema. The validator deliberately recognizes one family/version.
Its definition records are candidates to connect to the shared teaching
catalogue by stable ID/version. The current bounded pack compiles immutable
copies from those shared authoring definitions for its current-step reading;
do not maintain or edit copies by hand. Those inline copies are frozen in saved
question stamps. A future shared runtime reference/migration must preserve
opened teaching and mathematical identity. The added optional `support` field
preserves the existing prepared-file contract; its family is deliberately closed.

## Capacity and publication boundary

The current Library bank is bounded at 1,024 loaded questions and 8 MiB per
content/save file; an individual prepared question allows 32 steps and eight
choices per step. There are currently 311 Library questions. A target of
229 × 60 = **13,740 instances** does not fit by appending everything to the
current startup vector. It must not be "fixed" by removing the limits.

Before bulk runtime publication, add a compact stable-ID index and bounded
per-family/per-chapter loading through `CorpusPractice`. Saved attempts must
still resolve their frozen mathematics when a question is outside the currently
loaded chapter. Keep instance snapshots/version lookup separate from reading
metadata and the active page. Long derivations will likewise need an explicit
bounded checkpoint model before exceeding the current 32-step content limit.
These are future capacity checkpoints; they do not block the single-family
four-level implementation or the authoring pilot.

## Copyable task packet: maintain the first four-level family

> Work in `/Users/kogaryu/iggy3d/paths`. Read `AGENTS.md`,
> `docs/QUESTION_PRACTICE_FORMAT.md`, this workflow and the current architecture.
> Work only on the existing `linear_balance_ax_b` family across Learn, Practice, Solve
> and Independent in the existing Focus workspace. Use the golden 3x+5=20
> example and the generated authoring pilot as specifications; use `--publish`
> to reproduce `content/corpus/linear_support.json`. Keep the same mathematics
> at all four levels. Preserve
> `LayeredQuestionSession` as the single owner of support, typed responses,
> valid working, help exposure and completion, and `CorpusPractice` replay
> to retain that evidence and drafts. Preserve old saves and prepared questions.
> Reuse the exact linear kernel where its verified grammar applies. Provide
> a real multiline input at Independent; unsupported input must remain visible
> and report that it is not checked. Keep Given/Working and response adjacent,
> with in-place teaching and explicit Next. Pass the format document's concrete
> alternative-route, disclosure, input and save cases. Do not change textbook
> exercise state, add 3D, bulk-generate other subjects, take screenshots, launch
> windows, delegate or commit. Stop at one complete family and report exact
> checks and remaining limitations. If a required checker is missing, implement
> a bounded checker explicitly; never simulate success with an
> answer-key tile or claim arbitrary mathematical verification.

## Copyable task packet: author the next family

> Work on one named leaf and one explicitly assigned skill. Before generating
> a batch, write the family objective, domain, prerequisites, response type,
> bounds, source receipt, complete golden problem and independent verification.
> Supply all four support views and all current-step definitions/reasons using
> the shared format. Generate 6–12 varied instances only after the golden case
> passes. Check every solution, intermediate step and wrong choice; include a
> valid alternative response and a degeneracy case. Preserve stable IDs and
> record parameter variety separately from mathematical task families. Change
> only that family's authoring and its proving checker/coverage rows. Preserve
> current playable files, source pages and the other worker's 3D asset work.
> A family with no supported checker stays authoring-only with an explicit
> verification gap; proof work needs a rubric or supported proof checker.
> Run the smallest applicable checks, produce a result sheet and stop. No
> screenshots, native windows, delegation, commits or invented acceptance.

These packets apply equally to a Luna or Terra builder. They constrain the
work and make the important mistakes testable; mathematical and teaching
review still have named evidence requirements. No additional worker is launched
by this checkpoint.
