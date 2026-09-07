# P026: bring the question and symbol choices together

Status: implemented; Release build and three targeted checks passed;
uncommitted. The user confirmed improved question visibility, then reported
excess bottom space in a large window. [P027](P027_RESPONSIVE_SOLVING_WORKSPACE.md)
replaces the fixed activity cap with responsive sizing. No screenshots were taken.

The user's P025 feedback was that the question was too far from the choices
and that operation choices could use symbols instead of words. This checkpoint
addresses that presentation within the existing fixed solving workspace.

## Delivered behaviour

- Navigation and assistance controls sit above the problem. They no longer
  separate the working from its choices.
- The amber/gold problem board, current working and step prompt are centred over
  the activity. The operation buttons form a compact two-column group directly
  beneath them, with larger centred notation and the existing teal borders.
- The 3D viewport is bounded to 200 pixels high so tall windows cannot create
  a large gap between the equation and arithmetic spheres. It remains fixed
  through steps, help, completion and Next; the support panel remains separate.
- All six prepared questions display operation symbols. For the first question,
  the four choices are `÷ (-2)`, `× (-2)`, `- 2` and `+ 2`, with a shared
  instruction that the operation applies to both sides. Their displayed order
  still follows the existing answer bindings.
- Negative multiplication/division operands are parenthesized. Minus uses the
  same ASCII character as the original equations; the default font contains
  that character, division and multiplication. The font check rejected U+2212,
  so the delivered content does not depend on that missing glyph.
- Full explanations, hints and next moves retain their existing wording.
  The mathematical steps, correct options, arithmetic answers and final checks
  are unchanged.

## Ownership and content

The existing recipe generator owns the symbolic labels and concise operation
prompts. There is no UI translation of English commands or new equation parser.
The six generated cards and their linked deck references now use content
version 4, and preparation metadata identifies generator version 2. Version 3
was an intermediate candidate superseded after the glyph check; publication
continued to enforce increasing content versions.

`EquationSorterUi` owns centring, spacing, type size and the viewport bound.
The old vertical centring and full-width operation buttons are replaced. The
existing question, gallery and sorter owners retain judging, evidence, Next,
Replay and Resume; no production runtime model or new production file is added.

## Verification

The current CMake graph supplies `sorter`, `paths_sorter_input_tests`,
`paths_sorter_solve_tests` and `paths_bracket_recipe_tests`. The Release build
in `b` and all three targeted tests passed.

- Independent recipe checks decode each displayed operation and apply it to
  both sides using exact rational arithmetic. Only the accepted choice may
  produce the prepared next equation. This covers the six supplied questions
  and 96 coefficient/offset/right-side boundary combinations, alongside existing
  arithmetic, identity, determinism and publication refusal checks.
- Actual ImGui checks cover 1440×900, 800×600 and 360×480. They verify centred
  choices within 60 pixels of the working panel, the bounded activity area,
  supported font glyphs and the retained solving/input/Next/resume loop.
- Generation `--check` passes. The rebuilt executable validates the bundled
  bracket pack from `/private/tmp` before graphics startup. Bundled JSON agrees
  with source content.

Text evidence is under `build/compact-choices-evidence/`. The original maths
and mixed sorter packs, the practice-card grid and mathematical working/help
retain their previous values. Only the declared presentation/version fields
change in generated questions.

Production change: one existing C++ UI file **+28/-16 (net +12)** and the existing
generator **+5/-7 (net -2)**, for **net +10 lines** across two existing code files.
Two existing test files change by **+30/-6 (net +24)**. Thirteen existing content
files change: one recipe source, six questions and six linked decks.

## User visual checklist

Relaunch `./b/sorter --content content/sorter/bracket_practice_v1.json` from Paths.

- Look below the amber/gold problem board: the teal-bordered choices should be
  close to the working and show large symbols, not full operation sentences.
- On the first question, confirm `÷ (-2)`, `× (-2)`, `- 2` and `+ 2` are clear.
- Continue to arithmetic and open a hint. The spheres should remain near the
  equation, and the fixed problem/activity layout should stay still.

Subject/chapter selection remains the next capability after this formatting
feedback is resolved.
