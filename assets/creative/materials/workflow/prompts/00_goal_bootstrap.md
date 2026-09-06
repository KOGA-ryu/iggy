# Prompt 00: Bootstrap the Material Goal

Execute this prompt immediately after the material goal is created.

## Instructions

1. Read the active goal objective and current goal status.
2. Read the repository `AGENTS.md`.
3. Read `assets/creative/materials/AGENTS.md` completely.
4. Read:
   - `assets/creative/materials/QUALITY_BENCHMARK.md`
   - `assets/creative/materials/MEASURED_CONSTRUCTION_DOSSIER.md`
   - `assets/creative/materials/workflow/TEXTURE_WORKBENCH.md`
5. Inspect repository status without changing unrelated work.
6. Resolve the material package from the goal:
   - use the existing package when one owns the capability;
   - create a new package only when no canonical owner exists;
   - do not create a parallel `v2` merely to avoid repairing the live package.
7. Read the package `README.md`, `RESEARCH_INTENT.md`, profile, pattern files,
   latest manifests, canonical generator, Blender builder, and focused tests.
8. Create or update `<package>/WORKSTREAM.md` from
   `GOAL_DOSSIER_TEMPLATE.md`.
9. Rewrite the goal as one bounded capability in the dossier without changing
   the user's intended material, style, asset, engine, or delivery.
10. Classify the work as `hero-master`, `reusable-family`, or
    `bounded-variation` using `workflow/phases/01_scope_and_audit.md`.
11. Record exact included and excluded lanes.
12. Record whether the objective requires:
    - a research result;
    - generated maps;
    - a Blender material;
    - geometry integration;
    - an Unreal translation;
    - damage or overlays;
    - manual user acceptance.
13. Record all existing dirty paths in or near the package and identify which
    belong to the user.
14. Resolve the goal to a normalized workbench composition:
    - substrate;
    - construction or manufacturing state;
    - finish or coating;
    - optional condition or deposit;
    - stylization;
    - renderer-owned lighting or post-process.
15. Copy the relevant workbench tools and package donors into the dossier.
    Classify each as exact reuse, parameterized recipe, shared profile/motif,
    assembly graph, bespoke hero layer, missing tool, or rejected route.
16. For every unresolved major component, record at least two viable
    construction strategies and the evidence that will select between them.
    Do not advance with only one improvised route.

## Scope test

The capability must be narrow enough to complete and prove independently.

Good:

- intact oak side grain on the approved structural door;
- forged-scale material response on measured door hardware;
- utility-rope nested strand and yarn material;
- measured intact ashlar visual hierarchy;
- timber end-grain overlay;
- one end-check damage family.

Too broad:

- all wood;
- castle materials;
- make everything Arcane quality;
- complete damage system;
- Blender and Unreal asset pipeline.

If the active goal is broad, choose the first dependency that yields a visible
usable capability and record the remaining items as later candidates. Do not
discard them.

## Dossier requirements

Populate:

- goal;
- material ID;
- capability;
- target asset or scene;
- required final artifact;
- manual acceptance requirement;
- package root;
- scope;
- existing state;
- stage ledger;
- unrelated changes to preserve.
- normalized material composition;
- at-hand tool and donor inventory;
- multi-strategy table and selection proofs.

Set Stage 00 to `complete` only after all paths and boundaries are concrete.

## Exit gate

Do not advance unless:

- one canonical package is named;
- one visible capability is named;
- the actual target geometry or use is named;
- final deliverables are named;
- excluded damage and overlays are explicit;
- existing files and dirty state are recorded;
- manual acceptance and target-engine parity requirements are explicit;
- workflow tier and its proof burden are explicit;
- the selected workbench card and normalized composition are recorded;
- required tools are at hand or a missing reusable tool is named as the first
  dependency;
- every unresolved major component retains at least two strategies;
- `WORKSTREAM.md` exists and Stage 00 is complete.

## Handoff

At the bottom of Stage 00 evidence, record:

- package path;
- next prompt path;
- one sentence describing what Prompt 01 must verify.

Then execute `01_existing_asset_audit.md`.
