# 11 API Roadmap Drift Cleanup

Status: complete.

Goal: sync API/roadmap docs with current completed surfaces: TOML facade, package
facade, preview model, diagnostics, summary projection, and
run/lint/check/trace/package CLI.

Scope:
- Docs only.

Slices:
- Review current roadmap and authoring bucket docs after Authoring V1 closure.
- Update stale API-index CLI/facade wording.
- Keep production code untouched.

Verification:
- `git diff --check`
- Targeted `rg` for stale CLI/API wording.

Result:
- `engine/research/api_index.md` now names the TOML facade, package facade,
  preview model, diagnostic entry projection, summary projection, and current
  `iggy_scenario_toml_runner [--trace] [--check] [--lint] <path>` surface.
- Roadmap and authoring bucket README were already current from builder's
  Authoring V1 closure; no roadmap edits were needed.
- No code, behavior, CLI output, or exit-code changes.
