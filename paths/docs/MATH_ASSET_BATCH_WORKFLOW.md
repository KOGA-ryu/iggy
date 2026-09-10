# Bounded math-asset batch workflow

Proposed workflow, 10 September 2026. No workers have been launched and no model
settings have been changed. This is the asset-production companion to
[MATH_ASSET_REUSE_PLAN.md](MATH_ASSET_REUSE_PLAN.md), not a second textbook workflow.

The current AGENTS.md assigns this worker to assets and the other worker to the
textbook experience. Its direct-work rule remains in force until the user starts
a parallel asset batch. A planning discussion does not dispatch that batch.

## Division of work

| Role | Owns | Does not own |
| --- | --- | --- |
| Foundation/integration lead | Mathematical definitions, numerical limits, public family contracts, independent reference cases, shared registration and final installed build | Another worker's active output directory |
| Terra asset worker | One frozen family component or 3–5 configurations within an implemented family; private source/test output and concise return evidence | Shared enums, registry, CMake, app flags, master docs, renderer, textbook content or save logic |
| Textbook owner | Chapter placement, explanation, notation, exercises, disclosure, persistence and figure integration | Recomputing the asset's mathematical truth |
| User | Visual and interaction confirmation through manually launched executables | Automated numerical verification |

Terra is the intended production worker here because the user selected it.
No throughput, reliability or cost advantage has been measured in this project.
Calibrate one pilot before increasing concurrency or batch size.

## What can repeat

A configuration task changes bounded mathematical inputs, selected probes, layer
settings and the example's declared expectations. It reuses the existing owner,
geometry, controls and action routes. Its result can be an additional preset or
figure configuration; it need not become a new top-level Math Lab model.

A family task adds a genuinely missing mathematical producer behind a frozen
interface. For example, an LU trace and a Cholesky trace may share the same stage
player but cannot share an invented numerical algorithm. New definitions,
conditioning rules, domain conventions or proof assumptions return to the lead
before implementation proceeds with dependent work.

Do not hand a worker an entire advanced chapter and ask it to infer both the
mathematics and its representation. Use the canonical chapter brief to choose
one observable construction, then freeze its ordinary/contrast/failure cases.

## Preparation gates

1. **Capture the current source, including untracked files.** This checkout is
   dirty and the math asset tree is largely untracked. HEAD-only worktrees omit
   current assets. Freeze the required source/dependency closure with hashes;
   exclude generated media and never inspect images. Preserve the live textbook
   worker's files.
2. **Make the family boundary usable.** Existing pure kernels and builders such
   as PatchGeometry are the pattern. Expose the existing bounded SnapshotBuilder
   once if the pilot requires it; preserve its geometry/action behavior. Keep
   MathObjects semantic dispatch and shared registration under the lead.
3. **Freeze the packet.** It must name the exact inputs, units, validity rules,
   limits, expected results, file ownership and text-only checks. A template with
   blanks is not a dispatchable job.
4. **Choose the smallest batch.** First run one worker on three configurations
   from existing mathematics. After its integrated build and user review, try
   two workers in separate families, then at most three until integration keeps
   up. These are proposed staging limits, not assumptions about available slots.
5. **Use the existing preset mechanism first.** MathObjectPreset and ordered
   semantic actions already exist. There is no implemented general external
   asset-recipe loader or unattended batch runner. The lead must integrate new
   presets/configurations before claiming they are selectable in the app.

## File ownership during a batch

Each worker gets a unique output directory and a read-only frozen baseline.
Workers return new family-owned kernel/geometry/test files or a compact preset
fragment against existing contracts. They do not independently install into the
live shared checkout. The packet names every allowed output file and the exact
compile command or target available in that baseline.

Only the integration lead edits:

- src/runtime/math_objects/MathObjects.hpp and MathObjects.cpp;
- shared geometry-builder interfaces, if a reviewed change is necessary;
- CMakeLists.txt and app/math_lab_main.cpp;
- shared tests and architecture/workstream/checklist documents.

The lead can collect requested registration edits as a small textual patch, then
apply them to fresh live files. Never copy a worker's entire stale CMake or
registry over concurrent changes. Reconcile hashes before installation; rebuild
and rerun affected checks after integration. Keep all work uncommitted unless
the user requests a commit.

## Compact task packet

```text
Task ID / output directory:
Parent foundation and implemented baseline hash:
Exact output files and read-only dependencies:
Topic/card references (separate namespaces):
Observable construction and mathematical interpretation:
Fixed inputs, units, allowed controls and domains:
Ordinary / contrasting / invalid case, with independent expected results:
What is exact, numerical, illustrative or undefined:
Existing geometry, controls and semantic actions to reuse:
Snapshot/mesh budgets and failure behavior:
Exact compilation and CPU/text-only verification commands:
Requested shared registration edits, returned to the lead:
User launch command and two visual actions:
Stop condition: return any missing mathematical contract; do not invent it.
```

Every packet includes the absolute restriction: **no images, screenshots,
previews, offscreen captures, native windows or font-rasterization probes.**
Never run paths_native_math_tests or an unfiltered CTest suite. The lead inspects
selected commands and fixtures; application checks must use a confirmed
--validate route that exits before native host/font/persistence setup.

## First pilot: three configurations using existing mathematics

These inputs were executed through the current text-only Math Lab validator on
10 September 2026 and produced the expected values. They are prepared examples,
not new saved app presets, completed Terra tasks or human visual acceptance.
The worker's job is to package the three as reusable asset configurations with
independent expectations and a small regression; the lead owns menu registration.

### R01 — C6 maps onto C2

Source C6, target C2, f=(0,1,0,1,0,1). Expected kernel {0,2,4}, image {0,1},
homomorphism=true; both fibers have three elements. At full collapse there are
two reached target beads. This example is surjective and not injective.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --validate --object quotient --level 1 --object-preset 0 --set qu_target=0 --set qu_map0=0 --set qu_map1=1 --set qu_map2=0 --set qu_map3=1 --set qu_map4=0 --set qu_map5=1 --set qu_collapse=1
```

### R02 — C4 doubles into a proper image

Source and target C4, f=(0,2,0,2). Expected kernel {0,2}, image {0,2},
homomorphism=true; image size 2 while target size is 4. B1 and B3 remain unused.
This specifically checks that the quotient corresponds to the image, not the
whole target. The lab's generic surjective-map challenge is not its oracle.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --validate --object quotient --level 1 --object-preset 4 --set qu_target=2 --set qu_map0=0 --set qu_map1=2 --set qu_map2=0 --set qu_map3=2 --set qu_collapse=1
```

### R03 — One changed destination breaks preservation

Source and target C4, f=(0,2,0,3). For a=1, b=2, f(a+b)=3 whereas
f(a)+f(b)=2 modulo 4. Expected homomorphism=false and kernel undefined (metric
-1). The automatic witness selects that failing pair without repairing the map.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --validate --object quotient --level 0 --object-preset 4 --set qu_target=2 --set qu_map0=0 --set qu_map1=2 --set qu_map2=0 --set qu_map3=3
```

The user can remove --validate from these commands for a later manual visual
review. The agent must not run that windowed form.

## Scaling after the pilot

Once F01, F04 and F05 are implemented and their contracts are frozen, a useful
first parallel recipe batch is:

| Worker | Isolated assignment | Inputs decided by the lead |
| --- | --- | --- |
| A | F01 polynomial, hole and pole configurations | Exact factors, excluded domains, derivative/limit facts and marker meanings |
| B | F04 impulse, smoothing and circular-convolution configurations | Sample arrays, boundary convention, spacing and exact output arrays |
| C | F05 weighted PMF/CDF, binomial and sample-mean configurations | Population parameters, seed rules, analytic moments and supported empirical claims |

This batch is **blocked on those family contracts**, not ready merely because
its titles exist. Until then, family-kernel work can be split only after the
lead supplies the missing definitions and output boundaries. New algorithms
remain separate tasks even when they share a renderer.

## Completion evidence

A worker returns the owned files, exact mathematical cases checked, compile/test
results, requested integration edits and the manual launch/actions. Geometry
checks include capacities, index ownership, finite coordinates, unique IDs,
parameter bounds and undefined cases. Use a fresh CPU scene for independent
snapshot branches that can have identical revision numbers.

Current scene contracts cap primitives at 192, labels at 32, metrics at 12,
plots at 3 and series samples at 129. Indexed surfaces cap vertices at 4096;
the complete scene has its own separate budget. Larger diagrams need a reviewed
representation, not silent capacity increases or clipping.

The lead verifies the installed build and reports automated completion separately
from user visual acceptance. The textbook owner then connects the accepted asset
to a chapter; an asset worker does not add questions or learning-state behavior.
Record actual worker repairs and useful timings in the existing delivery note,
without inventing speedups or creating a second status ledger.
