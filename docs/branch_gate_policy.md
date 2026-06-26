# Branch Gate Policy

New branch logic in product source is blocked unless it has explicit controller approval.

## Gated Statements

The gate checks added C/C++ source lines under `src/` and `apps/` for:

- `if (...)`
- `else if (...)`
- `switch (...)`
- ternary expressions

Tests are not gated by default. Use `--include-tests` when a stricter audit is needed.

## Approval Format

Approved branches must have both:

1. A ledger entry in `docs/branch_gate_approvals.tsv`.
2. A nearby source comment using the same id:

```cpp
// branch-gate: BG-0001
if (condition) {
}
```

The approval id is for exceptions, not routine growth. Prefer command registries,
controller handlers, result objects, or descriptor tables before requesting an id.

## Commands

Check a committed slice:

```bash
tools/check_branch_gate.py --diff HEAD~1..HEAD
```

Check staged changes:

```bash
tools/check_branch_gate.py --cached
```

Check unstaged working tree changes:

```bash
tools/check_branch_gate.py
```

## Builder Rule

No builder slice may add source branch statements unless the order includes an
approval id from the controller. Passing tests is not enough to accept unapproved
branch growth.
