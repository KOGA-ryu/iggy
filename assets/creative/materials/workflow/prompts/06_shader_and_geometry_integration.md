# Prompt 06: Integrate the Maps with Geometry and the Live Shader

Build the versioned Blender material on the actual acceptance geometry and
preserve the target-engine translation boundary.

## Required reading

- package `WORKSTREAM.md`;
- package `CODED_DEMANDS.md`;
- `workflow/05_shader_integration/AGENTS.md`;
- final texture manifest;
- target-asset geometry profile and builder;
- current Blender build script and blend tests.

## Verify frozen build entry

Before changing the geometry builder, node graph, Blender tests, or any other
declared production target, run:

```bash
python3 assets/creative/materials/workflow/scripts/material_workflow_gate.py \
  verify-entry \
  --package <package-directory>
```

Stop on failure. Update the owning coded demand, freeze the revised blueprint,
and reopen build entry before continuing. Do not treat a documentation-only
freeze as an advisory checkpoint.

## Verify geometry first

Before assigning the material:

- construct or reopen the actual geometry;
- verify dimensions, silhouette, joins, and modifier results;
- verify UVs and named attributes;
- verify end, cut, component, face, and fracture identities;
- render neutral clay views;
- record any geometry failure the material must not hide.

If the required geometry or semantic attribute is absent, repair that bounded
contract before continuing. Do not invent the missing classification in the
shader.

## Build the node graph

Create or repair:

- versioned public material;
- versioned reusable node groups;
- stable metre coordinate reconstruction;
- element phase and variant;
- map sampling and color spaces;
- physical height amplitude;
- correct normal combination;
- broad and event roughness;
- physical material-layer blends;
- live stylization;
- distance hierarchy;
- default-off overlays.

Name nodes by semantic role. Keep the final data flow inspectable and testable.

Match the ordered surface-method contract exactly. Directional normal effects
must transform in their declared tangent frame before valid normal-detail
composition. Legacy gloss or blend-mode references must be translated into
the declared roughness, coating, or physical-layer operation.

Apply the reviewed geometry, node, shader, validation, proof, and Blender-test
blocks from `CODED_DEMANDS.md`. Update and re-review a demand before making any
implementation change that differs from its documented node inventory, socket
links, defaults, or code.

## Boolean and deformation safety

Prove the relevant behavior:

- object resize preserves scale;
- arbitrary length does not stretch the pattern;
- curve deformation does not swim;
- Geometry Nodes preserve attributes;
- Boolean-created faces receive explicit identity;
- end grain or fracture interiors are independently addressable;
- connected components do not share an accidental bounding-box coordinate.

## Build the proof fixture

The canonical builder should:

- render neutral geometry first;
- assign the live material;
- render close, hero, grazing, distance, and actual-asset views;
- pack required images;
- validate nodes, links, attributes, defaults, and prohibited lanes;
- save the `.blend`;
- write a Blender manifest and saved-file hash.

Use reduced render resolution for repair iterations. Produce final-resolution
proofs after the shader hierarchy reads.

## Reopen tests

Open the saved `.blend` in a fresh headless Blender process and verify:

- expected material and groups;
- coordinate nodes and links;
- map images and packing;
- color spaces;
- custom physical properties;
- geometry counts and dimensions;
- semantic attributes;
- prohibited generic noise or unsupported lanes;
- proof files;
- manifest schemas and saved-file hash.

## Target-engine contract

Update the profile and dossier with:

- Unreal or other engine material functions;
- required mesh attributes;
- coordinate operations;
- channel decode;
- normal combination;
- physical layer blend;
- distance behavior;
- stylization controls.

Keep parity false unless the target engine has rendered and passed the same
capability.

## Dossier update

Record:

- executed coded-demand IDs;
- material and node-group names;
- actual assigned geometry;
- coordinate and attribute proof;
- neutral and material render paths;
- build command;
- reopen-test command and result;
- Blender manifest;
- engine parity status;
- remaining visible risks.

Set Stage 06 to `complete` only when the saved file reopens and focused tests
pass.

## Exit gate

Do not advance unless:

- neutral geometry proof exists;
- the shader is live on the actual target geometry;
- scale and semantic attributes survive evaluation;
- authored maps and stylization are consumed;
- distance hierarchy is visible;
- images are packed;
- the `.blend` reopens green;
- frozen build entry remains valid;
- target-engine parity is honest.

Then execute `07_damage_and_overlays_optional.md`.
