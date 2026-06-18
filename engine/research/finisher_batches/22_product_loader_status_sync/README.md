# 22 Product Loader Status Sync

Status: complete.

Goal: update planning and API docs after Product Loop Packet 1 integrated the
load-only product scenario loader.

Scope:
- Docs/status sync only.
- Mark `RuntimeGameplayProductScenarioLoader` as complete for load-only product
  scenario setup.
- Mark `RuntimeGameplayTomlScenarioPackageReader` as the shared explicit
  package manifest/path reader.
- Frame Product Loop Packet 2 as product-owned loop state/step, not more loader
  work.

Hard stops preserved:
- No production code changes.
- No product runtime loop or frame execution.
- No raw device input in runtime session state.
- No presentation/camera state in gameplay truth.
- No save/load productization.
- No package discovery, scanning, watching, or source mutation.
- No new gameplay semantics.

Verification:
- `git diff --check`
- `git status --short --branch`
