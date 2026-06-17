# 15 Projection Status Mapping Audit

Status: pending.

Goal: audit duplicate status, summary, and diagnostic mapping between preview,
package, facade, and CLI.

Scope:
- Docs gate first.
- Implement only if there is a tiny pure helper that does not change CLI output
  or result shapes.

Verification:
- Docs diff review for audit-only work.
- Focused tests if a tiny helper is added.
