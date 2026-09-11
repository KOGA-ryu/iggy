# Paths workstreams

**Completed capability: reusable family recipe and checking command.**
`tools/author_question_family.py` now provides `init` and `check` for the
registered `linear_balance_v1` family. Writers edit one JSON recipe and two
Markdown templates, with a design brief describing the mathematical and
teaching limits. The command supplies IDs, numbering, set labels, choice
placement, links, metadata and a compact verification handoff.

The existing linear provider retains bounded original-case construction,
independent exact certificates and calculated teaching fields. Its standalone
packaging/receipt implementation was removed. The common runner reuses the
existing batch template and certificate functions, document compiler, model
replay, lesson disclosure gate and exporter contracts. It emits one ordinary
18-question chapter with one shared lesson, 21 decisions and 42 individual
wrong-choice corrections. It adds no runtime grammar, judge or renderer.

All 11 targeted tests pass. Checks cover the 512 allowed per-set seed
combinations, all compiled routes and
wrong choices, stable IDs, actual Markdown/number edits, malformed inputs,
missing feedback/disclosures and changes to sources during verification.
The candidate imports into a copy of the full library, reaching 719 questions
while preserving all 701 earlier stamps and document/package bytes. Saved
progress replay and unchanged repeat import pass. The live library is not changed by this
capability; checking produces a candidate for coordinator review.

- [Recipe authoring and exact commands](CONTENT_PRODUCTION.md#recipe-authoring)
- [Editable recipe](../content/authoring/learning/linear_family/recipe.json)
- [Question template](../content/authoring/learning/linear_family/questions.paths.md.in)
- [Lesson template](../content/authoring/learning/linear_family/lesson.md.in)
- [Complete teaching contract](../content/authoring/learning/linear_family/DESIGN.md)
- [Capability evidence](../build/family-recipe-evidence/verification.json)

The live baseline is completed Wave 04: **701 questions, 979 readings,
1,290 sections and 44 structured lessons in 51 documents**. Its 48 new
questions and four lessons, all preceding published source inventories, and
saved-progress preservation are bound in
[completion.json](../build/production/wave04/completion.json). All four writer
assignments are closed. Source-assisted production did not establish a token,
speed or weekly-allowance saving; observed effort remains in its measurement
record. No subsequent curriculum wave is assigned.

Work stays uncommitted and preserves separate 3D work. No screenshots, captures,
windows, font/ImGui probes or clipboard access. Routine content QA belongs to
the coordinator; visual quality and learner outcomes remain unobserved.

The next candidate is a second reviewed mathematical provider using this same
recipe and checking entry point. Only linear balance is supported now. Full
curriculum coverage, generic four-level support and demonstration packaging
remain separate unfinished capabilities.
