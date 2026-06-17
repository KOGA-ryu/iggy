# 04 CLI Output Contract Inventory

Status: pending.

Goal: inventory the current CLI output and exit-code contract before facade
extraction changes it.

Slices:
- Read CLI implementation and tests.
- Document modes, sections, exit codes, and locked tests in the finisher bucket
  packet or a roadmap appendix.
- Make no code changes unless they are docs only.

Verification:
- Docs diff review.
- `git diff --check`
