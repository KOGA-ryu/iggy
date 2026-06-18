# UI/Product Department

## Charter

Own product presentation, editor/preview surfaces, left-tab workspace profiles,
debug and authoring inspection flows, and user-facing design philosophy.

This department is planner/designer-led before implementation. Other
departments send capabilities here so UI/Product can decide how they appear in
the application.

## Roles

- Planner: owns UI roadmap and coordinates surface specs with other departments.
- Designer(s): create read-only surface specs, interaction flows, and information
  architecture.
- Builder: implements UI shell/model changes only after a spec is accepted.
- Researcher: inspects genre/tool UI references and current UI model surfaces.
- Reviewer: gates fit with existing UI shell, density, and product workflow.
- Finisher: trims docs, examples, and UI test/support ceremony.
- Apprentice/Spark: inventories UI model fields and creates surface matrices.

## Bucket

1. UI workspace profile surface spec: Play, Build, Script, Check, Actors, Items,
   Interactions, Package, Debug, Docs/Examples.
2. Preview model consumption spec.
3. Package/run/check/trace result inspector spec.
4. Authoring diagnostics display spec.
5. Runtime frame/debug inspector spec.
6. Implementation only after specs are accepted.

## Hard Stops

- Do not build mutation/editing UI before read-only preview consumption is
  settled.
- Do not add hidden runtime autorun.
- Do not add save/load UX before runtime owns the boundary.
- Do not let UI invent gameplay semantics.

## Verification

Docs/spec review first. For code: focused UI model tests, screenshot or model
snapshot checks where available, then full engine CTest before integration.
