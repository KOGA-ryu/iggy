# 18 Runtime Report Count Projection Cleanup

Status: complete.

Goal: turn the runtime report duplication audit into one small
behavior-preserving implementation.

Scope:
- Target exactly one repeated count/projection seam across runtime report code.
- Preserve public report/result fields.
- Preserve test-observed behavior.
- Do not touch CLI output, source-plan/TOML/facade/package/preview behavior,
  save/load, UI/Edi, gameplay semantics, or builder AI-map/region files.

Implementation:
- Added `RuntimeInventoryEventCountProjection`, a small pure helper over
  `InventoryEventRecorder2D`.
- Replaced duplicated inventory-event counting loops in raw gameplay frame and
  policy gameplay frame reporters.
- Left event emission, NPC movement projection, report structs, and result
  structs unchanged.

Verification:
- Focused raw/policy report tests.
- Full CTest because production runtime files changed.
- `git diff --check`
