**Paths: reusable maths content and question storage**

Research date: 2026-09-06. This is a research recommendation for discussion,
not a new build packet. It follows the prepared-card solver discussion in
[PLANNING_CHECKPOINT.md](/Users/kogaryu/iggy3d/paths/docs/PLANNING_CHECKPOINT.md).
Repository documentation, selected source files, licenses, and the current
Paths question owner were inspected. External engines were not installed or
executed, and no external question bank was imported into the game.

Use editable JSON for reviewed question content, retaining the builder's
existing question/step/option model as the runtime destination. Existing
projects can supply question templates, random variants, mathematical checks,
and sometimes worked steps. Paths still needs a small conversion layer that
turns selected material into its particular player actions and answer choices.

For gathering advanced prepared questions, STACK is the first candidate I
would evaluate. Numbas is the strongest alternative for authoring and exchange
format. SymPy is the first mathematical helper I would evaluate for our own
prepared cards and generated families. These are separate roles; adopting all
three is unnecessary for the first content capability.

| Candidate | What it supplies | Native format and integration | Assessment for Paths |
| --- | --- | --- | --- |
| [STACK](https://github.com/maths/moodle-qtype_stack) | Randomised mathematics, multipart assessment, answer tests, and a library spanning engineering mathematics and calculus. The library mixes completed material with templates and demonstration fragments. | Question definitions are primarily Moodle XML with Maxima expressions. A standalone API can evaluate a selected variant and return JSON. | Strongest first bank candidate for advanced content. Use a bounded subset during content preparation; inspect each chosen question's steps and license. [Library documentation](https://docs.stack-assessment.org/en/STACK_question_admin/Library/) |
| [Numbas](https://github.com/numbas/Numbas) | Question statements, parts, variables, optional worked advice, and single/multiple choice or mathematical inputs. | Its `.exam` source is JSON with a version comment. Embedded JME expressions and HTML/LaTeX still require evaluation and conversion. | Closest authoring structure to our game. Useful donor/editor candidate if a suitable licensed multipart question is available. [Question model](https://docs.numbas.org.uk/en/latest/question/reference.html), [format](https://www.numbas.org.uk/schema/) |
| [WeBWorK Open Problem Library](https://github.com/openwebwork/webwork-open-problem-library) | A large college question collection; the project describes coverage including calculus, differential equations, and linear algebra. | Questions are PG programs with macros. The separate renderer accepts a source and seed and can return JSON. | Strong breadth, but content extraction and license selection add work. A JSON response is not automatically a Paths step sequence. [Coverage](https://github.com/openwebwork/pg), [renderer API](https://github.com/openwebwork/renderer#renderer-api) |
| [SymPy](https://github.com/sympy/sympy) | Symbolic calculation, equation solving, calculus, matrices, and mathematical printing. | Python objects and expressions; a preparation script would emit our JSON. | Best small mathematical helper for original templates and verification. It does not supply a curriculum or automatically design all game choices. [Features](https://docs.sympy.org/latest/tutorials/intro-tutorial/features.html) |
| [mathgenerator](https://github.com/lukew3/mathgenerator) | A direct function call returns a problem/solution pair. Its source includes a calculus module. | Python functions and formatted strings. | Attractive for a quick experiment, but the maintainer explicitly says they are no longer interested in maintaining it. Pairs alone do not give us reviewed operation stages and distractors. It is not my foundation recommendation. |
| [PrairieLearn](https://github.com/PrairieLearn/PrairieLearn) | Programmable question generation and grading, including access to symbolic libraries. | A question directory generally contains `info.json`, `question.html`, and optional `server.py`. | Good architecture reference, but adopting a general assessment platform adds more machinery than this first capability needs. [Question format](https://docs.prairielearn.com/question/overview/) |

There are two especially useful implementation findings.

STACK's [standalone API](https://github.com/maths/moodle-qtype_stack/tree/master/api)
can operate outside a full Moodle installation. Its render response includes
question text, sample solution text, input/model-answer information, and the
variant seed. This gives us an existing extraction boundary. It still needs
the STACK/Maxima environment, and its text can contain HTML, LaTeX, placeholders,
and interactive assets. Treat it as a possible offline content tool; its JSON
response is not a direct game file.

SymPy's
[`integral_steps()`](https://docs.sympy.org/latest/modules/integrals/integrals.html#sympy.integrals.manualintegrate.integral_steps)
returns a rule tree with substeps for supported single-variable integration.
That is closer to our operation-stage idea than a final answer alone.
[SymPy Gamma's formatter](https://github.com/sympy/sympy_gamma/blob/master/app/logic/intsteps.py)
is a concrete example of presenting these rules. We would map a small set of
supported rule types into reviewed game stages, with explicit handling for
unsupported results. This does not establish a universal teaching-step engine
for trig, calculus, and linear algebra, or guarantee that a chosen route is
the only valid route.

Numbas also has a useful
[standard-integral authoring example](https://docs.numbas.org.uk/en/latest/authoring/example-gallery/standard-integral.html)
that develops randomisation and feedback for particular errors. It demonstrates
the kind of reusable question family we want. Its
[headless script](https://github.com/numbas/Numbas/blob/master/headless)
generates variables and tests model answers; the inspected script returns a
test result, not a complete Paths content export.

Licenses affect which content we can gather. Keep the engine license and the
individual question's license as separate provenance fields.

| Project | Verified license facts | Consequence for selecting material |
| --- | --- | --- |
| STACK | Software is GPLv3; its documentation is CC BY-SA 4.0. [Official statement](https://stack-assessment.org/Legal/Licenses/) | Check the selected bank/question and any embedded assets. The documentation license is not evidence that every linked question has identical terms. |
| Numbas | Runtime/editor are Apache 2.0. Content copyright belongs to its author, and the editor exposes a question license setting. [Software/content distinction](https://docs.numbas.org.uk/en/latest/licensing.html), [question settings](https://docs.numbas.org.uk/en/latest/question/reference.html#settings) | Record the particular question's license before adapting it. |
| WeBWorK OPL | Default contributions are CC BY-NC-SA 3.0 unless otherwise indicated. Alternative licenses can be specified by contributors. [OPL license](https://github.com/openwebwork/webwork-open-problem-library/blob/main/OPL_LICENSE) | The default noncommercial restriction matters if the game will be sold. Select an appropriately licensed subset or obtain permission for the particular content. |
| SymPy / Gamma | BSD-style three-clause licenses. [SymPy license](https://github.com/sympy/sympy/blob/master/LICENSE), [Gamma license](https://github.com/sympy/sympy_gamma/blob/master/LICENSE) | Useful code candidates for original content tooling; preserve applicable notices if distributing their code. |
| mathgenerator | MIT. [Repository license](https://github.com/lukew3/mathgenerator/blob/main/LICENSE) | Maintenance and output quality remain separate questions. |
| PrairieLearn | The repository license is AGPLv3. Public question-sharing flags distinguish CC BY-NC source sharing and CC BY-NC-ND public presentation. [Software license](https://github.com/PrairieLearn/PrairieLearn/blob/master/LICENSE), [sharing rules](https://docs.prairielearn.com/question/overview/#question-sharing) | Publicly viewable questions are not all freely adaptable game assets. |

For an actual imported record, retain source URL, repository revision or content
hash, original question identity, author, license and required notices. For a
generated variant, also retain the template and generator versions, seed, and
resolved parameters. A seed alone is insufficient if the generator changes.

The current builder format is a useful destination. In
[LayeredQuestionSession.hpp](/Users/kogaryu/iggy3d/paths/src/runtime/first_move/LayeredQuestionSession.hpp:29),
a question already contains identity/version, equation, skill, description,
and an ordered step list. Each step has a prompt, option IDs and labels, an
accepted-answer mask, a hint, an explanation, and a working line. Mathematical
judgment belongs to this model; GallerySession translates target selections
into its option submissions.

The current
[catalog constructor](/Users/kogaryu/iggy3d/paths/src/runtime/first_move/LayeredQuestionSession.cpp:134)
admits 1–64 questions per catalog, 1–32 steps per question, and 2–8 choices per
step. Guided interaction requires exactly one accepted choice; ArcadeCollect
supports multiple accepted choices. Import must respect those mode constraints.
A larger disk library can supply a selected catalog within these bounds.

The gallery's questions are currently compiled fixtures. The prepared source
adaptations in
[002_guided.json](/Users/kogaryu/iggy3d/paths/content/authoring/002_guided.json) and
[013_guided.json](/Users/kogaryu/iggy3d/paths/content/authoring/013_guided.json)
are existing authoring artifacts, not a functioning game JSON loader. Keep
their source records and versions intact when adding a deliberate import path.

| Storage choice | Recommendation |
| --- | --- |
| JSON | Use for reviewed cards and generated content packs. Nested steps, options, accepted sets, and source metadata fit naturally, and people can review changes directly. |
| CSV | Use for flat exports or a catalog overview. Encoding variable step/option lists in rows would complicate editing and validation. |
| Markdown/text | Retain original cards, teaching notes, and editorial explanations here. Require explicit structured fields for executable question meaning. |
| Parquet | Keep available for a later large analytics/export workload. It supports complex data, but its columnar bulk-storage strengths do not improve our first authoring workflow. [Apache's overview](https://parquet.apache.org/docs/overview/) |
| Database | Add only when a concrete persistence or query requirement needs it. Question authoring does not currently require a database service. |

Start with one JSON file per reviewed question in the existing `content/cards/`
area and one documented schema. Generated batches can later package the same
records into small JSON arrays. Keep player attempts and progress separate
from the content definitions. The
[JSON Schema standard](https://json-schema.org/overview/what-is-jsonschema)
can validate structure; mathematical correctness and cross-field answer
references still need our content validator.

For the schema evolution, preserve the existing authoring fields where they
fit. Replace the single `correct_option_id` with `accepted_option_ids` in a
deliberate next schema version. Translate IDs to the runtime bitmask at import,
after establishing the final option order. Colours and patrols remain runtime
assignments and never become mathematical answer keys.

This small example illustrates the proposed next authoring shape. It is an
original fixed trig example, not imported material, a completed schema, or a
file the current game can load:

```json
{
  "schema_version": 2,
  "question_id": "original_trig_sine_half",
  "content_version": 1,
  "source": { "kind": "original" },
  "title": "Find both angles",
  "skill": "trig.solve_sine_on_interval",
  "display_problem": ["sin(x) = 1/2; 0 <= x < 2*pi; angles in radians"],
  "steps": [
    {
      "id": "collect_solutions",
      "layer": "Solve on the interval",
      "prompt": "Select every value of x that solves the equation.",
      "options": [
        { "id": "o1", "text": "pi/6" },
        { "id": "o2", "text": "7*pi/6" },
        { "id": "o3", "text": "5*pi/6" },
        { "id": "o4", "text": "11*pi/6" }
      ],
      "accepted_option_ids": ["o1", "o3"],
      "recovery_text": "Sine must be positive and equal to 1/2.",
      "explanation": "The reference angle is pi/6. Sine is positive in quadrants I and II, giving pi/6 and 5*pi/6 on this interval.",
      "working_line": "sin(x) = 1/2"
    }
  ]
}
```

The import boundary maps `question_id/content_version` to runtime identity,
`display_problem` to the display equation, `layer` to `layerName`, `text` to
`label`, and `recovery_text` to `wrongHint`. String step/option identities need
a deterministic mapping to the current numeric IDs, retained for that content
version. Existing schema-v1 single-answer cards can map their key to a singleton
accepted set. No silent rewriting of authored files is needed.

Formatting mathematics also needs an explicit boundary. Store exact fractions
and expressions in a documented mathematical syntax during preparation;
preserve domain restrictions, angle units, and constants of integration.
Produce a readable plain-text display for the current game. LaTeX can be an
additional display representation when supported, but it is not the answer
identity or a universal interchange grammar. The inspected UI uses ImGui text;
advanced imported HTML/LaTeX or diagrams need conversion or a separate renderer
capability. Never silently discard a diagram that is necessary to solve a
question.

Before admitting a prepared question, check that every accepted ID exists,
the required number of accepted answers matches the interaction, and the
displayed choices are distinct and correctly classified under the actual
prompt. For generated content, verify supported intermediate transformations
and final answers with exact arithmetic or an appropriate symbolic check.
Inconclusive checks leave a record for review. Distractors must remain wrong
after parameter substitution; alternative valid methods need precise prompts
or explicit acceptance. A schema check alone cannot prove those properties.

The proposed preparation flow is:

```text
Selected external question or our prepared card/template
  -> resolve a concrete variant and retain its provenance
  -> produce reviewed operation/arithmetic stages and answer choices
  -> validate the complete record and write JSON
  -> import a bounded catalog into LayeredQuestionContent
  -> existing gallery and question judgment
```

For the next capability, prepare one short calculus question with its full
worked route in the proposed format. First look for a suitable STACK multipart
question with explicit reuse terms; evaluate whether its API output saves work
over authoring the same family with SymPy. Compare actual conversion effort,
display quality, and correctness on a few fixed variants before selecting a
dependency. If the selected source has only a final answer, explicitly author
the missing stages rather than calling them imported steps.

The first implementation gate should then demonstrate that this reviewed JSON
question can complete through the existing game owner. A trig multiple-answer
case and a small linear-system case are subsequent format probes, covering
domains/accepted sets and matrix notation respectively. Bulk harvesting, a
general prose solver, and additional game modes are not prerequisites for
that checkpoint.
