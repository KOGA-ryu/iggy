# 14 Runtime Report Duplication Audit

Status: pending.

Goal: docs-only audit/design packet for repeated count/event fields across raw,
policy, orchestrated, and runner reports.

Scope:
- No production changes.
- Identify a future smallest safe helper if any.

Anchor areas:
- Runtime frame reports.
- Policy reports.
- Orchestrated frame reports.
- Runner reports.

Verification:
- Docs diff review.
- `git diff --check`
