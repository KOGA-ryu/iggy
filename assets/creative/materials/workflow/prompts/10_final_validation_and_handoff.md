# Prompt 10: Final Validation, Boundary, and Goal Handoff

Perform the final independent audit of the exact goal objective.

## Required reading

- active goal objective and status;
- package `WORKSTREAM.md`;
- package `CODED_DEMANDS.md`;
- package `README.md`, `RESEARCH_INTENT.md`, profile, manifests, and tests;
- Stage 08 proofs and Stage 09 repair record;
- repository and material `AGENTS.md`.

## Run the independent package auditor

Run `workflow/scripts/audit_material_package.py` with the package profile,
pattern, manifests, generator, builder, tests, coded demands, reference delta,
output directory, and workflow state. Use `--state candidate` unless every
tier-required acceptance input already exists. The exact argument list belongs
in the package `README.md` or `WORKSTREAM.md`.

The auditor must reject:

- changed coded demands after freeze or production targets omitted from the
  frozen baseline;
- stale or extra canonical outputs;
- numeric dimensions or budgets without claim-ledger ownership;
- dormant image or attribute nodes;
- generated geometry attributes with no shader consumer or declared
  `storage_only_attributes` role;
- performance budget overruns;
- generated documentation drift;
- an `accepted` state without required actual-target proof and manual review.

Do not waive a red audit manually. Repair its canonical owner and rerun the
smallest affected gate.

## Final artifact audit

Verify:

- canonical pattern and generator;
- canonical Blender builder;
- final-resolution maps;
- texture manifest;
- packed `.blend`;
- Blender manifest and saved-file hash;
- neutral geometry proof;
- six adversarial proof groups;
- actual-asset proof;
- focused generator, contract, and reopen tests;
- package auditor green for the declared state;
- default-off exclusions;
- no unintended temporary backup or cache artifacts;
- no unrelated changes modified.

Recompute the saved-file hash and compare it with the manifest.

## Documentation audit

Ensure `README.md`, `RESEARCH_INTENT.md`, profile, manifest, tests, and
`WORKSTREAM.md` agree on:

- capability;
- measurements and proxy limits;
- coordinate contract;
- maps and packed channels;
- material and node names;
- distance hierarchy;
- stylization;
- damage and overlays;
- Blender status;
- target-engine parity;
- acceptance state.

Remove or repair stale quality claims. Do not claim engine parity from Blender
proof.

When the package uses
`workflow/scripts/render_coded_demands.py`, run its full material-specific
command with `--check`. A stale `CODED_DEMANDS.md` is a final-gate failure.

## Goal-objective audit

Read the objective literally. Determine:

- what has been achieved;
- whether manual user acceptance is required;
- whether any required target engine, asset, proof, or test remains;
- whether the output is `production-candidate` or `accepted`;
- whether a later capability is merely desirable rather than required.

Do not expand the objective during handoff.

## Final dossier

Complete:

- all stage statuses;
- verification commands and results;
- final visual review;
- final capability boundary;
- exclusions;
- Blender and target-engine parity;
- manual review state;
- next material capability.

Set Stage 10 to `complete` only if the final audit is green.

## Goal completion rule

Mark the active goal complete only when:

- its exact objective is achieved;
- no required work remains;
- required manual acceptance has been received;
- all current artifacts and tests match.

If the result is a production candidate awaiting required manual acceptance,
report that clearly and leave the goal active unless the goal objective only
required delivery of the candidate.

If a genuine blocker remains, follow the active goal's blocker policy. Do not
use blocked as a synonym for incomplete.

## Exit gate

Do not finish the chain unless:

- every stage has a terminal evidence-backed status;
- final artifacts match their manifests and hashes;
- the independent package audit is green for the declared state;
- focused tests and reopen validation are green;
- the exact goal objective has been audited;
- manual acceptance and target-engine parity are reported truthfully;
- the dossier and package documentation agree;
- remaining work is either outside the bounded capability or prevents goal
  completion explicitly.

## Handoff report

Lead with the outcome. Include:

- exact capability and state;
- best hero and diagnostic proofs;
- maps, `.blend`, manifest, generator, builder, and dossier links;
- research and measurement authorities;
- key layer and coordinate decisions;
- test counts and reopen result;
- damage and overlay exclusions;
- target-engine parity;
- remaining risk;
- next capability.

State that changes are uncommitted unless the user explicitly asked otherwise.
If the package was untracked before the goal, do not invent a LOC delta.
