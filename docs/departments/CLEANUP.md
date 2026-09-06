# Capability Cleanup Ledger

This ledger keeps cleanup work resumable without repeating a repository-wide
audit or a large handoff prompt.

## Accepted Baseline

- Commit: `ff8d7eb2`
- Scope: Creative authoring and its `i3dp` playtest handoff
- Work mode: direct, one capability at a time, no delegation by default
- Default verification: targeted and headless
- Default delivery: leave changes uncommitted for review

## Acceptance Rule

A cleanup workstream is complete when one department becomes the sole authority
for a semantic decision, displaced routes are deleted, and the final observable
behavior is protected. Rearrangement alone is not completion.

Production LOC should decrease and production file count should not increase.
An exception requires a concrete ownership reason stated before editing.

## Active Candidate

| ID | Capability | Symptom | Canonical Owner | Competing Decision | Expected Subtraction | Observable Proof | Status |
| --- | --- | --- | --- | --- | --- | --- | --- |
| CLR-001 | Grid-independent storey height | A new blockout defaults to three grid cells, so changing grid resolution can compress upper storeys even though architectural profiles are expressed in meters | Building and World Layout architecture/dimensions | Editor blockout draft owns a raw `Custom + 3 cells` default and converts to physical height only after a profile is explicitly selected | Remove the raw grid-dependent default as an independent policy and route blockout initialization through the canonical architectural profile resolution | On 1.0 m and 0.5 m grids, a Residential three-storey blockout generates finished-floor planes at 0, 3, and 6 meters; ceilings meet the underside of the next slab | Automated Green; uncommitted review |

## Evidence For CLR-001

- Backend blockout settings still default to Custom so explicit callers and
  saved provenance retain their authored cell dimensions.
- Only the transient Create-tab draft now declares Residential by default.
- Profile resolution moved from the UI implementation into the Building
  contract and runs again at the desktop command boundary against the active
  document grid.
- The former UI-local resolver was deleted.
- `creative_building_authoring_workflow_tests` now proves a Residential
  three-storey blockout on a 0.5 m grid resolves to six cells per storey and
  generates finished-floor planes at 0, 3, and 6 m with intermediate ceilings
  meeting the next slabs.
- `creative_editor_toolbox_tests` proves named profiles resolve exactly and
  Custom dimensions remain unchanged.
- `creative_desktop_world_layout_tool_command_tests` keeps the existing Create
  and Update command routes green.
- Focused result: 3/3 tests passed on 2026-07-24.
- Production delta is `+69/-51` (net +18). This is the bounded exception to
  the subtraction rule: the old UI-local resolver was deleted, while the
  replacement is a Building-owned contract plus rejection at the shared
  desktop command boundary so non-UI callers cannot bypass grid resolution.

## Workstream Limits For CLR-001

- Do not redesign grid settings, structural layer thickness, or manual Custom
  editing.
- Do not split files or add a second profile adapter.
- Preserve saved blockout provenance and explicit Custom values.
- Add or revise one end-to-end non-unit-grid regression; do not create a broad
  new test suite.
- Stop if existing saved Custom blockouts cannot be distinguished safely from a
  new draft.

## Completed Workstreams

Move CLR-001 here after the uncommitted checkpoint is accepted.
