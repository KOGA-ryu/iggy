# Coded Material Demands — Measured Chevron Voussoir Portal

This is the executable contract for the current capability. The code
blocks below are complete synchronized copies of the files that were
built and tested; they are not pseudocode, excerpts, or future intent.

## Document contract

- Material ID: `cathedral_stone_trim_fracture_v1`
- Capability: one intact measured chevron-voussoir portal order
- Profile schema: `iggy3d.material.cathedral_stone_trim_fracture_v1.profile.v2`
- Pattern schema: `iggy3d.pattern.chevron_voussoir_portal.v2`
- Texture manifest schema: `iggy3d.material.cathedral_stone_trim_fracture_v1.manifest.v2`
- Blender manifest schema: `iggy3d.material.cathedral_stone_trim_fracture_v1.blender_manifest.v2`
- Blender version inspected: `5.1.1`
- Target engine: Blender 5.1.1 / EEVEE; Unreal parity excluded
- Champion seed: `41723`
- Workflow tier: `hero-master`
- Delivery boundary: `production-candidate`
- Workstream dossier: `/Users/kogaryu/iggy3d/assets/creative/materials/cathedral_stone_trim_fracture_v1/WORKSTREAM.md`
- Reference delta: `/Users/kogaryu/iggy3d/assets/creative/materials/cathedral_stone_trim_fracture_v1/REFERENCE_DELTA.md`

## Complete texture inventory

| Texture ID | Filename | Meaning | Physical span | Resolution | Metres per texel | Bit depth | Color space | Channels | Coordinate and variation | Distance behavior | Blender consumer | Engine consumer | Test |
| --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- | --- | --- | --- | --- |
| basecolor | cathedral_stone_trim_fracture_v1_basecolor.png | Intrinsic broad twenty-shade calcarenite field. | 0.512000 m | 1024x1024 | 0.0005000000 | 8 | sRGB | RGB intrinsic broad twenty-shade calcarenite colour | IGGY_StoneUV_A local metres / 0.512 m; seeded phase and one of four rotations/mirrors. | Always present; semantic roughness deltas are gated separately. | IGGY_TrimBaseColor | Deferred. Unreal parity is excluded until this Blender capability passes and receives an independently tested reconstruction. | test_basecolor_contains_broad_related_variation_not_baked_light |
| orm | cathedral_stone_trim_fracture_v1_orm.png | Dielectric AO, calibrated broad roughness, and metalness. | 0.512000 m | 1024x1024 | 0.0005000000 | 8 | Non-Color | R ambient-occlusion=1, G roughness, B metallic=0 | Same 0.512 m frame as basecolor. | Always present; semantic roughness deltas are gated separately. | IGGY_TrimORM | Deferred. Unreal parity is excluded until this Blender capability passes and receives an independently tested reconstruction. | test_orm_is_dielectric_with_quiet_bounded_roughness |
| body_masks | cathedral_stone_trim_fracture_v1_body_masks.png | Measured-material fossil, silicate, and pore identities. | 0.064000 m | 1024x1024 | 0.0000625000 | 8 | Non-Color | R fossil fragment, G silicate grain, B visible pore identity | Blend IGGY_StoneUV_A / 0.064 m with IGGY_StoneUV_B / 0.091 m at 0.34. | Full at 0.30 m; smootherstep fade to zero at 3.50 m. | IGGY_BodyMasks64 and IGGY_BodyMasks91 | Deferred. Unreal parity is excluded until this Blender capability passes and receives an independently tested reconstruction. | test_generated_family_is_deterministic_and_preserves_core_body_bytes |
| body_normal | cathedral_stone_trim_fracture_v1_body_normal.png | Accepted measured-proxy OpenGL body relief. | 0.064000 m | 1024x1024 | 0.0000625000 | 8 | Non-Color | OpenGL XYZ | IGGY_StoneUV_A / 0.064 m; per-stone transformed and phased. | Full at 0.30 m; smootherstep fade to zero at 3.50 m. | IGGY_BodyNormal64 -> IGGY_OpenGLBodyNormal64mm | Deferred. Unreal parity is excluded until this Blender capability passes and receives an independently tested reconstruction. | test_every_image_is_packed_and_uses_the_declared_color_space |
| body_height | cathedral_stone_trim_fracture_v1_body_height.png | Accepted measured-proxy normalized height. | 0.064000 m | 1024x1024 | 0.0000625000 | 16 | Non-Color | normalized height with 0.00113 m declared range | IGGY_StoneUV_B / 0.091 m; independently transformed and phased. | Full at 0.30 m; smootherstep fade to zero at 3.50 m. | IGGY_BodyHeight91 -> IGGY_DecorrelatedBodyHeight91mm | Deferred. Unreal parity is excluded until this Blender capability passes and receives an independently tested reconstruction. | test_output_png_headers_match_declared_lane_formats |

All texture image nodes are packed into the saved blend. The
basecolor carries no mortar, sunlight, cavity shadow, or damage.
ORM red is one and blue is zero. The 16-bit height range is
0.00113 m, with only 34 percent applied as the secondary bump.

## Layer provenance

These are the independently reviewable branches that make the
material layered. Every source reaches a named output, consumer,
proof, and protected-rest rule.

| Layer | Physical meaning | Sources | Outputs | Consumers | Isolated proofs | Rest rule | Measurement claims |
| --- | --- | --- | --- | --- | --- | --- | --- |
| construction | Sixteen separate closed wedge-shaped voussoirs form the semicircular order with real centerline joints. | ["S01_old_sarum_catalogue_item_55","patterns/cathedral_trim_fracture_atlas_v1.json"] | ["voussoir mesh volumes","iggy_voussoir_id","IGGY_StoneUV_A","IGGY_StoneUV_B"] | ["Blender product collection","IGGY_SH_ChevronVoussoir_v002"] | ["neutral_clay_front","wireframe_proof"] | No surface texture is allowed to draw joints or repair a weak arch silhouette. | ["catalogue_height","catalogue_inner_chord","catalogue_outer_chord","catalogue_depth","authored_arch_count","inherited_joint","derived_radii","derived_outer_radius"] |
| macro | Broad quiet calcarenite value fields and bounded warm-cool changes prevent one flat colour per stone. | ["trim-specific deterministic broad colour compiler","surface_contract broad colour palette"] | ["cathedral_stone_trim_fracture_v1_basecolor.png","per-stone warm-cool variant"] | ["IGGY_TrimBaseColor","IGGY_Attribute_iggy_material_variant"] | ["live_material_front","distance_read"] | Broad fields retain quiet spans and may not contain mortar, photographed lighting, or universal cloud noise. | ["authored_broad_colour_span","authored_shade_count"] |
| medium | One phase-locked lateral chevron with roll-hollow-roll moulding is sampled into the front geometry of each stone. | ["S03_crsbi_chevron_guide","authored signed chevron field"] | ["front relief geometry","iggy_chevron_roll","iggy_chevron_hollow","iggy_chevron_quiet"] | ["voussoir front mesh","live moulding colour and roughness"] | ["moulding_proof","measured_close"] | The two 25 mm quiet zones remain free of roll and hollow displacement. | ["authored_moulding_zones","authored_roll_crest","authored_hollow_depth","authored_chevron_path","authored_chevron_shoulder","authored_band_width"] |
| edge_event | Real joints, selected hollow ink, and selected crest highlights articulate construction without outlining every edge. | ["voussoir boundaries","signed chevron derivatives"] | ["physical joint gaps","iggy_chevron_ink","iggy_chevron_highlight"] | ["arch silhouette and occlusion","selective graphic linework"] | ["live_material_grazing","identity_proof"] | Quiet faces and non-selected crests receive no universal ink or highlight. | ["inherited_joint","authored_roll_offsets","authored_hollow_offset"] |
| micro | Measured-proxy fossil, silicate, pore, normal, and height identities supply close calcarenite response. | ["cathedral_stone_v1 accepted body masks","cathedral_stone_v1 accepted body normal","cathedral_stone_v1 accepted body height"] | ["cathedral_stone_trim_fracture_v1_body_masks.png","cathedral_stone_trim_fracture_v1_body_normal.png","cathedral_stone_trim_fracture_v1_body_height.png"] | ["IGGY_BodyMasks64","IGGY_BodyMasks91","IGGY_BodyNormal64","IGGY_BodyHeight91"] | ["measured_close"] | Microdetail fades from full at 0.30 m to absent at 3.50 m and cannot replace the moulding. | ["inherited_body_spans","inherited_secondary_span","inherited_detail_distances","inherited_detail_zero"] |
| cumulative_color | Broad colour, per-stone variation, body identities, moulding values, and selective linework accumulate in a fixed causal order. | ["basecolor lane","body mask lane","moulding attributes","stone variant attribute"] | ["final unlit intrinsic stone base colour"] | ["Principled BSDF Base Color"] | ["live_material_front","identity_proof"] | Intrinsic colour excludes AO, scene shadow, damage, damp, lichen, soot, and generic grime. | ["authored_broad_colour_span","authored_shade_count"] |
| height_normal | Primary 64 mm tangent normal and decorrelated 91 mm height contribute measured-proxy body relief beneath real moulding geometry. | ["body normal lane","body height lane"] | ["combined Principled BSDF normal input"] | ["IGGY_BodyNormal64Node","IGGY_DecorrelatedBodyHeight91mm"] | ["measured_close","live_material_grazing"] | The 1.13 mm proxy range affects shading only and makes no silhouette or deep-fracture claim. | ["authored_normal_share","authored_height_share","proxy_relief_range"] |
| response | Broad porous calcarenite roughness is modified by body and moulding cause while AO remains one and metalness remains zero. | ["cathedral_stone_trim_fracture_v1_orm.png","body masks","roll and hollow attributes"] | ["final roughness","AO equals one","metalness equals zero","opacity equals one"] | ["Principled BSDF Roughness","Principled BSDF Metallic","Principled BSDF Alpha"] | ["live_material_grazing"] | Roughness is not inverted height, and no fake cavity AO is baked into colour. | ["dielectric_metallic","opaque_surface"] |
| stylization | Structure-selected ink and highlights preserve the chevron value rhythm after body microdetail fades. | ["iggy_chevron_ink","iggy_chevron_highlight","camera distance"] | ["selective ink colour","selective crest highlight"] | ["final base-colour composition"] | ["distance_read","moulding_proof"] | Linework is absent from unselected faces and does not encode scene lighting. | [] |

## Complete node and group inventory

Group: `IGGY_SH_ChevronVoussoir_v002`.
Exact graph size: 79 nodes and 103 links.

| Node name | `bl_idname` | Operation or mode | Unlinked inputs and defaults | Incoming links | Outgoing links | Contract role | Validation |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Group_Output | NodeGroupOutput | {} | [] | ["IGGY_DecorrelatedBodyHeight91mm.Normal -> Combined Normal","IGGY_DetailFullAt030mZeroAt3p5m.Result -> Detail Distance Fade","IGGY_IdentityProofRGB.Color -> Identity Proof","IGGY_MouldingProofRGB.Color -> Moulding Proof","IGGY_RoughnessMaximum.Value -> Combined Roughness","IGGY_SelectiveCrestHighlight.Color -> Combined Color"] | [] | Publish live material and proof outputs. | Present in executed Blender manifest; 6 incoming and 0 outgoing links. |
| IGGY_AddHollowRoughness | ShaderNodeMath | {"operation":"ADD"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_AddRollRoughness.Value -> Value","IGGY_HollowRoughnessDelta.Value -> Value"] | ["Value -> IGGY_RoughnessMinimum.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_AddPoreRoughness | ShaderNodeMath | {"operation":"ADD"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_PoreRoughnessDelta.Value -> Value","IGGY_SeparateORM.Green -> Value"] | ["Value -> IGGY_AddSilicateRoughness.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_AddRollRoughness | ShaderNodeMath | {"operation":"ADD"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_AddSilicateRoughness.Value -> Value","IGGY_RollRoughnessDelta.Value -> Value"] | ["Value -> IGGY_AddHollowRoughness.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_AddSilicateRoughness | ShaderNodeMath | {"operation":"ADD"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_AddPoreRoughness.Value -> Value","IGGY_SilicateRoughnessDelta.Value -> Value"] | ["Value -> IGGY_AddRollRoughness.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_Attribute_iggy_carved_trim | ShaderNodeAttribute | {"attribute_name":"iggy_carved_trim"} | [] | [] | ["Factor -> IGGY_FrontTimesCarved.Value"] | Read live geometry semantic `iggy_carved_trim`. | Present in executed Blender manifest; 0 incoming and 1 outgoing links. |
| IGGY_Attribute_iggy_chevron_front | ShaderNodeAttribute | {"attribute_name":"iggy_chevron_front"} | [] | [] | ["Factor -> IGGY_FrontTimesCarved.Value"] | Read live geometry semantic `iggy_chevron_front`. | Present in executed Blender manifest; 0 incoming and 1 outgoing links. |
| IGGY_Attribute_iggy_chevron_highlight | ShaderNodeAttribute | {"attribute_name":"iggy_chevron_highlight"} | [] | [] | ["Factor -> IGGY_LiveHighlightMask.Value"] | Read live geometry semantic `iggy_chevron_highlight`. | Present in executed Blender manifest; 0 incoming and 1 outgoing links. |
| IGGY_Attribute_iggy_chevron_hollow | ShaderNodeAttribute | {"attribute_name":"iggy_chevron_hollow"} | [] | [] | ["Factor -> IGGY_LiveHollowMask.Value"] | Read live geometry semantic `iggy_chevron_hollow`. | Present in executed Blender manifest; 0 incoming and 1 outgoing links. |
| IGGY_Attribute_iggy_chevron_ink | ShaderNodeAttribute | {"attribute_name":"iggy_chevron_ink"} | [] | [] | ["Factor -> IGGY_LiveInkMask.Value"] | Read live geometry semantic `iggy_chevron_ink`. | Present in executed Blender manifest; 0 incoming and 1 outgoing links. |
| IGGY_Attribute_iggy_chevron_quiet | ShaderNodeAttribute | {"attribute_name":"iggy_chevron_quiet"} | [] | [] | ["Factor -> IGGY_LiveQuietMask.Value"] | Read live geometry semantic `iggy_chevron_quiet`. | Present in executed Blender manifest; 0 incoming and 1 outgoing links. |
| IGGY_Attribute_iggy_chevron_roll | ShaderNodeAttribute | {"attribute_name":"iggy_chevron_roll"} | [] | [] | ["Factor -> IGGY_LiveRollMask.Value"] | Read live geometry semantic `iggy_chevron_roll`. | Present in executed Blender manifest; 0 incoming and 1 outgoing links. |
| IGGY_Attribute_iggy_material_phase | ShaderNodeAttribute | {"attribute_name":"iggy_material_phase"} | [] | [] | ["Vector -> IGGY_PhaseVectorLength.Vector"] | Read live geometry semantic `iggy_material_phase`. | Present in executed Blender manifest; 0 incoming and 1 outgoing links. |
| IGGY_Attribute_iggy_material_variant | ShaderNodeAttribute | {"attribute_name":"iggy_material_variant"} | [] | [] | ["Factor -> IGGY_HighVariant.Value","Factor -> IGGY_VariantNormalized.Value","Factor -> IGGY_VariantParity.Value"] | Read live geometry semantic `iggy_material_variant`. | Present in executed Blender manifest; 0 incoming and 3 outgoing links. |
| IGGY_Attribute_iggy_voussoir_id | ShaderNodeAttribute | {"attribute_name":"iggy_voussoir_id"} | [] | [] | ["Factor -> IGGY_VoussoirIdNormalized.Value"] | Read live geometry semantic `iggy_voussoir_id`. | Present in executed Blender manifest; 0 incoming and 1 outgoing links. |
| IGGY_BaseCoordinate_512mm | ShaderNodeVectorMath | {"operation":"SCALE"} | [{"default":[0.0,0.0,0.0],"identifier":"Vector_001","socket":"Vector","type":"VECTOR"},{"default":[0.0,0.0,0.0],"identifier":"Vector_002","socket":"Vector","type":"VECTOR"},{"default":1.953125,"identifier":"Scale","socket":"Scale","type":"VALUE"}] | ["IGGY_StoneUV_A.UV -> Vector"] | ["Vector -> IGGY_TrimBaseColor.Vector","Vector -> IGGY_TrimORM.Vector"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 2 outgoing links. |
| IGGY_Blend64mmAnd91mmBodyMasks | ShaderNodeMixRGB | {"blend_type":"MIX","use_clamp":false} | [{"default":0.3400000035762787,"identifier":"Fac","socket":"Factor","type":"VALUE"}] | ["IGGY_BodyMasks64.Color -> Color1","IGGY_BodyMasks91.Color -> Color2"] | ["Color -> IGGY_SeparateBodyMasks.Color"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_BodyHeight91 | ShaderNodeTexImage | {"extension":"REPEAT","interpolation":"Linear","projection":"FLAT"} | [] | ["IGGY_SecondaryCoordinate_91mm.Vector -> Vector"] | ["Color -> IGGY_BodyHeight91mmValue.Color"] | Consume packed `cathedral_stone_trim_fracture_v1_body_height.png` as Non-Color. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_BodyHeight91mmValue | ShaderNodeRGBToBW | {} | [] | ["IGGY_BodyHeight91.Color -> Color"] | ["Val -> IGGY_DecorrelatedBodyHeight91mm.Height"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_BodyMasks64 | ShaderNodeTexImage | {"extension":"REPEAT","interpolation":"Linear","projection":"FLAT"} | [] | ["IGGY_PrimaryCoordinate_64mm.Vector -> Vector"] | ["Color -> IGGY_Blend64mmAnd91mmBodyMasks.Color1"] | Consume packed `cathedral_stone_trim_fracture_v1_body_masks.png` as Non-Color. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_BodyMasks91 | ShaderNodeTexImage | {"extension":"REPEAT","interpolation":"Linear","projection":"FLAT"} | [] | ["IGGY_SecondaryCoordinate_91mm.Vector -> Vector"] | ["Color -> IGGY_Blend64mmAnd91mmBodyMasks.Color2"] | Consume packed `cathedral_stone_trim_fracture_v1_body_masks.png` as Non-Color. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_BodyNormal64 | ShaderNodeTexImage | {"extension":"REPEAT","interpolation":"Linear","projection":"FLAT"} | [] | ["IGGY_PrimaryCoordinate_64mm.Vector -> Vector"] | ["Color -> IGGY_OpenGLBodyNormal64mm.Color"] | Consume packed `cathedral_stone_trim_fracture_v1_body_normal.png` as Non-Color. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_CameraDistance | ShaderNodeCameraData | {} | [] | [] | ["View Distance -> IGGY_DetailFullAt030mZeroAt3p5m.Value","View Distance -> IGGY_LineFullAt030mZeroAt6m.Value"] | Provide view distance for detail hierarchy. | Present in executed Blender manifest; 0 incoming and 2 outgoing links. |
| IGGY_CoolVariantFactor | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.054999999701976776,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_HighVariant.Value -> Value"] | ["Value -> IGGY_PerStoneCoolWash.Factor"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_DecorrelatedBodyHeight91mm | ShaderNodeBump | {"invert":false} | [{"default":0.00038419998600147665,"identifier":"Distance","socket":"Distance","type":"VALUE"},{"default":0.10000000149011612,"identifier":"Filter Width","socket":"Filter Width","type":"VALUE"}] | ["IGGY_BodyHeight91mmValue.Val -> Height","IGGY_DetailFullAt030mZeroAt3p5m.Result -> Strength","IGGY_OpenGLBodyNormal64mm.Normal -> Normal"] | ["Normal -> Group_Output.Combined Normal"] | Add the decorrelated 91 mm height sample after the normal map. | Present in executed Blender manifest; 3 incoming and 1 outgoing links. |
| IGGY_DetailFullAt030mZeroAt3p5m | ShaderNodeMapRange | {"clamp":true,"interpolation_type":"SMOOTHERSTEP"} | [{"default":0.30000001192092896,"identifier":"From Min","socket":"From Min","type":"VALUE"},{"default":3.5,"identifier":"From Max","socket":"From Max","type":"VALUE"},{"default":1.0,"identifier":"To Min","socket":"To Min","type":"VALUE"},{"default":0.0,"identifier":"To Max","socket":"To Max","type":"VALUE"},{"default":4.0,"identifier":"Steps","socket":"Steps","type":"VALUE"},{"default":[0.0,0.0,0.0],"identifier":"Vector","socket":"Vector","type":"VECTOR"},{"default":[0.0,0.0,0.0],"identifier":"From_Min_FLOAT3","socket":"From Min","type":"VECTOR"},{"default":[1.0,1.0,1.0],"identifier":"From_Max_FLOAT3","socket":"From Max","type":"VECTOR"},{"default":[0.0,0.0,0.0],"identifier":"To_Min_FLOAT3","socket":"To Min","type":"VECTOR"},{"default":[1.0,1.0,1.0],"identifier":"To_Max_FLOAT3","socket":"To Max","type":"VECTOR"},{"default":[4.0,4.0,4.0],"identifier":"Steps_FLOAT3","socket":"Steps","type":"VECTOR"}] | ["IGGY_CameraDistance.View Distance -> Value"] | ["Result -> Group_Output.Detail Distance Fade","Result -> IGGY_DecorrelatedBodyHeight91mm.Strength","Result -> IGGY_FossilTimesDetailFade.Value","Result -> IGGY_PoreTimesDetailFade.Value","Result -> IGGY_PrimaryNormalStrength66Percent.Value","Result -> IGGY_SilicateTimesDetailFade.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 6 outgoing links. |
| IGGY_FossilColorAmount | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.12999999523162842,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_FossilTimesDetailFade.Value -> Value"] | ["Value -> IGGY_FossilFragmentColor.Factor"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_FossilFragmentColor | ShaderNodeMixRGB | {"blend_type":"MIX","use_clamp":false} | [{"default":[0.7454041838645935,0.5906188488006592,0.38132601976394653,1.0],"identifier":"Color2","socket":"Color2","type":"RGBA"}] | ["IGGY_FossilColorAmount.Value -> Factor","IGGY_PerStoneCoolWash.Color -> Color1"] | ["Color -> IGGY_SilicateGrainColor.Color1"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_FossilTimesDetailFade | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_DetailFullAt030mZeroAt3p5m.Result -> Value","IGGY_SeparateBodyMasks.Red -> Value"] | ["Value -> IGGY_FossilColorAmount.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_FrontTimesCarved | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_Attribute_iggy_carved_trim.Factor -> Value","IGGY_Attribute_iggy_chevron_front.Factor -> Value"] | ["Value -> IGGY_LiveHighlightMask.Value","Value -> IGGY_LiveHollowMask.Value","Value -> IGGY_LiveInkMask.Value","Value -> IGGY_LiveQuietMask.Value","Value -> IGGY_LiveRollMask.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 5 outgoing links. |
| IGGY_HighVariant | ShaderNodeMath | {"operation":"GREATER_THAN"} | [{"default":1.5,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_Attribute_iggy_material_variant.Factor -> Value"] | ["Value -> IGGY_CoolVariantFactor.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_HighlightTimesLineFade | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_LineFullAt030mZeroAt6m.Result -> Value","IGGY_LiveHighlightMask.Value -> Value"] | ["Value -> IGGY_SelectiveHighlightAmount.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_HollowCoolness | ShaderNodeMixRGB | {"blend_type":"MIX","use_clamp":false} | [{"default":[0.19806931912899017,0.16513219475746155,0.13286831974983215,1.0],"identifier":"Color2","socket":"Color2","type":"RGBA"}] | ["IGGY_HollowCoolnessFactor.Value -> Factor","IGGY_RollWarmth.Color -> Color1"] | ["Color -> IGGY_SelectiveChevronInk.Color1"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_HollowCoolnessFactor | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.2199999988079071,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_LiveHollowMask.Value -> Value"] | ["Value -> IGGY_HollowCoolness.Factor"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_HollowRoughnessDelta | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.02800000086426735,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_LiveHollowMask.Value -> Value"] | ["Value -> IGGY_AddHollowRoughness.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_IdentityProofRGB | ShaderNodeCombineColor | {"mode":"RGB"} | [] | ["IGGY_PhaseLengthNormalized.Result -> Green","IGGY_VariantNormalized.Value -> Blue","IGGY_VoussoirIdNormalized.Value -> Red"] | ["Color -> Group_Output.Identity Proof"] | Expose an isolated acceptance proof lane. | Present in executed Blender manifest; 3 incoming and 1 outgoing links. |
| IGGY_InkTimesLineFade | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_LineFullAt030mZeroAt6m.Result -> Value","IGGY_LiveInkMask.Value -> Value"] | ["Value -> IGGY_SelectiveInkAmount.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_LineFullAt030mZeroAt6m | ShaderNodeMapRange | {"clamp":true,"interpolation_type":"SMOOTHERSTEP"} | [{"default":0.30000001192092896,"identifier":"From Min","socket":"From Min","type":"VALUE"},{"default":6.0,"identifier":"From Max","socket":"From Max","type":"VALUE"},{"default":1.0,"identifier":"To Min","socket":"To Min","type":"VALUE"},{"default":0.0,"identifier":"To Max","socket":"To Max","type":"VALUE"},{"default":4.0,"identifier":"Steps","socket":"Steps","type":"VALUE"},{"default":[0.0,0.0,0.0],"identifier":"Vector","socket":"Vector","type":"VECTOR"},{"default":[0.0,0.0,0.0],"identifier":"From_Min_FLOAT3","socket":"From Min","type":"VECTOR"},{"default":[1.0,1.0,1.0],"identifier":"From_Max_FLOAT3","socket":"From Max","type":"VECTOR"},{"default":[0.0,0.0,0.0],"identifier":"To_Min_FLOAT3","socket":"To Min","type":"VECTOR"},{"default":[1.0,1.0,1.0],"identifier":"To_Max_FLOAT3","socket":"To Max","type":"VECTOR"},{"default":[4.0,4.0,4.0],"identifier":"Steps_FLOAT3","socket":"Steps","type":"VECTOR"}] | ["IGGY_CameraDistance.View Distance -> Value"] | ["Result -> IGGY_HighlightTimesLineFade.Value","Result -> IGGY_InkTimesLineFade.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 2 outgoing links. |
| IGGY_LiveHighlightMask | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_Attribute_iggy_chevron_highlight.Factor -> Value","IGGY_FrontTimesCarved.Value -> Value"] | ["Value -> IGGY_HighlightTimesLineFade.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_LiveHollowMask | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_Attribute_iggy_chevron_hollow.Factor -> Value","IGGY_FrontTimesCarved.Value -> Value"] | ["Value -> IGGY_HollowCoolnessFactor.Value","Value -> IGGY_HollowRoughnessDelta.Value","Value -> IGGY_MouldingProofRGB.Green"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 3 outgoing links. |
| IGGY_LiveInkMask | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_Attribute_iggy_chevron_ink.Factor -> Value","IGGY_FrontTimesCarved.Value -> Value"] | ["Value -> IGGY_InkTimesLineFade.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_LiveQuietMask | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_Attribute_iggy_chevron_quiet.Factor -> Value","IGGY_FrontTimesCarved.Value -> Value"] | ["Value -> IGGY_MouldingProofRGB.Blue"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_LiveRollMask | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_Attribute_iggy_chevron_roll.Factor -> Value","IGGY_FrontTimesCarved.Value -> Value"] | ["Value -> IGGY_MouldingProofRGB.Red","Value -> IGGY_RollRoughnessDelta.Value","Value -> IGGY_RollWarmthFactor.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 3 outgoing links. |
| IGGY_MouldingProofRGB | ShaderNodeCombineColor | {"mode":"RGB"} | [] | ["IGGY_LiveHollowMask.Value -> Green","IGGY_LiveQuietMask.Value -> Blue","IGGY_LiveRollMask.Value -> Red"] | ["Color -> Group_Output.Moulding Proof"] | Expose an isolated acceptance proof lane. | Present in executed Blender manifest; 3 incoming and 1 outgoing links. |
| IGGY_OpenGLBodyNormal64mm | ShaderNodeNormalMap | {"space":"TANGENT","uv_map":""} | [] | ["IGGY_BodyNormal64.Color -> Color","IGGY_PrimaryNormalStrength66Percent.Value -> Strength"] | ["Normal -> IGGY_DecorrelatedBodyHeight91mm.Normal"] | Decode OpenGL RGB into a tangent-space normal. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_PerStoneCoolWash | ShaderNodeMixRGB | {"blend_type":"MIX","use_clamp":false} | [{"default":[0.33716362714767456,0.34191441535949707,0.30054378509521484,1.0],"identifier":"Color2","socket":"Color2","type":"RGBA"}] | ["IGGY_CoolVariantFactor.Value -> Factor","IGGY_PerStoneWarmWash.Color -> Color1"] | ["Color -> IGGY_FossilFragmentColor.Color1"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_PerStoneWarmWash | ShaderNodeMixRGB | {"blend_type":"MIX","use_clamp":false} | [{"default":[0.6514056324958801,0.4125426113605499,0.18116424977779388,1.0],"identifier":"Color2","socket":"Color2","type":"RGBA"}] | ["IGGY_TrimBaseColor.Color -> Color1","IGGY_WarmVariantFactor.Value -> Factor"] | ["Color -> IGGY_PerStoneCoolWash.Color1"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_PhaseLengthNormalized | ShaderNodeMapRange | {"clamp":true,"interpolation_type":"LINEAR"} | [{"default":0.0,"identifier":"From Min","socket":"From Min","type":"VALUE"},{"default":0.75,"identifier":"From Max","socket":"From Max","type":"VALUE"},{"default":0.0,"identifier":"To Min","socket":"To Min","type":"VALUE"},{"default":1.0,"identifier":"To Max","socket":"To Max","type":"VALUE"},{"default":4.0,"identifier":"Steps","socket":"Steps","type":"VALUE"},{"default":[0.0,0.0,0.0],"identifier":"Vector","socket":"Vector","type":"VECTOR"},{"default":[0.0,0.0,0.0],"identifier":"From_Min_FLOAT3","socket":"From Min","type":"VECTOR"},{"default":[1.0,1.0,1.0],"identifier":"From_Max_FLOAT3","socket":"From Max","type":"VECTOR"},{"default":[0.0,0.0,0.0],"identifier":"To_Min_FLOAT3","socket":"To Min","type":"VECTOR"},{"default":[1.0,1.0,1.0],"identifier":"To_Max_FLOAT3","socket":"To Max","type":"VECTOR"},{"default":[4.0,4.0,4.0],"identifier":"Steps_FLOAT3","socket":"Steps","type":"VECTOR"}] | ["IGGY_PhaseVectorLength.Value -> Value"] | ["Result -> IGGY_IdentityProofRGB.Green"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_PhaseVectorLength | ShaderNodeVectorMath | {"operation":"LENGTH"} | [{"default":[0.0,0.0,0.0],"identifier":"Vector_001","socket":"Vector","type":"VECTOR"},{"default":[0.0,0.0,0.0],"identifier":"Vector_002","socket":"Vector","type":"VECTOR"},{"default":1.0,"identifier":"Scale","socket":"Scale","type":"VALUE"}] | ["IGGY_Attribute_iggy_material_phase.Vector -> Vector"] | ["Value -> IGGY_PhaseLengthNormalized.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_PoreColorAmount | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.17000000178813934,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_PoreTimesDetailFade.Value -> Value"] | ["Value -> IGGY_PoreIdentityColor.Factor","Value -> IGGY_PoreRoughnessDelta.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 2 outgoing links. |
| IGGY_PoreIdentityColor | ShaderNodeMixRGB | {"blend_type":"MIX","use_clamp":false} | [{"default":[0.15896083414554596,0.1119324266910553,0.07227185368537903,1.0],"identifier":"Color2","socket":"Color2","type":"RGBA"}] | ["IGGY_PoreColorAmount.Value -> Factor","IGGY_SilicateGrainColor.Color -> Color1"] | ["Color -> IGGY_RollWarmth.Color1"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_PoreRoughnessDelta | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.09000000357627869,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_PoreColorAmount.Value -> Value"] | ["Value -> IGGY_AddPoreRoughness.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_PoreTimesDetailFade | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_DetailFullAt030mZeroAt3p5m.Result -> Value","IGGY_SeparateBodyMasks.Blue -> Value"] | ["Value -> IGGY_PoreColorAmount.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_PrimaryCoordinate_64mm | ShaderNodeVectorMath | {"operation":"SCALE"} | [{"default":[0.0,0.0,0.0],"identifier":"Vector_001","socket":"Vector","type":"VECTOR"},{"default":[0.0,0.0,0.0],"identifier":"Vector_002","socket":"Vector","type":"VECTOR"},{"default":15.625,"identifier":"Scale","socket":"Scale","type":"VALUE"}] | ["IGGY_StoneUV_A.UV -> Vector"] | ["Vector -> IGGY_BodyMasks64.Vector","Vector -> IGGY_BodyNormal64.Vector"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 2 outgoing links. |
| IGGY_PrimaryNormalStrength66Percent | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.6600000262260437,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_DetailFullAt030mZeroAt3p5m.Result -> Value"] | ["Value -> IGGY_OpenGLBodyNormal64mm.Strength"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_RollRoughnessDelta | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":-0.017999999225139618,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_LiveRollMask.Value -> Value"] | ["Value -> IGGY_AddRollRoughness.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_RollWarmth | ShaderNodeMixRGB | {"blend_type":"MIX","use_clamp":false} | [{"default":[0.7011018991470337,0.4910208582878113,0.2581828534603119,1.0],"identifier":"Color2","socket":"Color2","type":"RGBA"}] | ["IGGY_PoreIdentityColor.Color -> Color1","IGGY_RollWarmthFactor.Value -> Factor"] | ["Color -> IGGY_HollowCoolness.Color1"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_RollWarmthFactor | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.1599999964237213,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_LiveRollMask.Value -> Value"] | ["Value -> IGGY_RollWarmth.Factor"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_RoughnessMaximum | ShaderNodeMath | {"operation":"MINIMUM"} | [{"default":0.8999999761581421,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_RoughnessMinimum.Value -> Value"] | ["Value -> Group_Output.Combined Roughness"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_RoughnessMinimum | ShaderNodeMath | {"operation":"MAXIMUM"} | [{"default":0.6200000047683716,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_AddHollowRoughness.Value -> Value"] | ["Value -> IGGY_RoughnessMaximum.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_SecondaryCoordinate_91mm | ShaderNodeVectorMath | {"operation":"SCALE"} | [{"default":[0.0,0.0,0.0],"identifier":"Vector_001","socket":"Vector","type":"VECTOR"},{"default":[0.0,0.0,0.0],"identifier":"Vector_002","socket":"Vector","type":"VECTOR"},{"default":10.98901081085205,"identifier":"Scale","socket":"Scale","type":"VALUE"}] | ["IGGY_StoneUV_B.UV -> Vector"] | ["Vector -> IGGY_BodyHeight91.Vector","Vector -> IGGY_BodyMasks91.Vector"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 2 outgoing links. |
| IGGY_SelectiveChevronInk | ShaderNodeMixRGB | {"blend_type":"MIX","use_clamp":false} | [{"default":[0.09758734703063965,0.07227185368537903,0.052860647439956665,1.0],"identifier":"Color2","socket":"Color2","type":"RGBA"}] | ["IGGY_HollowCoolness.Color -> Color1","IGGY_SelectiveInkAmount.Value -> Factor"] | ["Color -> IGGY_SelectiveCrestHighlight.Color1"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_SelectiveCrestHighlight | ShaderNodeMixRGB | {"blend_type":"MIX","use_clamp":false} | [{"default":[0.7991027235984802,0.6514056324958801,0.43415364623069763,1.0],"identifier":"Color2","socket":"Color2","type":"RGBA"}] | ["IGGY_SelectiveChevronInk.Color -> Color1","IGGY_SelectiveHighlightAmount.Value -> Factor"] | ["Color -> Group_Output.Combined Color"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_SelectiveHighlightAmount | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.14000000059604645,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_HighlightTimesLineFade.Value -> Value"] | ["Value -> IGGY_SelectiveCrestHighlight.Factor"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_SelectiveInkAmount | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.30000001192092896,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_InkTimesLineFade.Value -> Value"] | ["Value -> IGGY_SelectiveChevronInk.Factor"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_SeparateBodyMasks | ShaderNodeSeparateColor | {"mode":"RGB"} | [] | ["IGGY_Blend64mmAnd91mmBodyMasks.Color -> Color"] | ["Blue -> IGGY_PoreTimesDetailFade.Value","Green -> IGGY_SilicateTimesDetailFade.Value","Red -> IGGY_FossilTimesDetailFade.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 3 outgoing links. |
| IGGY_SeparateORM | ShaderNodeSeparateColor | {"mode":"RGB"} | [] | ["IGGY_TrimORM.Color -> Color"] | ["Green -> IGGY_AddPoreRoughness.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_SilicateColorAmount | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.09000000357627869,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_SilicateTimesDetailFade.Value -> Value"] | ["Value -> IGGY_SilicateGrainColor.Factor","Value -> IGGY_SilicateRoughnessDelta.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 2 outgoing links. |
| IGGY_SilicateGrainColor | ShaderNodeMixRGB | {"blend_type":"MIX","use_clamp":false} | [{"default":[0.3049873113632202,0.31854677200317383,0.28744083642959595,1.0],"identifier":"Color2","socket":"Color2","type":"RGBA"}] | ["IGGY_FossilFragmentColor.Color -> Color1","IGGY_SilicateColorAmount.Value -> Factor"] | ["Color -> IGGY_PoreIdentityColor.Color1"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_SilicateRoughnessDelta | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":-0.03999999910593033,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_SilicateColorAmount.Value -> Value"] | ["Value -> IGGY_AddSilicateRoughness.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_SilicateTimesDetailFade | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_DetailFullAt030mZeroAt3p5m.Result -> Value","IGGY_SeparateBodyMasks.Green -> Value"] | ["Value -> IGGY_SilicateColorAmount.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 2 incoming and 1 outgoing links. |
| IGGY_StoneUV_A | ShaderNodeUVMap | {"uv_map":"IGGY_StoneUV_A"} | [] | [] | ["UV -> IGGY_BaseCoordinate_512mm.Vector","UV -> IGGY_PrimaryCoordinate_64mm.Vector"] | Read a metre-authored, stone-local UV layer. | Present in executed Blender manifest; 0 incoming and 2 outgoing links. |
| IGGY_StoneUV_B | ShaderNodeUVMap | {"uv_map":"IGGY_StoneUV_B"} | [] | [] | ["UV -> IGGY_SecondaryCoordinate_91mm.Vector"] | Read a metre-authored, stone-local UV layer. | Present in executed Blender manifest; 0 incoming and 1 outgoing links. |
| IGGY_TrimBaseColor | ShaderNodeTexImage | {"extension":"REPEAT","interpolation":"Linear","projection":"FLAT"} | [] | ["IGGY_BaseCoordinate_512mm.Vector -> Vector"] | ["Color -> IGGY_PerStoneWarmWash.Color1"] | Consume packed `cathedral_stone_trim_fracture_v1_basecolor.png` as sRGB. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_TrimORM | ShaderNodeTexImage | {"extension":"REPEAT","interpolation":"Linear","projection":"FLAT"} | [] | ["IGGY_BaseCoordinate_512mm.Vector -> Vector"] | ["Color -> IGGY_SeparateORM.Color"] | Consume packed `cathedral_stone_trim_fracture_v1_orm.png` as Non-Color. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_VariantNormalized | ShaderNodeMath | {"operation":"DIVIDE"} | [{"default":3.0,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_Attribute_iggy_material_variant.Factor -> Value"] | ["Value -> IGGY_IdentityProofRGB.Blue"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_VariantParity | ShaderNodeMath | {"operation":"MODULO"} | [{"default":2.0,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_Attribute_iggy_material_variant.Factor -> Value"] | ["Value -> IGGY_WarmVariantFactor.Value"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_VoussoirIdNormalized | ShaderNodeMath | {"operation":"DIVIDE"} | [{"default":15.0,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_Attribute_iggy_voussoir_id.Factor -> Value"] | ["Value -> IGGY_IdentityProofRGB.Red"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |
| IGGY_WarmVariantFactor | ShaderNodeMath | {"operation":"MULTIPLY"} | [{"default":0.07000000029802322,"identifier":"Value_001","socket":"Value","type":"VALUE"},{"default":0.5,"identifier":"Value_002","socket":"Value","type":"VALUE"}] | ["IGGY_VariantParity.Value -> Value"] | ["Value -> IGGY_PerStoneWarmWash.Factor"] | Execute the named coordinate, mask, colour, or response operation. | Present in executed Blender manifest; 1 incoming and 1 outgoing links. |

## Complete shader flow

1. `IGGY_StoneUV_A` and `IGGY_StoneUV_B` read per-loop metre values authored from local tangential and radial coordinates before object rotation; bounding-box remapping is forbidden.
2. Four deterministic rotation/mirror variants and two independent seeded phase pairs are baked into the UV frames. `iggy_material_variant`, `iggy_material_phase`, and `iggy_voussoir_id` remain live in colour or identity proof routes.
3. The 0.512 m basecolor and ORM sample UV A; 64 mm masks and normal sample UV A; 91 mm masks and height sample UV B. Basecolor is sRGB. Every data lane is Non-Color.
4. `IGGY_SeparateORM` exposes roughness. `IGGY_SeparateBodyMasks` exposes fossil, silicate, and pore after a 0.34 blend between the incommensurate mask samples.
5. Base colour is layered in this exact order: broad field, per-stone warm wash, per-stone cool wash, fossil colour, silicate colour, pore colour, roll warmth, hollow coolness, selective ink, selective crest highlight.
6. `IGGY_OpenGLBodyNormal64mm` decodes tangent-space RGB at 66 percent of the distance gate. `IGGY_DecorrelatedBodyHeight91mm` receives that normal and adds non-inverted height with 0.00113 x 0.34 m distance.
7. ORM roughness receives bounded pore, silicate, roll, and hollow deltas, then clamps to 0.62–0.90. AO remains one. Metalness remains zero.
8. Roll, hollow, quiet, ink, and highlight masks come from the same signed chevron field that displaced the front mesh; they are multiplied by live front and carved-trim face identities.
9. Ink and highlight are sparse graphic colour layers rather than baked light. Geometry supplies the physical roll/hollow silhouette and occlusion.
10. Body microdetail is full at 0.30 m and zero at 3.50 m. Selective linework has a separate 6.00 m fade so the motif remains readable at gameplay distance.
11. `iggy_fracture_interior` is explicitly false on every face. Damage, wear, soot, damp, lichen, and repair have no nodes and no texture lanes in this capability.
12. The group outputs Combined Color, Combined Roughness, and Combined Normal to a single opaque Principled BSDF, then to Material Output.
13. Unreal reconstruction is deliberately absent. No parity claim can be made until a separate coded demand reproduces the metre frames, identity routes, two-scale relief, and distance gates.

## Workflow, performance, and acceptance contract

| Metric | Executed value | Maximum | Status |
| --- | ---: | ---: | --- |
| total_texture_bytes | 5568072 | 6500000 | green |
| product_objects | 16 | 16 | green |
| total_vertices | 26400 | 30000 | green |
| total_polygons | 26368 | 30000 | green |
| shader_nodes | 79 | 85 | green |
| shader_links | 103 | 110 | green |
| unique_images | 5 | 5 | green |
| proof_count | 9 | 9 | green |

Fast builds support `--proof-set none` and
`--proof-set changed --proof <proof-id>` only with an explicit
non-canonical output path. The canonical output requires
`--proof-set all`. This isolated fixture may reach
`production-candidate`; `accepted` additionally requires a proof
on the named actual target and explicit user review.

## DEM-TEX-001: Material-specific five-lane calcarenite family

### Demand

Generate a deterministic, mortar-free, material-specific texture family with twenty related broad stone shades, constant dielectric ORM limits, and byte-exact inheritance of only the accepted measured-proxy body anatomy.

### Authority

- Colour family: accepted cathedral calcarenite research translated into an authored twenty-shade palette; no source pixels retained.
- Body masks, normal, and height: byte-exact accepted `cathedral_stone_v1` outputs under the Sabucina comparable-calcarenite relief proxy.
- No claim that broad colour, roughness, or authored palette entries are laboratory albedo or optical roughness measurements.
- Ashlar construction, mortar, damage, and photographed lighting are excluded.

### Test code

Target: `tests/unit/cathedral_stone_trim_fracture_v1_generator_tests.py`

```python
#!/usr/bin/env python3
"""Contract tests for the measured chevron-voussoir texture generator."""

from __future__ import annotations

import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct
import sys
import tempfile
import unittest
import zlib

import numpy as np


REPO_ROOT = Path(__file__).resolve().parents[2]
PACKAGE = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "cathedral_stone_trim_fracture_v1"
)
GENERATOR_PATH = PACKAGE / "generate_cathedral_stone_trim_fracture_v1.py"
AUDITOR_PATH = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "workflow"
    / "scripts"
    / "audit_material_package.py"
)
PROFILE_PATH = PACKAGE / "profiles" / "cathedral_stone_trim_fracture_v1.json"
PATTERN_PATH = PACKAGE / "patterns" / "cathedral_trim_fracture_atlas_v1.json"
CORE_OUTPUT = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "cathedral_stone_v1"
    / "output"
)


def load_module() -> object:
    spec = importlib.util.spec_from_file_location(
        "chevron_generator_under_test",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError("Cannot load generator module")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_auditor() -> object:
    spec = importlib.util.spec_from_file_location(
        "material_auditor_under_test",
        AUDITOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError("Cannot load material package auditor")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def read_json(path: Path) -> dict:
    return json.loads(path.read_text())


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def decode_filter_zero_rgb8(path: Path) -> np.ndarray:
    payload = path.read_bytes()
    if payload[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssertionError(f"{path} is not a PNG")
    cursor = 8
    width = height = bit_depth = color_type = None
    compressed = bytearray()
    while cursor < len(payload):
        length = struct.unpack(">I", payload[cursor : cursor + 4])[0]
        chunk_type = payload[cursor + 4 : cursor + 8]
        chunk = payload[cursor + 8 : cursor + 8 + length]
        cursor += 12 + length
        if chunk_type == b"IHDR":
            width, height, bit_depth, color_type = struct.unpack(
                ">IIBB",
                chunk[:10],
            )
        elif chunk_type == b"IDAT":
            compressed.extend(chunk)
        elif chunk_type == b"IEND":
            break
    if (
        width is None
        or height is None
        or bit_depth != 8
        or color_type != 2
    ):
        raise AssertionError("Expected an RGB8 PNG")
    raw = zlib.decompress(bytes(compressed))
    stride = width * 3
    result = np.empty((height, width, 3), dtype=np.uint8)
    for row in range(height):
        start = row * (stride + 1)
        if raw[start] != 0:
            raise AssertionError("Expected filter-zero scanlines")
        result[row] = np.frombuffer(
            raw[start + 1 : start + 1 + stride],
            dtype=np.uint8,
        ).reshape(width, 3)
    return result


def png_header(path: Path) -> tuple[int, int, int, int]:
    payload = path.read_bytes()
    if payload[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssertionError(f"{path} is not a PNG")
    return struct.unpack(">IIBB", payload[16:26])


class ChevronGeneratorContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.generator = load_module()
        cls.auditor = load_auditor()
        cls.profile = read_json(PROFILE_PATH)
        cls.pattern = read_json(PATTERN_PATH)

    def test_source_bounded_voussoir_and_authored_completion_are_explicit(
        self,
    ) -> None:
        source = self.pattern["source_measurement"]
        self.assertEqual(source["source_id"], "S01_old_sarum_catalogue_item_55")
        self.assertEqual(source["catalogue_item"], 55)
        self.assertEqual(
            {
                "height_m": source["height_m"],
                "inner_chord_m": source["inner_chord_m"],
                "outer_chord_m": source["outer_chord_m"],
                "depth_m": source["depth_m"],
            },
            {
                "height_m": 0.2,
                "inner_chord_m": 0.14,
                "outer_chord_m": 0.18,
                "depth_m": 0.27,
            },
        )
        completion = self.pattern["authored_completion"]
        self.assertEqual(completion["count"], 16)
        self.assertAlmostEqual(completion["pitch_angle_deg"], 11.25, places=12)
        self.assertAlmostEqual(
            completion["joint_gap_centerline_m"],
            0.003,
            places=12,
        )
        self.assertIn("solved iteratively", completion["derivation"])

    def test_derived_radius_chords_and_joint_recompute_without_guessing(
        self,
    ) -> None:
        completion = self.pattern["authored_completion"]
        height = self.pattern["source_measurement"]["height_m"]
        body_angle = math.radians(completion["body_angle_deg"])
        pitch_angle = math.radians(completion["pitch_angle_deg"])
        inner_radius = completion["inner_radius_m"]
        outer_radius = completion["outer_radius_m"]
        inner_chord = 2.0 * inner_radius * math.sin(body_angle / 2.0)
        outer_chord = 2.0 * outer_radius * math.sin(body_angle / 2.0)
        mean_radius = inner_radius + height / 2.0
        gap = mean_radius * (pitch_angle - body_angle)
        self.assertAlmostEqual(inner_chord, 0.14, places=10)
        self.assertAlmostEqual(outer_chord, 0.1784852529, places=10)
        self.assertLessEqual(abs(outer_chord - 0.18), 0.005)
        self.assertAlmostEqual(gap, 0.003, places=10)
        self.assertAlmostEqual(outer_radius - inner_radius, 0.2, places=12)

    def test_moulding_is_one_centripetal_roll_hollow_roll_chevron_per_stone(
        self,
    ) -> None:
        moulding = self.pattern["moulding"]
        self.assertEqual(
            moulding["classification"],
            "centripetal lateral face chevron",
        )
        self.assertEqual(moulding["chevrons_per_voussoir"], 1)
        self.assertEqual(
            moulding["profile"],
            "quiet-roll-hollow-roll-quiet",
        )
        self.assertEqual(
            moulding["radial_zone_widths_m"],
            [0.025, 0.05, 0.05, 0.05, 0.025],
        )
        self.assertAlmostEqual(sum(moulding["radial_zone_widths_m"]), 0.2)
        self.assertEqual(moulding["roll_crest_m"], 0.012)
        self.assertEqual(moulding["hollow_depression_m"], 0.006)
        self.assertEqual(moulding["bevel_m"], 0.0)

    def test_first_capability_excludes_all_unproven_damage_and_jambs(
        self,
    ) -> None:
        self.assertEqual(
            set(self.pattern["exclusions"]),
            {
                "jamb reconstruction",
                "fracture",
                "edge damage",
                "wear",
                "weathering",
                "soot",
                "damp",
                "lichen",
                "Unreal parity",
            },
        )
        surface = self.profile["surface_contract"]
        self.assertIn("absent", surface["damage_rule"])
        self.assertEqual(surface["ao_rule"], (
            "Texture AO remains one. Only real geometry may produce "
            "contact occlusion."
        ))

    def test_texture_inventory_has_exact_scale_bit_depth_and_color_space(
        self,
    ) -> None:
        inventory = self.profile["texture_inventory"]
        self.assertEqual(
            set(inventory),
            {
                "basecolor",
                "orm",
                "body_masks",
                "body_normal",
                "body_height",
            },
        )
        for lane, entry in inventory.items():
            self.assertEqual(entry["resolution"], [1024, 1024], lane)
            self.assertAlmostEqual(
                entry["metres_per_texel"],
                entry["physical_span_m"] / 1024,
                places=12,
            )
        self.assertEqual(inventory["basecolor"]["color_space"], "sRGB")
        self.assertEqual(inventory["basecolor"]["bit_depth"], 8)
        self.assertEqual(inventory["body_height"]["bit_depth"], 16)
        for lane in ("orm", "body_masks", "body_normal", "body_height"):
            self.assertEqual(inventory[lane]["color_space"], "Non-Color")

    def test_generated_family_is_deterministic_and_preserves_core_body_bytes(
        self,
    ) -> None:
        core_files = {
            lane: CORE_OUTPUT / f"cathedral_stone_v1_stone_{lane}.png"
            for lane in ("body_masks", "body_normal", "body_height")
        }
        core_hashes_before = {
            lane: sha256(path) for lane, path in core_files.items()
        }
        with tempfile.TemporaryDirectory() as first_dir, tempfile.TemporaryDirectory() as second_dir:
            first = Path(first_dir)
            second = Path(second_dir)
            manifest_a = self.generator.generate(
                PROFILE_PATH,
                PATTERN_PATH,
                first,
            )
            manifest_b = self.generator.generate(
                PROFILE_PATH,
                PATTERN_PATH,
                second,
            )
            self.assertEqual(manifest_a["schema"], manifest_b["schema"])
            self.assertEqual(
                {
                    lane: entry["sha256"]
                    for lane, entry in manifest_a["files"].items()
                },
                {
                    lane: entry["sha256"]
                    for lane, entry in manifest_b["files"].items()
                },
            )
            for lane, core_hash in core_hashes_before.items():
                self.assertEqual(
                    sha256(
                        first
                        / f"cathedral_stone_trim_fracture_v1_{lane}.png"
                    ),
                    core_hash,
                )
        self.assertEqual(
            {
                lane: sha256(path) for lane, path in core_files.items()
            },
            core_hashes_before,
        )

    def test_basecolor_contains_broad_related_variation_not_baked_light(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            self.generator.generate(PROFILE_PATH, PATTERN_PATH, output)
            image = decode_filter_zero_rgb8(
                output / "cathedral_stone_trim_fracture_v1_basecolor.png"
            )
        luminance = (
            image[..., 0].astype(np.float32) * 0.2126
            + image[..., 1].astype(np.float32) * 0.7152
            + image[..., 2].astype(np.float32) * 0.0722
        )
        unique = np.unique(image.reshape(-1, 3), axis=0)
        self.assertGreater(len(unique), 400)
        self.assertGreater(float(luminance.std()), 4.0)
        self.assertLess(float(luminance.max() - luminance.min()), 75.0)
        self.assertGreaterEqual(int(image.min()), 80)
        self.assertLessEqual(int(image.max()), 230)

    def test_orm_is_dielectric_with_quiet_bounded_roughness(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            self.generator.generate(PROFILE_PATH, PATTERN_PATH, output)
            orm = decode_filter_zero_rgb8(
                output / "cathedral_stone_trim_fracture_v1_orm.png"
            )
        self.assertTrue(np.all(orm[..., 0] == 255))
        self.assertTrue(np.all(orm[..., 2] == 0))
        roughness = orm[..., 1].astype(np.float32) / 255.0
        self.assertGreaterEqual(float(roughness.min()), 0.66 - 1 / 255)
        self.assertLessEqual(float(roughness.max()), 0.82 + 1 / 255)
        self.assertGreater(float(roughness.std()), 0.005)

    def test_output_png_headers_match_declared_lane_formats(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            self.generator.generate(PROFILE_PATH, PATTERN_PATH, output)
            base = png_header(
                output / "cathedral_stone_trim_fracture_v1_basecolor.png"
            )
            orm = png_header(
                output / "cathedral_stone_trim_fracture_v1_orm.png"
            )
            masks = png_header(
                output / "cathedral_stone_trim_fracture_v1_body_masks.png"
            )
            normal = png_header(
                output / "cathedral_stone_trim_fracture_v1_body_normal.png"
            )
            height = png_header(
                output / "cathedral_stone_trim_fracture_v1_body_height.png"
            )
        self.assertEqual(base, (1024, 1024, 8, 2))
        self.assertEqual(orm, (1024, 1024, 8, 2))
        self.assertEqual(masks, (1024, 1024, 8, 2))
        self.assertEqual(normal, (1024, 1024, 8, 2))
        self.assertEqual(height, (1024, 1024, 16, 0))

    def test_workflow_claim_ledger_covers_every_numeric_claim_root(self) -> None:
        contract = self.profile["workflow_contract"]
        passed, detail = self.auditor.check_measurement_claims(
            contract,
            {"profile": self.profile, "pattern": self.pattern},
        )
        self.assertTrue(passed, detail["errors"])
        self.assertEqual(contract["tier"], "hero-master")
        self.assertEqual(
            contract["delivery_state"],
            "production-candidate",
        )
        self.assertGreaterEqual(detail["claim_count"], 30)

    def test_workflow_budget_and_acceptance_boundary_are_explicit(self) -> None:
        contract = self.profile["workflow_contract"]
        budget = contract["performance_budget"]
        self.assertEqual(budget["max_product_objects"], 16)
        self.assertEqual(budget["max_unique_images"], 5)
        self.assertEqual(budget["max_proof_count"], 9)
        self.assertTrue(
            contract["acceptance"][
                "actual_target_required_for_acceptance"
            ]
        )
        self.assertEqual(
            contract["acceptance"]["isolated_fixture_can_reach"],
            "production-candidate",
        )


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
```

### Profile code

Target: `assets/creative/materials/cathedral_stone_trim_fracture_v1/profiles/cathedral_stone_trim_fracture_v1.json`

```json
{
  "schema": "iggy3d.material.cathedral_stone_trim_fracture_v1.profile.v2",
  "profile_id": "cathedral_stone_trim_fracture_v1",
  "display_name": "Measured Old Sarum chevron-voussoir portal order",
  "intent": "One intact source-bounded row of individual roll-hollow-roll chevron voussoirs, carrying the accepted cathedral calcarenite body anatomy without ashlar mortar, fracture, wear, weathering, or invented arris rounding.",
  "seed": 41723,
  "workflow_contract": {
    "tier": "hero-master",
    "delivery_state": "production-candidate",
    "storage_only_attributes": [
      "iggy_fracture_interior"
    ],
    "claim_roots": [
      {
        "document": "profile",
        "pointer": "/construction_contract"
      },
      {
        "document": "profile",
        "pointer": "/moulding_contract"
      },
      {
        "document": "profile",
        "pointer": "/surface_contract"
      }
    ],
    "measurement_claims": [
      {
        "id": "catalogue_height",
        "document": "profile",
        "pointer": "/construction_contract/catalogue_height_m",
        "value": 0.2,
        "status": "measured",
        "source_id": "S01_old_sarum_catalogue_item_55"
      },
      {
        "id": "catalogue_inner_chord",
        "document": "profile",
        "pointer": "/construction_contract/catalogue_inner_chord_m",
        "value": 0.14,
        "status": "measured",
        "source_id": "S01_old_sarum_catalogue_item_55"
      },
      {
        "id": "catalogue_outer_chord",
        "document": "profile",
        "pointer": "/construction_contract/catalogue_outer_chord_m",
        "value": 0.18,
        "status": "measured",
        "source_id": "S01_old_sarum_catalogue_item_55"
      },
      {
        "id": "catalogue_depth",
        "document": "profile",
        "pointer": "/construction_contract/catalogue_depth_m",
        "value": 0.27,
        "status": "measured",
        "source_id": "S01_old_sarum_catalogue_item_55"
      },
      {
        "id": "catalogue_width_tolerance",
        "document": "profile",
        "pointer": "/construction_contract/catalogue_width_tolerance_m",
        "value": 0.005,
        "status": "authored",
        "reason": "Explicit acceptance tolerance for comparing the derived outer chord to the rounded catalogue dimension."
      },
      {
        "id": "authored_arch_count",
        "document": "profile",
        "pointer": "/construction_contract/count",
        "value": 16,
        "status": "authored",
        "reason": "A complete semicircle needs an explicit reusable count; S01 does not publish the original order."
      },
      {
        "id": "authored_pitch",
        "document": "profile",
        "pointer": "/construction_contract/pitch_angle_deg",
        "value": 11.25,
        "status": "authored",
        "reason": "Derived from sixteen equal pitches over 180 degrees."
      },
      {
        "id": "inherited_joint",
        "document": "profile",
        "pointer": "/construction_contract/joint_gap_centerline_m",
        "value": 0.003,
        "status": "inherited",
        "source_id": "cathedral_stone_v1_accepted_3mm_joint_contract"
      },
      {
        "id": "derived_body_angle",
        "document": "profile",
        "pointer": "/construction_contract/body_angle_deg",
        "value": 11.042294066,
        "status": "authored",
        "reason": "Solved from the authored pitch, inherited centreline joint, and measured inner chord."
      },
      {
        "id": "derived_radii",
        "document": "profile",
        "pointer": "/construction_contract/inner_radius_m",
        "value": 0.7275514105,
        "status": "authored",
        "reason": "Closure solution; S01 does not publish an arch radius."
      },
      {
        "id": "derived_outer_radius",
        "document": "profile",
        "pointer": "/construction_contract/outer_radius_m",
        "value": 0.9275514105,
        "status": "authored",
        "reason": "Inner radius plus measured 0.200 m radial height."
      },
      {
        "id": "derived_outer_chord",
        "document": "profile",
        "pointer": "/construction_contract/derived_outer_chord_m",
        "value": 0.1784852529,
        "status": "authored",
        "reason": "Closure result checked against the measured 0.180 m rounded catalogue value."
      },
      {
        "id": "derived_clear_diameter",
        "document": "profile",
        "pointer": "/construction_contract/clear_diameter_m",
        "value": 1.455102821,
        "status": "authored",
        "reason": "Twice the authored closure radius."
      },
      {
        "id": "derived_outer_diameter",
        "document": "profile",
        "pointer": "/construction_contract/outer_diameter_m",
        "value": 1.855102821,
        "status": "authored",
        "reason": "Twice the authored outer radius."
      },
      {
        "id": "authored_moulding_zones",
        "document": "profile",
        "pointer": "/moulding_contract/radial_zone_widths_m",
        "value": [0.025, 0.05, 0.05, 0.05, 0.025],
        "status": "authored",
        "reason": "Replaceable roll-hollow-roll section because S01 publishes the sequence but no section drawing."
      },
      {
        "id": "authored_roll_crest",
        "document": "profile",
        "pointer": "/moulding_contract/roll_crest_m",
        "value": 0.012,
        "status": "authored",
        "reason": "Replaceable readable relief; not a surveyed Old Sarum value."
      },
      {
        "id": "authored_hollow_depth",
        "document": "profile",
        "pointer": "/moulding_contract/hollow_depression_m",
        "value": 0.006,
        "status": "authored",
        "reason": "Replaceable readable hollow; not a surveyed Old Sarum value."
      },
      {
        "id": "authored_chevron_path",
        "document": "profile",
        "pointer": "/moulding_contract/chevron_apex_v",
        "value": 0.24,
        "status": "authored",
        "reason": "Places the centripetal apex while allowing the paired rolls to enter through the intrados."
      },
      {
        "id": "authored_chevron_shoulder",
        "document": "profile",
        "pointer": "/moulding_contract/chevron_shoulder_v",
        "value": 0.76,
        "status": "authored",
        "reason": "Completes one lateral chevron across the measured stone face."
      },
      {
        "id": "authored_band_width",
        "document": "profile",
        "pointer": "/moulding_contract/band_half_width_m",
        "value": 0.025,
        "status": "authored",
        "reason": "Matches the authored 50 mm roll and hollow zones."
      },
      {
        "id": "authored_roll_offsets",
        "document": "profile",
        "pointer": "/moulding_contract/roll_center_offsets_m",
        "value": [-0.05, 0.05],
        "status": "authored",
        "reason": "Centers the paired rolls around the hollow."
      },
      {
        "id": "authored_hollow_offset",
        "document": "profile",
        "pointer": "/moulding_contract/hollow_center_offset_m",
        "value": 0.0,
        "status": "authored",
        "reason": "Defines the hollow as the signed chevron centreline."
      },
      {
        "id": "authored_broad_colour_span",
        "document": "profile",
        "pointer": "/surface_contract/broad_color_span_m",
        "value": 0.512,
        "status": "authored",
        "reason": "Broad color decorrelation scale chosen to cover several stones without importing ashlar mortar."
      },
      {
        "id": "authored_shade_count",
        "document": "profile",
        "pointer": "/surface_contract/broad_color_shade_count",
        "value": 20,
        "status": "authored",
        "reason": "Explicit within-element palette resolution required by the material intent."
      },
      {
        "id": "inherited_body_spans",
        "document": "profile",
        "pointer": "/surface_contract/primary_body_span_m",
        "value": 0.064,
        "status": "inherited",
        "source_id": "cathedral_stone_v1_accepted_body_span"
      },
      {
        "id": "inherited_secondary_span",
        "document": "profile",
        "pointer": "/surface_contract/secondary_body_span_m",
        "value": 0.091,
        "status": "inherited",
        "source_id": "cathedral_stone_v1_accepted_decorrelation_span"
      },
      {
        "id": "authored_normal_share",
        "document": "profile",
        "pointer": "/surface_contract/primary_body_normal_strength_share",
        "value": 0.66,
        "status": "authored",
        "reason": "Splits the inherited proxy relief between primary normal and secondary bump without doubling amplitude."
      },
      {
        "id": "authored_height_share",
        "document": "profile",
        "pointer": "/surface_contract/secondary_body_height_strength_share",
        "value": 0.34,
        "status": "authored",
        "reason": "Complements the 0.66 primary share and decorrelates the secondary height."
      },
      {
        "id": "proxy_relief_range",
        "document": "profile",
        "pointer": "/surface_contract/body_relief_range_m",
        "value": 0.00113,
        "status": "proxy",
        "source_id": "sabucina_calcarenite_surface_topography_proxy"
      },
      {
        "id": "inherited_detail_distances",
        "document": "profile",
        "pointer": "/surface_contract/detail_full_distance_m",
        "value": 0.3,
        "status": "inherited",
        "source_id": "cathedral_stone_v1_accepted_distance_hierarchy"
      },
      {
        "id": "inherited_detail_zero",
        "document": "profile",
        "pointer": "/surface_contract/detail_zero_distance_m",
        "value": 3.5,
        "status": "inherited",
        "source_id": "cathedral_stone_v1_accepted_distance_hierarchy"
      },
      {
        "id": "dielectric_metallic",
        "document": "profile",
        "pointer": "/surface_contract/metallic",
        "value": 0.0,
        "status": "authored",
        "reason": "Opaque limestone is modeled as a nonmetal dielectric."
      },
      {
        "id": "opaque_surface",
        "document": "profile",
        "pointer": "/surface_contract/opacity",
        "value": 1.0,
        "status": "authored",
        "reason": "Intact architectural limestone is opaque."
      }
    ],
    "layer_provenance": [
      {
        "id": "construction",
        "physical_meaning": "Sixteen separate closed wedge-shaped voussoirs form the semicircular order with real centerline joints.",
        "sources": [
          "S01_old_sarum_catalogue_item_55",
          "patterns/cathedral_trim_fracture_atlas_v1.json"
        ],
        "outputs": [
          "voussoir mesh volumes",
          "iggy_voussoir_id",
          "IGGY_StoneUV_A",
          "IGGY_StoneUV_B"
        ],
        "consumers": [
          "Blender product collection",
          "IGGY_SH_ChevronVoussoir_v002"
        ],
        "proof_ids": [
          "neutral_clay_front",
          "wireframe_proof"
        ],
        "rest_rule": "No surface texture is allowed to draw joints or repair a weak arch silhouette.",
        "claim_ids": [
          "catalogue_height",
          "catalogue_inner_chord",
          "catalogue_outer_chord",
          "catalogue_depth",
          "authored_arch_count",
          "inherited_joint",
          "derived_radii",
          "derived_outer_radius"
        ]
      },
      {
        "id": "macro",
        "physical_meaning": "Broad quiet calcarenite value fields and bounded warm-cool changes prevent one flat colour per stone.",
        "sources": [
          "trim-specific deterministic broad colour compiler",
          "surface_contract broad colour palette"
        ],
        "outputs": [
          "cathedral_stone_trim_fracture_v1_basecolor.png",
          "per-stone warm-cool variant"
        ],
        "consumers": [
          "IGGY_TrimBaseColor",
          "IGGY_Attribute_iggy_material_variant"
        ],
        "proof_ids": [
          "live_material_front",
          "distance_read"
        ],
        "rest_rule": "Broad fields retain quiet spans and may not contain mortar, photographed lighting, or universal cloud noise.",
        "claim_ids": [
          "authored_broad_colour_span",
          "authored_shade_count"
        ]
      },
      {
        "id": "medium",
        "physical_meaning": "One phase-locked lateral chevron with roll-hollow-roll moulding is sampled into the front geometry of each stone.",
        "sources": [
          "S03_crsbi_chevron_guide",
          "authored signed chevron field"
        ],
        "outputs": [
          "front relief geometry",
          "iggy_chevron_roll",
          "iggy_chevron_hollow",
          "iggy_chevron_quiet"
        ],
        "consumers": [
          "voussoir front mesh",
          "live moulding colour and roughness"
        ],
        "proof_ids": [
          "moulding_proof",
          "measured_close"
        ],
        "rest_rule": "The two 25 mm quiet zones remain free of roll and hollow displacement.",
        "claim_ids": [
          "authored_moulding_zones",
          "authored_roll_crest",
          "authored_hollow_depth",
          "authored_chevron_path",
          "authored_chevron_shoulder",
          "authored_band_width"
        ]
      },
      {
        "id": "edge_event",
        "physical_meaning": "Real joints, selected hollow ink, and selected crest highlights articulate construction without outlining every edge.",
        "sources": [
          "voussoir boundaries",
          "signed chevron derivatives"
        ],
        "outputs": [
          "physical joint gaps",
          "iggy_chevron_ink",
          "iggy_chevron_highlight"
        ],
        "consumers": [
          "arch silhouette and occlusion",
          "selective graphic linework"
        ],
        "proof_ids": [
          "live_material_grazing",
          "identity_proof"
        ],
        "rest_rule": "Quiet faces and non-selected crests receive no universal ink or highlight.",
        "claim_ids": [
          "inherited_joint",
          "authored_roll_offsets",
          "authored_hollow_offset"
        ]
      },
      {
        "id": "micro",
        "physical_meaning": "Measured-proxy fossil, silicate, pore, normal, and height identities supply close calcarenite response.",
        "sources": [
          "cathedral_stone_v1 accepted body masks",
          "cathedral_stone_v1 accepted body normal",
          "cathedral_stone_v1 accepted body height"
        ],
        "outputs": [
          "cathedral_stone_trim_fracture_v1_body_masks.png",
          "cathedral_stone_trim_fracture_v1_body_normal.png",
          "cathedral_stone_trim_fracture_v1_body_height.png"
        ],
        "consumers": [
          "IGGY_BodyMasks64",
          "IGGY_BodyMasks91",
          "IGGY_BodyNormal64",
          "IGGY_BodyHeight91"
        ],
        "proof_ids": [
          "measured_close"
        ],
        "rest_rule": "Microdetail fades from full at 0.30 m to absent at 3.50 m and cannot replace the moulding.",
        "claim_ids": [
          "inherited_body_spans",
          "inherited_secondary_span",
          "inherited_detail_distances",
          "inherited_detail_zero"
        ]
      },
      {
        "id": "cumulative_color",
        "physical_meaning": "Broad colour, per-stone variation, body identities, moulding values, and selective linework accumulate in a fixed causal order.",
        "sources": [
          "basecolor lane",
          "body mask lane",
          "moulding attributes",
          "stone variant attribute"
        ],
        "outputs": [
          "final unlit intrinsic stone base colour"
        ],
        "consumers": [
          "Principled BSDF Base Color"
        ],
        "proof_ids": [
          "live_material_front",
          "identity_proof"
        ],
        "rest_rule": "Intrinsic colour excludes AO, scene shadow, damage, damp, lichen, soot, and generic grime.",
        "claim_ids": [
          "authored_broad_colour_span",
          "authored_shade_count"
        ]
      },
      {
        "id": "height_normal",
        "physical_meaning": "Primary 64 mm tangent normal and decorrelated 91 mm height contribute measured-proxy body relief beneath real moulding geometry.",
        "sources": [
          "body normal lane",
          "body height lane"
        ],
        "outputs": [
          "combined Principled BSDF normal input"
        ],
        "consumers": [
          "IGGY_BodyNormal64Node",
          "IGGY_DecorrelatedBodyHeight91mm"
        ],
        "proof_ids": [
          "measured_close",
          "live_material_grazing"
        ],
        "rest_rule": "The 1.13 mm proxy range affects shading only and makes no silhouette or deep-fracture claim.",
        "claim_ids": [
          "authored_normal_share",
          "authored_height_share",
          "proxy_relief_range"
        ]
      },
      {
        "id": "response",
        "physical_meaning": "Broad porous calcarenite roughness is modified by body and moulding cause while AO remains one and metalness remains zero.",
        "sources": [
          "cathedral_stone_trim_fracture_v1_orm.png",
          "body masks",
          "roll and hollow attributes"
        ],
        "outputs": [
          "final roughness",
          "AO equals one",
          "metalness equals zero",
          "opacity equals one"
        ],
        "consumers": [
          "Principled BSDF Roughness",
          "Principled BSDF Metallic",
          "Principled BSDF Alpha"
        ],
        "proof_ids": [
          "live_material_grazing"
        ],
        "rest_rule": "Roughness is not inverted height, and no fake cavity AO is baked into colour.",
        "claim_ids": [
          "dielectric_metallic",
          "opaque_surface"
        ]
      },
      {
        "id": "stylization",
        "physical_meaning": "Structure-selected ink and highlights preserve the chevron value rhythm after body microdetail fades.",
        "sources": [
          "iggy_chevron_ink",
          "iggy_chevron_highlight",
          "camera distance"
        ],
        "outputs": [
          "selective ink colour",
          "selective crest highlight"
        ],
        "consumers": [
          "final base-colour composition"
        ],
        "proof_ids": [
          "distance_read",
          "moulding_proof"
        ],
        "rest_rule": "Linework is absent from unselected faces and does not encode scene lighting.",
        "claim_ids": []
      }
    ],
    "performance_budget": {
      "max_total_texture_bytes": 6500000,
      "max_product_objects": 16,
      "max_total_vertices": 30000,
      "max_total_polygons": 30000,
      "max_shader_nodes": 85,
      "max_shader_links": 110,
      "max_unique_images": 5,
      "max_proof_count": 9
    },
    "acceptance": {
      "actual_target_required_for_acceptance": true,
      "actual_target_identifier": "pending-selection: production cathedral portal consumer",
      "isolated_fixture_can_reach": "production-candidate",
      "user_review_required_for_acceptance": true
    },
    "fast_build": {
      "canonical_output_requires_proof_set": "all",
      "temporary_modes": ["none", "changed"],
      "changed_mode_requires_explicit_proof_ids": true
    }
  },
  "source_policy": {
    "ai_generated_reference_capture": false,
    "raw_reference_pixels_used_as_runtime_texture": false,
    "reference_pixels_retained_in_repository": false,
    "measured_voussoir_source": "S01_old_sarum_catalogue_item_55",
    "chevron_typology_source": "S03_crsbi_chevron_guide",
    "comparative_tooling_source": "S02_gloucester_cathedral_archaeology",
    "material_body_source": "cathedral_stone_v1 accepted measured-proxy body lanes",
    "authored_completion_rule": "Radius, count, joint width, and roll-hollow-roll section are labeled authored translations because S01 does not publish them."
  },
  "construction_contract": {
    "catalogue_height_m": 0.2,
    "catalogue_inner_chord_m": 0.14,
    "catalogue_outer_chord_m": 0.18,
    "catalogue_depth_m": 0.27,
    "catalogue_width_tolerance_m": 0.005,
    "count": 16,
    "pitch_angle_deg": 11.25,
    "joint_gap_centerline_m": 0.003,
    "body_angle_deg": 11.042294066,
    "inner_radius_m": 0.7275514105,
    "outer_radius_m": 0.9275514105,
    "derived_outer_chord_m": 0.1784852529,
    "clear_diameter_m": 1.455102821,
    "outer_diameter_m": 1.855102821,
    "geometry_rule": "Sixteen separate closed wedge meshes; no connected annular ribbon, Boolean segmentation, global bevel, or inferred jamb reconstruction.",
    "joint_rule": "The three-millimetre working gap is measured at the stone centreline radius, is empty space between adjacent stone bodies, and remains visible under neutral clay. The resulting intrados gap is approximately 2.64 mm.",
    "arris_rule": "Unknown in the measured source; no bevel is authored."
  },
  "moulding_contract": {
    "classification": "one centripetal lateral face chevron per voussoir",
    "profile_sequence": "quiet-roll-hollow-roll-quiet",
    "radial_zone_widths_m": [0.025, 0.05, 0.05, 0.05, 0.025],
    "roll_crest_m": 0.012,
    "hollow_depression_m": 0.006,
    "chevron_apex_v": 0.24,
    "chevron_shoulder_v": 0.76,
    "band_half_width_m": 0.025,
    "roll_center_offsets_m": [-0.05, 0.05],
    "hollow_center_offset_m": 0.0,
    "scope_note": "The exact profile dimensions are replaceable authored translation, not a claim about the missing Old Sarum section drawing."
  },
  "coordinate_contract": {
    "origin": "arch centre at X=0, springing line Z=0, wall plane Y=0",
    "stone_parameter_u": "tangential position normalized -1 to 1 across one voussoir",
    "stone_parameter_v": "radial position normalized 0 at intrados and 1 at extrados",
    "stone_parameter_w": "front-to-back depth in metres",
    "uv_a": "IGGY_StoneUV_A stores transformed local tangential and radial metres",
    "uv_b": "IGGY_StoneUV_B stores an independently transformed local tangential and radial metre frame",
    "scale_rule": "UV values are authored from measured local coordinates and may not be rebuilt from a post-transform bounding box.",
    "transform_rule": "Object scale must remain (1,1,1); non-uniform scale is a validation failure."
  },
  "identity_contract": {
    "point_attributes": [
      "iggy_voussoir_id",
      "iggy_material_phase",
      "iggy_material_variant",
      "iggy_chevron_roll",
      "iggy_chevron_hollow",
      "iggy_chevron_quiet",
      "iggy_chevron_ink",
      "iggy_chevron_highlight"
    ],
    "face_attributes": [
      "iggy_carved_trim",
      "iggy_chevron_front",
      "iggy_fracture_interior"
    ],
    "fracture_default": 0,
    "damage_default": 0,
    "semantic_rule": "Front-face pattern masks are authored from the same signed chevron field that displaces the mesh. Side and rear vertices carry zero pattern masks. New or cut faces may never inherit fracture identity by triangle interpolation."
  },
  "surface_contract": {
    "broad_color_span_m": 0.512,
    "broad_color_shade_count": 20,
    "primary_body_span_m": 0.064,
    "secondary_body_span_m": 0.091,
    "primary_body_normal_strength_share": 0.66,
    "secondary_body_height_strength_share": 0.34,
    "body_relief_range_m": 0.00113,
    "detail_full_distance_m": 0.3,
    "detail_zero_distance_m": 3.5,
    "base_color_rule": "Twenty related warm, mid, pale, and cool calcarenite shades form broad blended fields inside every stone. No ashlar course, mortar, photographed light, generic cloud noise, or one-colour block is permitted.",
    "normal_rule": "OpenGL body normal is consumed through a Normal Map node; a decorrelated 91 mm height sample adds only 34 percent of the proxy-bounded range through a Bump node.",
    "roughness_rule": "Broad calibrated ORM roughness is modified only by live pore, silicate, roll, and hollow evidence.",
    "ao_rule": "Texture AO remains one. Only real geometry may produce contact occlusion.",
    "metallic": 0.0,
    "opacity": 1.0,
    "damage_rule": "Fracture, chips, checks, soot, damp, lichen, wear, and repair are absent from this capability."
  },
  "texture_inventory": {
    "basecolor": {
      "filename": "cathedral_stone_trim_fracture_v1_basecolor.png",
      "resolution": [1024, 1024],
      "physical_span_m": 0.512,
      "metres_per_texel": 0.0005,
      "bit_depth": 8,
      "color_space": "sRGB",
      "channels": "RGB intrinsic broad twenty-shade calcarenite colour"
    },
    "orm": {
      "filename": "cathedral_stone_trim_fracture_v1_orm.png",
      "resolution": [1024, 1024],
      "physical_span_m": 0.512,
      "metres_per_texel": 0.0005,
      "bit_depth": 8,
      "color_space": "Non-Color",
      "channels": "R ambient-occlusion=1, G roughness, B metallic=0"
    },
    "body_masks": {
      "filename": "cathedral_stone_trim_fracture_v1_body_masks.png",
      "resolution": [1024, 1024],
      "physical_span_m": 0.064,
      "metres_per_texel": 0.0000625,
      "bit_depth": 8,
      "color_space": "Non-Color",
      "channels": "R fossil fragment, G silicate grain, B visible pore identity"
    },
    "body_normal": {
      "filename": "cathedral_stone_trim_fracture_v1_body_normal.png",
      "resolution": [1024, 1024],
      "physical_span_m": 0.064,
      "metres_per_texel": 0.0000625,
      "bit_depth": 8,
      "color_space": "Non-Color",
      "channels": "OpenGL XYZ"
    },
    "body_height": {
      "filename": "cathedral_stone_trim_fracture_v1_body_height.png",
      "resolution": [1024, 1024],
      "physical_span_m": 0.064,
      "metres_per_texel": 0.0000625,
      "bit_depth": 16,
      "color_space": "Non-Color",
      "channels": "normalized height with 0.00113 m declared range"
    }
  },
  "shader_contract": {
    "node_group": "IGGY_SH_ChevronVoussoir_v002",
    "material": "IGGY_MAT_ChevronVoussoir_v002",
    "shading_model": "opaque Principled dielectric",
    "normal_map_space": "TANGENT",
    "live_color_order": [
      "broad basecolor",
      "per-stone warm/cool variant",
      "distance-faded fossil, silicate, and pore colour",
      "roll warmth",
      "hollow coolness",
      "selective chevron ink",
      "selective crest highlight"
    ],
    "proof_outputs": [
      "Combined Color",
      "Combined Roughness",
      "Combined Normal",
      "Moulding Proof",
      "Identity Proof",
      "Detail Distance Fade"
    ],
    "forbidden": [
      "noise nodes used as universal detail",
      "dormant image nodes",
      "dormant semantic attributes",
      "normal RGB connected directly to Principled Normal",
      "height inversion",
      "baked lighting in basecolor",
      "one flat decorative ribbon"
    ]
  }
}
```

### Pattern code

Target: `assets/creative/materials/cathedral_stone_trim_fracture_v1/patterns/cathedral_trim_fracture_atlas_v1.json`

```json
{
  "schema": "iggy3d.pattern.chevron_voussoir_portal.v2",
  "pattern_id": "old_sarum_item_55_chevron_order_v2",
  "seed": 41723,
  "source_measurement": {
    "source_id": "S01_old_sarum_catalogue_item_55",
    "catalogue_item": 55,
    "height_m": 0.2,
    "inner_chord_m": 0.14,
    "outer_chord_m": 0.18,
    "depth_m": 0.27
  },
  "authored_completion": {
    "count": 16,
    "pitch_angle_deg": 11.25,
    "joint_gap_centerline_m": 0.003,
    "body_angle_deg": 11.042294066,
    "inner_radius_m": 0.7275514105,
    "outer_radius_m": 0.9275514105,
    "derived_outer_chord_m": 0.1784852529,
    "clear_diameter_m": 1.455102821,
    "outer_diameter_m": 1.855102821,
    "derivation": "body_angle=pitch-joint_gap_centerline/(inner_radius+height/2); inner_radius=inner_chord/(2*sin(body_angle/2)); solved iteratively"
  },
  "moulding": {
    "classification": "centripetal lateral face chevron",
    "chevrons_per_voussoir": 1,
    "profile": "quiet-roll-hollow-roll-quiet",
    "radial_zone_widths_m": [0.025, 0.05, 0.05, 0.05, 0.025],
    "roll_crest_m": 0.012,
    "hollow_depression_m": 0.006,
    "band_half_width_m": 0.025,
    "roll_center_offsets_m": [-0.05, 0.05],
    "hollow_center_offset_m": 0.0,
    "chevron_line": "v_m = 0.048 + 0.104 * abs(u)",
    "front_segments": {
      "tangential": 32,
      "radial": 24
    },
    "bevel_m": 0.0
  },
  "material_variation": {
    "variants": 4,
    "uv_transforms": [
      {"quarter_turn": 0, "mirror_u": false, "mirror_v": false},
      {"quarter_turn": 1, "mirror_u": true, "mirror_v": false},
      {"quarter_turn": 2, "mirror_u": false, "mirror_v": true},
      {"quarter_turn": 3, "mirror_u": true, "mirror_v": true}
    ],
    "phase_rule": "seeded per-voussoir offsets are stored in metres and applied before texture scaling",
    "no_square_tile_rule": "IGGY_StoneUV_A and IGGY_StoneUV_B use different transforms and phases; 64 mm and 91 mm samples are incommensurate."
  },
  "render_contract": {
    "resolution": [900, 900],
    "engine": "BLENDER_EEVEE_NEXT",
    "transparent_background": false,
    "views": [
      "neutral_clay_front",
      "neutral_clay_grazing",
      "live_material_front",
      "live_material_grazing",
      "measured_close",
      "distance_read",
      "identity_proof",
      "wireframe_proof"
    ],
    "reopen_required": true
  },
  "exclusions": [
    "jamb reconstruction",
    "fracture",
    "edge damage",
    "wear",
    "weathering",
    "soot",
    "damp",
    "lichen",
    "Unreal parity"
  ]
}
```

### Generator code

Target: `assets/creative/materials/cathedral_stone_trim_fracture_v1/generate_cathedral_stone_trim_fracture_v1.py`

```python
#!/usr/bin/env python3
"""Generate the measured chevron-voussoir material texture family."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys
from typing import Any

import numpy as np


SCRIPT_ROOT = Path(__file__).resolve().parent
MATERIALS_ROOT = SCRIPT_ROOT.parent
if str(MATERIALS_ROOT) not in sys.path:
    sys.path.insert(0, str(MATERIALS_ROOT))

from pattern_lab_common import (  # noqa: E402
    normalized_range,
    periodic_fbm_rect,
    periodic_gaussian_blur,
    write_png_rgb8,
)


PROFILE_SCHEMA = "iggy3d.material.cathedral_stone_trim_fracture_v1.profile.v2"
PATTERN_SCHEMA = "iggy3d.pattern.chevron_voussoir_portal.v2"
MANIFEST_SCHEMA = "iggy3d.material.cathedral_stone_trim_fracture_v1.manifest.v2"
DEFAULT_PROFILE = SCRIPT_ROOT / "profiles" / "cathedral_stone_trim_fracture_v1.json"
DEFAULT_PATTERN = SCRIPT_ROOT / "patterns" / "cathedral_trim_fracture_atlas_v1.json"
DEFAULT_OUTPUT = SCRIPT_ROOT / "output"
CORE_OUTPUT = MATERIALS_ROOT / "cathedral_stone_v1" / "output"
CORE_MANIFEST = CORE_OUTPUT / "cathedral_stone_v1_manifest.json"
RESOLUTION = 1024


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text())


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_contract(
    profile: dict[str, Any],
    pattern: dict[str, Any],
) -> None:
    if profile.get("schema") != PROFILE_SCHEMA:
        raise ValueError("Unexpected chevron material profile schema")
    if pattern.get("schema") != PATTERN_SCHEMA:
        raise ValueError("Unexpected chevron pattern schema")
    construction = profile["construction_contract"]
    completion = pattern["authored_completion"]
    exact_pairs = (
        ("count", 16),
        ("pitch_angle_deg", 11.25),
        ("joint_gap_centerline_m", 0.003),
        ("body_angle_deg", 11.042294066),
        ("inner_radius_m", 0.7275514105),
        ("outer_radius_m", 0.9275514105),
        ("derived_outer_chord_m", 0.1784852529),
    )
    for key, expected in exact_pairs:
        actual = construction[key]
        if isinstance(expected, int):
            if actual != expected or completion[key] != expected:
                raise ValueError(f"{key} drifted from the measured chevron contract")
        elif not (
            abs(float(actual) - expected) <= 1.0e-10
            and abs(float(completion[key]) - expected) <= 1.0e-10
        ):
            raise ValueError(f"{key} drifted from the measured chevron contract")
    if construction["arris_rule"] != (
        "Unknown in the measured source; no bevel is authored."
    ):
        raise ValueError("Unknown arris radius must remain unbevelled")
    exclusions = set(pattern["exclusions"])
    required_exclusions = {
        "jamb reconstruction",
        "fracture",
        "edge damage",
        "wear",
        "weathering",
        "soot",
        "damp",
        "lichen",
        "Unreal parity",
    }
    if exclusions != required_exclusions:
        raise ValueError("Capability exclusions changed without new evidence")
    inventory = profile["texture_inventory"]
    if set(inventory) != {
        "basecolor",
        "orm",
        "body_masks",
        "body_normal",
        "body_height",
    }:
        raise ValueError("Texture inventory must contain exactly five consumed lanes")
    for lane, entry in inventory.items():
        if entry["resolution"] != [RESOLUTION, RESOLUTION]:
            raise ValueError(f"{lane} must be authored at 1024 square")
        expected_mpt = entry["physical_span_m"] / RESOLUTION
        if abs(entry["metres_per_texel"] - expected_mpt) > 1.0e-12:
            raise ValueError(f"{lane} metres-per-texel is inconsistent")


def stone_palette() -> np.ndarray:
    values = (
        (161, 132, 101),
        (166, 137, 104),
        (171, 142, 108),
        (175, 146, 112),
        (179, 150, 116),
        (183, 154, 119),
        (186, 158, 123),
        (189, 161, 126),
        (192, 164, 130),
        (194, 167, 134),
        (196, 170, 138),
        (198, 172, 141),
        (200, 175, 145),
        (202, 177, 148),
        (204, 179, 151),
        (206, 181, 154),
        (208, 184, 158),
        (210, 186, 161),
        (212, 188, 164),
        (214, 191, 168),
    )
    return np.asarray(values, dtype=np.float32) / 255.0


def build_broad_fields(seed: int) -> tuple[np.ndarray, np.ndarray]:
    broad = periodic_fbm_rect(
        RESOLUTION,
        cells_x=4,
        cells_y=3,
        seed=seed,
        octaves=4,
        persistence=0.52,
    )
    broad = periodic_gaussian_blur(broad, sigma_px=18.0)
    broad = normalized_range(broad)
    wash = periodic_fbm_rect(
        RESOLUTION,
        cells_x=7,
        cells_y=5,
        seed=seed + 101,
        octaves=3,
        persistence=0.45,
    )
    wash = periodic_gaussian_blur(wash, sigma_px=8.0)
    wash = normalized_range(wash)
    combined = np.clip(0.72 * broad + 0.28 * wash, 0.0, 1.0)
    return combined.astype(np.float32), wash.astype(np.float32)


def quantized_palette_field(
    field: np.ndarray,
    palette: np.ndarray,
) -> np.ndarray:
    position = np.clip(field, 0.0, 1.0) * (len(palette) - 1)
    lower = np.floor(position).astype(np.int32)
    upper = np.minimum(lower + 1, len(palette) - 1)
    blend = (position - lower)[..., np.newaxis]
    return (
        palette[lower] * (1.0 - blend) + palette[upper] * blend
    ).astype(np.float32)


def build_basecolor(seed: int) -> tuple[np.ndarray, np.ndarray]:
    broad, wash = build_broad_fields(seed)
    color = quantized_palette_field(broad, stone_palette())
    x = np.arange(RESOLUTION, dtype=np.float32)[np.newaxis, :] / RESOLUTION
    y = np.arange(RESOLUTION, dtype=np.float32)[:, np.newaxis] / RESOLUTION
    directional = (
        0.5
        + 0.5
        * np.sin(
            2.0 * np.pi * (2.0 * x + y)
            + 0.35 * np.sin(2.0 * np.pi * 3.0 * y)
        )
    )
    glaze = (directional - 0.5) * 0.022 + (wash - 0.5) * 0.028
    warm = np.asarray([1.0, 0.79, 0.57], dtype=np.float32)
    cool = np.asarray([0.69, 0.72, 0.70], dtype=np.float32)
    color = color * (1.0 + glaze[..., np.newaxis])
    color = color * (
        1.0
        + (wash - 0.5)[..., np.newaxis]
        * (warm - cool)[np.newaxis, np.newaxis, :]
        * 0.055
    )
    return np.clip(color, 0.0, 1.0), broad


def build_orm(broad: np.ndarray) -> np.ndarray:
    roughness = np.clip(0.72 + (broad - 0.5) * 0.11, 0.66, 0.82)
    orm = np.empty((RESOLUTION, RESOLUTION, 3), dtype=np.float32)
    orm[..., 0] = 1.0
    orm[..., 1] = roughness
    orm[..., 2] = 0.0
    return orm


def copy_measured_body_lanes(output: Path) -> dict[str, Path]:
    required = {
        "body_masks": CORE_OUTPUT / "cathedral_stone_v1_stone_body_masks.png",
        "body_normal": CORE_OUTPUT / "cathedral_stone_v1_stone_body_normal.png",
        "body_height": CORE_OUTPUT / "cathedral_stone_v1_stone_body_height.png",
    }
    destinations: dict[str, Path] = {}
    for lane, source in required.items():
        if not source.is_file():
            raise FileNotFoundError(
                f"Accepted cathedral stone body lane is missing: {source}"
            )
        destination = output / f"cathedral_stone_trim_fracture_v1_{lane}.png"
        shutil.copyfile(source, destination)
        destinations[lane] = destination
    return destinations


def generate(
    profile_path: Path,
    pattern_path: Path,
    output: Path,
) -> dict[str, Any]:
    profile = read_json(profile_path)
    pattern = read_json(pattern_path)
    validate_contract(profile, pattern)
    output.mkdir(parents=True, exist_ok=True)
    seed = int(profile["seed"])
    basecolor, broad = build_basecolor(seed)
    orm = build_orm(broad)
    files = {
        "basecolor": output / "cathedral_stone_trim_fracture_v1_basecolor.png",
        "orm": output / "cathedral_stone_trim_fracture_v1_orm.png",
    }
    write_png_rgb8(files["basecolor"], basecolor)
    write_png_rgb8(files["orm"], orm)
    files.update(copy_measured_body_lanes(output))
    core_manifest = read_json(CORE_MANIFEST)
    manifest = {
        "schema": MANIFEST_SCHEMA,
        "profile_schema": profile["schema"],
        "pattern_schema": pattern["schema"],
        "profile_id": profile["profile_id"],
        "pattern_id": pattern["pattern_id"],
        "seed": seed,
        "source_measurement": pattern["source_measurement"],
        "authored_completion": pattern["authored_completion"],
        "moulding": pattern["moulding"],
        "texture_inventory": profile["texture_inventory"],
        "source_policy": profile["source_policy"],
        "inherited_body_contract": {
            "source_package": "cathedral_stone_v1",
            "source_manifest_schema": core_manifest["schema"],
            "relief_proxy_measurement_id": (
                "sabucina_calcarenite_surface_topography_proxy"
            ),
            "body_relief_range_m": 0.00113,
            "raw_reference_pixels_used": False,
            "ashlar_construction_pattern_inherited": False,
        },
        "files": {
            lane: {
                "filename": path.name,
                "sha256": sha256(path),
                "bytes": path.stat().st_size,
            }
            for lane, path in sorted(files.items())
        },
        "validation": {
            "texture_lane_count": len(files),
            "twenty_shade_palette": len(stone_palette()) == 20,
            "orm_ao_constant_one": True,
            "orm_metallic_constant_zero": True,
            "broad_color_not_one_block_one_color": (
                float(np.std(basecolor)) > 0.025
            ),
            "accepted_body_maps_copied_byte_exact": all(
                sha256(files[lane])
                == sha256(
                    CORE_OUTPUT
                    / f"cathedral_stone_v1_stone_{lane}.png"
                )
                for lane in ("body_masks", "body_normal", "body_height")
            ),
            "damage_authored": False,
            "fracture_authored": False,
        },
    }
    manifest_path = output / "cathedral_stone_trim_fracture_v1_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    return manifest


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile", type=Path, default=DEFAULT_PROFILE)
    parser.add_argument("--pattern", type=Path, default=DEFAULT_PATTERN)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    return parser.parse_args(argv)


def main() -> int:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    args = parse_args(argv)
    manifest = generate(args.profile, args.pattern, args.output)
    print(
        json.dumps(
            {
                "status": "ok",
                "schema": manifest["schema"],
                "files": sorted(manifest["files"]),
            },
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

### Build and validation

```sh
/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python-exit-code 1 --python assets/creative/materials/cathedral_stone_trim_fracture_v1/generate_cathedral_stone_trim_fracture_v1.py
/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python-exit-code 1 --python tests/unit/cathedral_stone_trim_fracture_v1_generator_tests.py
```

Execution record: generator status `ok`; eleven generator contract tests green; five lanes present; copied body hashes equal the accepted core; damage and fracture remain false.

## DEM-GEO-002: Measured individual voussoirs, live shader, and reopen proof

### Demand

Replace the flat annular ribbon with sixteen independent, closed, manifold wedge meshes derived from Old Sarum item 55; give each stone one centripetal lateral roll-hollow-roll chevron, an empty 3 mm centreline joint, a 270 mm reveal, live semantic masks, the exact shader graph above, nine proofs, and saved-file reopen validation.

### Authority

- S01 publishes 0.200 m radial height, 0.140 m inner chord, 0.180 m outer chord, and 0.270 m depth.
- CRSBI supports one chevron per voussoir and the lateral, face, centripetal terminology.
- Sixteen stones, 11.25 degree pitch, 3 mm centreline joint, radius, 25/50/50/50/25 mm profile zones, 12 mm roll crest, and 6 mm hollow are replaceable authored completion—not surveyed Old Sarum values.
- Arris radius is unknown, so the builder creates no bevel modifier and no baked bevel.

### Test code

Target: `tests/unit/cathedral_stone_trim_fracture_v1_blend_tests.py`

```python
#!/usr/bin/env python3
"""Saved-file tests for the measured chevron-voussoir Blender asset."""

from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path
import sys
import unittest

import bpy


REPO_ROOT = Path(__file__).resolve().parents[2]
PACKAGE = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "cathedral_stone_trim_fracture_v1"
)
OUTPUT = PACKAGE / "output"
PATTERN = json.loads(
    (
        PACKAGE / "patterns" / "cathedral_trim_fracture_atlas_v1.json"
    ).read_text()
)
MANIFEST_PATH = OUTPUT / "cathedral_stone_trim_fracture_v1_blender_manifest.json"
GROUP_NAME = "IGGY_SH_ChevronVoussoir_v002"
MATERIAL_NAME = "IGGY_MAT_ChevronVoussoir_v002"
PRODUCT_COLLECTION_NAME = "IGGY_ChevronVoussoirPortal_Product"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def product_objects() -> list[bpy.types.Object]:
    collection = bpy.data.collections.get(PRODUCT_COLLECTION_NAME)
    if collection is None:
        raise AssertionError("Measured chevron product collection is missing")
    return sorted(collection.objects, key=lambda obj: obj.name)


def link_exists(
    tree: bpy.types.NodeTree,
    from_node: str,
    from_socket: str,
    to_node: str,
    to_socket: str,
) -> bool:
    return any(
        link.from_node.name == from_node
        and link.from_socket.name == from_socket
        and link.to_node.name == to_node
        and link.to_socket.name == to_socket
        for link in tree.links
    )


class ChevronSavedBlendTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if bpy.data.filepath == "":
            raise AssertionError("Test must run against the saved .blend file")
        cls.manifest = json.loads(MANIFEST_PATH.read_text())
        cls.objects = product_objects()
        cls.group = bpy.data.node_groups.get(GROUP_NAME)
        cls.material = bpy.data.materials.get(MATERIAL_NAME)

    def test_saved_file_and_scene_identify_the_measured_capability(self) -> None:
        self.assertEqual(
            Path(bpy.data.filepath).resolve(),
            (OUTPUT / "cathedral_stone_trim_fracture_v1.blend").resolve(),
        )
        scene = bpy.context.scene
        self.assertEqual(
            scene["iggy_asset_schema"],
            "iggy3d.asset.chevron_voussoir_portal.measured.v2",
        )
        self.assertEqual(
            scene["iggy_source_measurement"],
            "S01_old_sarum_catalogue_item_55",
        )
        self.assertEqual(scene["iggy_product_object_count"], 16)
        self.assertFalse(scene["iggy_fracture_enabled"])
        self.assertFalse(scene["iggy_damage_enabled"])
        self.assertEqual(scene["iggy_proof_set"], "all")
        self.assertEqual(
            json.loads(scene["iggy_selected_proofs"]),
            sorted(self.manifest["proofs"]),
        )

    def test_sixteen_individual_closed_wedges_replace_the_annular_ribbon(
        self,
    ) -> None:
        self.assertEqual(len(self.objects), 16)
        self.assertEqual(len({obj.data.name for obj in self.objects}), 16)
        for stone_id, obj in enumerate(self.objects):
            with self.subTest(obj=obj.name):
                self.assertEqual(obj.type, "MESH")
                self.assertEqual(obj["iggy_voussoir_id"], stone_id)
                self.assertEqual(
                    obj["iggy_role"],
                    "measured_chevron_voussoir",
                )
                self.assertEqual(
                    obj["iggy_source_id"],
                    "S01_old_sarum_catalogue_item_55",
                )
                self.assertEqual(tuple(round(v, 10) for v in obj.scale), (1.0, 1.0, 1.0))
                self.assertEqual(len(obj.modifiers), 0)
                self.assertTrue(obj.data["iggy_closed_volume"])
                self.assertEqual(obj.data["iggy_bevel_m"], 0.0)
                usage = [0] * len(obj.data.edges)
                lookup = {
                    tuple(sorted(edge.vertices)): edge.index
                    for edge in obj.data.edges
                }
                for polygon in obj.data.polygons:
                    for edge_key in polygon.edge_keys:
                        usage[lookup[tuple(sorted(edge_key))]] += 1
                self.assertTrue(all(count == 2 for count in usage))

    def test_catalogue_dimensions_and_centerline_joint_recompute(self) -> None:
        completion = PATTERN["authored_completion"]
        source = PATTERN["source_measurement"]
        body_angle = math.radians(completion["body_angle_deg"])
        pitch_angle = math.radians(completion["pitch_angle_deg"])
        inner_radius = completion["inner_radius_m"]
        outer_radius = completion["outer_radius_m"]
        inner_chord = 2.0 * inner_radius * math.sin(body_angle / 2.0)
        outer_chord = 2.0 * outer_radius * math.sin(body_angle / 2.0)
        centerline_gap = (
            inner_radius + source["height_m"] / 2.0
        ) * (pitch_angle - body_angle)
        self.assertAlmostEqual(inner_chord, 0.14, places=10)
        self.assertAlmostEqual(outer_chord, 0.1784852529, places=10)
        self.assertLessEqual(abs(outer_chord - source["outer_chord_m"]), 0.005)
        self.assertAlmostEqual(centerline_gap, 0.003, places=10)
        for obj in self.objects:
            self.assertEqual(obj["iggy_catalogue_radial_height_m"], 0.2)
            self.assertEqual(obj["iggy_catalogue_inner_chord_m"], 0.14)
            self.assertEqual(obj["iggy_catalogue_outer_chord_m"], 0.18)
            self.assertEqual(obj["iggy_catalogue_depth_m"], 0.27)
            self.assertEqual(obj["iggy_joint_gap_centerline_m"], 0.003)

    def test_front_geometry_carries_roll_hollow_quiet_and_linework_masks(
        self,
    ) -> None:
        required_point = {
            "iggy_voussoir_id": "INT",
            "iggy_material_phase": "FLOAT_VECTOR",
            "iggy_material_variant": "INT",
            "iggy_chevron_roll": "FLOAT",
            "iggy_chevron_hollow": "FLOAT",
            "iggy_chevron_quiet": "FLOAT",
            "iggy_chevron_ink": "FLOAT",
            "iggy_chevron_highlight": "FLOAT",
        }
        required_face = {
            "iggy_carved_trim": "BOOLEAN",
            "iggy_chevron_front": "BOOLEAN",
            "iggy_fracture_interior": "BOOLEAN",
        }
        for obj in self.objects:
            mesh = obj.data
            self.assertEqual(
                sorted(layer.name for layer in mesh.uv_layers),
                ["IGGY_StoneUV_A", "IGGY_StoneUV_B"],
            )
            for name, data_type in required_point.items():
                attribute = mesh.attributes.get(name)
                self.assertIsNotNone(attribute, name)
                self.assertEqual(attribute.domain, "POINT")
                self.assertEqual(attribute.data_type, data_type)
            for name, data_type in required_face.items():
                attribute = mesh.attributes.get(name)
                self.assertIsNotNone(attribute, name)
                self.assertEqual(attribute.domain, "FACE")
                self.assertEqual(attribute.data_type, data_type)
            front = mesh.attributes["iggy_chevron_front"]
            fracture = mesh.attributes["iggy_fracture_interior"]
            self.assertEqual(
                sum(bool(item.value) for item in front.data),
                mesh["iggy_front_face_count"],
            )
            self.assertTrue(all(not item.value for item in fracture.data))
            for name in (
                "iggy_chevron_roll",
                "iggy_chevron_hollow",
                "iggy_chevron_quiet",
                "iggy_chevron_ink",
                "iggy_chevron_highlight",
            ):
                values = [item.value for item in mesh.attributes[name].data]
                self.assertLessEqual(min(values), 0.001, name)
                self.assertGreaterEqual(max(values), 0.90, name)

    def test_shader_has_exact_live_texture_coordinate_and_identity_routes(
        self,
    ) -> None:
        self.assertIsNotNone(self.group)
        self.assertEqual(
            self.group["iggy_schema"],
            "iggy3d.shader.chevron_voussoir.measured_roll_hollow_roll.v2",
        )
        required_nodes = {
            "IGGY_StoneUV_A": "ShaderNodeUVMap",
            "IGGY_StoneUV_B": "ShaderNodeUVMap",
            "IGGY_BaseCoordinate_512mm": "ShaderNodeVectorMath",
            "IGGY_PrimaryCoordinate_64mm": "ShaderNodeVectorMath",
            "IGGY_SecondaryCoordinate_91mm": "ShaderNodeVectorMath",
            "IGGY_TrimBaseColor": "ShaderNodeTexImage",
            "IGGY_TrimORM": "ShaderNodeTexImage",
            "IGGY_BodyMasks64": "ShaderNodeTexImage",
            "IGGY_BodyMasks91": "ShaderNodeTexImage",
            "IGGY_BodyNormal64": "ShaderNodeTexImage",
            "IGGY_BodyHeight91": "ShaderNodeTexImage",
            "IGGY_OpenGLBodyNormal64mm": "ShaderNodeNormalMap",
            "IGGY_DecorrelatedBodyHeight91mm": "ShaderNodeBump",
            "IGGY_DetailFullAt030mZeroAt3p5m": "ShaderNodeMapRange",
            "IGGY_SelectiveChevronInk": "ShaderNodeMixRGB",
            "IGGY_SelectiveCrestHighlight": "ShaderNodeMixRGB",
        }
        for name, bl_idname in required_nodes.items():
            node = self.group.nodes.get(name)
            self.assertIsNotNone(node, name)
            self.assertEqual(node.bl_idname, bl_idname)
        self.assertTrue(
            link_exists(
                self.group,
                "IGGY_StoneUV_A",
                "UV",
                "IGGY_BaseCoordinate_512mm",
                "Vector",
            )
        )
        self.assertTrue(
            link_exists(
                self.group,
                "IGGY_StoneUV_A",
                "UV",
                "IGGY_PrimaryCoordinate_64mm",
                "Vector",
            )
        )
        self.assertTrue(
            link_exists(
                self.group,
                "IGGY_StoneUV_B",
                "UV",
                "IGGY_SecondaryCoordinate_91mm",
                "Vector",
            )
        )
        self.assertTrue(
            link_exists(
                self.group,
                "IGGY_BodyNormal64",
                "Color",
                "IGGY_OpenGLBodyNormal64mm",
                "Color",
            )
        )
        self.assertTrue(
            link_exists(
                self.group,
                "IGGY_OpenGLBodyNormal64mm",
                "Normal",
                "IGGY_DecorrelatedBodyHeight91mm",
                "Normal",
            )
        )
        self.assertTrue(
            link_exists(
                self.group,
                "IGGY_DecorrelatedBodyHeight91mm",
                "Normal",
                "Group_Output",
                "Combined Normal",
            )
        )
        self.assertFalse(
            self.group.nodes["IGGY_DecorrelatedBodyHeight91mm"].invert
        )

    def test_every_image_is_packed_and_uses_the_declared_color_space(self) -> None:
        expected = {
            "IGGY_TrimBaseColor": "sRGB",
            "IGGY_TrimORM": "Non-Color",
            "IGGY_BodyMasks64": "Non-Color",
            "IGGY_BodyMasks91": "Non-Color",
            "IGGY_BodyNormal64": "Non-Color",
            "IGGY_BodyHeight91": "Non-Color",
        }
        for name, color_space in expected.items():
            node = self.group.nodes[name]
            self.assertIsNotNone(node.image, name)
            self.assertIsNotNone(node.image.packed_file, name)
            self.assertEqual(node.image.colorspace_settings.name, color_space)

    def test_all_declared_shader_attributes_are_live_in_the_group(self) -> None:
        expected = {
            "iggy_carved_trim",
            "iggy_chevron_front",
            "iggy_chevron_highlight",
            "iggy_chevron_hollow",
            "iggy_chevron_ink",
            "iggy_chevron_quiet",
            "iggy_chevron_roll",
            "iggy_material_phase",
            "iggy_material_variant",
            "iggy_voussoir_id",
        }
        nodes = [
            node
            for node in self.group.nodes
            if node.bl_idname == "ShaderNodeAttribute"
        ]
        self.assertEqual({node.attribute_name for node in nodes}, expected)
        for node in nodes:
            self.assertTrue(
                any(link.from_node == node for link in self.group.links),
                node.attribute_name,
            )

    def test_principled_material_consumes_group_color_roughness_and_normal(
        self,
    ) -> None:
        self.assertIsNotNone(self.material)
        tree = self.material.node_tree
        self.assertTrue(
            link_exists(
                tree,
                "Chevron_Voussoir_System",
                "Combined Color",
                "Chevron_Voussoir_BSDF",
                "Base Color",
            )
        )
        self.assertTrue(
            link_exists(
                tree,
                "Chevron_Voussoir_System",
                "Combined Roughness",
                "Chevron_Voussoir_BSDF",
                "Roughness",
            )
        )
        self.assertTrue(
            link_exists(
                tree,
                "Chevron_Voussoir_System",
                "Combined Normal",
                "Chevron_Voussoir_BSDF",
                "Normal",
            )
        )
        principled = tree.nodes["Chevron_Voussoir_BSDF"]
        self.assertEqual(principled.inputs["Metallic"].default_value, 0.0)
        self.assertAlmostEqual(principled.inputs["IOR"].default_value, 1.46)
        self.assertFalse(self.material["iggy_fracture_enabled"])
        self.assertFalse(self.material["iggy_damage_enabled"])

    def test_manifest_and_nine_proofs_survive_saved_file_reopen(self) -> None:
        self.assertEqual(
            self.manifest["schema"],
            "iggy3d.material.cathedral_stone_trim_fracture_v1.blender_manifest.v2",
        )
        self.assertEqual(self.manifest["geometry"]["object_count"], 16)
        self.assertTrue(self.manifest["geometry"]["all_meshes_manifold"])
        self.assertEqual(self.manifest["build_mode"]["proof_set"], "all")
        self.assertTrue(
            self.manifest["build_mode"]["canonical_candidate_complete"]
        )
        self.assertTrue(
            self.manifest["validation"]["complete_proof_set"]
        )
        self.assertEqual(len(self.manifest["proofs"]), 9)
        required = {
            "neutral_clay_front",
            "neutral_clay_grazing",
            "live_material_front",
            "live_material_grazing",
            "measured_close",
            "distance_read",
            "moulding_proof",
            "identity_proof",
            "wireframe_proof",
        }
        self.assertEqual(set(self.manifest["proofs"]), required)
        hashes = set()
        for proof in self.manifest["proofs"].values():
            path = OUTPUT / "proofs" / proof["filename"]
            self.assertTrue(path.is_file(), path)
            actual_hash = sha256(path)
            self.assertEqual(actual_hash, proof["sha256"])
            hashes.add(actual_hash)
        self.assertEqual(len(hashes), 9)
        self.assertTrue(all(self.manifest["validation"].values()) is False)
        for key, value in self.manifest["validation"].items():
            if key in {"damage_authored", "fracture_authored"}:
                self.assertFalse(value, key)
            elif key == "proof_count":
                self.assertEqual(value, 9)
            else:
                self.assertTrue(value, key)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
```

### Blender builder, validation, manifest, and proof code

Target: `assets/creative/materials/cathedral_stone_trim_fracture_v1/build_cathedral_stone_trim_fracture_v1.py`

```python
#!/usr/bin/env python3
"""Build and prove the measured Old Sarum chevron-voussoir portal order."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import random
import sys
from typing import Any, Iterable

import bmesh
import bpy
from mathutils import Vector


SCRIPT_ROOT = Path(__file__).resolve().parent
PROFILE_PATH = SCRIPT_ROOT / "profiles" / "cathedral_stone_trim_fracture_v1.json"
PATTERN_PATH = SCRIPT_ROOT / "patterns" / "cathedral_trim_fracture_atlas_v1.json"
GENERATOR_PATH = SCRIPT_ROOT / "generate_cathedral_stone_trim_fracture_v1.py"
DEFAULT_OUTPUT = SCRIPT_ROOT / "output"
GROUP_NAME = "IGGY_SH_ChevronVoussoir_v002"
MATERIAL_NAME = "IGGY_MAT_ChevronVoussoir_v002"
CLAY_MATERIAL_NAME = "IGGY_MAT_ChevronVoussoir_ClayProof"
MOULDING_PROOF_MATERIAL_NAME = "IGGY_MAT_ChevronMoulding_Proof"
IDENTITY_PROOF_MATERIAL_NAME = "IGGY_MAT_ChevronIdentity_Proof"
WIREFRAME_PROOF_MATERIAL_NAME = "IGGY_MAT_ChevronWireframe_Proof"
PRODUCT_COLLECTION_NAME = "IGGY_ChevronVoussoirPortal_Product"
PROOF_COLLECTION_NAME = "IGGY_ChevronVoussoirPortal_Proof"
PROOF_IDS = (
    "neutral_clay_front",
    "neutral_clay_grazing",
    "live_material_front",
    "live_material_grazing",
    "measured_close",
    "distance_read",
    "moulding_proof",
    "identity_proof",
    "wireframe_proof",
)
BLENDER_MANIFEST_SCHEMA = (
    "iggy3d.material.cathedral_stone_trim_fracture_v1.blender_manifest.v2"
)


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text())


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_generator() -> Any:
    spec = importlib.util.spec_from_file_location(
        "iggy_chevron_texture_generator",
        GENERATOR_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError("Cannot load chevron texture generator")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def ensure_texture_family(output: Path) -> dict[str, Any]:
    generator = load_generator()
    return generator.generate(PROFILE_PATH, PATTERN_PATH, output)


def reset_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for block in (
        bpy.data.meshes,
        bpy.data.curves,
        bpy.data.cameras,
        bpy.data.lights,
        bpy.data.materials,
        bpy.data.node_groups,
    ):
        for item in list(block):
            block.remove(item, do_unlink=True)
    for collection in list(bpy.data.collections):
        if collection.name != bpy.context.scene.collection.name:
            bpy.data.collections.remove(collection)


def collection(name: str) -> bpy.types.Collection:
    result = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(result)
    return result


def add_node(
    tree: bpy.types.NodeTree,
    bl_idname: str,
    name: str,
    location: tuple[float, float],
) -> bpy.types.Node:
    node = tree.nodes.new(bl_idname)
    node.name = name
    node.label = name
    node.location = location
    return node


def link(
    tree: bpy.types.NodeTree,
    source: bpy.types.Node,
    source_socket: str,
    target: bpy.types.Node,
    target_socket: str,
) -> None:
    tree.links.new(
        source.outputs[source_socket],
        target.inputs[target_socket],
    )


def math_node(
    tree: bpy.types.NodeTree,
    name: str,
    operation: str,
    location: tuple[float, float],
    *,
    value_1: float | None = None,
) -> bpy.types.Node:
    node = add_node(tree, "ShaderNodeMath", name, location)
    node.operation = operation
    if value_1 is not None:
        node.inputs[1].default_value = value_1
    return node


def vector_scale_node(
    tree: bpy.types.NodeTree,
    name: str,
    scale: float,
    location: tuple[float, float],
) -> bpy.types.Node:
    node = add_node(tree, "ShaderNodeVectorMath", name, location)
    node.operation = "SCALE"
    node.inputs["Scale"].default_value = scale
    return node


def mix_color_node(
    tree: bpy.types.NodeTree,
    name: str,
    color: tuple[float, float, float, float],
    location: tuple[float, float],
) -> bpy.types.Node:
    node = add_node(tree, "ShaderNodeMixRGB", name, location)
    node.blend_type = "MIX"
    node.inputs["Color2"].default_value = color
    return node


def srgb_channel(value: float) -> float:
    value = max(0.0, min(1.0, value))
    if value <= 0.04045:
        return value / 12.92
    return ((value + 0.055) / 1.055) ** 2.4


def linear_rgba(
    red: int,
    green: int,
    blue: int,
    alpha: float = 1.0,
) -> tuple[float, float, float, float]:
    return (
        srgb_channel(red / 255.0),
        srgb_channel(green / 255.0),
        srgb_channel(blue / 255.0),
        alpha,
    )


def load_image(path: Path, *, non_color: bool) -> bpy.types.Image:
    if not path.is_file():
        raise FileNotFoundError(path)
    image = bpy.data.images.load(str(path), check_existing=True)
    image.colorspace_settings.name = "Non-Color" if non_color else "sRGB"
    image.pack()
    return image


def create_material_group(output: Path) -> bpy.types.NodeTree:
    group = bpy.data.node_groups.new(GROUP_NAME, "ShaderNodeTree")
    group.color_tag = "SHADER"
    group.description = (
        "Measured chevron-voussoir shading: metre-stable local UVs, broad "
        "twenty-shade calcarenite, two-scale body anatomy, live roll/hollow "
        "identity, and distance-gated microdetail."
    )
    for name, socket_type in (
        ("Combined Color", "NodeSocketColor"),
        ("Combined Roughness", "NodeSocketFloat"),
        ("Combined Normal", "NodeSocketVector"),
        ("Moulding Proof", "NodeSocketColor"),
        ("Identity Proof", "NodeSocketColor"),
        ("Detail Distance Fade", "NodeSocketFloat"),
    ):
        group.interface.new_socket(
            name=name,
            in_out="OUTPUT",
            socket_type=socket_type,
        )
    output_node = add_node(
        group,
        "NodeGroupOutput",
        "Group_Output",
        (2480, 120),
    )

    uv_a = add_node(
        group,
        "ShaderNodeUVMap",
        "IGGY_StoneUV_A",
        (-2420, 680),
    )
    uv_a.uv_map = "IGGY_StoneUV_A"
    uv_b = add_node(
        group,
        "ShaderNodeUVMap",
        "IGGY_StoneUV_B",
        (-2420, 300),
    )
    uv_b.uv_map = "IGGY_StoneUV_B"
    base_coordinate = vector_scale_node(
        group,
        "IGGY_BaseCoordinate_512mm",
        1.0 / 0.512,
        (-2190, 720),
    )
    primary_coordinate = vector_scale_node(
        group,
        "IGGY_PrimaryCoordinate_64mm",
        1.0 / 0.064,
        (-2190, 520),
    )
    secondary_coordinate = vector_scale_node(
        group,
        "IGGY_SecondaryCoordinate_91mm",
        1.0 / 0.091,
        (-2190, 280),
    )
    link(group, uv_a, "UV", base_coordinate, "Vector")
    link(group, uv_a, "UV", primary_coordinate, "Vector")
    link(group, uv_b, "UV", secondary_coordinate, "Vector")

    image_specs = (
        (
            "IGGY_TrimBaseColor",
            "cathedral_stone_trim_fracture_v1_basecolor.png",
            False,
            base_coordinate,
            900,
        ),
        (
            "IGGY_TrimORM",
            "cathedral_stone_trim_fracture_v1_orm.png",
            True,
            base_coordinate,
            670,
        ),
        (
            "IGGY_BodyMasks64",
            "cathedral_stone_trim_fracture_v1_body_masks.png",
            True,
            primary_coordinate,
            420,
        ),
        (
            "IGGY_BodyMasks91",
            "cathedral_stone_trim_fracture_v1_body_masks.png",
            True,
            secondary_coordinate,
            190,
        ),
        (
            "IGGY_BodyNormal64",
            "cathedral_stone_trim_fracture_v1_body_normal.png",
            True,
            primary_coordinate,
            -60,
        ),
        (
            "IGGY_BodyHeight91",
            "cathedral_stone_trim_fracture_v1_body_height.png",
            True,
            secondary_coordinate,
            -300,
        ),
    )
    images: dict[str, bpy.types.Node] = {}
    for name, filename, non_color, coordinate, y in image_specs:
        node = add_node(group, "ShaderNodeTexImage", name, (-1930, y))
        node.image = load_image(output / filename, non_color=non_color)
        node.extension = "REPEAT"
        node.interpolation = "Linear"
        link(group, coordinate, "Vector", node, "Vector")
        images[name] = node

    orm = add_node(
        group,
        "ShaderNodeSeparateColor",
        "IGGY_SeparateORM",
        (-1680, 670),
    )
    orm.mode = "RGB"
    link(group, images["IGGY_TrimORM"], "Color", orm, "Color")
    masks_blend = add_node(
        group,
        "ShaderNodeMixRGB",
        "IGGY_Blend64mmAnd91mmBodyMasks",
        (-1680, 320),
    )
    masks_blend.blend_type = "MIX"
    masks_blend.inputs["Factor"].default_value = 0.34
    link(group, images["IGGY_BodyMasks64"], "Color", masks_blend, "Color1")
    link(group, images["IGGY_BodyMasks91"], "Color", masks_blend, "Color2")
    masks = add_node(
        group,
        "ShaderNodeSeparateColor",
        "IGGY_SeparateBodyMasks",
        (-1440, 320),
    )
    masks.mode = "RGB"
    link(group, masks_blend, "Color", masks, "Color")

    camera = add_node(
        group,
        "ShaderNodeCameraData",
        "IGGY_CameraDistance",
        (-1680, -580),
    )
    detail_fade = add_node(
        group,
        "ShaderNodeMapRange",
        "IGGY_DetailFullAt030mZeroAt3p5m",
        (-1440, -580),
    )
    detail_fade.clamp = True
    detail_fade.interpolation_type = "SMOOTHERSTEP"
    detail_fade.inputs["From Min"].default_value = 0.3
    detail_fade.inputs["From Max"].default_value = 3.5
    detail_fade.inputs["To Min"].default_value = 1.0
    detail_fade.inputs["To Max"].default_value = 0.0
    link(group, camera, "View Distance", detail_fade, "Value")
    line_fade = add_node(
        group,
        "ShaderNodeMapRange",
        "IGGY_LineFullAt030mZeroAt6m",
        (-1440, -730),
    )
    line_fade.clamp = True
    line_fade.interpolation_type = "SMOOTHERSTEP"
    line_fade.inputs["From Min"].default_value = 0.3
    line_fade.inputs["From Max"].default_value = 6.0
    line_fade.inputs["To Min"].default_value = 1.0
    line_fade.inputs["To Max"].default_value = 0.0
    link(group, camera, "View Distance", line_fade, "Value")

    attributes: dict[str, bpy.types.Node] = {}
    attribute_specs = (
        ("roll", "iggy_chevron_roll", -1180, 80),
        ("hollow", "iggy_chevron_hollow", -1180, -50),
        ("quiet", "iggy_chevron_quiet", -1180, -180),
        ("ink", "iggy_chevron_ink", -1180, -310),
        ("highlight", "iggy_chevron_highlight", -1180, -440),
        ("front", "iggy_chevron_front", -1180, -570),
        ("carved", "iggy_carved_trim", -1180, -700),
        ("variant", "iggy_material_variant", -1180, -830),
        ("stone_id", "iggy_voussoir_id", -1180, -960),
        ("phase", "iggy_material_phase", -1180, -1090),
    )
    for key, attribute_name, x, y in attribute_specs:
        node = add_node(
            group,
            "ShaderNodeAttribute",
            f"IGGY_Attribute_{attribute_name}",
            (x, y),
        )
        node.attribute_name = attribute_name
        attributes[key] = node

    front_carved = math_node(
        group,
        "IGGY_FrontTimesCarved",
        "MULTIPLY",
        (-900, -620),
    )
    link(group, attributes["front"], "Factor", front_carved, "Value")
    link(group, attributes["carved"], "Factor", front_carved, "Value_001")

    live_masks: dict[str, bpy.types.Node] = {}
    for index, key in enumerate(("roll", "hollow", "quiet", "ink", "highlight")):
        node = math_node(
            group,
            f"IGGY_Live{key.title()}Mask",
            "MULTIPLY",
            (-680, -100 - index * 130),
        )
        link(group, attributes[key], "Factor", node, "Value")
        link(group, front_carved, "Value", node, "Value_001")
        live_masks[key] = node

    parity = math_node(
        group,
        "IGGY_VariantParity",
        "MODULO",
        (-900, 920),
        value_1=2.0,
    )
    link(group, attributes["variant"], "Factor", parity, "Value")
    warm_factor = math_node(
        group,
        "IGGY_WarmVariantFactor",
        "MULTIPLY",
        (-680, 920),
        value_1=0.07,
    )
    link(group, parity, "Value", warm_factor, "Value")
    high_variant = math_node(
        group,
        "IGGY_HighVariant",
        "GREATER_THAN",
        (-900, 790),
        value_1=1.5,
    )
    link(group, attributes["variant"], "Factor", high_variant, "Value")
    cool_factor = math_node(
        group,
        "IGGY_CoolVariantFactor",
        "MULTIPLY",
        (-680, 790),
        value_1=0.055,
    )
    link(group, high_variant, "Value", cool_factor, "Value")

    warm_mix = mix_color_node(
        group,
        "IGGY_PerStoneWarmWash",
        linear_rgba(211, 172, 118),
        (-410, 920),
    )
    link(group, images["IGGY_TrimBaseColor"], "Color", warm_mix, "Color1")
    link(group, warm_factor, "Value", warm_mix, "Factor")
    cool_mix = mix_color_node(
        group,
        "IGGY_PerStoneCoolWash",
        linear_rgba(157, 158, 149),
        (-170, 920),
    )
    link(group, warm_mix, "Color", cool_mix, "Color1")
    link(group, cool_factor, "Value", cool_mix, "Factor")

    color_factors: dict[str, bpy.types.Node] = {}
    for index, (key, channel, amount) in enumerate(
        (
            ("fossil", "Red", 0.13),
            ("silicate", "Green", 0.09),
            ("pore", "Blue", 0.17),
        )
    ):
        faded = math_node(
            group,
            f"IGGY_{key.title()}TimesDetailFade",
            "MULTIPLY",
            (-1130 + index * 40, 570 - index * 120),
        )
        link(group, masks, channel, faded, "Value")
        link(group, detail_fade, "Result", faded, "Value_001")
        amount_node = math_node(
            group,
            f"IGGY_{key.title()}ColorAmount",
            "MULTIPLY",
            (-890 + index * 40, 570 - index * 120),
            value_1=amount,
        )
        link(group, faded, "Value", amount_node, "Value")
        color_factors[key] = amount_node

    fossil_mix = mix_color_node(
        group,
        "IGGY_FossilFragmentColor",
        linear_rgba(224, 202, 166),
        (80, 810),
    )
    link(group, cool_mix, "Color", fossil_mix, "Color1")
    link(group, color_factors["fossil"], "Value", fossil_mix, "Factor")
    silicate_mix = mix_color_node(
        group,
        "IGGY_SilicateGrainColor",
        linear_rgba(150, 153, 146),
        (310, 760),
    )
    link(group, fossil_mix, "Color", silicate_mix, "Color1")
    link(group, color_factors["silicate"], "Value", silicate_mix, "Factor")
    pore_mix = mix_color_node(
        group,
        "IGGY_PoreIdentityColor",
        linear_rgba(111, 94, 76),
        (540, 710),
    )
    link(group, silicate_mix, "Color", pore_mix, "Color1")
    link(group, color_factors["pore"], "Value", pore_mix, "Factor")

    moulding_specs = (
        (
            "roll",
            "IGGY_RollWarmth",
            linear_rgba(218, 186, 139),
            0.16,
        ),
        (
            "hollow",
            "IGGY_HollowCoolness",
            linear_rgba(123, 113, 102),
            0.22,
        ),
    )
    previous_color: bpy.types.Node = pore_mix
    for index, (key, name, tint, amount) in enumerate(moulding_specs):
        factor = math_node(
            group,
            f"{name}Factor",
            "MULTIPLY",
            (610, 420 - index * 130),
            value_1=amount,
        )
        link(group, live_masks[key], "Value", factor, "Value")
        mix_node = mix_color_node(
            group,
            name,
            tint,
            (840, 570 - index * 70),
        )
        link(group, previous_color, "Color", mix_node, "Color1")
        link(group, factor, "Value", mix_node, "Factor")
        previous_color = mix_node

    ink_line = math_node(
        group,
        "IGGY_InkTimesLineFade",
        "MULTIPLY",
        (800, 190),
    )
    link(group, live_masks["ink"], "Value", ink_line, "Value")
    link(group, line_fade, "Result", ink_line, "Value_001")
    ink_amount = math_node(
        group,
        "IGGY_SelectiveInkAmount",
        "MULTIPLY",
        (1020, 190),
        value_1=0.30,
    )
    link(group, ink_line, "Value", ink_amount, "Value")
    ink_mix = mix_color_node(
        group,
        "IGGY_SelectiveChevronInk",
        linear_rgba(88, 76, 65),
        (1240, 500),
    )
    link(group, previous_color, "Color", ink_mix, "Color1")
    link(group, ink_amount, "Value", ink_mix, "Factor")

    highlight_line = math_node(
        group,
        "IGGY_HighlightTimesLineFade",
        "MULTIPLY",
        (1020, 40),
    )
    link(group, live_masks["highlight"], "Value", highlight_line, "Value")
    link(group, line_fade, "Result", highlight_line, "Value_001")
    highlight_amount = math_node(
        group,
        "IGGY_SelectiveHighlightAmount",
        "MULTIPLY",
        (1240, 40),
        value_1=0.14,
    )
    link(group, highlight_line, "Value", highlight_amount, "Value")
    highlight_mix = mix_color_node(
        group,
        "IGGY_SelectiveCrestHighlight",
        linear_rgba(231, 211, 176),
        (1470, 500),
    )
    link(group, ink_mix, "Color", highlight_mix, "Color1")
    link(group, highlight_amount, "Value", highlight_mix, "Factor")

    pore_rough = math_node(
        group,
        "IGGY_PoreRoughnessDelta",
        "MULTIPLY",
        (250, -20),
        value_1=0.09,
    )
    link(group, color_factors["pore"], "Value", pore_rough, "Value")
    silicate_rough = math_node(
        group,
        "IGGY_SilicateRoughnessDelta",
        "MULTIPLY",
        (250, -150),
        value_1=-0.04,
    )
    link(
        group,
        color_factors["silicate"],
        "Value",
        silicate_rough,
        "Value",
    )
    add_pore = math_node(
        group,
        "IGGY_AddPoreRoughness",
        "ADD",
        (500, -20),
    )
    link(group, orm, "Green", add_pore, "Value")
    link(group, pore_rough, "Value", add_pore, "Value_001")
    add_silicate = math_node(
        group,
        "IGGY_AddSilicateRoughness",
        "ADD",
        (720, -20),
    )
    link(group, add_pore, "Value", add_silicate, "Value")
    link(group, silicate_rough, "Value", add_silicate, "Value_001")
    roll_rough = math_node(
        group,
        "IGGY_RollRoughnessDelta",
        "MULTIPLY",
        (500, -250),
        value_1=-0.018,
    )
    link(group, live_masks["roll"], "Value", roll_rough, "Value")
    hollow_rough = math_node(
        group,
        "IGGY_HollowRoughnessDelta",
        "MULTIPLY",
        (500, -380),
        value_1=0.028,
    )
    link(group, live_masks["hollow"], "Value", hollow_rough, "Value")
    add_roll = math_node(
        group,
        "IGGY_AddRollRoughness",
        "ADD",
        (940, -20),
    )
    link(group, add_silicate, "Value", add_roll, "Value")
    link(group, roll_rough, "Value", add_roll, "Value_001")
    add_hollow = math_node(
        group,
        "IGGY_AddHollowRoughness",
        "ADD",
        (1160, -20),
    )
    link(group, add_roll, "Value", add_hollow, "Value")
    link(group, hollow_rough, "Value", add_hollow, "Value_001")
    rough_min = math_node(
        group,
        "IGGY_RoughnessMinimum",
        "MAXIMUM",
        (1380, -20),
        value_1=0.62,
    )
    link(group, add_hollow, "Value", rough_min, "Value")
    rough_max = math_node(
        group,
        "IGGY_RoughnessMaximum",
        "MINIMUM",
        (1600, -20),
        value_1=0.90,
    )
    link(group, rough_min, "Value", rough_max, "Value")

    normal_map = add_node(
        group,
        "ShaderNodeNormalMap",
        "IGGY_OpenGLBodyNormal64mm",
        (250, -560),
    )
    normal_map.space = "TANGENT"
    primary_normal_strength = math_node(
        group,
        "IGGY_PrimaryNormalStrength66Percent",
        "MULTIPLY",
        (0, -560),
        value_1=0.66,
    )
    link(
        group,
        detail_fade,
        "Result",
        primary_normal_strength,
        "Value",
    )
    link(
        group,
        primary_normal_strength,
        "Value",
        normal_map,
        "Strength",
    )
    link(group, images["IGGY_BodyNormal64"], "Color", normal_map, "Color")
    height_value = add_node(
        group,
        "ShaderNodeRGBToBW",
        "IGGY_BodyHeight91mmValue",
        (250, -760),
    )
    link(group, images["IGGY_BodyHeight91"], "Color", height_value, "Color")
    bump = add_node(
        group,
        "ShaderNodeBump",
        "IGGY_DecorrelatedBodyHeight91mm",
        (560, -650),
    )
    bump.invert = False
    bump.inputs["Distance"].default_value = 0.00113 * 0.34
    link(group, detail_fade, "Result", bump, "Strength")
    link(group, height_value, "Val", bump, "Height")
    link(group, normal_map, "Normal", bump, "Normal")

    moulding_proof = add_node(
        group,
        "ShaderNodeCombineColor",
        "IGGY_MouldingProofRGB",
        (1730, -300),
    )
    moulding_proof.mode = "RGB"
    link(group, live_masks["roll"], "Value", moulding_proof, "Red")
    link(group, live_masks["hollow"], "Value", moulding_proof, "Green")
    link(group, live_masks["quiet"], "Value", moulding_proof, "Blue")
    id_normalized = math_node(
        group,
        "IGGY_VoussoirIdNormalized",
        "DIVIDE",
        (1040, -1000),
        value_1=15.0,
    )
    link(group, attributes["stone_id"], "Factor", id_normalized, "Value")
    phase_length = add_node(
        group,
        "ShaderNodeVectorMath",
        "IGGY_PhaseVectorLength",
        (1040, -1130),
    )
    phase_length.operation = "LENGTH"
    link(group, attributes["phase"], "Vector", phase_length, "Vector")
    phase_normalized = add_node(
        group,
        "ShaderNodeMapRange",
        "IGGY_PhaseLengthNormalized",
        (1270, -1130),
    )
    phase_normalized.clamp = True
    phase_normalized.inputs["From Min"].default_value = 0.0
    phase_normalized.inputs["From Max"].default_value = 0.75
    phase_normalized.inputs["To Min"].default_value = 0.0
    phase_normalized.inputs["To Max"].default_value = 1.0
    link(group, phase_length, "Value", phase_normalized, "Value")
    variant_normalized = math_node(
        group,
        "IGGY_VariantNormalized",
        "DIVIDE",
        (1040, -1260),
        value_1=3.0,
    )
    link(
        group,
        attributes["variant"],
        "Factor",
        variant_normalized,
        "Value",
    )
    identity_proof = add_node(
        group,
        "ShaderNodeCombineColor",
        "IGGY_IdentityProofRGB",
        (1730, -1040),
    )
    identity_proof.mode = "RGB"
    link(group, id_normalized, "Value", identity_proof, "Red")
    link(group, phase_normalized, "Result", identity_proof, "Green")
    link(group, variant_normalized, "Value", identity_proof, "Blue")

    link(group, highlight_mix, "Color", output_node, "Combined Color")
    link(group, rough_max, "Value", output_node, "Combined Roughness")
    link(group, bump, "Normal", output_node, "Combined Normal")
    link(group, moulding_proof, "Color", output_node, "Moulding Proof")
    link(group, identity_proof, "Color", output_node, "Identity Proof")
    link(
        group,
        detail_fade,
        "Result",
        output_node,
        "Detail Distance Fade",
    )
    group["iggy_schema"] = (
        "iggy3d.shader.chevron_voussoir.measured_roll_hollow_roll.v2"
    )
    group["iggy_primary_body_span_m"] = 0.064
    group["iggy_secondary_body_span_m"] = 0.091
    group["iggy_body_relief_range_m"] = 0.00113
    group["iggy_damage_enabled"] = False
    return group


def configure_material(group: bpy.types.NodeTree) -> bpy.types.Material:
    material = bpy.data.materials.new(MATERIAL_NAME)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = add_node(
        tree,
        "ShaderNodeOutputMaterial",
        "Material_Output",
        (580, 160),
    )
    principled = add_node(
        tree,
        "ShaderNodeBsdfPrincipled",
        "Chevron_Voussoir_BSDF",
        (300, 160),
    )
    system = add_node(
        tree,
        "ShaderNodeGroup",
        "Chevron_Voussoir_System",
        (-100, 220),
    )
    system.node_tree = group
    link(tree, system, "Combined Color", principled, "Base Color")
    link(tree, system, "Combined Roughness", principled, "Roughness")
    link(tree, system, "Combined Normal", principled, "Normal")
    principled.inputs["Metallic"].default_value = 0.0
    principled.inputs["IOR"].default_value = 1.46
    principled.inputs["Diffuse Roughness"].default_value = 0.16
    link(tree, principled, "BSDF", output, "Surface")
    material["iggy_schema"] = (
        "iggy3d.material.cathedral_stone_trim_fracture_v1.blender.v2"
    )
    material["iggy_measured_source"] = "S01_old_sarum_catalogue_item_55"
    material["iggy_fracture_enabled"] = False
    material["iggy_damage_enabled"] = False
    return material


def simple_material(
    name: str,
    color: tuple[float, float, float, float],
    roughness: float,
) -> bpy.types.Material:
    material = bpy.data.materials.new(name)
    material.diffuse_color = color
    material.use_nodes = True
    principled = material.node_tree.nodes.get("Principled BSDF")
    if principled is None:
        raise RuntimeError("Default Principled BSDF is missing")
    principled.inputs["Base Color"].default_value = color
    principled.inputs["Roughness"].default_value = roughness
    principled.inputs["Metallic"].default_value = 0.0
    return material


def group_proof_material(
    name: str,
    group: bpy.types.NodeTree,
    output_name: str,
) -> bpy.types.Material:
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = add_node(
        tree,
        "ShaderNodeOutputMaterial",
        "Material_Output",
        (430, 80),
    )
    principled = add_node(
        tree,
        "ShaderNodeBsdfPrincipled",
        f"{name}_BSDF",
        (170, 80),
    )
    system = add_node(
        tree,
        "ShaderNodeGroup",
        f"{name}_System",
        (-180, 120),
    )
    system.node_tree = group
    link(tree, system, output_name, principled, "Base Color")
    principled.inputs["Roughness"].default_value = 0.72
    principled.inputs["Metallic"].default_value = 0.0
    link(tree, principled, "BSDF", output, "Surface")
    return material


def wireframe_material() -> bpy.types.Material:
    material = bpy.data.materials.new(WIREFRAME_PROOF_MATERIAL_NAME)
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = add_node(
        tree,
        "ShaderNodeOutputMaterial",
        "Material_Output",
        (500, 80),
    )
    principled = add_node(
        tree,
        "ShaderNodeBsdfPrincipled",
        "Wireframe_Proof_BSDF",
        (240, 80),
    )
    wire = add_node(
        tree,
        "ShaderNodeWireframe",
        "Measured_Mesh_Wireframe",
        (-260, 80),
    )
    wire.use_pixel_size = True
    wire.inputs["Size"].default_value = 0.65
    mix_node = add_node(
        tree,
        "ShaderNodeMixRGB",
        "Wire_Over_Clay",
        (-20, 130),
    )
    mix_node.inputs["Color1"].default_value = linear_rgba(204, 196, 182)
    mix_node.inputs["Color2"].default_value = linear_rgba(24, 27, 30)
    link(tree, wire, "Fac", mix_node, "Factor")
    link(tree, mix_node, "Color", principled, "Base Color")
    principled.inputs["Roughness"].default_value = 0.82
    link(tree, principled, "BSDF", output, "Surface")
    return material


def compact_band(distance_m: float, half_width_m: float) -> float:
    normalized = abs(distance_m) / half_width_m
    if normalized >= 1.0:
        return 0.0
    return math.cos(0.5 * math.pi * normalized) ** 2


def chevron_masks(
    u: float,
    radial_m: float,
    moulding: dict[str, Any],
) -> dict[str, float]:
    chevron_line_m = 0.048 + 0.104 * abs(u)
    signed = radial_m - chevron_line_m
    half_width = float(moulding["band_half_width_m"])
    roll_a = compact_band(
        signed - float(moulding["roll_center_offsets_m"][0]),
        half_width,
    )
    roll_b = compact_band(
        signed - float(moulding["roll_center_offsets_m"][1]),
        half_width,
    )
    roll = max(roll_a, roll_b)
    hollow = compact_band(
        signed - float(moulding["hollow_center_offset_m"]),
        half_width,
    )
    quiet = max(0.0, 1.0 - max(roll, hollow))
    ink = max(
        hollow,
        0.38 * compact_band(signed + 0.025, half_width * 0.55),
    )
    highlight = max(
        roll_a * (0.74 + 0.26 * max(0.0, -u)),
        roll_b * (0.74 + 0.26 * max(0.0, u)),
    )
    return {
        "roll": min(1.0, roll),
        "hollow": min(1.0, hollow),
        "quiet": min(1.0, quiet),
        "ink": min(1.0, ink),
        "highlight": min(1.0, highlight),
    }


def rotate_uv(
    value: tuple[float, float],
    quarter_turns: int,
    mirror_u: bool,
    mirror_v: bool,
) -> tuple[float, float]:
    u, v = value
    if mirror_u:
        u = -u
    if mirror_v:
        v = -v
    for _ in range(quarter_turns % 4):
        u, v = -v, u
    return u, v


def set_point_float_attribute(
    mesh: bpy.types.Mesh,
    name: str,
    values: Iterable[float],
) -> None:
    attribute = mesh.attributes.new(name=name, type="FLOAT", domain="POINT")
    for item, value in zip(attribute.data, values, strict=True):
        item.value = float(value)


def set_point_int_attribute(
    mesh: bpy.types.Mesh,
    name: str,
    values: Iterable[int],
) -> None:
    attribute = mesh.attributes.new(name=name, type="INT", domain="POINT")
    for item, value in zip(attribute.data, values, strict=True):
        item.value = int(value)


def set_point_vector_attribute(
    mesh: bpy.types.Mesh,
    name: str,
    values: Iterable[tuple[float, float, float]],
) -> None:
    attribute = mesh.attributes.new(
        name=name,
        type="FLOAT_VECTOR",
        domain="POINT",
    )
    for item, value in zip(attribute.data, values, strict=True):
        item.vector = value


def set_face_boolean_attribute(
    mesh: bpy.types.Mesh,
    name: str,
    values: Iterable[bool],
) -> None:
    attribute = mesh.attributes.new(name=name, type="BOOLEAN", domain="FACE")
    for item, value in zip(attribute.data, values, strict=True):
        item.value = bool(value)


def build_voussoir_mesh(
    stone_id: int,
    pattern: dict[str, Any],
) -> bpy.types.Mesh:
    completion = pattern["authored_completion"]
    moulding = pattern["moulding"]
    source = pattern["source_measurement"]
    segments_u = int(moulding["front_segments"]["tangential"])
    segments_v = int(moulding["front_segments"]["radial"])
    body_angle = math.radians(float(completion["body_angle_deg"]))
    inner_radius = float(completion["inner_radius_m"])
    radial_height = float(source["height_m"])
    depth = float(source["depth_m"])
    rng = random.Random(int(pattern["seed"]) + stone_id * 104729)
    variant = stone_id % int(pattern["material_variation"]["variants"])
    transform_a = pattern["material_variation"]["uv_transforms"][variant]
    transform_b = pattern["material_variation"]["uv_transforms"][
        (variant + 1) % int(pattern["material_variation"]["variants"])
    ]
    phase_a = (rng.random() * 0.512, rng.random() * 0.512)
    phase_b = (rng.random() * 0.091, rng.random() * 0.091)

    vertices: list[tuple[float, float, float]] = []
    uv_a_values: list[tuple[float, float]] = []
    uv_b_values: list[tuple[float, float]] = []
    roll_values: list[float] = []
    hollow_values: list[float] = []
    quiet_values: list[float] = []
    ink_values: list[float] = []
    highlight_values: list[float] = []

    for front in (True, False):
        for radial_index in range(segments_v + 1):
            v = radial_index / segments_v
            radius = inner_radius + radial_height * v
            radial_m = radial_height * v
            for tangent_index in range(segments_u + 1):
                u = -1.0 + 2.0 * tangent_index / segments_u
                angle = 0.5 * body_angle * u
                masks = chevron_masks(u, radial_m, moulding)
                relief = (
                    float(moulding["roll_crest_m"]) * masks["roll"]
                    - float(moulding["hollow_depression_m"]) * masks["hollow"]
                    if front
                    else 0.0
                )
                x = radius * math.cos(angle)
                z = radius * math.sin(angle)
                y = -0.5 * depth - relief if front else 0.5 * depth
                vertices.append((x, y, z))
                tangent_m = (
                    u
                    * 0.5
                    * body_angle
                    * (inner_radius + 0.5 * radial_height)
                )
                centred = (tangent_m, radial_m - 0.5 * radial_height)
                transformed_a = rotate_uv(
                    centred,
                    int(transform_a["quarter_turn"]),
                    bool(transform_a["mirror_u"]),
                    bool(transform_a["mirror_v"]),
                )
                transformed_b = rotate_uv(
                    centred,
                    int(transform_b["quarter_turn"]),
                    bool(transform_b["mirror_u"]),
                    bool(transform_b["mirror_v"]),
                )
                uv_a_values.append(
                    (
                        transformed_a[0] + phase_a[0],
                        transformed_a[1] + phase_a[1],
                    )
                )
                uv_b_values.append(
                    (
                        transformed_b[0] + phase_b[0],
                        transformed_b[1] + phase_b[1],
                    )
                )
                roll_values.append(masks["roll"] if front else 0.0)
                hollow_values.append(masks["hollow"] if front else 0.0)
                quiet_values.append(masks["quiet"] if front else 0.0)
                ink_values.append(masks["ink"] if front else 0.0)
                highlight_values.append(
                    masks["highlight"] if front else 0.0
                )

    row = segments_u + 1
    grid = row * (segments_v + 1)

    def index(front: bool, radial: int, tangent: int) -> int:
        return (0 if front else grid) + radial * row + tangent

    faces: list[tuple[int, int, int, int]] = []
    front_flags: list[bool] = []
    for radial in range(segments_v):
        for tangent in range(segments_u):
            faces.append(
                (
                    index(True, radial, tangent),
                    index(True, radial + 1, tangent),
                    index(True, radial + 1, tangent + 1),
                    index(True, radial, tangent + 1),
                )
            )
            front_flags.append(True)
    front_face_count = len(faces)
    for radial in range(segments_v):
        for tangent in range(segments_u):
            faces.append(
                (
                    index(False, radial, tangent),
                    index(False, radial, tangent + 1),
                    index(False, radial + 1, tangent + 1),
                    index(False, radial + 1, tangent),
                )
            )
            front_flags.append(False)
    for radial in range(segments_v):
        faces.append(
            (
                index(True, radial, 0),
                index(False, radial, 0),
                index(False, radial + 1, 0),
                index(True, radial + 1, 0),
            )
        )
        front_flags.append(False)
        faces.append(
            (
                index(True, radial, segments_u),
                index(True, radial + 1, segments_u),
                index(False, radial + 1, segments_u),
                index(False, radial, segments_u),
            )
        )
        front_flags.append(False)
    for tangent in range(segments_u):
        faces.append(
            (
                index(True, 0, tangent),
                index(True, 0, tangent + 1),
                index(False, 0, tangent + 1),
                index(False, 0, tangent),
            )
        )
        front_flags.append(False)
        faces.append(
            (
                index(True, segments_v, tangent),
                index(False, segments_v, tangent),
                index(False, segments_v, tangent + 1),
                index(True, segments_v, tangent + 1),
            )
        )
        front_flags.append(False)

    mesh = bpy.data.meshes.new(f"IGGY_ChevronVoussoir_{stone_id:02d}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.validate(verbose=True, clean_customdata=False)
    mesh.update(calc_edges=True)
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    for polygon in mesh.polygons[:front_face_count]:
        polygon.use_smooth = True

    uv_a = mesh.uv_layers.new(name="IGGY_StoneUV_A")
    uv_b = mesh.uv_layers.new(name="IGGY_StoneUV_B")
    for loop in mesh.loops:
        uv_a.data[loop.index].uv = uv_a_values[loop.vertex_index]
        uv_b.data[loop.index].uv = uv_b_values[loop.vertex_index]

    point_count = len(vertices)
    set_point_int_attribute(
        mesh,
        "iggy_voussoir_id",
        [stone_id] * point_count,
    )
    set_point_int_attribute(
        mesh,
        "iggy_material_variant",
        [variant] * point_count,
    )
    set_point_vector_attribute(
        mesh,
        "iggy_material_phase",
        [(phase_a[0], phase_a[1], phase_b[0])] * point_count,
    )
    set_point_float_attribute(mesh, "iggy_chevron_roll", roll_values)
    set_point_float_attribute(mesh, "iggy_chevron_hollow", hollow_values)
    set_point_float_attribute(mesh, "iggy_chevron_quiet", quiet_values)
    set_point_float_attribute(mesh, "iggy_chevron_ink", ink_values)
    set_point_float_attribute(
        mesh,
        "iggy_chevron_highlight",
        highlight_values,
    )
    set_face_boolean_attribute(
        mesh,
        "iggy_carved_trim",
        [True] * len(faces),
    )
    set_face_boolean_attribute(
        mesh,
        "iggy_chevron_front",
        front_flags,
    )
    set_face_boolean_attribute(
        mesh,
        "iggy_fracture_interior",
        [False] * len(faces),
    )
    mesh["iggy_front_face_count"] = front_face_count
    mesh["iggy_closed_volume"] = True
    mesh["iggy_bevel_m"] = 0.0
    return mesh


def build_product(
    pattern: dict[str, Any],
    product_collection: bpy.types.Collection,
    material: bpy.types.Material,
) -> list[bpy.types.Object]:
    count = int(pattern["authored_completion"]["count"])
    pitch = math.radians(
        float(pattern["authored_completion"]["pitch_angle_deg"])
    )
    objects: list[bpy.types.Object] = []
    for stone_id in range(count):
        mesh = build_voussoir_mesh(stone_id, pattern)
        obj = bpy.data.objects.new(
            f"IGGY_ChevronVoussoir_{stone_id:02d}",
            mesh,
        )
        center_angle = (stone_id + 0.5) * pitch
        obj.rotation_euler[1] = -center_angle
        obj.data.materials.append(material)
        obj["iggy_role"] = "measured_chevron_voussoir"
        obj["iggy_source_id"] = "S01_old_sarum_catalogue_item_55"
        obj["iggy_catalogue_item"] = 55
        obj["iggy_voussoir_id"] = stone_id
        obj["iggy_center_angle_deg"] = math.degrees(center_angle)
        obj["iggy_catalogue_radial_height_m"] = 0.2
        obj["iggy_catalogue_inner_chord_m"] = 0.14
        obj["iggy_catalogue_outer_chord_m"] = 0.18
        obj["iggy_catalogue_depth_m"] = 0.27
        obj["iggy_body_angle_deg"] = float(
            pattern["authored_completion"]["body_angle_deg"]
        )
        obj["iggy_joint_gap_centerline_m"] = 0.003
        obj["iggy_damage_enabled"] = False
        obj["iggy_fracture_enabled"] = False
        product_collection.objects.link(obj)
        objects.append(obj)
    return objects


def add_cube(
    name: str,
    location: tuple[float, float, float],
    dimensions: tuple[float, float, float],
    target_collection: bpy.types.Collection,
    material: bpy.types.Material,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    for source_collection in list(obj.users_collection):
        source_collection.objects.unlink(obj)
    target_collection.objects.link(obj)
    obj.data.materials.append(material)
    obj["iggy_role"] = "proof_context_only"
    return obj


def build_proof_context(
    proof_collection: bpy.types.Collection,
) -> list[bpy.types.Object]:
    backdrop_material = simple_material(
        "IGGY_MAT_ProofBackdrop",
        linear_rgba(68, 72, 76),
        0.9,
    )
    support_material = simple_material(
        "IGGY_MAT_ProofSupport",
        linear_rgba(126, 118, 106),
        0.82,
    )
    backdrop = add_cube(
        "IGGY_ProofBackdrop",
        (0.0, 0.30, 0.72),
        (2.70, 0.10, 1.75),
        proof_collection,
        backdrop_material,
    )
    left = add_cube(
        "IGGY_ProofSpringing_Left",
        (-0.83, 0.02, -0.12),
        (0.34, 0.31, 0.24),
        proof_collection,
        support_material,
    )
    right = add_cube(
        "IGGY_ProofSpringing_Right",
        (0.83, 0.02, -0.12),
        (0.34, 0.31, 0.24),
        proof_collection,
        support_material,
    )
    return [backdrop, left, right]


def add_area_light(
    name: str,
    location: tuple[float, float, float],
    energy: float,
    size: float,
    color: tuple[float, float, float],
    target: tuple[float, float, float],
) -> bpy.types.Object:
    data = bpy.data.lights.new(name=name, type="AREA")
    data.energy = energy
    data.shape = "DISK"
    data.size = size
    data.color = color
    obj = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(obj)
    obj.location = location
    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    return obj


def configure_lighting() -> None:
    add_area_light(
        "IGGY_Key_LargeNeutral",
        (-2.7, -3.2, 3.8),
        620.0,
        3.0,
        (1.0, 0.91, 0.82),
        (0.0, 0.0, 0.72),
    )
    add_area_light(
        "IGGY_Fill_Cool",
        (3.0, -1.1, 2.1),
        260.0,
        2.5,
        (0.72, 0.82, 1.0),
        (0.0, 0.0, 0.70),
    )
    add_area_light(
        "IGGY_Grazing_Rim",
        (-2.0, -0.25, 1.0),
        340.0,
        1.0,
        (1.0, 0.76, 0.56),
        (-0.1, -0.1, 0.72),
    )
    world = bpy.context.scene.world
    world.use_nodes = True
    background = world.node_tree.nodes.get("Background")
    if background is not None:
        background.inputs["Color"].default_value = (0.018, 0.022, 0.028, 1.0)
        background.inputs["Strength"].default_value = 0.18


def create_camera() -> bpy.types.Object:
    data = bpy.data.cameras.new("IGGY_ChevronProofCamera")
    data.lens = 58.0
    data.sensor_width = 36.0
    obj = bpy.data.objects.new("IGGY_ChevronProofCamera", data)
    bpy.context.scene.collection.objects.link(obj)
    bpy.context.scene.camera = obj
    return obj


def aim_camera(
    camera: bpy.types.Object,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    lens: float,
) -> None:
    camera.location = location
    camera.data.lens = lens
    direction = Vector(target) - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def configure_scene() -> None:
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 900
    scene.render.resolution_y = 900
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.film_transparent = False
    scene.render.use_file_extension = True
    scene.render.image_settings.color_depth = "8"
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = -0.35
    scene.view_settings.gamma = 1.0
    scene.render.resolution_percentage = 100


def assign_product_material(
    product: list[bpy.types.Object],
    material: bpy.types.Material,
) -> None:
    for obj in product:
        obj.data.materials.clear()
        obj.data.materials.append(material)


def render_view(
    output: Path,
    filename: str,
    camera: bpy.types.Object,
    location: tuple[float, float, float],
    target: tuple[float, float, float],
    lens: float,
) -> Path:
    aim_camera(camera, location, target, lens)
    path = output / filename
    bpy.context.scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    if not path.is_file():
        raise RuntimeError(f"Render was not written: {path}")
    return path


def render_proofs(
    output: Path,
    camera: bpy.types.Object,
    product: list[bpy.types.Object],
    live: bpy.types.Material,
    clay: bpy.types.Material,
    moulding_proof: bpy.types.Material,
    identity_proof: bpy.types.Material,
    wireframe: bpy.types.Material,
    selected_proofs: set[str],
) -> dict[str, Path]:
    proofs = output / "proofs"
    proofs.mkdir(parents=True, exist_ok=True)
    views: dict[str, Path] = {}
    if selected_proofs & {"neutral_clay_front", "neutral_clay_grazing"}:
        assign_product_material(product, clay)
    if "neutral_clay_front" in selected_proofs:
        views["neutral_clay_front"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_neutral_clay_front.png",
            camera,
            (0.0, -4.0, 0.67),
            (0.0, 0.0, 0.56),
            62.0,
        )
    if "neutral_clay_grazing" in selected_proofs:
        views["neutral_clay_grazing"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_neutral_clay_grazing.png",
            camera,
            (-2.25, -2.55, 1.08),
            (0.0, -0.02, 0.62),
            58.0,
        )
    if selected_proofs & {
        "live_material_front",
        "live_material_grazing",
        "measured_close",
        "distance_read",
    }:
        assign_product_material(product, live)
    if "live_material_front" in selected_proofs:
        views["live_material_front"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_live_material_front.png",
            camera,
            (0.0, -4.0, 0.67),
            (0.0, 0.0, 0.56),
            62.0,
        )
    if "live_material_grazing" in selected_proofs:
        views["live_material_grazing"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_live_material_grazing.png",
            camera,
            (-2.25, -2.55, 1.08),
            (0.0, -0.02, 0.62),
            58.0,
        )
    if "measured_close" in selected_proofs:
        views["measured_close"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_measured_close.png",
            camera,
            (0.0, -1.02, 1.02),
            (0.0, -0.08, 0.84),
            72.0,
        )
    if "distance_read" in selected_proofs:
        views["distance_read"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_distance_read.png",
            camera,
            (0.0, -7.2, 0.72),
            (0.0, 0.0, 0.56),
            72.0,
        )
    if "moulding_proof" in selected_proofs:
        assign_product_material(product, moulding_proof)
        views["moulding_proof"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_moulding_proof.png",
            camera,
            (0.0, -3.85, 0.67),
            (0.0, 0.0, 0.56),
            62.0,
        )
    if "identity_proof" in selected_proofs:
        assign_product_material(product, identity_proof)
        views["identity_proof"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_identity_proof.png",
            camera,
            (0.0, -3.85, 0.67),
            (0.0, 0.0, 0.56),
            62.0,
        )
    if "wireframe_proof" in selected_proofs:
        assign_product_material(product, wireframe)
        views["wireframe_proof"] = render_view(
            proofs,
            "cathedral_stone_trim_fracture_v1_wireframe_proof.png",
            camera,
            (-1.45, -3.1, 1.18),
            (0.0, 0.0, 0.62),
            60.0,
        )
    assign_product_material(product, live)
    return views


def resolve_proof_selection(
    output: Path,
    proof_set: str,
    requested_proofs: Iterable[str],
) -> set[str]:
    requested = set(requested_proofs)
    unknown = requested - set(PROOF_IDS)
    if unknown:
        raise ValueError("Unknown proof ids: " + ", ".join(sorted(unknown)))
    if proof_set == "all":
        if requested:
            raise ValueError("--proof is only valid with --proof-set changed")
        selected = set(PROOF_IDS)
    elif proof_set == "changed":
        if not requested:
            raise ValueError("--proof-set changed requires at least one --proof")
        selected = requested
    elif proof_set == "none":
        if requested:
            raise ValueError("--proof-set none cannot receive --proof")
        selected = set()
    else:
        raise ValueError(f"Unsupported proof set: {proof_set}")
    if proof_set != "all" and output.resolve() == DEFAULT_OUTPUT.resolve():
        raise ValueError(
            "Fast or changed proof builds require an explicit non-canonical "
            "--output directory"
        )
    return selected


def edge_manifold(mesh: bpy.types.Mesh) -> bool:
    usage = [0] * len(mesh.edges)
    edge_lookup = {
        tuple(sorted(edge.vertices)): edge.index
        for edge in mesh.edges
    }
    for polygon in mesh.polygons:
        for edge_index in polygon.edge_keys:
            usage[edge_lookup[tuple(sorted(edge_index))]] += 1
    return all(count == 2 for count in usage)


def node_inventory(group: bpy.types.NodeTree) -> list[dict[str, Any]]:
    def socket_default(socket: bpy.types.NodeSocket) -> Any:
        if not hasattr(socket, "default_value"):
            return None
        value = socket.default_value
        if isinstance(value, (str, bool, int, float)):
            return value
        try:
            return [float(component) for component in value]
        except (TypeError, ValueError):
            return str(value)

    result = []
    for node in sorted(group.nodes, key=lambda item: item.name):
        entry: dict[str, Any] = {
            "name": node.name,
            "bl_idname": node.bl_idname,
            "inputs": [
                {
                    "name": socket.name,
                    "identifier": socket.identifier,
                    "type": socket.type,
                    "linked": socket.is_linked,
                    "default": (
                        None if socket.is_linked else socket_default(socket)
                    ),
                }
                for socket in node.inputs
            ],
            "outputs": [
                {
                    "name": socket.name,
                    "identifier": socket.identifier,
                    "type": socket.type,
                    "linked": socket.is_linked,
                }
                for socket in node.outputs
            ],
        }
        if node.bl_idname == "ShaderNodeTexImage":
            entry["image"] = node.image.name if node.image is not None else None
            entry["image_packed"] = bool(
                node.image is not None and node.image.packed_file is not None
            )
            entry["color_space"] = (
                node.image.colorspace_settings.name
                if node.image is not None
                else None
            )
            entry["extension"] = node.extension
            entry["interpolation"] = node.interpolation
            entry["projection"] = node.projection
        if node.bl_idname == "ShaderNodeAttribute":
            entry["attribute_name"] = node.attribute_name
        if node.bl_idname == "ShaderNodeMath":
            entry["operation"] = node.operation
        if node.bl_idname == "ShaderNodeVectorMath":
            entry["operation"] = node.operation
        if node.bl_idname == "ShaderNodeMixRGB":
            entry["blend_type"] = node.blend_type
            entry["use_clamp"] = node.use_clamp
        if node.bl_idname in {
            "ShaderNodeSeparateColor",
            "ShaderNodeCombineColor",
        }:
            entry["mode"] = node.mode
        if node.bl_idname == "ShaderNodeNormalMap":
            entry["space"] = node.space
            entry["uv_map"] = node.uv_map
        if node.bl_idname == "ShaderNodeBump":
            entry["invert"] = node.invert
        if node.bl_idname == "ShaderNodeUVMap":
            entry["uv_map"] = node.uv_map
        if node.bl_idname == "ShaderNodeMapRange":
            entry["interpolation_type"] = node.interpolation_type
            entry["clamp"] = node.clamp
        result.append(entry)
    return result


def link_inventory(group: bpy.types.NodeTree) -> list[dict[str, str]]:
    return sorted(
        (
            {
                "from_node": item.from_node.name,
                "from_socket": item.from_socket.name,
                "to_node": item.to_node.name,
                "to_socket": item.to_socket.name,
            }
            for item in group.links
        ),
        key=lambda item: (
            item["from_node"],
            item["from_socket"],
            item["to_node"],
            item["to_socket"],
        ),
    )


def geometry_summary(
    product: list[bpy.types.Object],
    pattern: dict[str, Any],
) -> dict[str, Any]:
    first = product[0]
    inner_radius = float(pattern["authored_completion"]["inner_radius_m"])
    outer_radius = float(pattern["authored_completion"]["outer_radius_m"])
    body_angle = math.radians(
        float(pattern["authored_completion"]["body_angle_deg"])
    )
    inner_chord = 2.0 * inner_radius * math.sin(0.5 * body_angle)
    outer_chord = 2.0 * outer_radius * math.sin(0.5 * body_angle)
    mean_radius = 0.5 * (inner_radius + outer_radius)
    pitch = math.radians(
        float(pattern["authored_completion"]["pitch_angle_deg"])
    )
    centerline_gap = mean_radius * (pitch - body_angle)
    return {
        "object_count": len(product),
        "all_separate_mesh_datablocks": (
            len({obj.data.name for obj in product}) == len(product)
        ),
        "vertices_per_object": len(first.data.vertices),
        "polygons_per_object": len(first.data.polygons),
        "front_polygons_per_object": int(first.data["iggy_front_face_count"]),
        "all_meshes_manifold": all(edge_manifold(obj.data) for obj in product),
        "all_object_scales_unit": all(
            all(abs(component - 1.0) < 1.0e-9 for component in obj.scale)
            for obj in product
        ),
        "catalogue_inner_chord_m": inner_chord,
        "derived_outer_chord_m": outer_chord,
        "centerline_joint_gap_m": centerline_gap,
        "clear_diameter_m": 2.0 * inner_radius,
        "outer_diameter_m": 2.0 * outer_radius,
        "bevel_modifier_count": sum(
            1
            for obj in product
            for modifier in obj.modifiers
            if modifier.type == "BEVEL"
        ),
        "boolean_modifier_count": sum(
            1
            for obj in product
            for modifier in obj.modifiers
            if modifier.type == "BOOLEAN"
        ),
    }


def write_blender_manifest(
    output: Path,
    texture_manifest: dict[str, Any],
    pattern: dict[str, Any],
    product: list[bpy.types.Object],
    group: bpy.types.NodeTree,
    views: dict[str, Path],
    blend_path: Path,
    proof_set: str,
) -> dict[str, Any]:
    geometry = geometry_summary(product, pattern)
    node_data = node_inventory(group)
    links = link_inventory(group)
    required_attributes = {
        "iggy_voussoir_id",
        "iggy_material_phase",
        "iggy_material_variant",
        "iggy_chevron_roll",
        "iggy_chevron_hollow",
        "iggy_chevron_quiet",
        "iggy_chevron_ink",
        "iggy_chevron_highlight",
        "iggy_carved_trim",
        "iggy_chevron_front",
        "iggy_fracture_interior",
    }
    attribute_names = {
        attribute.name
        for obj in product
        for attribute in obj.data.attributes
        if not attribute.name.startswith(".")
        and attribute.name
        not in {"position", "sharp_face", "IGGY_StoneUV_A", "IGGY_StoneUV_B"}
    }
    manifest = {
        "schema": BLENDER_MANIFEST_SCHEMA,
        "blender_version": bpy.app.version_string,
        "texture_manifest_schema": texture_manifest["schema"],
        "pattern_schema": pattern["schema"],
        "build_mode": {
            "proof_set": proof_set,
            "selected_proofs": sorted(views),
            "canonical_candidate_complete": (
                proof_set == "all" and set(views) == set(PROOF_IDS)
            ),
        },
        "source_measurement": pattern["source_measurement"],
        "authored_completion": pattern["authored_completion"],
        "geometry": geometry,
        "attributes": {
            "required": sorted(required_attributes),
            "present": sorted(attribute_names),
            "all_required_present": required_attributes <= attribute_names,
            "fracture_values_all_zero": all(
                not item.value
                for obj in product
                for item in obj.data.attributes[
                    "iggy_fracture_interior"
                ].data
            ),
        },
        "uv_layers": {
            obj.name: sorted(layer.name for layer in obj.data.uv_layers)
            for obj in product
        },
        "shader": {
            "group_name": group.name,
            "node_count": len(group.nodes),
            "link_count": len(group.links),
            "nodes": node_data,
            "links": links,
            "all_image_nodes_packed": all(
                entry.get("image_packed", True) for entry in node_data
            ),
            "normal_map_nodes": [
                entry["name"]
                for entry in node_data
                if entry["bl_idname"] == "ShaderNodeNormalMap"
            ],
            "bump_nodes": [
                entry["name"]
                for entry in node_data
                if entry["bl_idname"] == "ShaderNodeBump"
            ],
            "attribute_nodes": sorted(
                entry["attribute_name"]
                for entry in node_data
                if entry["bl_idname"] == "ShaderNodeAttribute"
            ),
        },
        "proofs": {
            name: {
                "filename": path.name,
                "sha256": sha256(path),
                "bytes": path.stat().st_size,
            }
            for name, path in sorted(views.items())
        },
        "blend": {
            "filename": blend_path.name,
            "sha256": sha256(blend_path),
            "bytes": blend_path.stat().st_size,
        },
        "validation": {
            "individual_voussoirs": geometry["object_count"] == 16,
            "closed_manifold_stones": geometry["all_meshes_manifold"],
            "catalogue_inner_chord_within_0p005m": (
                abs(geometry["catalogue_inner_chord_m"] - 0.14) <= 0.005
            ),
            "catalogue_outer_chord_within_0p005m": (
                abs(geometry["derived_outer_chord_m"] - 0.18) <= 0.005
            ),
            "centerline_joint_gap_is_0p003m": (
                abs(geometry["centerline_joint_gap_m"] - 0.003) <= 1.0e-9
            ),
            "no_bevel_modifiers": geometry["bevel_modifier_count"] == 0,
            "no_boolean_modifiers": geometry["boolean_modifier_count"] == 0,
            "required_attributes_present": required_attributes <= attribute_names,
            "all_texture_nodes_packed": all(
                entry.get("image_packed", True) for entry in node_data
            ),
            "normal_map_live": any(
                link_item["from_node"] == "IGGY_OpenGLBodyNormal64mm"
                and link_item["to_node"] == "IGGY_DecorrelatedBodyHeight91mm"
                for link_item in links
            ),
            "height_not_inverted": (
                group.nodes["IGGY_DecorrelatedBodyHeight91mm"].invert is False
            ),
            "damage_authored": False,
            "fracture_authored": False,
            "proof_count": len(views),
            "complete_proof_set": set(views) == set(PROOF_IDS),
        },
    }
    manifest_path = (
        output / "cathedral_stone_trim_fracture_v1_blender_manifest.json"
    )
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    return manifest


def build(
    output: Path,
    proof_set: str = "all",
    requested_proofs: Iterable[str] = (),
) -> dict[str, Any]:
    profile = read_json(PROFILE_PATH)
    pattern = read_json(PATTERN_PATH)
    if profile["schema"] != (
        "iggy3d.material.cathedral_stone_trim_fracture_v1.profile.v2"
    ):
        raise ValueError("Unexpected chevron profile schema")
    if pattern["schema"] != "iggy3d.pattern.chevron_voussoir_portal.v2":
        raise ValueError("Unexpected chevron pattern schema")
    selected_proofs = resolve_proof_selection(
        output,
        proof_set,
        requested_proofs,
    )
    texture_manifest = ensure_texture_family(output)
    reset_scene()
    configure_scene()
    product_collection = collection(PRODUCT_COLLECTION_NAME)
    group = create_material_group(output)
    live = configure_material(group)
    product = build_product(pattern, product_collection, live)
    views: dict[str, Path] = {}
    if selected_proofs:
        proof_collection = collection(PROOF_COLLECTION_NAME)
        clay = simple_material(
            CLAY_MATERIAL_NAME,
            linear_rgba(186, 181, 171),
            0.86,
        )
        moulding_proof = group_proof_material(
            MOULDING_PROOF_MATERIAL_NAME,
            group,
            "Moulding Proof",
        )
        identity_proof = group_proof_material(
            IDENTITY_PROOF_MATERIAL_NAME,
            group,
            "Identity Proof",
        )
        wireframe = wireframe_material()
        build_proof_context(proof_collection)
        configure_lighting()
        camera = create_camera()
        views = render_proofs(
            output,
            camera,
            product,
            live,
            clay,
            moulding_proof,
            identity_proof,
            wireframe,
            selected_proofs,
        )
    scene = bpy.context.scene
    scene["iggy_asset_schema"] = (
        "iggy3d.asset.chevron_voussoir_portal.measured.v2"
    )
    scene["iggy_source_measurement"] = (
        "S01_old_sarum_catalogue_item_55"
    )
    scene["iggy_product_object_count"] = len(product)
    scene["iggy_proof_set"] = proof_set
    scene["iggy_selected_proofs"] = json.dumps(sorted(views))
    scene["iggy_fracture_enabled"] = False
    scene["iggy_damage_enabled"] = False
    blend_path = output / "cathedral_stone_trim_fracture_v1.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path), check_existing=False)
    manifest = write_blender_manifest(
        output,
        texture_manifest,
        pattern,
        product,
        group,
        views,
        blend_path,
        proof_set,
    )
    return manifest


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument(
        "--proof-set",
        choices=("none", "changed", "all"),
        default="all",
    )
    parser.add_argument("--proof", action="append", default=[])
    return parser.parse_args(argv)


def main() -> int:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    args = parse_args(argv)
    manifest = build(args.output, args.proof_set, args.proof)
    print(
        json.dumps(
            {
                "status": "ok",
                "schema": manifest["schema"],
                "object_count": manifest["geometry"]["object_count"],
                "proof_count": manifest["validation"]["proof_count"],
                "proof_set": manifest["build_mode"]["proof_set"],
            },
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

### Build and validation

```sh
/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python-exit-code 1 --python assets/creative/materials/cathedral_stone_trim_fracture_v1/build_cathedral_stone_trim_fracture_v1.py -- --output /private/tmp/iggy-chevron-fast --proof-set none
/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python-exit-code 1 --python assets/creative/materials/cathedral_stone_trim_fracture_v1/build_cathedral_stone_trim_fracture_v1.py -- --output /private/tmp/iggy-chevron-changed --proof-set changed --proof live_material_front
/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python-exit-code 1 --python assets/creative/materials/cathedral_stone_trim_fracture_v1/build_cathedral_stone_trim_fracture_v1.py
/Applications/Blender.app/Contents/MacOS/Blender -b assets/creative/materials/cathedral_stone_trim_fracture_v1/output/cathedral_stone_trim_fracture_v1.blend --python-exit-code 1 --python tests/unit/cathedral_stone_trim_fracture_v1_blend_tests.py
```

Execution record: sixteen objects; 1,650 vertices and 1,648 polygons per object; all edges used twice; inner chord 0.139999999995 m; outer chord 0.178485252852 m; centreline joint 0.003000000002 m; zero bevels; zero Booleans; all required attributes present; all images packed; 79 live group nodes and 103 exact links; nine unique proof hashes; nine reopen tests green.

## Completion audit

- [x] Every demand has complete executable test and production code directly beneath it.
- [x] Every new helper called by the code is defined in the same complete file.
- [x] All five textures have live Blender consumers.
- [x] Every final node and link is enumerated from the executed Blender manifest.
- [x] Normal RGB reaches Principled only through Normal Map and Bump nodes.
- [x] All semantic pattern masks are live in colour, roughness, or proof outputs.
- [x] Every numeric construction, moulding, and surface claim is measured, proxy, inherited, or explicitly authored.
- [x] All nine causal layers name sources, outputs, live consumers, isolated proofs, rest rules, and supporting claims.
- [x] Texture memory, object, vertex, polygon, shader, image, and proof budgets are explicit and green.
- [x] Fast builds cannot overwrite canonical output.
- [x] Neutral clay, live material, close, distance, grazing, mask, identity, and wireframe proofs exist.
- [x] Fracture, damage, weathering, jamb reconstruction, and Unreal parity remain explicitly outside the capability.
- [x] The isolated fixture is only a production candidate; actual-target proof and user review remain required for accepted status.

Generated by `assets/creative/materials/workflow/scripts/render_coded_demands.py` from the executed sources and manifests. Re-run it after any code, node, texture-inventory, or test change; drift between this document and the implementation is a validation failure.
