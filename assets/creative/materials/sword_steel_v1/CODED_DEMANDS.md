# Sword Steel Coded Demands

## Document contract

- Tier: `reusable-family`
- Consumer: WPN-001 analytic diagnostic blade; actual production mesh absent.
- Capability: intact clean multi-band sword polish.
- Changed production targets: profile, builder, focused tests, canonical output.
- Excluded: colour texture, hamon, hada, pattern weld, engraving, coating,
  stylization, oxidation, dirt, scratches from use, impact damage, and engine
  parity claims.

## DEM-SWORD-POLISH-001: Authored macro polish and finite medium grind

### Demand

Generate two independent 16-bit scalar fields in blade-local U/V. The macro
field is a 512x128 interpolation of an authored 5x8 control lattice and affects
roughness only. The medium field remains the deterministic 1024x256 finite
47-track grammar and affects roughness plus micrometre bump. Neither field may
affect base colour, metalness, silhouette, AO, or blade identity.

### Authority

The professional sword workflow supports a quiet plain conductor, linear
brushing, multiple irregularity bands, straight blade UVs, and separate
material response. The exact lattice, track count, amplitudes, and regional
gains are authored translations for this diagnostic fixture.

### Textures

```json
{
  "sword_steel_v1_macro_polish.png": {
    "meaning": "broad polish organization",
    "resolution": [512, 128],
    "span_m": [0.889, 0.0318],
    "bit_depth": 16,
    "color_space": "Non-Color",
    "consumer": "Macro_Polish_Field",
    "lanes": ["roughness"]
  },
  "sword_steel_v1_grind_field.png": {
    "meaning": "finite resolvable longitudinal abrasive passes",
    "resolution": [1024, 256],
    "span_m": [0.889, 0.0318],
    "bit_depth": 16,
    "color_space": "Non-Color",
    "consumer": "Authored_Longitudinal_Grind_Field",
    "lanes": ["roughness", "normal"]
  }
}
```

### Geometry semantics

```text
sinc_sword_region: FLOAT_COLOR CORNER; R body, G fuller, B bevel, A edge
IGGY_BladeUV: CORNER UV; U shoulder-to-tip, V across blade width
macro and grind maps: EXTEND, Linear, Non-Color
condition attributes: absent
```

The complete changed profile record is:

```json
{
  "macro_polish": {
    "map_resolution": [512, 128],
    "map_span_m": [0.889, 0.0318],
    "map_bit_depth": 16,
    "control_grid_shape": [5, 8],
    "u_knots": [0.0, 0.12, 0.27, 0.44, 0.61, 0.78, 0.9, 1.0],
    "v_knots": [0.0, 0.22, 0.5, 0.78, 1.0],
    "control_values": [
      [0.43, 0.50, 0.37, 0.58, 0.47, 0.65, 0.52, 0.46],
      [0.55, 0.60, 0.45, 0.64, 0.51, 0.70, 0.56, 0.49],
      [0.48, 0.53, 0.41, 0.59, 0.46, 0.62, 0.50, 0.44],
      [0.40, 0.47, 0.35, 0.54, 0.42, 0.58, 0.46, 0.40],
      [0.46, 0.51, 0.39, 0.56, 0.45, 0.61, 0.49, 0.43]
    ],
    "roughness_amplitude": 0.028
  },
  "medium_grind": {
    "map_resolution": [1024, 256],
    "map_span_m": [0.889, 0.0318],
    "map_bit_depth": 16,
    "track_count": 47,
    "roughness_amplitude": 0.018,
    "bump_distance_m": 0.000004,
    "bump_strength": 0.22
  },
  "micro_response": {
    "owner": "principled_anisotropy",
    "coordinate": "IGGY_BladeUV tangent",
    "texture_map": null
  },
  "regions": {
    "body": {"roughness": 0.38, "anisotropy": 0.18, "macro_strength": 0.80, "grind_strength": 1.00},
    "fuller": {"roughness": 0.34, "anisotropy": 0.20, "macro_strength": 0.95, "grind_strength": 0.82},
    "bevel": {"roughness": 0.24, "anisotropy": 0.35, "macro_strength": 0.52, "grind_strength": 0.46},
    "edge": {"roughness": 0.16, "anisotropy": 0.42, "macro_strength": 0.18, "grind_strength": 0.12}
  }
}
```

### Test code

Target: `tests/unit/sword_steel_v1_tests.py`.

```python
def test_profile_declares_three_distinct_finish_bands(self):
    profile = json.loads(PROFILE_PATH.read_text())
    finish = profile["finish"]
    self.assertEqual(finish["macro_polish"]["map_resolution"], [512, 128])
    self.assertEqual(finish["macro_polish"]["control_grid_shape"], [5, 8])
    self.assertEqual(finish["medium_grind"]["map_resolution"], [1024, 256])
    self.assertEqual(finish["medium_grind"]["track_count"], 47)
    self.assertEqual(finish["micro_response"]["owner"], "principled_anisotropy")
    self.assertIsNone(finish["micro_response"]["texture_map"])
    for values in finish["regions"].values():
        self.assertIn("macro_strength", values)
        self.assertIn("grind_strength", values)
```

### Production code

Target: `assets/creative/materials/sword_steel_v1/build_sword_steel_v1.py`.

```python
def generate_macro_polish_field(profile: dict[str, Any]) -> np.ndarray:
    contract = profile["finish"]["macro_polish"]
    resolution_x, resolution_y = contract["map_resolution"]
    u_knots = np.asarray(contract["u_knots"], dtype=np.float32)
    v_knots = np.asarray(contract["v_knots"], dtype=np.float32)
    controls = np.asarray(contract["control_values"], dtype=np.float32)
    if controls.shape != tuple(contract["control_grid_shape"]):
        raise ValueError("macro polish control lattice has the wrong shape")
    if controls.shape != (len(v_knots), len(u_knots)):
        raise ValueError("macro polish knots do not match the control lattice")
    if np.any(np.diff(u_knots) <= 0.0) or np.any(np.diff(v_knots) <= 0.0):
        raise ValueError("macro polish knots must increase strictly")
    u = np.linspace(0.0, 1.0, resolution_x, dtype=np.float32)
    v = np.linspace(0.0, 1.0, resolution_y, dtype=np.float32)
    rows = np.stack([np.interp(u, u_knots, row) for row in controls])
    field = np.empty((resolution_y, resolution_x), dtype=np.float32)
    for column in range(resolution_x):
        field[:, column] = np.interp(v, v_knots, rows[:, column])
    field = np.clip(field, 0.0, 1.0).astype(np.float32)
    if not np.isfinite(field).all():
        raise RuntimeError("macro polish compiler produced non-finite values")
    return field


def generate_grind_field(profile: dict[str, Any]) -> np.ndarray:
    contract = profile["finish"]["medium_grind"]
    resolution_x, resolution_y = contract["map_resolution"]
    track_count = contract["track_count"]
    if (resolution_x, resolution_y, track_count) != (1024, 256, 47):
        raise ValueError("sword grind field contract changed unexpectedly")
    u = np.linspace(0.0, 1.0, resolution_x, dtype=np.float32)
    v = np.linspace(-1.0, 1.0, resolution_y, dtype=np.float32)
    u_field, v_field = np.meshgrid(u, v)
    signed = np.zeros_like(u_field)
    for track_index in range(track_count):
        nominal = -0.97 + (track_index + 0.5) * 1.94 / track_count
        center = nominal + 0.014 * math.sin(track_index * 2.399963)
        width = 0.0016 + 0.0038 * (
            0.5 + 0.5 * math.sin(track_index * 1.618034)
        )
        pressure = math.sin(track_index * 2.173 + 0.4)
        if track_index % 7 == 0 or track_index % 11 == 0:
            pressure *= 0.12
        wander = 0.0026 * np.sin(
            math.tau * u_field * (1.0 + track_index % 3)
            + track_index * 0.73
        )
        envelope = np.interp(
            u,
            [0.0, 0.22, 0.51, 0.78, 1.0],
            [0.72, 1.0, 0.82 + 0.15 * math.sin(track_index), 0.94, 0.66],
        )[np.newaxis, :]
        signed += pressure * envelope * np.exp(
            -0.5 * ((v_field - center - wander) / width) ** 2
        )
    signed /= max(float(np.max(np.abs(signed))), 1.0e-6)
    grind = np.clip(0.5 + signed * 0.5, 0.0, 1.0).astype(np.float32)
    if grind.shape != (resolution_y, resolution_x):
        raise RuntimeError("grind compiler produced the wrong shape")
    if not np.isfinite(grind).all() or grind.min() < 0.0 or grind.max() > 1.0:
        raise RuntimeError("grind compiler produced illegal values")
    return grind
```

### Build and validation

```sh
python3 -m unittest tests.unit.sword_steel_v1_tests
/Applications/Blender.app/Contents/MacOS/Blender --background --python assets/creative/materials/sword_steel_v1/build_sword_steel_v1.py
```

### Proof

Isolate `macro` and `finish` renders at the same close framing. Reject blobs,
faces, repeated emblems, hard bands, one-pixel chains, and a medium field that
remains visible at gameplay distance.

## DEM-SWORD-POLISH-002: Region-weighted physical assembly and reopen proof

### Demand

Use one Principled conductor, two packed image textures, one UV map, one
tangent, and one bump. Region baselines own roughness and anisotropy. Region
gains independently weight macro roughness and medium roughness/bump. The
sub-texel band is the anisotropic microfacet response and has no texture.

### Authority

The clean conductor F0, body roughness, and tangent anisotropy are exact
accepted donors. Other finish amplitudes and region gains are authored
translations and remain adjustable profile values.

### Nodes

```text
UVMap(IGGY_BladeUV) -> Macro_Polish_Field.Vector
UVMap(IGGY_BladeUV) -> Authored_Longitudinal_Grind_Field.Vector
VertexColor(sinc_sword_region) -> regional roughness, anisotropy, macro gain, grind gain
(Macro-0.5) * macro amplitude * regional macro gain -> roughness sum
(Grind-0.5) * medium amplitude * regional grind gain -> roughness sum
regional roughness + macro contribution + medium contribution -> Principled.Roughness
regional grind gain * bump strength -> Bump.Strength
Grind -> Bump.Height -> Principled.Normal
Tangent(IGGY_BladeUV) -> Principled.Tangent
regional anisotropy -> Principled.Anisotropic
Principled -> MaterialOutput
```

### Shader flow

```text
blade UV + corner region attribute
-> exact conductor donor
-> region finish baselines
-> signed macro roughness modulation
-> signed medium roughness modulation and micrometre bump
-> tangent anisotropic microresponse
-> one physical material output
```

### Test code

Target: `tests/unit/sword_steel_v1_tests.py`.

```python
def test_manifest_records_multi_band_shader_contract(self):
    manifest = self.manifest
    self.assertEqual(manifest["material"]["principled_count"], 1)
    self.assertEqual(manifest["material"]["mix_shader_count"], 0)
    self.assertEqual(manifest["material"]["image_texture_count"], 2)
    self.assertEqual(manifest["material"]["bump_count"], 1)
    self.assertEqual(manifest["finish"]["micro_response"]["texture_map"], None)
    self.assertEqual(
        manifest["finish"]["micro_response"]["owner"],
        "principled_anisotropy",
    )
    self.assertTrue(manifest["reopen_validated"])

def test_multi_band_maps_and_proofs_exist(self):
    for key in ("macro_polish_field", "grind_field", "saved_blend", "comparison_board"):
        path = Path(self.manifest["outputs"][key]["path"])
        self.assertTrue(path.is_file(), key)
        self.assertGreater(path.stat().st_size, 1024)
    for view in ("front", "grazing", "gameplay", "clay", "regions", "macro", "finish"):
        path = Path(self.manifest["outputs"]["renders"][view]["path"])
        self.assertTrue(path.is_file(), view)
        self.assertGreater(path.stat().st_size, 1024)
```

### Production code

Target: `assets/creative/materials/sword_steel_v1/build_sword_steel_v1.py`.

```python
def _center_scale_mask_field(
    tree: bpy.types.NodeTree,
    field_socket: bpy.types.NodeSocket,
    mask_socket: bpy.types.NodeSocket,
    amplitude: float,
    name: str,
    x: float,
    y: float,
) -> bpy.types.NodeSocket:
    centered = tree.nodes.new("ShaderNodeMath")
    centered.name = f"Center_{name}"
    centered.operation = "SUBTRACT"
    centered.inputs[1].default_value = 0.5
    centered.location = (x, y)
    tree.links.new(field_socket, centered.inputs[0])
    scaled = tree.nodes.new("ShaderNodeMath")
    scaled.name = f"{name}_Amplitude"
    scaled.operation = "MULTIPLY"
    scaled.inputs[1].default_value = amplitude
    scaled.location = (x + 180.0, y)
    tree.links.new(centered.outputs[0], scaled.inputs[0])
    masked = tree.nodes.new("ShaderNodeMath")
    masked.name = f"{name}_Regional_Gain"
    masked.operation = "MULTIPLY"
    masked.location = (x + 360.0, y)
    tree.links.new(scaled.outputs[0], masked.inputs[0])
    tree.links.new(mask_socket, masked.inputs[1])
    return masked.outputs[0]
```

The material builder calls `_weighted_sum` four times for roughness,
anisotropy, `macro_strength`, and `grind_strength`; calls
`_center_scale_mask_field` for both images; adds both signed contributions to
the regional roughness; links `grind_strength * bump_strength` to one Bump
Strength socket; and validates this exact topology:

```python
expected = {
    "principled_count": 1,
    "mix_shader_count": 0,
    "image_texture_count": 2,
    "bump_count": 1,
    "tangent_count": 1,
}
if node_counts != expected:
    raise RuntimeError(f"Sword-steel topology drifted: {node_counts}")
```

Saved-file validation requires two named packed images and writes their names
to the manifest:

```python
image_nodes = [
    node for node in tree.nodes if node.bl_idname == "ShaderNodeTexImage"
]
if len(image_nodes) != 2 or any(node.image is None for node in image_nodes):
    raise RuntimeError("saved sword steel does not have two live finish maps")
if any(node.image.packed_file is None for node in image_nodes):
    raise RuntimeError("saved sword finish maps are not packed")
manifest["reopen_validation"] = {
    "blend_sha256": sha256_file(blend_path),
    "object": obj.name,
    "material": material.name,
    "packed_images": sorted(node.image.name for node in image_nodes),
}
```

### Build and validation

```sh
/Applications/Blender.app/Contents/MacOS/Blender --background --python assets/creative/materials/sword_steel_v1/build_sword_steel_v1.py -- --validate-only
python3 -m unittest tests.unit.sword_steel_v1_tests
python3 -m py_compile assets/creative/materials/sword_steel_v1/build_sword_steel_v1.py
```

### Proof

The complete build emits neutral, grazing, gameplay, clay, region, macro, and
medium proofs. The canonical comparison board shows the physical views plus
both finish bands. Acceptance remains diagnostic because the fixture is not
the factory-generated arming sword and engine parity is unproved.

## DEM-SWORD-ACTUAL-003: Factory arming-sword semantic adapter and matched proof

### Demand

Install the already accepted clean-steel graph on the exact measured
`arming_sword_v1` blade generated on BATMAN. Preserve its 270 vertices and 268
all-quad polygons. Add only one corner UV and one corner colour semantic. The
actual blade has a lenticular body, a straight secondary bevel over the outer
22 percent of each half-width, a 0.4 mm edge land, and no fuller. Render the
old palette blade and the clean-steel blade under the same front camera and
lights, then emit three-quarter, grazing, gameplay, clay, region, macro, and
medium proofs. Save, pack, reopen, and validate in a separate Blender process.

### Authority

- Assembly source:
  `linux-worker:/home/kogaRyu/blender-refs/tools/assemble_arming_sword.py`,
  SHA-256 `767515f2ba2599fcb077ab12c8f3c8ac1d9453abef559347c0cc0777edc53392`.
- Blade source: `generators/sg/parts/blade.py`, SHA-256
  `e2fa5851ba20d41064eafb964a03d2f716a68d4307bf54a79a8e00b0bdb46322`.
- Finish source: `tools/finish_arming_sword.py`, SHA-256
  `b9de19e5194ce36131cdab690b3f76a2bd005d53198f5279cd604195a4dd7000`.
- Blender authority: 5.1.1 on BATMAN.
- Evaluated dimensions: X 0.048 m, Y 0.006 m, Z 0.780 m.
- Generator values: shoulder/pre-tip width 0.048/0.018 m;
  shoulder/near-tip thickness 0.006/0.002 m; width exponent 1.15; distal
  exponent 1.6; secondary bevel fraction 0.22; edge land 0.0004 m.
- Face audit: source zone counts are plausible but not spatially correct. One
  14-face center strip is mislabeled `blade_edge`, while the matching negative
  edge strip is not. The adapter therefore records 28 side-face disagreements
  and derives the region from actual cross-section position. This is a source
  correction at the semantic boundary, not a geometry edit.
- Abrasive track population and response amplitudes remain authored transfers
  from the accepted diagnostic material. No historical finishing sequence is
  claimed.

### Textures

| File | Meaning | Resolution | Physical span | Colour space | Live lane |
| --- | --- | ---: | ---: | --- | --- |
| `arming_sword_v1_macro_polish.png` | broad polish organisation | 512x128, 16-bit | 0.780x0.048 m | Non-Color | signed roughness modulation only |
| `arming_sword_v1_grind_field.png` | 47 finite longitudinal abrasive tracks | 1024x256, 16-bit | 0.780x0.048 m | Non-Color | signed roughness plus 4 micrometre bump |

Both maps use `EXTEND`, linear filtering, and `IGGY_BladeUV`. Neither enters
base colour or metalness. The macro field is not copied into the bump lane.

### Geometry semantics

```text
source object: SW_blade.001, selected by the factory's blade_ob reference and
               independently checked as the largest SW_blade* mesh
source axes: X width, Y thickness, +Z shoulder to tip
IGGY_BladeUV: CORNER UV
  U = clamp((vertex.z - z_min) / 0.780, 0, 1)
  half_width(U) = 0.5 * lerp(0.048, 0.018, U^1.15)
  V = 0.5 + vertex.x / (2 * half_width(U))
  V is mirrored when polygon.normal.y < -0.1 so both blade faces retain a
  deliberate hilt-to-tip tangent without sampling the same handed field
sinc_sword_region: FLOAT_COLOR CORNER
  R body, G fuller, B secondary bevel, A cutting edge
  fuller = exact zero for all 268 polygons
  shoulder cap = body; tip cap = edge
  side edge when abs(face_center.x) / half_width(U) >= 0.95
  side bevel when ratio >= 0.78 and < 0.95
  all remaining side faces = body
expected polygon counts: body 176, fuller 0, bevel 56, edge 36
```

### Nodes and shader flow

No shader node is added or forked for this integration. The builder imports
`build_sword_steel_v1.py`, changes only its versioned material name, then calls
`generate_macro_polish_field`, `generate_grind_field`, and
`build_sword_steel_material`. The resulting graph remains one
`ShaderNodeBsdfPrincipled`, two `ShaderNodeTexImage`, one `ShaderNodeUVMap`,
one `ShaderNodeVertexColor`, one `ShaderNodeTangent`, and one
`ShaderNodeBump`, with no Mix Shader and no Noise, Voronoi, or Wave node.

```text
IGGY_BladeUV -> both Non-Color finish images
sinc_sword_region -> exact region roughness, anisotropy, macro gain, grind gain
constant clean conductor -> Base Color; Metallic = 1
region roughness + signed macro + signed medium -> clamped Roughness
medium field * region grind gain -> 4 micrometre Bump -> Normal
IGGY_BladeUV Tangent + region anisotropy -> Principled tangent response
Principled -> Material Output
```

### Test code

Target: `tests/unit/arming_sword_v1_clean_steel_tests.py`.

The complete executable file is the red/green authority. It requires the
actual source identifiers and dimensions, `has_fuller == false`, region counts
`176/0/56/36`, import and use of the canonical shader builder, the absence of
analytic fixture construction and generic procedural texture nodes, the
portable output manifest, two packed image consumers, all seven actual-asset
proof types, and a successful separate-process reopen. The pre-implementation
gate is recorded as two `FileNotFoundError` failures for the absent profile and
builder, with both output tests skipped.

### Profile code

Target: `profiles/arming_sword_v1_clean_steel.json`.

```json
{
  "schema": "iggy3d.arming-sword-clean-steel-profile.v1",
  "material_id": "sword_steel_v1",
  "integration_id": "arming_sword_v1_clean_steel",
  "capability": "pristine_bright_ground_steel_on_factory_arming_sword",
  "consumer": {
    "asset_id": "arming_sword_v1",
    "actual_production_mesh_present": true,
    "source_host": "linux-worker",
    "source_object": "SW_blade.001",
    "source_factory_relative_path": "tools/assemble_arming_sword.py",
    "source_finish_relative_path": "tools/finish_arming_sword.py",
    "source_blade_relative_path": "generators/sg/parts/blade.py",
    "source_factory_sha256": "767515f2ba2599fcb077ab12c8f3c8ac1d9453abef559347c0cc0777edc53392",
    "source_finish_sha256": "b9de19e5194ce36131cdab690b3f76a2bd005d53198f5279cd604195a4dd7000",
    "source_blade_sha256": "e2fa5851ba20d41064eafb964a03d2f716a68d4307bf54a79a8e00b0bdb46322",
    "blade_length_m": 0.78,
    "shoulder_width_m": 0.048,
    "pre_tip_width_m": 0.018,
    "shoulder_thickness_m": 0.006,
    "near_tip_thickness_m": 0.002,
    "expected_vertex_count": 270,
    "expected_polygon_count": 268,
    "expected_quad_count": 268
  },
  "geometry": {
    "cross_section": "lenticular_with_secondary_bevel",
    "blade_axis": "+Z",
    "width_axis": "X",
    "thickness_axis": "Y",
    "profile_width_exponent": 1.15,
    "distal_taper_exponent": 1.6,
    "secondary_bevel_half_width_fraction": 0.22,
    "edge_land_m": 0.0004,
    "has_fuller": false,
    "source_zone_role": "audit_only_not_semantic_owner",
    "expected_source_zone_mismatch_polygon_count": 28,
    "expected_region_polygon_counts": {"body": 176, "fuller": 0, "bevel": 56, "edge": 36},
    "region_attribute": "sinc_sword_region",
    "blade_uv": "IGGY_BladeUV"
  },
  "donors": {
    "expected_f0_linear": [0.56, 0.57, 0.58],
    "body_roughness": 0.38,
    "body_anisotropy": 0.18
  },
  "finish": {
    "macro_polish": {
      "map_resolution": [512, 128],
      "map_span_m": [0.78, 0.048],
      "map_bit_depth": 16,
      "control_grid_shape": [5, 8],
      "u_knots": [0.0, 0.12, 0.27, 0.44, 0.61, 0.78, 0.9, 1.0],
      "v_knots": [0.0, 0.22, 0.5, 0.78, 1.0],
      "control_values": [[0.43, 0.5, 0.37, 0.58, 0.47, 0.65, 0.52, 0.46], [0.55, 0.6, 0.45, 0.64, 0.51, 0.7, 0.56, 0.49], [0.48, 0.53, 0.41, 0.59, 0.46, 0.62, 0.5, 0.44], [0.4, 0.47, 0.35, 0.54, 0.42, 0.58, 0.46, 0.4], [0.46, 0.51, 0.39, 0.56, 0.45, 0.61, 0.49, 0.43]],
      "roughness_amplitude": 0.028
    },
    "medium_grind": {"map_resolution": [1024, 256], "map_span_m": [0.78, 0.048], "map_bit_depth": 16, "track_count": 47, "roughness_amplitude": 0.018, "bump_distance_m": 0.000004, "bump_strength": 0.22},
    "micro_response": {"owner": "principled_anisotropy", "coordinate": "IGGY_BladeUV tangent", "texture_map": null},
    "regions": {
      "body": {"enabled": true, "roughness": 0.38, "anisotropy": 0.18, "macro_strength": 0.8, "grind_strength": 1.0},
      "fuller": {"enabled": false, "roughness": 0.0, "anisotropy": 0.0, "macro_strength": 0.0, "grind_strength": 0.0},
      "bevel": {"enabled": true, "roughness": 0.24, "anisotropy": 0.35, "macro_strength": 0.52, "grind_strength": 0.46},
      "edge": {"enabled": true, "roughness": 0.16, "anisotropy": 0.42, "macro_strength": 0.18, "grind_strength": 0.12}
    }
  },
  "workflow_contract": {"surface_method_contract": {"effect_stack": ["factory_lenticular_geometry", "geometry_derived_region_identity", "clean_conductor_identity", "macro_polish_roughness", "finite_medium_grind_response", "anisotropic_microresponse"], "damage_placement_state": "absent", "engine_parity": "unverified"}}
}
```

### Blender builder code

Target: `build_arming_sword_v1_clean_steel.py`.

The complete file must define and call these exact reviewed responsibilities:

```python
def execute_factory_prefix(factory_root: Path) -> dict[str, Any]:
    finish_path = factory_root / "tools" / "finish_arming_sword.py"
    source = finish_path.read_text()
    marker = "\nMID = 0.33\n"
    if marker not in source:
        raise RuntimeError("factory finish render boundary changed")
    namespace: dict[str, Any] = {
        "__name__": "__iggy_arming_sword_factory__",
        "__file__": str(finish_path),
    }
    exec(compile(source.split(marker, 1)[0], str(finish_path), "exec"), namespace)
    return namespace


def select_generated_blade(namespace: dict[str, Any]) -> bpy.types.Object:
    candidates = [
        obj for obj in bpy.data.objects
        if obj.type == "MESH" and obj.name.startswith("SW_blade")
    ]
    if not candidates:
        raise RuntimeError("factory produced no SW_blade mesh")
    selected = max(candidates, key=lambda obj: len(obj.data.polygons))
    if namespace.get("blade_ob") is not selected or selected.name != "SW_blade.001":
        raise RuntimeError("factory blade reference or object identity drifted")
    if (len(selected.data.vertices), len(selected.data.polygons)) != (270, 268):
        raise RuntimeError("factory blade topology drifted")
    return selected


def author_blade_semantics(
    blade: bpy.types.Object,
    source_zones: list[str],
    profile: dict[str, Any],
) -> dict[str, Any]:
    mesh = blade.data
    if len(source_zones) != len(mesh.polygons):
        raise RuntimeError("factory zone list no longer matches blade polygons")
    geometry = profile["geometry"]
    consumer = profile["consumer"]
    z_min = min(vertex.co.z for vertex in mesh.vertices)
    z_max = max(vertex.co.z for vertex in mesh.vertices)
    length_m = z_max - z_min
    if abs(length_m - consumer["blade_length_m"]) > 1.0e-6:
        raise RuntimeError("factory blade length drifted")
    old_uv = mesh.uv_layers.get(geometry["blade_uv"])
    if old_uv is not None:
        mesh.uv_layers.remove(old_uv)
    uv = mesh.uv_layers.new(name=geometry["blade_uv"])
    old_region = mesh.color_attributes.get(geometry["region_attribute"])
    if old_region is not None:
        mesh.color_attributes.remove(old_region)
    region = mesh.color_attributes.new(
        name=geometry["region_attribute"], type="FLOAT_COLOR", domain="CORNER"
    )
    counts = {"body": 0, "fuller": 0, "bevel": 0, "edge": 0}
    source_mismatches = 0
    vectors = {
        "body": (1.0, 0.0, 0.0, 0.0),
        "fuller": (0.0, 1.0, 0.0, 0.0),
        "bevel": (0.0, 0.0, 1.0, 0.0),
        "edge": (0.0, 0.0, 0.0, 1.0),
    }
    for polygon, source_zone in zip(mesh.polygons, source_zones):
        t_center = min(max((polygon.center.z - z_min) / length_m, 0.0), 1.0)
        half_width_m = 0.5 * (
            consumer["shoulder_width_m"]
            + (consumer["pre_tip_width_m"] - consumer["shoulder_width_m"])
            * (t_center ** geometry["profile_width_exponent"])
        )
        ratio = abs(polygon.center.x) / max(half_width_m, 1.0e-8)
        is_cap = abs(polygon.normal.z) > 0.9
        if is_cap:
            semantic = "edge" if polygon.center.z > z_min + length_m * 0.5 else "body"
        elif ratio >= 0.95:
            semantic = "edge"
        elif ratio >= 1.0 - geometry["secondary_bevel_half_width_fraction"]:
            semantic = "bevel"
        else:
            semantic = "body"
        if not is_cap and ((source_zone == "blade_edge") != (semantic == "edge")):
            source_mismatches += 1
        counts[semantic] += 1
        for loop_index in polygon.loop_indices:
            vertex = mesh.vertices[mesh.loops[loop_index].vertex_index]
            u = min(max((vertex.co.z - z_min) / length_m, 0.0), 1.0)
            vertex_half_width_m = 0.5 * (
                consumer["shoulder_width_m"]
                + (consumer["pre_tip_width_m"] - consumer["shoulder_width_m"])
                * (u ** geometry["profile_width_exponent"])
            )
            v = min(max(0.5 + vertex.co.x / (2.0 * vertex_half_width_m), 0.0), 1.0)
            if polygon.normal.y < -0.1:
                v = 1.0 - v
            uv.data[loop_index].uv = (u, v)
            region.data[loop_index].color = vectors[semantic]
    if counts != geometry["expected_region_polygon_counts"]:
        raise RuntimeError(f"actual blade region counts drifted: {counts}")
    if source_mismatches != geometry["expected_source_zone_mismatch_polygon_count"]:
        raise RuntimeError("source zone mismatch count drifted")
    quad_count = sum(len(polygon.vertices) == 4 for polygon in mesh.polygons)
    if quad_count != consumer["expected_quad_count"]:
        raise RuntimeError("factory blade is no longer all quads")
    blade["iggy_material_consumer_id"] = profile["integration_id"]
    blade["iggy_source_zone_mismatch_polygon_count"] = source_mismatches
    return {
        "vertex_count": len(mesh.vertices),
        "polygon_count": len(mesh.polygons),
        "quad_count": quad_count,
        "dimensions_m": [round(float(value), 6) for value in blade.dimensions],
        "region_polygon_counts": counts,
        "source_zone_mismatch_polygon_count": source_mismatches,
        "has_fuller": False,
        "region_attribute": geometry["region_attribute"],
        "blade_uv": geometry["blade_uv"],
    }
```

The same file must load the canonical module by absolute file path, validate
the three worker source hashes before execution, generate the two maps through
the canonical functions, build the canonical node graph, preserve the old
palette material for the matched baseline, disable only the blade's inverted
hull during material judgment, render all named views, pack all images, save
`arming_sword_v1_clean_steel.blend`, write only relative output paths to the
portable manifest, and implement `--validate-only` by reopening the saved file
and checking the actual object, dimensions, attributes, node counts, and two
packed images.

### Build and validation

```sh
python3 -m unittest tests.unit.arming_sword_v1_clean_steel_tests
python3 assets/creative/materials/workflow/scripts/material_workflow_gate.py freeze --package assets/creative/materials/sword_steel_v1 --workstream assets/creative/materials/sword_steel_v1/WORKSTREAM.md --coded-demands assets/creative/materials/sword_steel_v1/CODED_DEMANDS.md --tier reusable-family --target assets/creative/materials/sword_steel_v1/profiles/arming_sword_v1_clean_steel.json --target assets/creative/materials/sword_steel_v1/build_arming_sword_v1_clean_steel.py --target tests/unit/arming_sword_v1_clean_steel_tests.py --red-gate-evidence "actual sword integration: profile and builder absent" --revision-note "Install accepted clean steel on measured BATMAN arming sword"
python3 assets/creative/materials/workflow/scripts/material_workflow_gate.py open-build --package assets/creative/materials/sword_steel_v1
ssh linux-worker blender --background --python /tmp/iggy-sword-steel/sword_steel_v1/build_arming_sword_v1_clean_steel.py -- --factory-root /home/kogaRyu/blender-refs --output-root /tmp/arming_sword_v1_clean_steel
ssh linux-worker blender --background --python /tmp/iggy-sword-steel/sword_steel_v1/build_arming_sword_v1_clean_steel.py -- --validate-only --blend-path /tmp/arming_sword_v1_clean_steel/arming_sword_v1_clean_steel.blend --manifest-path /tmp/arming_sword_v1_clean_steel/arming_sword_v1_clean_steel_manifest.json
python3 -m unittest tests.unit.arming_sword_v1_clean_steel_tests
python3 -m py_compile assets/creative/materials/sword_steel_v1/build_arming_sword_v1_clean_steel.py
```

### Proof

The board order is baseline, clean front, three-quarter, grazing, gameplay,
clay, regions, macro, and medium. Baseline and clean front share camera,
lights, exposure, resolution, and blade-outline state. Reject if the body is a
uniform grey bar, the bevel does not change reflection, the edge highlight is
broken, any green fuller region appears, the old tag error migrates into the
new semantic mask, medium tracks survive at gameplay distance, the tip turns
black, or the clean blade depends on the surrounding sword outline to read.

### Execution record

- Code reviewed: source topology, zone mismatch, local frame, and canonical
  shader reuse reviewed before production edit.
- Red test: 4 tests run; 2 errors for absent profile/builder; 2 output tests
  skipped.
- Applied: pending frozen revision.
- Green test: pending.
- Visual result: pending actual-asset board.
- Deviations from documented code: none before implementation.
- Demand status: reviewed, awaiting workflow freeze.

## DEM-SWORD-ACTUAL-004: Broken factory-finish isolation repair

### Demand

Do not execute the current whole-sword finish prefix after the cheap worker
gate proved it aborts on the missing `SW_grip` `zone` attribute. Execute the
hash-locked measured assembly directly. Reconstruct only the old blade
baseline from the finish source's exact blade swatches, palette multiply,
white `Col` attribute, `_dirty(..., strength=0.30)`, smooth shading, metallic
default, and roughness 0.8. Do not add an outline to baseline or clean steel.
Keep every non-blade part in the assembly as neutral untextured context rather
than inventing replacement material policy.

### Authority

The worker failure is:

```text
KeyError: bpy_prop_collection[key]: key "zone" not found
finish_arming_sword.py -> zone_names(grip_ob, {1:"grip_wood",2:"grip_wrap"})
```

Immediately before that failure, the assembly independently reports the
actual blade as 270 vertices and 268 all-quad polygons, the total sword as
1.398 kg, and its centre of gravity as 87 mm ahead of the guard. The failure
therefore invalidates only the old whole-sword finish route, not the selected
geometry consumer.

### Test code

Target: `tests/unit/arming_sword_v1_clean_steel_tests.py`, method
`test_actual_consumer_builder_reuses_canonical_shader_and_source_geometry`.

```python
source = BUILDER_PATH.read_text()
self.assertIn("execute_factory_prefix", source)
self.assertIn("apply_factory_blade_baseline", source)
self.assertNotIn('marker = "\\nMID = 0.33\\n"', source)
self.assertNotIn("zone_names(grip_ob", source)
```

### Production code

Target: `build_arming_sword_v1_clean_steel.py`.

```python
def execute_factory_prefix(factory_root: Path) -> dict[str, Any]:
    assembly_path = factory_root / "tools" / "assemble_arming_sword.py"
    for name in ("Cube", "Light", "Camera"):
        obj = bpy.data.objects.get(name)
        if obj is not None:
            bpy.data.objects.remove(obj, do_unlink=True)
    namespace: dict[str, Any] = {
        "__name__": "__iggy_arming_sword_factory__",
        "__file__": str(assembly_path),
    }
    exec(compile(assembly_path.read_text(), str(assembly_path), "exec"), namespace)
    return namespace


def apply_factory_blade_baseline(
    blade: bpy.types.Object,
    source_zones: list[str],
    factory_root: Path,
) -> bpy.types.Material:
    from sg import stylize

    swatches = {"blade": "iron", "blade_edge": "stone_light", "ricasso": "iron"}
    stylize._set_uvs(blade, source_zones, swatches)
    old_col = blade.data.color_attributes.get("Col")
    if old_col is not None:
        blade.data.color_attributes.remove(old_col)
    col = blade.data.color_attributes.new("Col", "BYTE_COLOR", "CORNER")
    for datum in col.data:
        datum.color = (1.0, 1.0, 1.0, 1.0)
    stylize._dirty(blade, strength=0.30)
    image = bpy.data.images.load(str(factory_root / "textures" / "T_Palette_Master_01.png"))
    material = bpy.data.materials.new("M_ArmingSword_BladeBaseline")
    material.use_nodes = True
    tree = material.node_tree
    principled = tree.nodes["Principled BSDF"]
    texture = tree.nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    vertex_colour = tree.nodes.new("ShaderNodeVertexColor")
    vertex_colour.layer_name = "Col"
    multiply = tree.nodes.new("ShaderNodeMix")
    multiply.data_type = "RGBA"
    multiply.blend_type = "MULTIPLY"
    multiply.inputs["Factor"].default_value = 1.0
    tree.links.new(texture.outputs["Color"], multiply.inputs["A"])
    tree.links.new(vertex_colour.outputs["Color"], multiply.inputs["B"])
    tree.links.new(multiply.outputs["Result"], principled.inputs["Base Color"])
    principled.inputs["Roughness"].default_value = 0.8
    blade.data.materials.clear()
    blade.data.materials.append(material)
    for polygon in blade.data.polygons:
        polygon.material_index = 0
        polygon.use_smooth = True
    return material
```

### Build and validation

The same low-resolution worker command must now pass assembly, semantic
counts, map generation, node creation, nine proof renders, save, and manifest
emission. Only after that pass may the 720x1080 48-sample candidate run.

### Proof

Baseline and clean front retain the exact same camera, lights, exposure, and
absence of blade outline. Reject if surrounding untextured parts obscure the
blade comparison; this repair does not authorize reconstructing their broken
material route.

The first repaired contact sheet is itself rejected because Blender's default
2 m `Cube` remained visible and occluded the sword. The named-default removal
above must make the same low-resolution board show the complete sword before
the final-resolution run is authorized.

The next board is also rejected as a reflection proof: once the cube was gone,
the provisional 650/210/900 area-light energies saturated every physical and
clay surface to white. Use the already accepted sword-steel diagnostic
envelope exactly—key 22, fill 5, grazing 24—and retain the dark neutral world.
This is a proof-only repair; it may not alter the shader or maps.

The accepted-energy board remains rejected because the area-light dimensions
were transposed for the consumer: key 0.78x0.055 m and fill 0.58x0.10 m are
wide horizontal cards, causing the blade to reflect one broad white field.
Use tall sword-studio strips instead: key 0.06x0.68 m, fill 0.12x0.50 m, and
grazing 0.018x0.70 m. Preserve the 22/5/24 energies and all material values.

The vertical strips expose the edge transition but their clean-conductor
reflection still reaches display ceiling. Set the shared proof exposure to
`-1.25` stops under AgX Medium High Contrast. Apply it to baseline and all
material panels. Do not reduce conductor F0, region response, or finish-map
amplitudes to solve a camera-exposure defect.

Final-resolution review rejects the close grazing panel alone: the 24-energy
strip is coaligned with the camera and blade face and renders as a featureless
white card. Move the grazing strip to `(-0.18, -0.11, 0.46)`, lower its energy
to `8`, and use fill `0.4` in that view. No other view or material input may
change in this repair.

### Execution record

- Code reviewed: complete repair above.
- Red test: worker stopped at the factory finish `zone` lookup before render.
- Applied: complete in `build_arming_sword_v1_clean_steel.py` and the
  `arming_sword_v1_clean_steel.json` actual-consumer profile.
- Green test: 9 focused tests pass; Blender 5.1.1 completed nine 720x1080
  48-sample renders, saved the blend, and reopened it in a separate process.
- Visual result: production candidate. The off-axis grazing proof separates
  broad body response and continuous cutting edges; the region proof contains
  176 body, 0 fuller, 56 bevel, and 36 edge polygons.
- Deviations: the baseline boundary is narrower and more honest than DEM-003.
- Demand status: executed and verified; awaiting manual visual acceptance.

## DEM-SWORD-STUDY-001: Licensed four-sword construction and section study

### Demand

Create one non-production reference study from Clint Bellanger's licensed
Historical Swords Set. Download the exact hash-locked source blend, inspect it
without saving or changing it, identify blade, guard, grip, and pommel as
disconnected construction components, intersect each blade at five normalized
longitudinal stations, and render one fixed comparison board. The study must
make different blade-plane, fuller, taper, and tip strategies available to our
own sword construction and steel-shader work without promoting the donor
models, materials, textures, or dimensions as accepted production authority.

### Authority

- Source: `historical_swords_set_v1`, OpenGameArt page
  `https://opengameart.org/content/historical-swords-set`.
- Creator: Clint Bellanger.
- Selected license: CC BY 3.0, with retained attribution.
- Exact archive: 26,102,712 bytes; SHA-256
  `b090eddf53f63cb8fd8a3bfac0c3dbe36749d934ca079595485629c2a2eb55de`.
- Creator description: four simple "historical-ish" swords; it is not a
  museum survey or measurement record.
- Model-audited dimensions and section points are evidence about these files
  only. They may teach construction strategies but may never be laundered into
  measured facts about real arming swords, bastard swords, longswords, or
  claymores.

### Textures

No source texture or material enters the sword-steel shader. The source's
`LeatherDark` and `Steel` slots are recorded only as component evidence. Study
renders use one neutral workbench clay response. No new colour, roughness,
normal, height, metalness, damage, condition, or runtime texture is produced.

### Geometry semantics

The source local frame is `+Y` blade length, `X` blade width, and `Z` blade
thickness. Connected-component ownership is computed from mesh adjacency:

- largest `Y` span: blade;
- largest `X` span among remaining steel components: guard;
- remaining leather component: grip;
- remaining steel component nearest the grip end: pommel.

Every result records vertex, edge, polygon, material-slot, local bounds, and
dimensions. Blade cross sections are exact line-plane intersections with the
unchanged source mesh at `0.12`, `0.25`, `0.50`, `0.75`, and `0.88` of blade
length. Each section records metre-space points and a separately normalized
shape for comparison. Normalization never replaces the metre record.

### Nodes

No shader node group or runtime material changes. Blender Workbench owns only
the neutral proof display. Source materials are not evaluated as appearance
evidence.

### Shader flow

There is no shader integration in this demand. The output is a geometry study
that will later stress the already-existing clean-steel graph. A future shader
transfer demand must consume the audited geometry and semantic regions without
changing this source study.

### Test code

Target: `tests/unit/sword_historical_donor_study_tests.py`.

The complete tests load the provenance JSON and generated manifest. They fail
unless the source URL, CC BY attribution, byte count, hash, four expected
objects, three axes, and five stations are exact; the builder contains no
Blender save call or procedural material authoring; the downloaded source
matches the frozen hash; all four objects contain blade/guard/grip/pommel
records; every section carries metre and normalized points; sixteen geometry
panels plus four section charts are hash-locked; the 2400 by 2240 board exists;
and acceptance, runtime-material, AI-imagery, and Unreal-parity claims remain
false.

### Production code

Target: `build_historical_swords_set_study_v1.py`.

The executable owns these complete operations:

```python
def fetch_hash_locked_source(provenance, source_path):
    expected = provenance["source_archive"]
    if not source_path.is_file():
        temporary = source_path.with_suffix(".download")
        urllib.request.urlretrieve(provenance["download_url"], temporary)
        temporary.replace(source_path)
    if source_path.stat().st_size != expected["bytes"]:
        raise RuntimeError("historical sword donor byte count drifted")
    if sha256_file(source_path) != expected["sha256"]:
        raise RuntimeError("historical sword donor hash drifted")


def section_intersections(mesh, component_vertex_ids, station_y_m):
    points = []
    for edge in mesh.edges:
        a, b = edge.vertices
        if a not in component_vertex_ids or b not in component_vertex_ids:
            continue
        ca, cb = mesh.vertices[a].co, mesh.vertices[b].co
        da = float(ca.y - station_y_m)
        db = float(cb.y - station_y_m)
        if abs(da) < 1.0e-9:
            points.append((float(ca.x), float(ca.z)))
        if da * db < 0.0:
            t = -da / (db - da)
            points.append((float(ca.x + t * (cb.x - ca.x)), float(ca.z + t * (cb.z - ca.z))))
    return deduplicate_and_sort_section(points)
```

The complete file also downloads only to the declared study output, opens the
source once, validates exact source objects, audits connected components,
renders fixed whole-sword, isolated-hilt, blade three-quarter, and topology
views, writes SVG and PNG
cross-section charts, assembles the board, hashes every artifact, writes one
manifest, and re-hashes the source after rendering. It does not save a blend.

### Build and validation

```sh
python3 -m unittest tests.unit.sword_historical_donor_study_tests
/Applications/Blender.app/Contents/MacOS/Blender --background --factory-startup \
  --python assets/creative/materials/sword_steel_v1/build_historical_swords_set_study_v1.py
python3 -m unittest tests.unit.sword_historical_donor_study_tests
```

The red gate must fail or skip only because the builder and generated study do
not exist. The green gate must prove the frozen archive, four construction
records, five sections per blade, artifact hashes, immutable source, and
non-production boundary.

### Proof

The fixed four-row board shows, for every sword: whole silhouette, isolated
hilt construction, blade under oblique light, blade topology/plane proof, and
five exact model cross sections. Reject the study if source materials obscure
geometry, if oblique light does not expose different planes, if sections are drawn
from guessed templates rather than mesh intersections, if a fuller is inferred
from colour, if labels confuse model dimensions with historical measurements,
or if any donor is presented as an accepted production sword.

### Execution record

- Code reviewed: provenance, connected-component ownership, section-plane
  intersection, render isolation, output scope, and source immutability.
- Red test: two blueprint tests passed and four output tests skipped before the
  builder existed.
- Applied: one hash-locked downloader/auditor, four construction censuses,
  twenty exact section intersections, sixteen geometry panels, four section
  charts, one fixed comparison board, and one manifest.
- Green test: focused output and provenance gates pass after visual review is
  registered.
- Visual result: usable reference study with source limits. The first board's
  rolled oblique composition and overlapping 4x-thickness sections were
  rejected and repaired as one batch. BastardSword is the fuller-termination
  donor; LongSword is the constant-thickness negative control.
- Demand status: executed and verified as a non-production reference study;
  no donor, material, historical measurement, or Unreal acceptance follows.

## DEM-SWORD-TRANSFER-001: Unchanged clean-steel response on four donor blades

### Demand

Build one disposable diagnostic comparison that applies the canonical
`IGGY_MAT_SwordSteel_CleanGround_v001` graph unchanged to the four hash-locked
blade components audited by `DEM-SWORD-STUDY-001`. This gate asks whether one
established material response reveals different compound sections, fuller
termination, bevel planes, and distal taper. It may add geometry-owned UV and
region attributes, but it may not fork material parameters, maps, nodes,
lighting, or exposure per sword.

### Frozen authorities and exclusions

- Canonical graph owner: `build_sword_steel_v1.py`, SHA-256
  `76030d14d442ad2c1d4550b6d150c82caa536a8ac63ee8a94208a9e51ead7bbb`.
- Donor archive: `historical_swords_set.blend`, SHA-256
  `b090eddf53f63cb8fd8a3bfac0c3dbe36749d934ca079595485629c2a2eb55de`.
- Macro map authority: canonical generated field, SHA-256
  `dcffc56eed667746218c001503e923defcc9b9a8855b3ace371237d2cb084e35`.
- Grind map authority: canonical generated field, SHA-256
  `52c7b2f15c8a176d67d9c0d6e722f49f3b2be86c511eea7a6a438eca902f6c63`.
- Source donor geometry is copied into the temporary proof scene but never
  changed or saved back. No donor material or texture is consumed.
- Damage, oxidation, scratches, colour variants, decoration, shader tuning,
  source acceptance, manual material acceptance, saved-blend packaging, and
  Unreal parity are excluded.

### Geometry adapter

The source-local frame remains `+Y` length, `X` width, and `Z` thickness.
Every copied blade receives `IGGY_BladeUV` with normalized root-to-tip `U` and
edge-to-edge `V`. This deliberately matches the canonical graph's normalized
whole-blade mapping; the diagnostic records that its physical map span changes
with donor dimensions.

Every face receives exactly one corner-domain `sinc_sword_region` value. Caps
remain body. Longitudinal faces use an interpolated local width/thickness
envelope, source polygon normal, and normalized face centre:

```python
def classify_longitudinal_face(
    sword_name, normal, x_norm, z_norm, cross_section_rise, inner_x_norm
):
    if abs(x_norm) > 0.82 and abs(normal.x) > 0.72:
        return "edge"
    has_recess = sword_name in {"BastardSword", "ClaymoreSword"}
    if (
        has_recess
        and inner_x_norm < 0.40
        and cross_section_rise > 0.08
    ):
        return "fuller"
    if abs(x_norm) > 0.52 or abs(normal.x) > 0.55:
        return "bevel"
    return "body"
```

`cross_section_rise` is calculated from each longitudinal polygon's two
same-station section endpoints as `outer_abs_z - inner_abs_z`, after both axes
are normalized by the interpolated local envelope. A positive rise therefore
means the surface climbs from a central valley toward an outer shoulder. This
is an explicit model adapter, not a claim that the donor labels are
historically authoritative. Reject it if ArmingSword or LongSword invents a
fuller, if BastardSword loses the audited recess, if any blade lacks body or
bevel ownership, or if the one-hot corner sum differs from one. A donor may
report zero edge polygons when its cross section runs directly from bevel to a
zero-width cutting line. The adapter must record that missing construction and
must not fabricate an edge land merely to exercise the shader's edge lane.

### Shader and maps

The builder imports the canonical module and calls, without replacement:

```python
profile, conductor, tangent = canonical.load_contracts()
macro = canonical.generate_macro_polish_field(profile)
grind = canonical.generate_grind_field(profile)
physical, material_contract = canonical.build_sword_steel_material(
    profile, conductor, tangent, macro_image, grind_image
)
```

One material instance is shared by all four blades. Required live topology is
one Principled BSDF, zero Mix Shader nodes, two image textures, one tangent,
and one bump node. The conductor is linear RGB `[0.56, 0.57, 0.58]`, metallic
one, body roughness `0.38`, and body anisotropy `0.18`. Every generated map
must hash-match the canonical output above.

### Proof contract

Produce a 4x4 board with 720x240 panels and exact 2880x960 resolution. Rows are
ArmingSword, BastardSword, LongSword, and ClaymoreSword. Columns are:

1. neutral physical response;
2. grazing strip response;
3. gameplay-distance response under the same neutral rig;
4. isolated one-hot regions: red body, green fuller, blue bevel, white edge
   where a separately modelled edge land exists.

All rows use the same camera rules, light geometry, energy, exposure, colour
management, samples, material, and maps. Only framing derives from blade
length. The first build remains `pending_visual_review`; after original-size
inspection, a review-only pass may bind the board hash, strongest separation,
weakest separation, classification defects, and next decision. It may not set
manual acceptance.

### Targets and validation

- Production target: `build_historical_swords_clean_steel_response_v1.py`.
- Contract target: `tests/unit/sword_historical_response_board_tests.py`.
- Output root: `output/historical_swords_clean_steel_response_v1/`.
- Manifest status: `DIAGNOSTIC_MULTI_GEOMETRY_RESPONSE_NOT_ACCEPTANCE`.

```sh
python3 -m unittest tests.unit.sword_historical_response_board_tests
/Applications/Blender.app/Contents/MacOS/Blender --background --factory-startup \
  --python assets/creative/materials/sword_steel_v1/build_historical_swords_clean_steel_response_v1.py
python3 -m unittest tests.unit.sword_historical_response_board_tests
```

### Execution record

- Static review: canonical import, immutable authorities, adapter thresholds,
  one shared graph, proof matrix, stale-output cleanup, and exclusion flags
  reviewed before implementation.
- Red test: one expected failure for the absent builder and five skipped output
  tests. First Blender execution then rejected a stricter but false assumption:
  ArmingSword has no separately modelled edge-land strip. The reviewed repair
  allows an honest zero edge count rather than inventing face ownership.
- First visual proof: rejected before registration because the normal-sign
  fuller heuristic reduced BastardSword's audited long recess to a root dot.
  Batched repair changes the semantic owner to normalized cross-section rise;
  shader, maps, lighting, framing, and exposure remain frozen.
- Second visual proof: semantic repair passed, but the rotated donor's stale
  axis read produced undersized blades and the shared neutral key compressed
  several broad planes toward white. Proof-only repair frames from the longest
  evaluated in-plane dimension and changes the one shared key/fill/grazing
  energies from `18/4/16` to `10/2.5/12`. Material and geometry ownership stay
  frozen. Original-size inspection of that repair showed the blade ends too
  close to the panel boundary, so the same repair envelope increases only the
  longitudinal framing margin from `1.10` to `1.35`; no additional art or
  material variable changes. The next original-size view exposed the actual
  equation defect: `ortho_scale` owns visible frame width, not height. The final
  proof contract is `max(length * 1.35, width * aspect * 2.8)`; this replaces
  the incorrect division by aspect rather than adding another aesthetic nudge.
- Applied: canonical graph import, byte-identical map generation, four source
  blade copies, normalized UVs, section-derived one-hot semantics, sixteen
  matched Cycles panels, comparison board, manifest, and review-only hash gate.
- Visual result: `diagnostic_response_pass_with_geometry_gaps`. BastardSword
  gives the strongest fuller/plane separation; LongSword is the weakest and
  confirms the constant-thickness negative control. ArmingSword exposes the
  absent edge-land limitation; ClaymoreSword keeps forte complexity local.
- Demand status: executed and visually reviewed as a diagnostic comparison.
  No donor, shader, saved blend, manual acceptance, or Unreal promotion follows.
