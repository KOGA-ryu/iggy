# Prompt 00: Consumer Census and Sword Typology

## Objective

Bind the material work to one named sword and determine what kind of blade it
is before any pattern, hamon, patina, or damage is invented.

## Inputs

- the candidate sword meshes, renders, concept sheets, and measurements;
- any Box asset identifier or stable local export path;
- intended game camera, hero distance, and engine;
- historical, fictional, or shipped-game references for the same sword role;
- the current `sword_steel_v1` donor manifest and proof board.

## Procedure

1. Give every sword a stable asset ID. Record source location without copying
   unlicensed pixels into the package.
2. Record blade length, maximum width, thickness, edge land, fuller width,
   ricasso length, tip length, grip axis, and units. Classify every value as
   measured, catalogued, proxy, authored translation, or unknown.
3. Translate the visible blade into text:
   - straight, curved, single-edged, or double-edged;
   - lenticular, diamond, hollow, hexagonal, fullered, ridged, or unknown
     section;
   - homogeneous, differentially hardened, pattern-welded, crucible/watered,
     laminated, coated, magical, or unknown steel construction;
   - mirror, satin, stone-polished, belt-ground, hammered, etched, painted, or
     unknown finish;
   - pristine, maintained, used, excavated, ceremonial, or unknown condition.
4. Record where the sword appears on screen and which portions are occluded by
   the hand, guard, scabbard, particles, or animation.
5. Score reference authority, view coverage, construction evidence,
   finish-frequency evidence, and ambiguity from zero to three.
6. Select one workflow tier and one visible capability. Do not choose “complete
   sword material” as the first capability.

## Required output

Add a consumer record to `WORKSTREAM.md` containing:

```text
asset_id:
source_location:
actual_mesh_available:
workflow_tier:
active_capability:
blade_family:
construction_family:
finish_family:
condition_state:
measured_dimensions:
unknown_dimensions:
camera_and_distance:
reference_score:
actual_target_acceptance_required:
```

## Rejection rules

- Do not infer Damascus from decorative waves.
- Do not infer a hamon from a pale edge stripe.
- Do not infer a fuller from a painted dark band.
- Do not call corrosion “forged texture.”
- Do not use the scimitar measurement proxy to claim a straight arming-sword
  section.
- If the mesh is unavailable, state `diagnostic fixture`; do not pretend the
  consumer is present.

## Exit gate

Proceed only when one sword, one blade family, one finish state, and one
capability are named. Ambiguous construction features remain disabled.
