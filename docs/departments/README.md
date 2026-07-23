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

## Audit And Implementation Loop

Department work uses two distinct roles:

1. **Sol audits and architects.** Sol traces live ownership and data flow,
   classifies deficiencies from current evidence, defines the target file
   layout and contracts, names proving tests, and writes bounded implementation
   orders. A raw count or large file is not itself a deficiency.
2. **Luna implements.** Luna receives an order only after the target state,
   allowed files, invariants, migration map, tests, and stop conditions are
   explicit. Luna should not make architecture or scope decisions while
   editing.
3. **Sol reviews the completed batch.** Review covers the aggregate diff,
   contract closure, targeted headless gates, and department evidence. A batch
   is either accepted or returned with a focused repair order.
4. **The dashboard advances.** Accepted work updates `AUDIT.md`, `TODO.md`,
   `TESTING.md`, generated maps, and the bounded checkpoint commit before the
   next dependency wave begins.

The repository receives a shallow inventory first, then deep audits and
implementation in dependency order. Do not wait for a deep audit of every
department before repairing the highest dependency wave; lower-wave findings
must be revalidated after upstream contracts change.

User-facing progress is reported at four milestones rather than per command:
audit complete, implementation plan ready, implementation complete, and review
accepted or repair requested.

## Finding Standard

Every finding must name a concrete surface, current evidence, a classification,
a disposition, and a priority. Use the following interpretations:

- **Canonical Owner**: the intended single source of truth.
- **Required Adapter**: a live boundary whose translation or side effects make
  it necessary.
- **Duplicate Implementation**: multiple live owners implement the same policy.
- **Legacy Reachable**: obsolete policy remains on a live route.
- **Test-only Production**: production code has no production caller but is
  retained by tests.
- **Unreachable**: no live caller or state reader exists.
- **Ownership Undecided**: the surface is live, but its department boundary is
  unresolved.
- **Contract Risk**: the route is live but permits stale, partial, bypassed, or
  contradictory state.

`Investigate` is not implementation authorization. A surface may be deleted,
consolidated, moved, or repaired only when the audit names its replacement and
the proving gate.

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
