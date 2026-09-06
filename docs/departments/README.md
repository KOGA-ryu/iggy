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

## Capability Cleanup Loop

Department work is direct and capability-sized:

1. Start from a visible failure, stale live route, or proven unreachable
   surface. Do not reopen a broad repository audit for every workstream.
2. Trace one semantic value from its user entry through the department that
   owns its meaning to the final document, render, collision, persistence, or
   runtime result.
3. Name the canonical owner and every competing decision. If there is no
   concrete conflict, deletion, or dependency reduction, stop without
   rearranging files.
4. Route live consumers through the canonical owner and delete displaced code,
   declarations, tests, and files. Prefer subtraction and merging.
5. Run the smallest current headless gate that proves the ownership boundary
   and final observable behavior.
6. Review the aggregate diff and evidence once. Do not duplicate a passing
   implementation gate unless evidence is inconsistent or the boundary is high
   risk.
7. Update the owning department records and generated maps at the accepted
   capability checkpoint, not after every internal commit.

`CLEANUP.md` carries the accepted baseline, active candidate, evidence, and
workstream limits so new tasks can resume without large handoff prompts.

Production LOC should normally decrease, and production file count should not
increase. A raw count or large file is not itself a deficiency, and moving code
without removing ambiguity or dependency is not accepted cleanup.

User-facing progress is reported at three milestones: conflict identified,
implementation ready for verification, and checkpoint accepted or repair
requested.

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

The root `AGENTS.md`, governance files under `docs/departments/`, and
`tools/repo_departments.py` are implicitly owned by Foundation and Build. They
are excluded from `ownership.tsv` to avoid a self-referential generated map.

## Physical Reorganization

This map precedes physical source movement. Move files only after the ownership
assignment and dependency direction are reviewed. The first physical candidate
is the flat `apps/iggy3d_creative/` directory; shared `src/` layers should move
only when a department boundary is demonstrably stronger than the current
engine boundary.
