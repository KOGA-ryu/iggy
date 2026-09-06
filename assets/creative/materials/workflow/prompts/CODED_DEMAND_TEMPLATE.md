# Coded Material Demands

This document belongs at `<material-package>/CODED_DEMANDS.md`.

Copy the demand structure once for every visible or semantic implementation
result. Replace the instructional text with actual material-specific facts and
complete executable code. Do not leave placeholder text in the package copy.

## Document contract

- Material ID:
- Capability:
- Profile schema:
- Pattern schema:
- Texture manifest schema:
- Blender manifest schema:
- Blender version inspected:
- Target engine:
- Champion seed:
- Workflow tier:
- Delivery boundary:
- Workstream dossier:
- Reference delta:

## Complete texture inventory

| Texture ID | Filename | Meaning | Physical span or repeat | Resolution | Metres per texel | Bit depth | Color space | Channels | Range and zero | Coordinate and variation | Distance behavior | Blender consumer | Engine consumer | Test |
| --- | --- | --- | --- | ---: | ---: | ---: | --- | --- | --- | --- | --- | --- | --- | --- |

Every runtime and proof texture must appear here before the first demand.

## Complete node and group inventory

| Group | Node name | `bl_idname` | Operation or mode | Inputs and units | Incoming links | Outgoing links | Contract role | Validation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |

Every new or changed live node must appear here. Do not list layout-only
reroutes unless a test or contract depends on them.

## Complete shader flow

Document the exact end-to-end flow:

1. coordinate source and units;
2. element identity, phase, mirror, and rotation;
3. texture sampling and color-space handling;
4. packed-channel decode;
5. base-color layers;
6. height decode and normal combination;
7. roughness, AO, and metalness;
8. physical material-layer blends;
9. stylization;
10. distance hierarchy;
11. damage and overlays;
12. BSDF and Material Output;
13. target-engine reconstruction.

## Ordered surface-method contract

| Order | Effect ID | Physical role | Exact operation | Inputs | Mask sources | Coordinate frame and scale | Outputs | Live consumers | Quiet rule | Isolated proof | Legacy translation |
| ---: | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

For new or materially revised `hero-master` and `reusable-family` work, this
table must match `workflow_contract.surface_method_contract.effect_stack`.
Do not write `blend`, `grunge`, `noise`, or `detail` without naming the exact
operation and material meaning.

## Geometry-to-map transfer

| Source geometry | Forms retained as geometry | Forms baked | Output maps or IDs | Projection/cage boundary | Scale authority | Proof |
| --- | --- | --- | --- | --- | --- | --- |

Use this table whenever sculpted, Boolean, displaced, or high-poly information
is baked. State why each frequency remains geometry or becomes a map.

## Damage placement method

- Enabled:
- Eligible semantic regions:
- Protected or prohibited regions:
- Event families:
- Count/density bounds:
- Size/depth/orientation bounds:
- Force or exposure direction:
- Quiet-area rule:
- Wrong-location negative proof:

## Workflow, performance, and acceptance contract

Document:

- measured, proxy, inherited, authored, and omitted claim ownership;
- texture-memory budget;
- object, vertex, polygon, node, link, image, and proof budgets;
- fast no-proof and changed-proof commands;
- canonical-output protection;
- actual-target proof required for acceptance;
- storage-only attributes and why they remain unconsumed.

## Demand record

### DEM-[TYPE]-[NUMBER]: Exact visible or semantic result

#### Demand

State one observable outcome and its acceptance boundary.

#### Authority

List exact source IDs, measurements, authored translations, proxy limits, and
unsupported claims.

#### Textures

List the inventory rows consumed or produced by this demand. Explain why every
channel exists and where it remains quiet.

#### Geometry semantics

List exact attribute names, Blender domains and data types, units, defaults,
assignment owner, modifier survival rule, and Boolean or fracture behavior.

#### Nodes

List exact inventory rows. Include group interface sockets, node names,
`bl_idname`, operations, defaults, and links.

#### Shader flow

Trace this demand from coordinates or attribute input to final visible output.
State distance and target-engine behavior.

#### Test code

Target: `absolute or repository-relative test path`

Insertion or replacement: `exact class, function, or context`

```python
# Write complete executable test code here.
# The package copy may not retain these instructional comments.
```

#### Profile or pattern code

Target: `exact profile or pattern path`

Parent object or replacement: `exact JSON key or complete file`

```json
{
  "write": "complete valid JSON here"
}
```

Omit this section only when the demand changes no authored data.

#### Generator code

Target: `exact generator path`

Insertion or replacement: `exact complete symbol`

```python
# Write complete executable generator code here.
```

Omit only when the demand produces or consumes no generated data.

#### Blender builder code

Target: `exact builder path`

Insertion or replacement: `exact complete symbol`

```python
# Write complete executable bpy code here.
```

Omit only when the demand has no Blender integration.

#### Validation and manifest code

Target: `exact generator, builder, or test path`

Insertion or replacement: `exact complete symbol`

```python
# Write complete validation and manifest code here.
```

#### Build and validation

List exact noninteractive commands in execution order. For every command,
state the expected failing or passing assertion.

Include:

- smallest red/green gate;
- no-proof temporary build;
- one-changed-proof temporary build;
- complete canonical build;
- automatic package audit;
- saved-file reopen gate.

#### Proof

List:

- isolated layer proof;
- neutral geometry proof;
- live shader proof;
- close, gameplay, and distance behavior;
- professional comparison;
- rejection condition.

#### Execution record

- Code reviewed:
- Red test:
- Applied:
- Green test:
- Visual result:
- Deviations from documented code:
- Demand status:

## Demand types

Use:

- `GEO`: geometry or semantic attributes;
- `TEX`: pattern, map, packing, or filtering;
- `SHD`: coordinates, nodes, material response, or stylization;
- `DMG`: damage, wear, repair, or overlay;
- `PRF`: proof, manifest, save, reopen, or target-engine parity.

## Completion audit

Before implementation begins, verify:

- no demand lacks code;
- no code block contains ellipses, `pass`, TODOs, or pseudocode;
- every called new helper is defined in the block or identified as an existing
  canonical symbol;
- every texture has a consumer;
- every live node has an inventory row;
- every shader lane reaches a final output;
- every demand has a failing test or absent-output gate;
- every demand has an isolated visual proof;
- every numeric claim under the declared roots has authority;
- performance budgets exist;
- fast builds cannot write canonical output;
- actual-target acceptance requirements are explicit;
- the ordered surface-method contract is complete when required;
- every sculpt-to-map transfer has an explicit frequency boundary;
- enabled damage has eligible and prohibited placement semantics;
- excluded damage and parity claims remain explicitly false.
