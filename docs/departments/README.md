# Department Operating Model

This directory is the repository's ownership and product-observability layer.
It does not replace source code, tests, or design documents. It answers four
questions without requiring a repository-wide search:

1. Which department owns a file or capability?
2. What is being worked on, deferred, or blocked?
3. What automated evidence exists?
4. What still needs a person to test?

## Sources Of Truth

- `registry.tsv` defines the departments.
- `ownership.tsv` assigns every governed tracked file to exactly one department.
- `<department>/DEPARTMENT.md` defines ownership and dependency direction.
- `<department>/TODO.md` records capability maturity and delivery state.
- `<department>/TESTING.md` records automated gates and manual acceptance cases.

`INDEX.md`, `DEPENDENCIES.md`, `TEST_QUEUE.md`, and every `FILES.md` are
generated. Do not edit them directly.

## Status Model

Maturity and delivery are deliberately separate.

Maturity:

`Missing -> Prototype -> Stable Recipe -> UI Bindable -> Integrated`

`Support` is used for infrastructure rather than a product capability.
`Deferred` means the capability is intentionally outside the current product
slice.

Delivery:

`Needs Audit -> Planned -> In Progress -> Automated Green -> Manual Test Needed -> Accepted`

`Blocked` and `Deferred` are terminal holding states until an explicit ruling.
Automated green is not accepted if the result still needs visual or interactive
judgment.

## Maintenance

Run:

```bash
python3 tools/repo_departments.py generate
python3 tools/repo_departments.py check
```

When adding, deleting, or moving a governed file, update `ownership.tsv` in the
same change. When changing ownership, public wiring, or a proving test, update
the owning department documents in the same change.

The governance files under `docs/departments/` and
`tools/repo_departments.py` are implicitly owned by Foundation and Build. They
are excluded from `ownership.tsv` to avoid a self-referential generated map.

## Physical Reorganization

This map precedes physical source movement. Move files only after the ownership
assignment and dependency direction are reviewed. The first physical candidate
is the flat `apps/iggy3d_creative/` directory; shared `src/` layers should move
only when a department boundary is demonstrably stronger than the current
engine boundary.
