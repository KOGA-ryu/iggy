# Forged-Iron Coded Demands

Blueprint revision 6. The blocks below are the reviewed implementation
boundary for the intact forged-iron repair. Production code may not diverge
without revising and refreezing this document.

## DEM-MAP-001: Compile intact scale, finite tooling, and worked-face maps

### Demand

The generator shall preserve the finite authored colour, scale-plate,
planishing, and cross-peen vocabulary; remove hammer-derived conductor
exposure; add a finite worked-face vocabulary; and emit twelve shader input
maps plus compatibility maps and a causal proof.

Default metalness must be exactly zero. Planishing and cross-peen may change
normal, height, and roughness but not physical-layer identity. Worked-face
brushing must have finite coverage, encoded direction, protected rest, a
separate normal, and no height/damage claim. It uses a separate `2.436 x
0.492 m` field with at least twelve irregular passes, not the smaller scale
tile that produced repeated cross-stock rows.

### Test code

```python
material = generator.generate_material(recipe, resolution_x=768, resolution_y=128)
self.assertEqual(float(material["metalness"].min()), 0.0)
self.assertEqual(float(material["metalness"].max()), 0.0)
self.assertEqual(material["worked_normal"].shape, (512, 768, 3))
self.assertEqual(material["worked_response"].shape, (512, 768, 3))
worked = material["worked_response"][..., 0]
rest = material["worked_response"][..., 2]
self.assertGreater(float((worked > 0.10).mean()), 0.16)
self.assertLess(float((worked > 0.10).mean()), 0.58)
self.assertGreater(float((rest > 0.75).mean()), 0.35)
np.testing.assert_allclose(material["layer_response"][..., 2], 0.0)
```

```python
self.assertEqual(
    set(manifest["outputs"]),
    {
        "base_color", "scale_color", "iron_color", "roughness",
        "metalness", "layer_response", "height", "macro_height",
        "scale_height", "micro_height", "normal", "macro_normal",
        "scale_normal", "micro_normal", "worked_normal",
        "worked_response", "masks", "proof",
    },
)
self.assertTrue(manifest["constraints"]["hammering_never_implies_exposed_iron"])
self.assertEqual(manifest["layer_statistics"]["conductive_fraction_above_0_90"], 0.0)
```

### Generator code

```python
exposed_iron = np.zeros_like(activity, dtype=np.float32)
metalness = exposed_iron

worked_height = np.zeros_like(activity)
worked_mask = np.zeros_like(activity)
worked_angle = np.zeros_like(activity)
for authored_pass in recipe["worked_surface"]["passes"]:
    pass_mask, signed_bundle_height = _rasterize_worked_pass(
        worked_x, worked_y, authored_pass,
        period_x=worked_length, period_y=worked_width
    )
    worked_height += signed_bundle_height
    stronger = pass_mask > worked_mask
    worked_angle = np.where(
        stronger,
        (float(authored_pass["rotation_deg"]) + 90.0) / 180.0,
        worked_angle,
    )
    worked_mask = np.maximum(worked_mask, pass_mask)
worked_height = np.clip(worked_height, -0.000004, 0.000004)
worked_normal = height_to_normal(worked_height, metres_per_pixel, strength=1.0)
detail_priority = np.clip(1.0 - worked_mask * 0.78, 0.0, 1.0)
worked_response = np.stack(
    [worked_mask, np.clip(worked_angle, 0.0, 1.0), detail_priority],
    axis=-1,
)
roughness = np.clip(scale_roughness - worked_mask * 0.05, 0.56, 0.84)
```

The actual implementation must use periodic metre deltas, finite superellipse
pass masks, declared 2.4-4.8 mm bundle spacing, longitudinal breakup inside
each pass, OpenGL tangent normal output, 16-bit physical height lanes, and
manifest hashes.

## DEM-SHADER-002: Separate fabrication and surface coordinates in Blender

### Demand

The public material shall use a five-attribute coordinate contract, three atlas
vectors, twelve image nodes, four independently controlled normal bands,
fabrication-scaled macro displacement, and a named UV tangent. The intact
dielectric shader is default. The conductor shader is reachable only through
the default-zero contact-polish mask.

### Test code

```python
self.assertIn("sinc_iron_fabrication_scale", payload["strap_attributes"])
self.assertEqual(payload["strap_fabrication_scale"], 1.0)
self.assertEqual(payload["texture_node_count"], 12)
self.assertEqual(payload["tangent_uv_maps"], ["IGGY_IronUV"])
self.assertEqual(payload["group_inputs"]["Worked Normal Strength"], 0.62)
self.assertEqual(payload["default_conductor_mask"], 0.0)
self.assertEqual(payload["noise_node_count"], 0)
```

```python
self.assertEqual(
    payload["node_groups"],
    [
        "IGGY_SH_AuthoredIronLanes_v004",
        "IGGY_SH_ForgeScaleLayer_v002",
        "IGGY_SH_HammerPlanes_v003",
        "IGGY_SH_IronCoordinates_v004",
        "IGGY_SH_IronMicroSurface_v002",
        "IGGY_SH_NormalCombine_v003",
        "IGGY_SH_ReferenceForgedIron_v003",
        "IGGY_SH_SurfaceHeight_v003",
        "IGGY_SH_WorkedIronSurface_v001",
    ],
)
```

### Blender builder

```python
fabrication = ShaderNodeAttribute("sinc_iron_fabrication_scale")
safe_scale = MAXIMUM(fabrication.Fac, 1.0)
macro_u = sinc_iron_u_m / safe_scale
macro_v = sinc_iron_v_m / safe_scale
surface_u = sinc_iron_u_m
surface_v = sinc_iron_v_m
worked_u = sinc_iron_u_m
worked_v = sinc_iron_v_m
macro_atlas = atlas_vector(macro_u, macro_v, sinc_seed)
surface_atlas = atlas_vector(surface_u, surface_v, sinc_seed)
worked_atlas = atlas_vector(
    worked_u, worked_v, sinc_seed,
    physical_tile_m=(2.436, 0.492),
)
```

```python
macro_maps = (macro_normal, macro_height, semantic_masks)
surface_maps = (
    scale_color, iron_color, layer_response, scale_normal, scale_height,
    micro_normal, micro_height,
)
worked_maps = (worked_normal, worked_response)
for image in macro_maps:
    image.vector = macro_atlas
for image in surface_maps:
    image.vector = surface_atlas
for image in worked_maps:
    image.vector = worked_atlas

tangent = ShaderNodeTangent()
tangent.direction_type = "UV_MAP"
tangent.uv_map = "IGGY_IronUV"
oxide_bsdf.Tangent = tangent.Tangent
iron_bsdf.Tangent = tangent.Tangent
oxide_bsdf.Anisotropic = worked_mask * 0.18
oxide_bsdf.Anisotropic_Rotation = worked_angle
```

The live builder uses explicit Blender nodes with these semantic names:

- `Component_Metre_And_Fabrication_Coordinates`
- `Twelve_Authored_Physical_Lanes`
- `Finite_Hammer_Planes`
- `Finite_Worked_Surface`
- `Whiteout_Four_Frequency_Normal`
- `Component_Local_Worked_Tangent`
- `Physical_Oxide_And_Optional_Contact_Iron`
- `Fabrication_Scaled_Metre_Displacement`

The macro height decoder multiplies only the decoded macro hammer lane by the
safe fabrication scale. Broad scale colour, scale-lip response, micro, and
worked response retain raw world-metre coordinates and do not scale in
amplitude. Worked response uses its larger independent tile. This prevents a
ten-times giant hammer from also turning millimetre oxide and finish structure
into centimetre bands or a small finish tile into repeated rows.

## DEM-TARGET-003: Assign and prove the actual openwork hinge

### Demand

The canonical builder shall import the approved eight mesh objects from
`openwork_strap_hinge_geometry_v1.blend`, preserve their geometry, replace
neutral clay only in the proof copy, write semantic coordinates and
`IGGY_IronUV`, set fabrication scale ten, assign the canonical forged-iron
material, render adversarial target proofs, pack all twelve authored images,
save, and reopen in a separate process.

### Test code

```python
expected_target = {
    "SM_GH018_FixedLeaf",
    "SM_GH018_MovingLeaf_Openwork",
    "SM_GH018_Pintle",
    "MovingKnuckle_01",
    "FixedKnuckle_02",
    "MovingKnuckle_03",
    "FixedKnuckle_04",
    "MovingKnuckle_05",
}
self.assertEqual(set(payload["target_objects"]), expected_target)
for item in payload["target_objects"].values():
    self.assertEqual(item["materials"], ["IGGY_MAT_ReferenceForgedIron_v003"])
    self.assertEqual(item["fabrication_scale"], 10.0)
    self.assertIn("IGGY_IronUV", item["uv_layers"])
```

```python
for proof_name in (
    "forged_iron_v1_actual_hinge_clay_front.png",
    "forged_iron_v1_actual_hinge_front.png",
    "forged_iron_v1_actual_hinge_grazing.png",
    "forged_iron_v1_actual_hinge_base_colour.png",
    "forged_iron_v1_actual_hinge_worked_response.png",
):
    self.assertTrue((MATERIAL_ROOT / "output" / proof_name).is_file())
```

### Production code

```python
with bpy.data.libraries.load(str(ACTUAL_HINGE_SOURCE), link=False) as (
    data_from, data_to
):
    data_to.objects = [
        name for name in ACTUAL_HINGE_OBJECT_NAMES
        if name in data_from.objects
    ]
for index, obj in enumerate(data_to.objects):
    target_collection.objects.link(obj)
    obj.data.materials.clear()
    obj.data.materials.append(iron_material)
    write_component_coordinates(
        obj,
        seed=index,
        fabrication_scale=10.0,
        tangent_uv="IGGY_IronUV",
    )
```

The proof renderer must use neutral material light, grazing white light, and
unlit data outputs without fog, bloom, depth of field, cinematic grading, or
damage. Front framing must derive one world-space bound from all eight target
objects, not from the moving leaf alone. Orthographic scale is at least `1.15`
times the complete assembly span so the 4.06 m pointed leaf, pivot, and fixed
leaf are simultaneously visible with a quiet margin.

```python
target_bounds = [
    obj.matrix_world @ Vector(corner)
    for obj in target_objects
    for corner in obj.bound_box
]
target_span_x = max(point.x for point in target_bounds) - min(
    point.x for point in target_bounds
)
camera.data.ortho_scale = max(
    target_span_z * 1.35,
    target_span_x * 1.15,
)
```

The manifest records the source hash, exact target dimensions,
attribute inventory, material assignment, render hashes, and
`unreal_runtime_parity_verified: false`.

## Execution commands

```sh
python3 -m unittest tests.unit.forged_iron_v1_profile_tests
/Applications/Blender.app/Contents/MacOS/Blender --background --python-expr 'import runpy,sys; sys.argv=["tests/unit/forged_iron_v1_generator_tests.py"]; runpy.run_path(sys.argv[0],run_name="__main__")'
/Applications/Blender.app/Contents/MacOS/Blender --background --factory-startup --python assets/creative/materials/forged_iron_v1/build_forged_iron_v1.py -- --proof-resolution-x 900 --proof-resolution-y 570
python3 -m unittest tests.unit.forged_iron_v1_blend_tests
```

The final visual decision follows inspection of the actual-hinge front,
grazing, base-colour, worked-response, close specimen, and gameplay specimen
proofs. Automated green establishes reproducibility and live routing, not
manual acceptance.

## DEM-CUMULATIVE-004: Assemble the five accepted intact-material donors

### Demand

Build one disposable-but-durable cumulative proof on the approved eight-part
openwork hinge. The proof shall consume the accepted conductor, tangent,
broad-plane, continuous-oxide, and two-BSDF mixer donors without changing the
canonical `forged_iron_v1.blend` or its v003 material. This is the integration
gate before a canonical v004 rewrite.

The intact state is mandatory: the explicit exposed-conductor value is exactly
zero. No edge field, curvature, hammer activity, broad plane, colour field, or
implicit metallic minimum may alter it. Rust, pits, scratches, abrasion,
polish, chips, scale fracture, deposits, and narrative condition remain absent.

### Production code

#### Frozen constants and ownership

```python
CONDUCTOR_F0_LINEAR = (0.56, 0.57, 0.58)
CONDUCTOR_ROUGHNESS = 0.38
ANISOTROPY = 0.18
GIANT_LEAF_FORGING_AMPLITUDE_M = 0.006
EXPOSED_CONDUCTOR = 0.0
OXIDE_MINIMUM_THICKNESS_M = 0.000020
OXIDE_MAXIMUM_THICKNESS_M = 0.000090
TANGENT_UV = "IGGY_IronTangentUV_v001"
FIELD_UV = "IGGY_IronFieldUV_v001"

TANGENT_MODES = {
    "SM_GH018_FixedLeaf": "longitudinal_leaf",
    "SM_GH018_MovingLeaf_Openwork": "longitudinal_leaf",
    "SM_GH018_Pintle": "axial_pin",
    "MovingKnuckle_01": "circumferential_barrel",
    "FixedKnuckle_02": "circumferential_barrel",
    "MovingKnuckle_03": "circumferential_barrel",
    "FixedKnuckle_04": "circumferential_barrel",
    "MovingKnuckle_05": "circumferential_barrel",
}
```

The conductor owns optical identity only. The oxide owns visible intact colour,
dielectric roughness, coverage, and thickness. Broad forging planes own one
normal lane only. Component UVs own tangent direction. The explicit exposure
value is the sole mixer owner.

#### Exact component-frame installation

```python
uv_m = common.component_tangent_uv(
    vertices,
    loop_vertex_indices,
    polygon_loop_starts,
    polygon_loop_totals,
    polygon_normals,
    mode=TANGENT_MODES[obj.name],
)
for loop_index, coordinate_m in enumerate(uv_m):
    tangent_layer.data[loop_index].uv = coordinate_m
tangent_min = uv_m.min(axis=0)
tangent_span = np.maximum(uv_m.max(axis=0) - tangent_min, 1.0e-6)
if TANGENT_MODES[obj.name] == "longitudinal_leaf":
    field_coordinate_mode = "object_xz_host"
    field_coordinate_m = arrays["vertices"][:, (0, 2)][loop_vertex_indices]
else:
    field_coordinate_mode = "component_tangent_host"
    field_coordinate_m = uv_m
field_min = field_coordinate_m.min(axis=0)
field_span = np.maximum(field_coordinate_m.max(axis=0) - field_min, 1.0e-6)
for loop_index, coordinate_m in enumerate(field_coordinate_m):
    field_layer.data[loop_index].uv = (
        coordinate_m - field_min
    ) / field_span
frame = {
    "field_coordinate_mode": field_coordinate_mode,
    "minimum_m": field_min,
    "maximum_m": field_min + field_span,
    "span_m": field_span,
    "tangent_minimum_m": tangent_min,
    "tangent_span_m": tangent_span,
}
```

`TANGENT_UV` remains metre-scaled and is read only by `ShaderNodeTangent`.
`FIELD_UV` is the non-repeating zero-to-one host domain for packed, per-object
proof images. Leaf optical fields use their actual object-local X/Z stock
dimensions; they must not inherit bevel- and aperture-boundary tangent
projections. The smoke proof rejected the former coupling because it expanded
the 3.112 m moving leaf to a false 6.253 m optical domain. Cylindrical parts use
their component tangent host. The proof may pack those images into its own
`.blend`; it may not turn them into a square tiled master or claim Unreal
delivery.

#### Exact broad-plane construction

```python
amplitude_m = GIANT_LEAF_FORGING_AMPLITUDE_M * min(
    1.0,
    max(host_cross_stock_m, 1.0e-6) / 0.410,
)
unit_rails = (
    (-0.5, ((0.00, 0.00), (0.16, 0.35), (0.37, -0.20),
            (0.61, 0.50), (0.82, -0.30), (1.00, 0.00))),
    ( 0.0, ((0.00, 0.10), (0.22, -0.40), (0.49, 0.45),
            (0.71, -0.35), (1.00, 0.05))),
    ( 0.5, ((0.00, 0.00), (0.18, 0.28), (0.45, -0.32),
            (0.78, 0.38), (1.00, 0.00))),
)
rails = [
    {
        "y_m": host_min_v_m + (rail_v + 0.5) * host_cross_stock_m,
        "knots_m": [
            [host_min_u_m + knot_u * host_length_m, knot_h * amplitude_m]
            for knot_u, knot_h in knots
        ],
    }
    for rail_v, knots in unit_rails
]
forging_height_m = common.broad_forging_plane_field(u_m, v_m, rails)
forging_normal = common.height_to_normal_nonperiodic(
    forging_height_m,
    meters_per_pixel_x=host_length_m / (resolution_x - 1),
    meters_per_pixel_y=host_cross_stock_m / (resolution_y - 1),
)
```

There are three open rails, five or six irregular knots per rail, no closed
motif event, no opposite-edge wrap, and no displacement or silhouette claim.
Narrow knuckles scale the 6 mm giant-leaf calibration by their cross-stock
span so the leaf calibration is not blindly imposed on small stock.

#### Exact continuous oxide construction

```python
thickness_rails = [
    {"y_m": min_v, "knots_m": [
        [min_u, 0.000045], [min_u + length * 0.23, 0.000062],
        [min_u + length * 0.58, 0.000038], [max_u, 0.000053]]},
    {"y_m": min_v + width * 0.5, "knots_m": [
        [min_u, 0.000052], [min_u + length * 0.34, 0.000033],
        [min_u + length * 0.72, 0.000071], [max_u, 0.000048]]},
    {"y_m": max_v, "knots_m": [
        [min_u, 0.000049], [min_u + length * 0.27, 0.000057],
        [min_u + length * 0.65, 0.000041], [max_u, 0.000055]]},
]
oxide = common.continuous_oxide_layer_fields(
    u_m,
    v_m,
    thickness_rails,
    longitudinal_modes=[],
    minimum_thickness_m=OXIDE_MINIMUM_THICKNESS_M,
    maximum_thickness_m=OXIDE_MAXIMUM_THICKNESS_M,
)
response = common.smoothstep(0.0, 1.0, oxide["thickness_response"])
thin_srgb = np.asarray((0.090, 0.100, 0.116), dtype=np.float32)
thick_srgb = np.asarray((0.158, 0.174, 0.202), dtype=np.float32)
oxide_base_linear = common.srgb_to_linear(
    common.mix(
        np.broadcast_to(thin_srgb, response.shape + (3,)),
        np.broadcast_to(thick_srgb, response.shape + (3,)),
        response,
    )
)
oxide_roughness = np.clip(0.655 + response * 0.105, 0.655, 0.760)
```

Coverage is one, oxide metalness is zero, and oxide surface height is zero at
every texel. Thickness changes optical colour and roughness only. Compression
modes remain disabled because the accepted B thermal-rail candidate did not
need them.

#### Exact Blender response topology

```python
oxide_bsdf = nodes.new("ShaderNodeBsdfPrincipled")
oxide_bsdf.name = "Complete_Intact_Oxide_Response"
oxide_bsdf.inputs["Metallic"].default_value = 0.0
oxide_bsdf.inputs["IOR"].default_value = 2.10

conductor_bsdf = nodes.new("ShaderNodeBsdfPrincipled")
conductor_bsdf.name = "Complete_Hidden_Iron_Response"
conductor_bsdf.inputs["Base Color"].default_value = (
    *CONDUCTOR_F0_LINEAR, 1.0
)
conductor_bsdf.inputs["Metallic"].default_value = 1.0
conductor_bsdf.inputs["Roughness"].default_value = CONDUCTOR_ROUGHNESS
conductor_bsdf.inputs["Anisotropic IOR Level"].default_value = ANISOTROPY

tangent = nodes.new("ShaderNodeTangent")
tangent.direction_type = "UV_MAP"
tangent.uv_map = TANGENT_UV
links.new(tangent.outputs["Tangent"], conductor_bsdf.inputs["Tangent"])

normal_map = nodes.new("ShaderNodeNormalMap")
normal_map.space = "TANGENT"
normal_map.uv_map = FIELD_UV
links.new(normal_image.outputs["Color"], normal_map.inputs["Color"])
links.new(normal_map.outputs["Normal"], oxide_bsdf.inputs["Normal"])
links.new(normal_map.outputs["Normal"], conductor_bsdf.inputs["Normal"])

exposure = nodes.new("ShaderNodeValue")
exposure.name = "Explicit_Exposed_Conductor_Only_Owner"
exposure.outputs[0].default_value = EXPOSED_CONDUCTOR
surface = nodes.new("ShaderNodeMixShader")
surface.name = "Complete_Oxide_And_Conductor_Responses"
links.new(exposure.outputs[0], surface.inputs["Fac"])
links.new(oxide_bsdf.outputs["BSDF"], surface.inputs[1])
links.new(conductor_bsdf.outputs["BSDF"], surface.inputs[2])
```

The production script shall use socket-name fallbacks for Blender-version
differences, but it may not replace the two complete lobes with interpolated
metalness parameters. The default combined render must be invariant to the
hidden conductor because `EXPOSED_CONDUCTOR` is zero.

### Test code

```python
self.assertIn("component_tangent_uv", source)
self.assertIn("continuous_oxide_layer_fields", source)
self.assertIn("ShaderNodeMixShader", source)
self.assertIn("EXPOSED_CONDUCTOR = 0.0", source)
for prohibited in (
    "scale_plates", "_irregular_plate", "contact_polish",
    "edge_wear", "rust_mask", "damage_mask",
):
    self.assertNotIn(prohibited, source)
```

The proof package shall contain a separate `.blend`, manifest, actual-hinge
neutral front, three-quarter, grazing, gameplay-distance, moving-light A and B,
oxide-colour lane, roughness lane, forging-normal lane, and exposure lane. The
manifest records source hashes before and after and fails if either the
canonical blend or approved geometry blend changes. Required numerical gates:

```python
assert manifest["exposed_conductor"]["minimum"] == 0.0
assert manifest["exposed_conductor"]["maximum"] == 0.0
assert manifest["oxide"]["coverage_minimum"] == 1.0
assert manifest["oxide"]["coverage_maximum"] == 1.0
assert manifest["oxide"]["metalness_maximum"] == 0.0
assert manifest["oxide"]["surface_height_abs_maximum_m"] == 0.0
assert manifest["node_topology"]["principled_count_per_material"] == 2
assert manifest["node_topology"]["mix_shader_count_per_material"] == 1
assert manifest["canonical_source_unchanged"] is True
assert manifest["unreal_parity_verified"] is False
```

The front camera must frame the complete 4.06 m assembly. In Blender's
orthographic camera the accepted horizontal gate is:

```python
base_scale = max(span_z * aspect * 1.48, span_x * 1.12)
camera.data.ortho_scale = base_scale
```

The earlier `span_x / aspect` expression is rejected because its smoke proof
cropped both the pointed tail and fixed leaf.

Only this cumulative proof is authorized by revision 5. Canonical v004
integration remains gated on visual critique of the complete actual hinge.

## DEM-EXPLORATION-005: Compare intact oxide surface-layer strategies

### Demand

Render one disposable reduced-resolution board on the approved eight-part
hinge before changing the cumulative candidate. Rows are the current smooth
control, quiet compact scale, heavier forged scale, and compressed directional
scale. Columns are front, grazing, gameplay, and causal-layer response. Every
row uses the same geometry, cameras, lights, exposure, conductor, complete
oxide coverage, and two-BSDF topology.

This demand owns selection only. It may not change the canonical blend, save a
new blend, update the production profile, run a full-resolution build, or
declare acceptance. Each row is removed from Blender data before the next row
is built so candidate images and materials do not accumulate in memory.

### Authority

- Professional steel breakdowns separate base response, broad imperfection,
  high- and low-frequency structure, and small roughness changes instead of
  copying one field into all channels.
- Written Substance breakdowns begin from uniform height and roughness, add
  low-opacity large/medium/small passes, preserve flat rest regions, and keep
  condition masks out of relief when they do not improve material response.
- Published mill-scale sections motivate the measured-like 8--35 micrometre
  compact candidate and the phase-informed blue-black/warm-black palette. The
  20--90 micrometre heavy candidate remains an authored giant-forge
  translation, not a universal measurement.
- The existing smooth cumulative proof is the control. Its conductor, tangent,
  broad forging plane, continuous coverage, and physical mixer remain fixed.

### Textures

Every object receives four packed in-memory float images per candidate:

1. `OxideBase`: linear RGB optical colour; nonrepeating component field;
   no lighting or AO.
2. `OxideRoughness`: scalar independent composition of baseline, thermal,
   compression, morphology, and grain responses.
3. `CombinedSurfaceNormal`: OpenGL tangent normal derived once from broad
   forging height plus bounded oxide morphology height.
4. `LayerResponse`: diagnostic RGB where red is thermal macro, green is
   compression flow, and blue is detail priority multiplied by micro grain.

The exploratory target pitch remains 4 mm per texel with the existing
128--768 by 64--384 component bounds. Micro grain below that footprint changes
aggregate reflection only; the proof makes no literal grain-size claim.

### Geometry semantics

`IGGY_IronTangentUV_v001` remains metre-scaled and owns tangent direction.
`IGGY_IronFieldUV_v001` remains the nonrepeating zero-to-one per-object image
domain. Leaves use object-local X/Z; cylindrical parts use their component
tangent host. Geometry, modifiers, object transforms, and source files remain
unchanged.

### Nodes and shader flow

Each row uses exactly two Principled BSDF nodes, one Mix Shader, one UV Map,
four Image Texture nodes, one Normal Map, one Tangent, and one explicit
exposure Value. The oxide lobe is dielectric (`Metallic = 0`, `IOR = 2.10`, no
coat). The hidden iron lobe remains conductive. The tangent drives both lobes;
the oxide anisotropy is zero except for the compressed candidate's bounded
directional response. The mix factor is exactly zero.

`component frame -> authored metre fields -> independent colour/roughness/
height response -> one combined tangent normal -> complete oxide BSDF ->
zero-exposure physical layer mix -> output`

### Test code

`tests/unit/forged_iron_v1_intact_oxide_sweep_tests.py` requires the four
candidate IDs, all six causal owners, zero conductor exposure, the two-BSDF
topology, reduced Eevee proof settings, no saved blend, prohibited condition
vocabulary absence, per-row cleanup, four proofs per row, and an exploratory
manifest with unchanged source hashes.

### Production code

`compare_intact_oxide_layers_v1.py` shall import the accepted cumulative proof
helpers and implement these exact pure field operations:

```python
def open_mode_field(u_m, v_m, modes, phase_offset):
    value = np.zeros_like(u_m, dtype=np.float64)
    weight = 0.0
    for wavelength_m, angle_degrees, amplitude, phase in modes:
        angle = math.radians(angle_degrees)
        projected_m = u_m * math.cos(angle) + v_m * math.sin(angle)
        value += amplitude * np.sin(
            math.tau * projected_m / wavelength_m + phase + phase_offset
        )
        weight += abs(amplitude)
    return (value / weight).astype(np.float32)

thermal_macro_response = smoothstep(0.0, 1.0, thickness_response)
compression_flow_response = open_mode_field(u_m, v_m, compression_modes, phase)
medium_morphology_response = open_mode_field(u_m, v_m, morphology_modes, phase)
micro_grain_response = open_mode_field(u_m, v_m, grain_modes, phase)
detail_priority = 0.25 + 0.75 * smoothstep(
    -0.20, 0.65, open_mode_field(u_m, v_m, rest_modes, phase)
)
combined_surface_height_m = (
    broad_forging_height_m
    + medium_morphology_response
    * detail_priority
    * recipe.surface_relief_amplitude_m
)
roughness = clip(
    recipe.roughness_base
    + (thermal_macro_response - 0.5) * recipe.thermal_roughness_amplitude
    + compression_flow_response * recipe.compression_roughness_amplitude
    + medium_morphology_response * detail_priority
      * recipe.morphology_roughness_amplitude
    + micro_grain_response * detail_priority
      * recipe.grain_roughness_amplitude,
    recipe.roughness_minimum,
    recipe.roughness_maximum,
)
```

The complete candidate records in the production file own thickness envelope,
palette, mode banks, relief, roughness amplitudes, and oxide anisotropy. Mode
banks are sums of open incommensurate waves; they may not create finite closed
plates or stamped motifs. A stable SHA-256-derived component phase prevents
unrelated parts from sharing a landmark.

After each row the script clears target material slots, removes only the
tracked exploratory materials and images, and verifies their datablocks are no
longer present. It never calls `save_as_mainfile` or an orphan-wide purge.
The renderer first requests `BLENDER_EEVEE_NEXT` and falls back narrowly to
`BLENDER_EEVEE` when the installed Blender enum reports that versioned name;
the manifest records the selected enum rather than claiming one blindly.

### Build and validation

```bash
python3 -m unittest tests.unit.forged_iron_v1_intact_oxide_sweep_tests
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/forged_iron_v1/compare_intact_oxide_layers_v1.py
python3 -m unittest tests.unit.forged_iron_v1_intact_oxide_sweep_tests
```

The first run must fail because the renderer is absent. The post-build run must
pass the static and generated-manifest contracts. No broad test loop is
authorized.

### Proof

`output/intact_oxide_layer_sweep_v1/comparison_board.png` is the only selection
plate. The script also retains the sixteen source tiles and `manifest.json`.
The board is a direct lossless row/column stack of those tiles, with order
recorded in the manifest; its assembler may not rerender or recolour them.
The board must be critiqued for metal identity, broad/medium/micro hierarchy,
quiet regions, directional manufacturing read, angle response, gameplay read,
symbols, equal-frequency noise, and obvious procedural repetition. Only a
ranked recommendation and rejected reasons survive selection.

## DEM-PROOF-006: Adversarial actual-hinge board for the researched root

### Demand

Reopen the frozen `output/forged_iron_v1.blend` and judge only its canonical
`IGGY_MAT_ReferenceForgedIron_v003` material on the approved eight-part hinge.
This demand is a proof correction, not a material integration. It shall add the
missing three-quarter, close moving-light, gameplay, pivot, roughness, and
normal evidence without changing a material value, pattern recipe, texture,
mesh, source manifest, or `.blend`.

The board remains `USER_NOT_ACCEPTED_PROOF_ONLY`. It may expose a failure and
request a bounded repair; it may not promote the root because all files exist.
The archived cumulative hinge, oxide sweeps, quiet repair, diamond sweep, and
cat-face sweep are ineligible inputs. Rust, damage, contact polish, soot, dirt,
blood, decorative stamps, and AI-generated imagery remain absent.

### Frozen consumer and panel inventory

```python
SOURCE_BLEND = SCRIPT_ROOT / "output" / "forged_iron_v1.blend"
SOURCE_MANIFEST = SCRIPT_ROOT / "output" / "forged_iron_v1_manifest.json"
MATERIAL_NAME = "IGGY_MAT_ReferenceForgedIron_v003"
MATERIAL_GROUP_NAME = "IGGY_SH_ReferenceForgedIron_v003"
PROOF_NODE_NAME = "IGGY_IRON_PROOF_MODE"

TARGET_NAMES = (
    "SM_GH018_FixedLeaf",
    "SM_GH018_MovingLeaf_Openwork",
    "SM_GH018_Pintle",
    "MovingKnuckle_01",
    "FixedKnuckle_02",
    "MovingKnuckle_03",
    "FixedKnuckle_04",
    "MovingKnuckle_05",
)

PANEL_SPECS = (
    ("front", "01 ROOT FRONT", "combined", "neutral_full"),
    ("three_quarter", "02 ROOT THREE QUARTER", "combined", "neutral_oblique"),
    ("grazing", "03 ROOT GRAZING", "combined", "left_strip_oblique"),
    ("gameplay", "04 GAMEPLAY DISTANCE", "combined", "neutral_wide"),
    ("close_neutral", "05 CLOSE NEUTRAL", "combined", "neutral_openwork_crop"),
    ("close_light_left", "06 CLOSE LIGHT LEFT", "combined", "left_strip_same_crop"),
    ("close_light_right", "07 CLOSE LIGHT RIGHT", "combined", "right_strip_same_crop"),
    ("pivot_close", "08 PIVOT AND TANGENT", "combined", "neutral_pivot_crop"),
    ("base_colour", "09 UNLIT BASE COLOUR", "base_colour", "full_front"),
    ("roughness", "10 ROUGHNESS", "roughness", "full_front"),
    ("normal", "11 COMBINED NORMAL", "normal", "full_front"),
    ("worked_response", "12 WORKED RESPONSE", "worked_response", "full_front"),
)
```

All tiles are `900 x 360`. The board is a lossless presentation render of four
columns by three rows at `3600 x 1080`; its orthographic camera uses the exact
`16.0`-unit horizontal span implied by four `4.0`-unit panel cells. Combined panels share the same objects,
material, world, exposure, and three named target lights. Only camera framing,
view direction, and the explicitly named moving strip position may change.
Data panels change only `IGGY_IRON_PROOF_MODE` and use the material's existing
emissive proof route.

### Exact proof execution

The complete production implementation is
`render_actual_hinge_acceptance_board_v1.py`. Its route is frozen as:

```python
source_hash_before = sha256_file(SOURCE_BLEND)
scene, material, proof_mode, targets, lights = validate_root()
isolate_actual_hinge(targets, lights)
configure_render(scene, 900, 360)
configure_world(scene)

minimum, maximum = object_bounds(targets)
center = (minimum + maximum) * 0.5
span = maximum - minimum
assert abs(span.x - 4.06) < 0.001
assert abs(span.z - 0.41) < 0.001
full_scale = span.x * 1.12
close_center = Vector((0.72, center.y, center.z))

proof_mode.outputs[0].default_value = 0.0
set_neutral_lights(lights, center)
front_camera(scene.camera, center, full_scale)
render_tile(scene, output_root, "front")

set_neutral_lights(lights, center)
oblique_camera(scene.camera, center, full_scale * 1.03,
               x_offset=-0.26, height=0.92)
render_tile(scene, output_root, "three_quarter")

set_moving_strip(lights, center, side="left")
oblique_camera(scene.camera, center, full_scale * 1.02,
               x_offset=-0.18, height=0.58)
render_tile(scene, output_root, "grazing")

set_neutral_lights(lights, center)
front_camera(scene.camera, center, span.x * 1.62)
render_tile(scene, output_root, "gameplay")

set_neutral_lights(lights, close_center, close=True)
front_camera(scene.camera, close_center, 2.05)
render_tile(scene, output_root, "close_neutral")

for side in ("left", "right"):
    set_moving_strip(lights, close_center, side=side)
    front_camera(scene.camera, close_center, 2.05)
    render_tile(scene, output_root, f"close_light_{side}")

set_neutral_lights(lights, pivot_center, close=True)
oblique_camera(scene.camera, pivot_center, 0.78,
               x_offset=-0.05, height=0.18)
render_tile(scene, output_root, "pivot_close")

front_camera(scene.camera, center, full_scale)
for panel_id, proof_value in (
    ("base_colour", 1.0),
    ("roughness", 2.0),
    ("normal", 5.0),
    ("worked_response", 7.0),
):
    proof_mode.outputs[0].default_value = proof_value
    render_tile(scene, output_root, panel_id)

board = assemble_board(scene, output_root, panels, 3600, 1080)
source_hash_after = sha256_file(SOURCE_BLEND)
assert source_hash_after == source_hash_before
```

`validate_root` fails unless the source manifest status is
`LAYERED_NODE_FORGED_IRON_BUILT`, the public group is exact, all eight meshes
use the one canonical material, and each mesh owns `IGGY_IronUV`.
`assemble_board` removes scene objects only from the unsaved in-memory proof
session, creates emission-only image panels and labels, and renders one board.
The production file contains no `save_as_mainfile` or `save_mainfile` route.

### Manifest and stale-output gate

The output manifest shall record:

```python
assert manifest["status"] == "USER_NOT_ACCEPTED_PROOF_ONLY"
assert manifest["candidate"]["source_unchanged"] is True
assert manifest["candidate"]["source_blend_sha256_before"] == sha256_file(SOURCE_BLEND)
assert manifest["candidate"]["source_blend_sha256_after"] == sha256_file(SOURCE_BLEND)
assert len(manifest["panels"]) == 12
assert manifest["board"]["resolution"] == [3600, 1080]
assert manifest["render_contract"]["no_saved_blend"] is True
assert manifest["render_contract"]["uses_ai_generated_imagery"] is False
assert manifest["review_contract"]["manual_acceptance_required"] is True
assert manifest["review_contract"]["unreal_parity_verified"] is False
```

Every panel and the final board records bytes and SHA-256. Tests reopen each PNG
header, require the declared dimensions, and compare every artifact hash to the
manifest. A stale board, a rerendered tile, or a changed source blend therefore
fails the focused gate.

### Visual rejection gate

After the board is rendered, reject or request repair if any of these reads
first:

1. the worked-response lane forms bars, stamps, lozenges, faces, or decorative
   symbols;
2. broad colour looks like a cloudy overlay detached from oxide history;
3. roughness copies colour or normal rather than changing the highlight for a
   stated cause;
4. the opposed close lights reveal repeated rows or equal-frequency noise;
5. the material reads as gray plastic, painted stone, or brushed aluminium;
6. medium manufacturing structure disappears between close and gameplay
   distance.

The board can establish `repair requested` or `production candidate`. Only the
user or another named manual authority can establish `accepted`.

### Focused commands

```sh
python3 -m unittest tests.unit.forged_iron_v1_acceptance_board_tests
/Applications/Blender.app/Contents/MacOS/Blender \
  --background \
  assets/creative/materials/forged_iron_v1/output/forged_iron_v1.blend \
  --python \
  assets/creative/materials/forged_iron_v1/render_actual_hinge_acceptance_board_v1.py
python3 -m unittest tests.unit.forged_iron_v1_acceptance_board_tests
```

## DEM-EXPLORATION-007: Connected oxide and worked-luster repair gate

### Demand

Repair only the two rejected causal owners exposed by the actual-hinge board:
oxide organization and worked luster. Render one disposable A--D comparison on
the same eight-part hinge. Do not alter geometry, save a Blender file, register
a donor, add condition, or silently replace the researched root.

Rows are fixed:

1. `A_frozen_root_control` -- untouched `IGGY_MAT_ReferenceForgedIron_v003`;
2. `B_connected_oxide_no_luster` -- full connected oxide and zero visible
   worked luster;
3. `C_connected_oxide_continuous_luster` -- B plus one continuous,
   component-aligned luster response;
4. `D_connected_oxide_open_rail_luster` -- B plus sparse open rails whose
   endpoints remain beyond the component field.

Columns are fixed: full front, identical close neutral, identical close
left/right absolute-light difference, gameplay, connected-oxide diagnostic,
independent roughness, and luster diagnostic. The matrix is reduced resolution
and uses the same camera, light, exposure, geometry, and object transforms for
every row.

### Causal ownership

- Geometry owns silhouette, apertures, bevel lands, and pivot construction.
- `IGGY_IronTangentUV_v001` owns component direction in metres.
- `IGGY_IronFieldUV_v001` owns the finite nonrepeating per-component image
  domain.
- Broad forging height owns the only visible normal change. Oxide surface
  height is exactly zero.
- Thermal oxide thickness owns only a restrained dark cool/warm optical shift
  and a narrow roughness shift.
- An aperiodic 8--40 mm response owns medium roughness organization only. It
  cannot change colour, height, metalness, or coverage.
- Aggregate grain owns no literal marks; it changes roughness by at most 0.01.
- Worked luster owns only dielectric anisotropic response. It cannot claim
  colour, normal, height, conductor exposure, scratches, or wear.
- The intact oxide lobe has `Metallic = 0`, `IOR = 2.10`, full coverage, and no
  coat. The hidden iron lobe remains metallic. The explicit physical-layer mix
  is exactly zero for every candidate.

### Exact fields

Each B--D component creates four live images:

```python
oxide_base_linear = srgb_to_linear(mix(thin_srgb, thick_srgb, thermal))
oxide_roughness = clip(
    0.715
    + (thermal - 0.5) * 0.040
    + connected_medium * 0.032
    + aggregate_grain * 0.010,
    0.665,
    0.765,
)
combined_normal = height_to_normal_nonperiodic(broad_forging_height_m)
oxide_surface_height_m = zeros_like(thermal)
oxide_metalness = zeros_like(thermal)
oxide_coverage = ones_like(thermal)
luster_control = stack([amount, rotation, rest], axis=-1)
```

`connected_medium` and `aggregate_grain` come from independent seeded
aperiodic value fields with distinct physical cell sizes. The colour field,
roughness field, and normal field may therefore share material history but may
not share a grayscale map.

B sets luster amount to zero. C uses a continuous amount across the eligible
surface with no finite mask boundary. D computes two or three open curved
rails over an extended `u = -0.25..1.25` domain, varies width continuously,
and clips only at the component boundary. No rail may close, expose a cap, or
repeat the same phase between components.

### Nodes

Each B--D physical material contains exactly two complete Principled BSDFs,
one Mix Shader, one UV Map, four Image Textures, one Normal Map, one Tangent,
one zero-exposure Value, and bounded math nodes that map the luster image's red
channel into oxide anisotropy. Green controls anisotropic rotation. The tangent
drives both complete lobes. The normal map drives both lobes, but contains only
broad forging-plane relief.

The control row uses the frozen root material and its existing proof selector.
Its oxide, roughness, and worked-response panels are explicitly labelled as
root proxies; the renderer may change only the selector value in memory and
must restore combined mode afterward.

### Selection and rejection

The manifest initially records a manual visual review requirement. After the
board is inspected, the production file receives one explicit ranked decision
and is rerun so the decision, complete defect ledger, source hashes, panel
hashes, and board hash are reproducible.

Reject any candidate when the beauty views show a closed silhouette, visible
endpoint, repeated row, cloud island, copied lanes, equal-frequency noise,
decorative symbol, or detail that reads before the broad dark metal group.
Selection advances only one response strategy to a later integration pass; it
does not accept or modify `forged_iron_v1`.

### Focused gate

```sh
python3 -m unittest tests.unit.forged_iron_v1_connected_oxide_luster_tests
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/forged_iron_v1/compare_connected_oxide_luster_v1.py
python3 -m unittest tests.unit.forged_iron_v1_connected_oxide_luster_tests
```

## DEM-PRODUCTION-008: Noncanonical B-derived v004 candidate and Cycles proof

### Demand

Promote only the selected `B_connected_oxide_no_luster` response recipe into a
separate, reopenable Blender candidate. The build may save
`output/forged_iron_connected_oxide_candidate_v004/forged_iron_connected_oxide_candidate_v004.blend`.
It may not overwrite `output/forged_iron_v1.blend`, rename v003, claim user
acceptance, register a donor, add condition, or advance Unreal parity.

The candidate retains the approved eight-object actual hinge, metre component
frames, full-coverage oxide, binary hidden conductor, broad forging-plane
normal, and B's independent roughness. Worked luster is explicitly zero. No
archived oxide, quiet-repair, cat-face, diamond, or cumulative prototype is an
eligible donor.

### Shared response group

Create one live shared group named
`IGGY_SH_ConnectedOxideForgedIron_v004`. Every target material instantiates
that same group and supplies object-specific packed fields through exactly four
live images:

1. linear `OxideBase` optical colour;
2. scalar `OxideRoughness`;
3. OpenGL `BroadForgingNormal` derived only from broad forging height;
4. packed `LusterControl`, with red amount exactly zero, green rotation 0.5,
   and blue rest 1.0.

Each object material owns one nonrepeating field UV node, four image nodes, one
component tangent, one shared-group instance, and one material output. The
shared group owns one normal decoder, two complete Principled lobes, one
physical Mix Shader, one explicit zero-exposure Value, and output sockets for
shader, base colour, roughness, normal colour, metalness, and luster response.

The oxide lobe is dielectric (`Metallic = 0`, `IOR = 2.10`, coat 0). The hidden
iron lobe is conductive and remains physically complete. Both receive the same
broad normal and component tangent. The mix factor is linked only from the
named zero-exposure Value. No displacement node is authorized because this
candidate's oxide height is exactly zero and its broad form already exists as a
normal field.

### Candidate packaging and reopen gate

The build manifest records source hashes before and after, build-script and
selected-recipe-script hashes, every target/material/image, node counts,
field ranges, packed-image state, and locked absences. After saving, reopen the
candidate and prove:

```python
assert source_v003_sha256_before == source_v003_sha256_after
assert all(target_has_one_v004_material for target in target_objects)
assert one_shared_group_is_used_by_all_targets
assert shared_group_principled_count == 2
assert shared_group_mix_shader_count == 1
assert shared_group_normal_map_count == 1
assert all(material_image_texture_count == 4 for material in target_materials)
assert all(luster_amount_maximum == 0.0 for component in components)
assert all(oxide_height_absolute_maximum_m == 0.0 for component in components)
assert all(images_are_packed)
```

### Cycles comparison board

Reopen frozen v003 and the saved v004 candidate separately. Render both under
the same Cycles configuration, scene transforms, cameras, lights, AgX view,
and 720 by 240 tile resolution. Required paired proofs are full front, close
neutral, opposed close-light absolute difference, grazing, pivot close,
gameplay, unlit base colour, independent roughness, and normal.

The final board is a 6 by 3 paired matrix: each proof places v003 immediately
beside v004. Source panels remain separate hash-locked PNGs. The board may add
labels but may not recolour or replace a source panel.

The Cycles review must state whether v004 removes the cloud/stamp signature,
preserves dark forged-iron identity, establishes a better broad/medium/micro
handoff, keeps the plate quiet, and improves the smooth coated pivot. A passing
file contract cannot establish visual acceptance. v004 remains noncanonical
until the user accepts an actual-asset board.

### Focused gate

```sh
python3 -m unittest tests.unit.forged_iron_v004_candidate_tests
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/forged_iron_v1/build_connected_oxide_candidate_v004.py
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/forged_iron_v1/render_connected_oxide_candidate_cycles_v004.py
python3 -m unittest tests.unit.forged_iron_v004_candidate_tests
```

## DEM-EXPLORATION-009: Audited v004.1 optical-response calibration

### Demand

Keep the saved v004 candidate, geometry, component coordinates, shared physical
group, connected thermal response, aperiodic roughness fields, full oxide
coverage, hidden conductor, zero oxide height, and zero worked luster frozen.
Render one disposable four-row Cycles matrix that changes only:

- the two endpoints of the connected oxide palette;
- the baseline and amplitudes of the already-owned independent roughness terms;
- a scalar multiplier on the already-owned broad forging height before normal
  derivation.

No new mask, pattern primitive, coordinate, periodic wave, finite mark, image
source, or saved Blender revision is authorized.

### Candidate records

`A_current_v004_control` is the exact saved v004 response. The other rows use
only anchors already present in `profiles/forged_iron_v1.json` under
`palette.forge_skin_srgb`:

1. `B_audited_dark_half` — `#252a31` to `#41464f`, roughness baseline 0.715,
   thermal amplitude 0.040, connected-medium amplitude 0.040, aggregate-grain
   amplitude 0.010, broad-normal multiplier 1.35.
2. `C_audited_compressed_cool` — `#252a31` to `#353b46`, roughness baseline
   0.680, thermal amplitude 0.035, connected-medium amplitude 0.055,
   aggregate-grain amplitude 0.012, broad-normal multiplier 1.75.
3. `D_audited_cool_to_warm` — `#292e36` to `#574f4b`, roughness baseline
   0.735, thermal amplitude 0.050, connected-medium amplitude 0.045,
   aggregate-grain amplitude 0.010, broad-normal multiplier 1.50.

All B--D anchors must be found verbatim in the audited profile before Blender
renders. No interpolated anchor may be labelled audited.

### Exact field calibration

```python
oxide_base_linear = srgb_to_linear(mix(thin_anchor, thick_anchor, thermal))
oxide_roughness = clip(
    roughness_base
    + (thermal - 0.5) * thermal_amplitude
    + connected_medium * medium_amplitude
    + aggregate_grain * grain_amplitude,
    roughness_minimum,
    roughness_maximum,
)
combined_normal = height_to_normal_nonperiodic(
    broad_forging_height_m * broad_normal_multiplier,
    meters_per_pixel_x=length_m / (resolution_x - 1),
    meters_per_pixel_y=width_m / (resolution_y - 1),
)
```

The colour field may not receive `connected_medium` or `aggregate_grain`.
Roughness may not receive the normal grayscale. The broad-normal multiplier may
not change geometry, height output, silhouette, or oxide height.

### Proof and selection

Use the saved v004 candidate actual hinge, 32 Cycles samples, identical cameras,
lights, exposure, and 640 by 220 tiles. Columns are close neutral, grazing,
pivot close, gameplay, base colour, roughness, and normal. The final board is a
four-row by seven-column labelled matrix. Candidate resources are removed after
each row; neither v003 nor v004 is saved.

Select at most one response. Reject any row that becomes lamp-driven gray,
uniform primer, saturated blue/brown, cloudy, spotty, striped, symbol-capable,
or noisy before the broad dark material group. A selected response remains a
calibration recipe only until integrated and proven on a new candidate file.

### Focused gate

```sh
python3 -m unittest tests.unit.forged_iron_v004_calibration_tests
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/forged_iron_v1/compare_v004_optical_response_calibration.py
python3 -m unittest tests.unit.forged_iron_v004_calibration_tests
```

## DEM-PRODUCTION-010: Integrate selected C as noncanonical v004.1

### Demand

Build exactly one separate candidate from
`C_audited_compressed_cool`. Open saved v004 as the source, reuse its actual
hinge, object transforms, component frames, and
`IGGY_SH_ConnectedOxideForgedIron_v004` group without changing that group's
topology. Replace only the four packed component fields with C-calibrated data,
then save and reopen
`output/forged_iron_connected_oxide_candidate_v004_1/forged_iron_connected_oxide_candidate_v004_1.blend`.

The integrated constants are frozen:

```python
thin_hex = "#252a31"
thick_hex = "#353b46"
roughness_base = 0.680
thermal_amplitude = 0.035
medium_amplitude = 0.055
grain_amplitude = 0.012
roughness_minimum = 0.600
roughness_maximum = 0.760
broad_normal_multiplier = 1.75
```

No new coordinate, mask, pattern, luster, condition, height, shader lobe, or
geometry change is authorized. Oxide coverage remains one; oxide metalness,
oxide height, conductor exposure, and worked-luster amount remain zero.

### Package contract

Every actual-hinge object receives one uniquely named v004.1 material and four
packed images. All eight materials instantiate the unchanged v004 shared group.
The manifest records v004 and v003 hashes, the selected calibration-manifest
hash, all component field ranges, material/image ownership, and the reopen
contract. v004 and v003 must remain hash-identical.

### Paired proof

Render saved v004 immediately beside v004.1 in Cycles under identical close
neutral, grazing, pivot, gameplay, base-colour, independent-roughness, and
normal proofs. Use 720 by 240 source tiles and a labelled four-column paired
board. The proof may not save either source.

Review must decide whether v004.1 restores readable forge-skin colour without
clouds, preserves quiet gameplay grouping, and improves the roughness/normal
handoff. The smooth pivot remains visible and may keep the result at repair
requested; it may not be disguised by a new barrel pattern.

### Focused gate

```sh
python3 -m unittest tests.unit.forged_iron_v004_1_integration_tests
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/forged_iron_v1/build_connected_oxide_candidate_v004_1.py
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/forged_iron_v1/render_connected_oxide_candidate_cycles_v004_1.py
python3 -m unittest tests.unit.forged_iron_v004_1_integration_tests
```

## DEM-PRODUCTION-011: Audit construction-owned response by stock lineage

### Demand

Do not author a new normal pattern. Open the saved v004.1 candidate and audit
the existing broad-normal owner against three real construction identities:

```python
stock_lineages = {
    "moving_leaf_stock": (
        "SM_GH018_MovingLeaf_Openwork",
        "MovingKnuckle_01",
        "MovingKnuckle_03",
        "MovingKnuckle_05",
    ),
    "fixed_leaf_stock": (
        "SM_GH018_FixedLeaf",
        "FixedKnuckle_02",
        "FixedKnuckle_04",
    ),
    "pintle_stock": ("SM_GH018_Pintle",),
}
```

The leaf and its knuckles are one stock lineage: the eye or alternating
projections are formed from the strap while flat, then rolled, trued on a
drift/sizing pin, and optionally welded. A knuckle is therefore not an
independent cylindrical-stock material. The pintle is separate stock forged
toward round and finished by rotation, swage, or bottom fuller. These
construction identities supersede a shape-only leaf-versus-cylinder split.

### Written-evidence contract

Record at least four written sources, including a public-domain forging manual
and one practicing historical-reproduction shop or preservation project. For
each source, store URL, authority, observed operation, eligible transfer, and
prohibited inference. The evidence must establish:

- strap-to-eye continuity;
- split/rolled alternating projections where applicable;
- sizing/drift and weld-seam logic;
- separate pintle stock and round-finishing method;
- final smoothing/overlapping-blow behavior rather than decorative hammer
  stamps.

No video viewing, AI imagery, or invented feature dimensions are authorized.

### Numeric audit

Regenerate the exact selected-C fields without changing or saving the blend.
For all eight components record:

- construction class and stock-lineage owner;
- current tangent and field-coordinate modes;
- field and tangent spans in metres;
- field resolution and metre pitch;
- raw broad-height range and robust percentiles;
- decoded normal-angle percentiles and maximum in degrees;
- texel fractions above 0.25, 0.5, 1.0, and 2.0 degrees;
- dominant and power-centroid wavelengths along both local axes;
- source material, packed image ownership, and frame attributes.

Resample every standardized broad-height field to one audit grid and produce a
complete cross-component Pearson correlation matrix. Explicitly detect that
the same normalized three-rail/sixteen-knot recipe restarts on every component
and that component-name-seeded optical fields break continuity between a leaf
and its rolled knuckles.

### Diagnostic proof

Render four 960 by 320 emission-only actual-hinge panels with identical front
framing:

1. stock lineage: moving stock, fixed stock, and pintle use three flat colours;
2. current field frame: normalized U/V shows every per-object reset;
3. existing broad height: one global metre scale, never per-object normalized;
4. integrated normal angle: one fixed zero-to-two-degree scale.

Assemble them into a labelled 1920 by 640 board. The source blend hash must be
identical before and after. Output
`output/forged_iron_v004_1_component_response_audit_v1/manifest.json` with
status `DIAGNOSTIC_AUDIT_NO_MATERIAL_CHANGE`. The audit may reject an owner but
may not create, integrate, accept, or promote a material.

### Focused gate

```sh
python3 -m unittest tests.unit.forged_iron_v004_1_component_audit_tests
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/forged_iron_v1/audit_v004_1_component_response.py
python3 -m unittest tests.unit.forged_iron_v004_1_component_audit_tests
```

## DEM-PRODUCTION-012: Disposable construction-owned normal-response sweep

### Capability and mutation boundary

Open the saved v004.1 actual hinge independently for each row and render one
reduced four-row comparison. Never save the Blender file. Reuse the exact
v004.1 colour, roughness, full-coverage oxide, zero luster, geometry, cameras,
lights, exposure, tangent UV, field UV, and shared
`IGGY_SH_ConnectedOxideForgedIron_v004` topology. The only candidate data that
may change is the packed tangent normal supplied to
`Broad_Forging_Normal_Only`.

The rows are frozen:

```python
CANDIDATES = (
    "A_v004_1_control",
    "B_parent_rest_stock",
    "C_separate_axial_pintle",
    "D_combined_construction",
)
```

- A uses the saved v004.1 normal unchanged.
- B changes only `moving_leaf_stock` and `fixed_leaf_stock`, including each
  lineage's rolled knuckles. The pintle must remain byte-equivalent to A.
- C changes only `pintle_stock`. Both leaves and all knuckles must remain
  byte-equivalent to A.
- D combines the exact accepted-for-comparison B and C arrays; it may not
  introduce a third operation.

Status is
`EXPLORATORY_CONSTRUCTION_RESPONSE_NOT_CANONICAL`. No `.blend`, map package,
profile change, shader revision, donor record, acceptance, condition, damage,
rust, scratch, dent, contact polish, or AI-generated image is authorized.

### Measured proxy and authored review amplitudes

```python
HUMAN_HAMMER_ENVELOPE_M = (0.041275, 0.031750)
FABRICATION_SCALE = 10.0
GIANT_HAMMER_ENVELOPE_M = (0.412750, 0.317500)
STRAP_REVIEW_HEIGHT_P2P_M = 0.003000
PINTLE_REVIEW_HEIGHT_P2P_M = 0.001000
NORMAL_ANGLE_DISPLAY_MAX_DEG = 2.0
HEIGHT_DISPLAY_ABS_MAX_M = 0.006
```

The first three values are a declared later-period tool-envelope proxy and the
already approved giant fabrication transform. They bound spatial correlation,
not a surviving impression. The two height ranges are authored review
translations chosen below the failed v004.1 integrated height range. They are
not measurements of the Met hinge, a medieval tool, oxide thickness, or
silhouette displacement. They may advance only if the isolated and lit proofs
justify their visible response.

### Parent-rest-stock coordinate contract

```python
STOCK_LINEAGES = {
    "moving_leaf_stock": (
        "SM_GH018_MovingLeaf_Openwork",
        "MovingKnuckle_01", "MovingKnuckle_03", "MovingKnuckle_05",
    ),
    "fixed_leaf_stock": (
        "SM_GH018_FixedLeaf",
        "FixedKnuckle_02", "FixedKnuckle_04",
    ),
    "pintle_stock": ("SM_GH018_Pintle",),
}
```

For a leaf, sample rest `u` from its object-space stock-length extent and rest
`v` from its object-space cross-stock extent. For a knuckle, sample its
circumferential field `u` as the unrolled continuation of the parent leaf at
the hinge edge, and sample rest `v` from its actual axial Z interval rather
than restarting an abstract 0--1 field. Moving-eye `u` continues in the
negative direction from the moving leaf root; fixed-eye `u` continues in the
positive direction from the fixed leaf root. Every knuckle in one lineage
uses the same analytic parent field and its own real axial interval. No
component name enters the parent-field seed.

Record an explicit seam-continuity proof for every knuckle by comparing the
analytic height at the leaf root with the analytic height at the corresponding
unrolled-eye boundary over that knuckle's axial interval. The maximum error
must be at floating-point tolerance. The existing normalized `IGGY_IronFieldUV`
remains the image sampler; the script compiles the shared analytic rest field
into each object's existing image domain and does not add a second shader
coordinate route.

### Overlapping, partially obliterated strap field

`overlapping_obliterated_work_field(rest_u_m, rest_v_m, lineage)` owns the
entire B strap response. It must create no bitmap noise, literal ellipse,
depression stamp, finite symbol, regular row, or per-pixel random value.

1. Derive the number of pass centres from lineage length and `0.55` times the
   giant hammer length. Place centres with an irrational golden-ratio phase
   in `v`, not on a grid. Fixed and moving stock use two declared lineage
   phases.
2. Give each centre a compact C2 radial support whose longitudinal and
   cross-stock radii are `0.72` and `0.70` times the giant proxy envelope.
   Adjacent centres therefore overlap strongly; the support boundary is never
   interpreted as a mark edge.
3. Give each centre one shallow local affine target plane selected from a
   finite signed slope/offset vocabulary. At least one-third of the centres
   are zero-slope rest anchors. The field is

   ```python
   height = sum(weight_i * local_plane_i) / max(sum(weight_i), epsilon)
   ```

   not a sum of dents. Weighted replacement makes later adjacent work erase
   the literal face of earlier work.
4. Blend uncovered support toward zero, remove the field mean, and robustly
   scale the 1st-to-99th-percentile range to
   `STRAP_REVIEW_HEIGHT_P2P_M`. Do not normalize each object. One scale is
   computed per parent lineage, then sampled by its leaf and knuckles.
5. Convert each compiled object field with
   `height_to_normal_nonperiodic` at that object's metre pitch. Knuckle tangent
   U remains circumferential, but its height phase and axial placement remain
   owned by the parent rest stock.

The expected isolated result is irregularly merged planar drift with protected
quiet intervals. Closed hammer silhouettes, a repeated three-lobe gesture,
cat faces, diamonds, and identical moving/fixed lineage arrays are failures.

### Separate progressive-round pintle field

`progressive_round_pintle_field(axial_u_m, circumferential_v_m)` owns the
entire C pintle response. The written manuals establish progressive
square-to-octagonal-to-round working, light finishing blows with rotation, and
swage use. They do not establish periodic rings or visible final facets.

1. Map axial U and circumferential V in metres from the existing `axial_pin`
   frame. Never derive height from axial U alone; that would create rings.
2. Start from an eight-fold circumferential residual as a weak construction
   ancestor. Modulate its amplitude and phase with a finite, nonperiodic axial
   control curve so equal facets do not persist down the complete pin.
3. Add only low-amplitude four-fold and three-fold residuals to break perfect
   symmetry. They remain construction translations, not decorative waves.
4. Taper the residual to zero at the two axial ends, subtract its mean, and
   robustly scale its 1st-to-99th-percentile range to
   `PINTLE_REVIEW_HEIGHT_P2P_M`.
5. Derive the tangent normal at the pintle's axial/circumferential metre pitch.
   No circumference-independent axial height, repeated ring, rivet, seam,
   scratch, or oxide change is permitted.

The expected isolated result is a nearly round pin whose highlight contains a
weak, slowly changing axial memory of progressive forging. Obvious octagonal
striping, barber-pole twist, periodic rings, or a lumpy cast cylinder fails.

### Exact proof matrix

Use 32-sample Cycles, AgX Medium High Contrast, zero exposure, identical source
geometry, and 640 by 220 tiles. Reopen v004.1 before every candidate row and
disable persistent render data. The six columns are:

1. isolated integrated height using one fixed plus/minus 6 mm scale;
2. isolated normal angle using one fixed zero-to-two-degree heatmap;
3. close neutral actual hinge;
4. grazing actual hinge;
5. pivot close actual hinge;
6. gameplay-distance actual hinge.

Assemble a labelled 3840 by 880 board. The manifest must record source hashes
before and after; candidate/object height and angle percentiles; colour,
roughness, luster, and normal array hashes; parent-stock coordinate ranges;
seam-continuity errors; every rendered tile; the board hash; and the complete
visual defect ledger. Preserve only the comparison decision and rejected
reasons. Do not retain candidate images in a `.blend`.

### Rejection and focused gate

Reject any row with visible stamps, repeated lobes, diamonds, faces, stripes,
rings, UV seams, phase resets, equal-frequency noise, changing the wrong stock
lineage, or merely making the whole hinge busier. A technical green gate does
not select the image. The selected strategy, if any, remains an exploratory
normal owner and requires a separate coded integration demand.

```sh
python3 -m unittest tests.unit.forged_iron_v004_1_construction_response_tests
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/forged_iron_v1/compare_v004_1_construction_response_v1.py
python3 -m unittest tests.unit.forged_iron_v004_1_construction_response_tests
```

## DEM-PRODUCTION-013: Disposable clean-metal response-routing sweep

### Question and hard boundary

Test whether the missing clean forged-metal read belongs in geometry plane
quality, broad roughness organization, direction-dependent reflection, or a
combination of the latter two. Open the saved v004.1 actual hinge independently
for every row. Never save a Blender file.

Freeze these source owners in every row:

- v004.1 base colour and full-coverage dielectric oxide;
- v004.1 broad normal, including its known failure, so this board changes no
  normal variable while judging response lanes;
- zero exposed conductor, oxide height, condition, damage, and contact polish;
- actual hinge geometry, field UV, tangent UV, cameras, lights, exposure, and
  colour management;
- source `IGGY_SH_ConnectedOxideForgedIron_v004` datablock and its disabled
  anisotropy multiplier.

Rows are exact:

```python
CANDIDATES = (
    "A_v004_1_control",
    "B_construction_roughness_only",
    "C_directional_reflection_only",
    "D_roughness_plus_direction",
)
```

- A is byte-for-byte source response.
- B changes only the sampled roughness image.
- C changes only a disposable copy of the source group by setting the existing
  `Worked_Luster_Disabled` multiplier from `0.0` to `1.0`, then feeds a uniform
  direction-response amount. It changes no graph topology or visible texture
  pattern.
- D is the exact B roughness plus exact C response enable.

Status is `EXPLORATORY_REFLECTION_ROUTING_NOT_CANONICAL`. No integration,
profile mutation, output `.blend`, donor promotion, manual acceptance, or
engine-parity claim is authorized.

### Geometry-first moving-strip proof

Before judging B--D, replace every target material in memory with one neutral
clay shader: base colour `(0.42, 0.42, 0.42)`, metalness `0`, roughness `0.38`,
and no normal input. Render 640 by 220 Cycles tiles for:

1. full hinge, moving strip from the left;
2. full hinge, moving strip from the right;
3. pivot close, moving strip from the left;
4. pivot close, moving strip from the right;
5. construction tangent direction encoded from minus-one-to-one into
   zero-to-one RGB.

Assemble a 3200 by 220 board. Critique leaf plane continuity, bevel lands,
rolled-eye cross-sections, bearing gaps, and pintle roundness. A geometry-owned
highlight defect must be recorded before material selection; no response row
may claim to repair it.

### Open broad-roughness owner

Use the accepted coordinate contract only; import no failed height or normal
field from DEM-PRODUCTION-012.

For `moving_leaf_stock` and `fixed_leaf_stock`, evaluate one continuous
nonperiodic cubic-Hermite curve across the full parent-rest-stock U domain.
The curve uses irregular normalized stations
`(0.00, 0.12, 0.29, 0.51, 0.73, 0.90, 1.00)` and one declared signed value
record per lineage. Add a smaller open cross-stock tilt whose magnitude is
controlled by a second Hermite curve. The exact operation is:

```python
delta_r = 0.040 * (
    0.78 * longitudinal_curve(u_parent)
    + 0.22 * tilt_curve(u_parent) * remap(v_parent, -1, 1)
)
roughness_B = clip(roughness_A + delta_r, 0.58, 0.80)
```

Compile the same analytic lineage field into the leaf and every rolled eye at
its real axial position. Do not rescale its range, frequency, or phase per
object, use component-name seeds, or introduce compact supports, distance
fields, closed masks, texture noise, height, or colour correlation. After
sampling, one scalar DC correction per component may preserve the source
roughness mean; it may not alter the field's shape or contrast.

For `pintle_stock`, use the separate axial/circumferential frame. A direct
axial-only roughness term is prohibited because it would form rings. Use one
first-order circumferential passage with a slowly varying axial phase and a
maximum amplitude of `0.020`:

```python
delta_r_pin = 0.020 * axial_curve(u_pin) * cos(theta + phase_curve(u_pin))
```

This is an open side-to-side response, not repeated bands. Preserve the exact
source roughness mean within `0.005` per component and keep every value within
the frozen `0.58--0.80` review envelope. The isolated roughness proof must use
one absolute zero-to-one scale.

### Uniform direction-dependent reflection owner

The source group already routes `Component Tangent` to both Principled
responses. Its oxide anisotropy input receives
`Luster Control.R * 0.0`; `.G` controls rotation. C/D may only:

1. copy the group in memory;
2. change the existing multiplier's second input from `0.0` to `1.0`;
3. supply a uniform four-by-four control image with
   `R = 0.28`, `G = 0.50`, and `B = 1.0`;
4. retain leaf-longitudinal, eye-circumferential, and pin-axial tangents from
   `IGGY_IronTangentUV_v001`.

`0.28` is an authored selection value, not a measured material constant. The
uniform image may orient and elongate the highlight but may not draw brushing
lines, grooves, scratches, stamps, or a fixed lighting direction. The source
group multiplier must remain `0.0` and its topology hash must remain unchanged
after every row.

### Material proof matrix

Use 32-sample Cycles, AgX Medium High Contrast, zero exposure, identical
640-by-220 tiles, persistent data disabled, and a fresh source reopen for each
row. Columns are:

1. isolated absolute roughness;
2. isolated direction-response amount;
3. close neutral;
4. moving strip from the left;
5. moving strip from the right;
6. pivot close;
7. gameplay distance.

Assemble a labelled 4480 by 880 board. Record per-object source and candidate
image hashes, roughness statistics and mean drift, response amount, group
value change, source-group topology/value immutability, all tile hashes, board
hash, and visual review. A row fails if it creates broad stripes, clouds,
closed motifs, baked lighting, brushed-aluminium lines, plastic highlights,
excessively narrow highlights, tangent discontinuities, gameplay noise, or a
claim that contradicts the clay proof.

Select at most one response recipe. The result is still a disposable response
decision and requires a new coded integration demand.

### Focused gate

```sh
python3 -m unittest tests.unit.forged_iron_v004_1_reflection_routing_tests
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/forged_iron_v1/compare_v004_1_reflection_routing_v1.py
python3 -m unittest tests.unit.forged_iron_v004_1_reflection_routing_tests
```

## DEM-PRODUCTION-014: Integrate selected clean-metal response as v004.2

### Demand

Create exactly one reopenable, noncanonical v004.2 candidate from the saved
v004.1 actual-hinge file. The only visible material change is the response
recipe selected by DEM-PRODUCTION-013:

```python
SOURCE_GROUP = "IGGY_SH_ConnectedOxideForgedIron_v004"
TARGET_GROUP = "IGGY_SH_ConnectedOxideForgedIron_v004_2"
DIRECTIONAL_RESPONSE_AMOUNT = 0.28
DIRECTIONAL_RESPONSE_ROTATION = 0.50
DIRECTIONAL_RESPONSE_REST = 1.00
SOURCE_MULTIPLIER = 0.00
TARGET_MULTIPLIER = 1.00
TANGENT_UV = "IGGY_IronTangentUV_v001"
```

Open v004.1, copy its shared group once, rename only the copy, and change only
`Worked_Luster_Disabled.inputs[1]` from zero to one. The copied group must
retain nine nodes, twenty links, eleven interface sockets, and topology hash
`ce47658c611f97e6b4032596b223a7016fb05bf1daf30646b89c72b04227523b`.
The saved v004.1 group must stay at zero and the saved v004.1 file must remain
byte-identical.

### Textures

For each of the eight actual-hinge components, preserve the packed v004.1
`Continuous_Oxide_Optical_Colour`, `Independent_Oxide_Roughness`, and
`Broad_Forging_Normal_Only` images without changing a pixel, resolution,
colour space, interpolation mode, extension mode, or node link. Replace only
`Explicit_Zero_Worked_Luster_Control` with one packed 4 by 4 float Non-Color
image. Every texel is exactly linear RGB `(0.28, 0.50, 1.00)` with alpha one.
The red channel is oxide anisotropy amount, green is anisotropy rotation, blue
is reserved/rest. It is a uniform control record, not a visible texture,
brushing field, groove field, scratch field, mask, or baked light direction.

### Geometry semantics and shader flow

Preserve all object meshes, transforms, modifiers, material slots, component
frames, and UV layers. `IGGY_IronTangentUV_v001` remains the sole construction
direction owner: leaf U longitudinal, rolled-eye U circumferential, and pin U
axial. The live shader path remains:

`packed component fields -> unchanged v004.1 material image nodes -> copied
v004.2 group -> existing Separate_Luster_Amount_Rotation_Rest -> existing
Worked_Luster_Disabled MULTIPLY now at 1.0 -> Complete_Intact_Oxide_Response
Anisotropic; green -> Anisotropic Rotation; Component Tangent -> Principled
Tangent -> unchanged physical oxide/conductor mix -> Material Output`.

No node, socket, link, BSDF, roughness, normal, height, colour, coverage,
metalness, exposure, geometry, condition, or engine adapter may be added or
changed. Oxide coverage stays one; oxide metalness, oxide height, exposed
conductor, rust, damage, scratches, dents, contact polish, soot, dirt, and
blood stay zero or absent.

### Test code

`tests/unit/forged_iron_v004_2_integration_tests.py` must fail unless:

```python
assert manifest["status"] == "NONCANONICAL_ACTUAL_ASSET_PROOF_CANDIDATE"
assert manifest["source_v004_1_unchanged"] is True
assert manifest["direction_response"] == {
    "amount": 0.28,
    "rotation": 0.50,
    "rest": 1.0,
    "source_multiplier": 0.0,
    "candidate_multiplier": 1.0,
    "visible_texture_pattern": False,
}
assert manifest["shared_group_contract"]["node_count"] == 9
assert manifest["shared_group_contract"]["link_count"] == 20
assert manifest["shared_group_contract"]["interface_socket_count"] == 11
assert manifest["shared_group_contract"]["topology_sha256"] == (
    "ce47658c611f97e6b4032596b223a7016fb05bf1daf30646b89c72b04227523b"
)
assert len(manifest["components"]) == 8
assert all(c["frozen_lane_hashes_match_source"] for c in manifest["components"].values())
assert all(c["luster_control_resolution"] == [4, 4] for c in manifest["components"].values())
assert all(c["luster_amount_range"] == [0.28, 0.28] for c in manifest["components"].values())
assert manifest["separate_process_reopen"]["verified"] is True
assert manifest["manual_acceptance_established"] is False
assert manifest["unreal_parity_verified"] is False
```

The separate-process Blender test also opens the saved candidate and asserts
eight unique v004.2 materials, eight users of the v004.2 group, all required
images packed, all three construction UV layers present, source multiplier
zero, candidate multiplier one, and live 4 by 4 uniform luster controls.

### Production code

`build_connected_oxide_candidate_v004_2.py` owns only this deterministic
derivation. Its complete mutation order is:

1. hash the saved v004.1 blend and manifest;
2. open v004.1 and validate its group, multiplier, materials, image nodes, and
   packed state;
3. record float-pixel hashes for the three frozen images on every component;
4. copy the source group once, set its versioned name and multiplier, and
   prove unchanged topology; retain the now-unconsumed source group with a
   fake user only so the saved-file reopen can compare its zero default to the
   candidate's one default;
5. retarget each existing material's one group instance to the copied group,
   rename the material to the v004.2 prefix, create and pack its uniform 4 by
   4 control image, and replace only the luster image node;
6. re-hash the three frozen images and fail if any changed;
7. save only the v004.2 path, reopen it in-process for an immediate structural
   guard, verify the v004.1 disk hash is unchanged, and write the manifest.

`verify_connected_oxide_candidate_v004_2.py` is then run by a new Blender
process with the candidate as its startup file. It repeats the saved-file
material, group, pixel, packing, UV, and source-hash assertions, writes
`reopen_contract.json`, and marks `separate_process_reopen.verified` true in
the manifest. It never saves a Blender file.

### Paired proof

`render_connected_oxide_candidate_cycles_v004_2.py` independently opens saved
v004.1 and saved v004.2 with persistent render data disabled. Use 32-sample
Cycles, AgX Medium High Contrast, zero exposure, identical 640 by 220 tiles,
geometry, cameras, lights, and colour management. Render these seven columns:

1. close neutral;
2. moving strip from the left;
3. moving strip from the right;
4. pivot close;
5. gameplay distance;
6. live luster-control red channel on one absolute zero-to-one scale;
7. live source normal RGB.

Assemble a 4480 by 440 two-row board with v004.1 above v004.2. Hash every tile,
the board, both source blends, both candidate manifests, and the renderer.
Source-normal component pixel hashes and decoded proof-tile pixels must match;
PNG container hashes remain independently recorded but are not compared
because two Blender writes may encode identical pixels differently. Only the
physical response and direction-amount proof may change.

Reject v004.2 if it creates tangent seams, brushing lines, grooves, narrow
plastic highlights, baked-light read, source-image drift, gameplay noise,
condition cues, or any claim to repair the geometry-owned pivot. Passing this
board may establish v004.2 only as the preferred forged-iron repair candidate;
it cannot establish material acceptance, donor registration, Unreal parity,
or approval of the existing broad normal.

### Focused gate

```sh
python3 -m unittest tests.unit.forged_iron_v004_2_integration_tests
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/forged_iron_v1/build_connected_oxide_candidate_v004_2.py
/Applications/Blender.app/Contents/MacOS/Blender --background \
  assets/creative/materials/forged_iron_v1/output/forged_iron_connected_oxide_candidate_v004_2/forged_iron_connected_oxide_candidate_v004_2.blend \
  --python assets/creative/materials/forged_iron_v1/verify_connected_oxide_candidate_v004_2.py
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/forged_iron_v1/render_connected_oxide_candidate_cycles_v004_2.py
python3 -m unittest tests.unit.forged_iron_v004_2_integration_tests
```

## DEM-PRODUCTION-015: Test geometry normals against the inherited broad normal

### Demand

Resolve one binary causal-ownership question on the saved v004.2 actual hinge:
does the inherited broad normal improve the intact forged-iron read, or does it
melt geometry-owned planes and cylindrical curvature? This is a disposable
comparison, not a new material family, texture pass, candidate integration, or
acceptance event. It has exactly two rows:

```python
ROWS = (
    "A_v004_2_control",
    "B_geometry_normals_only",
)
SOURCE_GROUP = "IGGY_SH_ConnectedOxideForgedIron_v004_2"
NORMAL_NODE = "Broad_Forging_Normal_Decode"
CONTROL_NORMAL_STRENGTH = 1.0
GEOMETRY_NORMAL_STRENGTH = 0.0
```

Open the saved v004.2 blend independently for each row with persistent render
data disabled. Row A uses the saved materials unchanged. For row B, copy the
shared group only in memory and set only
`Broad_Forging_Normal_Decode.inputs["Strength"]` from one to zero. Retarget an
in-memory copy of each component material to that disposable group. Never save
a blend. Hash v004.2 before and after both rows and fail if it changes.

### Frozen lanes and absent inventions

Preserve the packed component base-colour, roughness, source-normal, and
direction-control images byte-for-float. Preserve the v004.2 direction amount,
rotation, construction tangent UVs, oxide/conductor mix, metalness, roughness,
colour, geometry, object transforms, modifiers, exposure, lights, and cameras.
Do not author or generate any replacement normal, height, colour, roughness,
pattern, noise, scratches, dents, pits, oxidation change, exposed-conductor
mask, damage, contact polish, rust, soot, dirt, or baked light direction.

The source normal image remains present in row B but has zero applied influence.
This isolates normal ownership without changing or deleting the packed source.
The inherited geometry-only moving-strip clay proof is hash-registered as the
geometry baseline; it is not regenerated or claimed as new evidence.

### Exact graph contract

The disposable group must retain the saved v004.2 node count, link count,
interface-socket count, and topology hash. Its complete default-value diff from
the saved group must be exactly:

```python
[
    {
        "socket": "Broad_Forging_Normal_Decode:0:Strength",
        "source": 1.0,
        "candidate": 0.0,
    }
]
```

After each row, the saved source group's default must still be one. The
direction-response multiplier must still be one, and its packed red-channel
amount must remain exactly 0.28 on all eight actual-hinge components.

### Test code

`tests/unit/forged_iron_v004_2_geometry_normal_control_tests.py` must fail
unless the executable comparison contract and generated manifest establish:

```python
assert manifest["schema"] == "iggy3d.forged_iron_v004_2_geometry_normal_control.v1"
assert manifest["status"] == "EXPLORATORY_BROAD_NORMAL_REMOVAL_NOT_CANONICAL"
assert list(manifest["rows"]) == list(ROWS)
assert manifest["source_hashes"]["sources_unchanged"] is True
assert manifest["frozen_lane_contract"]["all_component_lane_hashes_match"] is True
assert manifest["group_contract"]["topology_matches_source"] is True
assert manifest["group_contract"]["default_differences"] == [
    {
        "socket": "Broad_Forging_Normal_Decode:0:Strength",
        "source": 1.0,
        "candidate": 0.0,
    }
]
assert manifest["board"]["resolution"] == [4480, 440]
assert manifest["saved_blend_created"] is False
assert manifest["manual_acceptance_established"] is False
assert manifest["unreal_parity_verified"] is False
```

Static tests must also reject Blender save calls, procedural Noise or Voronoi
nodes, any code path that constructs a replacement normal or height image, more
than two rows, and any row-specific edit other than the one Normal Map Strength
default.

### Production code

`compare_v004_2_geometry_normal_control_v1.py` owns the complete disposable
experiment:

1. hash the saved v004.2 blend, manifest, and inherited clay board;
2. open v004.2 fresh for row A, validate the saved group and eight materials,
   render the physical and isolated proofs, and close the row without saving;
3. open v004.2 fresh for row B, copy the group in memory, change exactly one
   Strength default, prove topology equality and the exact default diff, copy
   and retarget each material in memory, and render the same proofs;
4. compare per-component float hashes for base colour, roughness, source normal,
   and direction control; compare source-normal proof pixels; prove the saved
   source group remains at strength one after each row;
5. re-hash the saved v004.2 blend and fail on drift;
6. assemble the paired board and write only PNG proofs plus one JSON manifest.

No `.blend` output, texture package, material catalog promotion, or donor record
is permitted in this step.

### Paired proof and decision gate

Use 32-sample Cycles, AgX Medium High Contrast, zero exposure, identical 640 by
220 tiles, and the saved v004.2 actual hinge. Render seven aligned columns:

1. close neutral;
2. moving strip from the left;
3. moving strip from the right;
4. pivot close;
5. gameplay distance;
6. applied broad-normal amount on an absolute zero-to-one scale;
7. frozen source-normal RGB.

Assemble a 4480 by 440 board with A above B. Hash every tile, decoded proof
pixels, the board, source blend, source manifest, inherited clay board, and
comparison script. Compare the five physical panels numerically, but choose by
visual causality: broad-plane stability, aperture and bevel readability,
moving/fixed leaf continuity, cylindrical pivot response, and gameplay-distance
material identity. Reject row B if it makes the metal sterile, dead, faceted,
or dependent on presentation lighting. Reject row A if the inherited normal
visibly clouds, dents, melts, stripes, or invents broad form unsupported by the
geometry proof.

Select at most one normal-ownership recipe. A row can justify a later v004.3
integration demand, but this disposable proof cannot change the preferred saved
candidate, establish user acceptance, register a donor, or establish Unreal
parity.

### Focused gate

```sh
python3 -m unittest tests.unit.forged_iron_v004_2_geometry_normal_control_tests
/Applications/Blender.app/Contents/MacOS/Blender --background \
  --python assets/creative/materials/forged_iron_v1/compare_v004_2_geometry_normal_control_v1.py
python3 -m unittest tests.unit.forged_iron_v004_2_geometry_normal_control_tests
```
