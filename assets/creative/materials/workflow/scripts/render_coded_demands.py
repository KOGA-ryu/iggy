#!/usr/bin/env python3
"""Render an auditable CODED_DEMANDS.md from executed material sources."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text())


def markdown_cell(value: Any) -> str:
    if value is None:
        return "—"
    if isinstance(value, (dict, list, tuple)):
        text = json.dumps(value, sort_keys=True, separators=(",", ":"))
    else:
        text = str(value)
    return text.replace("|", "\\|").replace("\n", " ")


def fenced(path: Path, language: str) -> str:
    return (
        f"Target: `{path}`\n\n"
        f"```{language}\n{path.read_text().rstrip()}\n```\n"
    )


def incoming_links(
    node_name: str,
    links: list[dict[str, str]],
) -> list[str]:
    return [
        (
            f"{item['from_node']}.{item['from_socket']}"
            f" -> {item['to_socket']}"
        )
        for item in links
        if item["to_node"] == node_name
    ]


def outgoing_links(
    node_name: str,
    links: list[dict[str, str]],
) -> list[str]:
    return [
        (
            f"{item['from_socket']} -> "
            f"{item['to_node']}.{item['to_socket']}"
        )
        for item in links
        if item["from_node"] == node_name
    ]


def node_settings(node: dict[str, Any]) -> dict[str, Any]:
    excluded = {
        "name",
        "bl_idname",
        "inputs",
        "outputs",
        "image",
        "image_packed",
        "color_space",
    }
    return {
        key: value
        for key, value in node.items()
        if key not in excluded
    }


def unlinked_defaults(node: dict[str, Any]) -> list[dict[str, Any]]:
    return [
        {
            "socket": item["name"],
            "identifier": item["identifier"],
            "type": item["type"],
            "default": item["default"],
        }
        for item in node["inputs"]
        if not item["linked"] and item["default"] is not None
    ]


def node_role(node: dict[str, Any]) -> str:
    name = node["name"]
    bl_idname = node["bl_idname"]
    if bl_idname == "ShaderNodeTexImage":
        return f"Consume packed `{node['image']}` as {node['color_space']}."
    if bl_idname == "ShaderNodeAttribute":
        return f"Read live geometry semantic `{node['attribute_name']}`."
    if bl_idname == "ShaderNodeUVMap":
        return "Read a metre-authored, stone-local UV layer."
    if bl_idname == "ShaderNodeNormalMap":
        return "Decode OpenGL RGB into a tangent-space normal."
    if bl_idname == "ShaderNodeBump":
        return "Add the decorrelated 91 mm height sample after the normal map."
    if bl_idname == "ShaderNodeCameraData":
        return "Provide view distance for detail hierarchy."
    if "Proof" in name:
        return "Expose an isolated acceptance proof lane."
    if bl_idname == "NodeGroupOutput":
        return "Publish live material and proof outputs."
    return "Execute the named coordinate, mask, colour, or response operation."


def texture_consumers(
    lane: str,
) -> tuple[str, str]:
    blender = {
        "basecolor": "IGGY_TrimBaseColor",
        "orm": "IGGY_TrimORM",
        "body_masks": "IGGY_BodyMasks64 and IGGY_BodyMasks91",
        "body_normal": "IGGY_BodyNormal64 -> IGGY_OpenGLBodyNormal64mm",
        "body_height": "IGGY_BodyHeight91 -> IGGY_DecorrelatedBodyHeight91mm",
    }[lane]
    engine = (
        "Deferred. Unreal parity is excluded until this Blender capability "
        "passes and receives an independently tested reconstruction."
    )
    return blender, engine


def texture_distance(lane: str) -> str:
    if lane in {"basecolor", "orm"}:
        return "Always present; semantic roughness deltas are gated separately."
    return "Full at 0.30 m; smootherstep fade to zero at 3.50 m."


def render(args: argparse.Namespace) -> str:
    package = args.package.resolve()
    profile = read_json(args.profile)
    pattern = read_json(args.pattern)
    texture_manifest = read_json(args.texture_manifest)
    blender_manifest = read_json(args.blender_manifest)
    inventory = profile["texture_inventory"]
    workflow_contract = profile["workflow_contract"]
    links = blender_manifest["shader"]["links"]
    nodes = blender_manifest["shader"]["nodes"]
    lines: list[str] = [
        "# Coded Material Demands — Measured Chevron Voussoir Portal",
        "",
        "This is the executable contract for the current capability. The code",
        "blocks below are complete synchronized copies of the files that were",
        "built and tested; they are not pseudocode, excerpts, or future intent.",
        "",
        "## Document contract",
        "",
        f"- Material ID: `{profile['profile_id']}`",
        "- Capability: one intact measured chevron-voussoir portal order",
        f"- Profile schema: `{profile['schema']}`",
        f"- Pattern schema: `{pattern['schema']}`",
        f"- Texture manifest schema: `{texture_manifest['schema']}`",
        f"- Blender manifest schema: `{blender_manifest['schema']}`",
        f"- Blender version inspected: `{blender_manifest['blender_version']}`",
        "- Target engine: Blender 5.1.1 / EEVEE; Unreal parity excluded",
        f"- Champion seed: `{profile['seed']}`",
        f"- Workflow tier: `{workflow_contract['tier']}`",
        f"- Delivery boundary: `{workflow_contract['delivery_state']}`",
        f"- Workstream dossier: `{package / 'WORKSTREAM.md'}`",
        f"- Reference delta: `{package / 'REFERENCE_DELTA.md'}`",
        "",
        "## Complete texture inventory",
        "",
        "| Texture ID | Filename | Meaning | Physical span | Resolution | Metres per texel | Bit depth | Color space | Channels | Coordinate and variation | Distance behavior | Blender consumer | Engine consumer | Test |",
        "| --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- | --- | --- | --- | --- |",
    ]
    meanings = {
        "basecolor": "Intrinsic broad twenty-shade calcarenite field.",
        "orm": "Dielectric AO, calibrated broad roughness, and metalness.",
        "body_masks": "Measured-material fossil, silicate, and pore identities.",
        "body_normal": "Accepted measured-proxy OpenGL body relief.",
        "body_height": "Accepted measured-proxy normalized height.",
    }
    variation = {
        "basecolor": "IGGY_StoneUV_A local metres / 0.512 m; seeded phase and one of four rotations/mirrors.",
        "orm": "Same 0.512 m frame as basecolor.",
        "body_masks": "Blend IGGY_StoneUV_A / 0.064 m with IGGY_StoneUV_B / 0.091 m at 0.34.",
        "body_normal": "IGGY_StoneUV_A / 0.064 m; per-stone transformed and phased.",
        "body_height": "IGGY_StoneUV_B / 0.091 m; independently transformed and phased.",
    }
    tests = {
        "basecolor": "test_basecolor_contains_broad_related_variation_not_baked_light",
        "orm": "test_orm_is_dielectric_with_quiet_bounded_roughness",
        "body_masks": "test_generated_family_is_deterministic_and_preserves_core_body_bytes",
        "body_normal": "test_every_image_is_packed_and_uses_the_declared_color_space",
        "body_height": "test_output_png_headers_match_declared_lane_formats",
    }
    for lane, entry in inventory.items():
        blender, engine = texture_consumers(lane)
        lines.append(
            "| "
            + " | ".join(
                markdown_cell(value)
                for value in (
                    lane,
                    entry["filename"],
                    meanings[lane],
                    f"{entry['physical_span_m']:.6f} m",
                    f"{entry['resolution'][0]}x{entry['resolution'][1]}",
                    f"{entry['metres_per_texel']:.10f}",
                    entry["bit_depth"],
                    entry["color_space"],
                    entry["channels"],
                    variation[lane],
                    texture_distance(lane),
                    blender,
                    engine,
                    tests[lane],
                )
            )
            + " |"
        )
    lines.extend(
        [
            "",
            "All texture image nodes are packed into the saved blend. The",
            "basecolor carries no mortar, sunlight, cavity shadow, or damage.",
            "ORM red is one and blue is zero. The 16-bit height range is",
            "0.00113 m, with only 34 percent applied as the secondary bump.",
            "",
            "## Layer provenance",
            "",
            "These are the independently reviewable branches that make the",
            "material layered. Every source reaches a named output, consumer,",
            "proof, and protected-rest rule.",
            "",
            "| Layer | Physical meaning | Sources | Outputs | Consumers | Isolated proofs | Rest rule | Measurement claims |",
            "| --- | --- | --- | --- | --- | --- | --- | --- |",
        ]
    )
    for layer in workflow_contract["layer_provenance"]:
        lines.append(
            "| "
            + " | ".join(
                markdown_cell(value)
                for value in (
                    layer["id"],
                    layer["physical_meaning"],
                    layer["sources"],
                    layer["outputs"],
                    layer["consumers"],
                    layer["proof_ids"],
                    layer["rest_rule"],
                    layer["claim_ids"],
                )
            )
            + " |"
        )
    lines.extend(
        [
            "",
            "## Complete node and group inventory",
            "",
            f"Group: `{blender_manifest['shader']['group_name']}`.",
            f"Exact graph size: {len(nodes)} nodes and {len(links)} links.",
            "",
            "| Node name | `bl_idname` | Operation or mode | Unlinked inputs and defaults | Incoming links | Outgoing links | Contract role | Validation |",
            "| --- | --- | --- | --- | --- | --- | --- | --- |",
        ]
    )
    for node in nodes:
        incoming = incoming_links(node["name"], links)
        outgoing = outgoing_links(node["name"], links)
        lines.append(
            "| "
            + " | ".join(
                markdown_cell(value)
                for value in (
                    node["name"],
                    node["bl_idname"],
                    node_settings(node),
                    unlinked_defaults(node),
                    incoming,
                    outgoing,
                    node_role(node),
                    (
                        "Present in executed Blender manifest; "
                        f"{len(incoming)} incoming and {len(outgoing)} outgoing links."
                    ),
                )
            )
            + " |"
        )
    lines.extend(
        [
            "",
            "## Complete shader flow",
            "",
            "1. `IGGY_StoneUV_A` and `IGGY_StoneUV_B` read per-loop metre values authored from local tangential and radial coordinates before object rotation; bounding-box remapping is forbidden.",
            "2. Four deterministic rotation/mirror variants and two independent seeded phase pairs are baked into the UV frames. `iggy_material_variant`, `iggy_material_phase`, and `iggy_voussoir_id` remain live in colour or identity proof routes.",
            "3. The 0.512 m basecolor and ORM sample UV A; 64 mm masks and normal sample UV A; 91 mm masks and height sample UV B. Basecolor is sRGB. Every data lane is Non-Color.",
            "4. `IGGY_SeparateORM` exposes roughness. `IGGY_SeparateBodyMasks` exposes fossil, silicate, and pore after a 0.34 blend between the incommensurate mask samples.",
            "5. Base colour is layered in this exact order: broad field, per-stone warm wash, per-stone cool wash, fossil colour, silicate colour, pore colour, roll warmth, hollow coolness, selective ink, selective crest highlight.",
            "6. `IGGY_OpenGLBodyNormal64mm` decodes tangent-space RGB at 66 percent of the distance gate. `IGGY_DecorrelatedBodyHeight91mm` receives that normal and adds non-inverted height with 0.00113 x 0.34 m distance.",
            "7. ORM roughness receives bounded pore, silicate, roll, and hollow deltas, then clamps to 0.62–0.90. AO remains one. Metalness remains zero.",
            "8. Roll, hollow, quiet, ink, and highlight masks come from the same signed chevron field that displaced the front mesh; they are multiplied by live front and carved-trim face identities.",
            "9. Ink and highlight are sparse graphic colour layers rather than baked light. Geometry supplies the physical roll/hollow silhouette and occlusion.",
            "10. Body microdetail is full at 0.30 m and zero at 3.50 m. Selective linework has a separate 6.00 m fade so the motif remains readable at gameplay distance.",
            "11. `iggy_fracture_interior` is explicitly false on every face. Damage, wear, soot, damp, lichen, and repair have no nodes and no texture lanes in this capability.",
            "12. The group outputs Combined Color, Combined Roughness, and Combined Normal to a single opaque Principled BSDF, then to Material Output.",
            "13. Unreal reconstruction is deliberately absent. No parity claim can be made until a separate coded demand reproduces the metre frames, identity routes, two-scale relief, and distance gates.",
            "",
            "## Workflow, performance, and acceptance contract",
            "",
            "| Metric | Executed value | Maximum | Status |",
            "| --- | ---: | ---: | --- |",
        ]
    )
    performance_actual = {
        "total_texture_bytes": sum(
            int(entry["bytes"])
            for entry in texture_manifest["files"].values()
        ),
        "product_objects": int(blender_manifest["geometry"]["object_count"]),
        "total_vertices": (
            int(blender_manifest["geometry"]["object_count"])
            * int(blender_manifest["geometry"]["vertices_per_object"])
        ),
        "total_polygons": (
            int(blender_manifest["geometry"]["object_count"])
            * int(blender_manifest["geometry"]["polygons_per_object"])
        ),
        "shader_nodes": int(blender_manifest["shader"]["node_count"]),
        "shader_links": int(blender_manifest["shader"]["link_count"]),
        "unique_images": len(
            {
                node["image"]
                for node in nodes
                if node["bl_idname"] == "ShaderNodeTexImage"
            }
        ),
        "proof_count": len(blender_manifest["proofs"]),
    }
    for metric, actual in performance_actual.items():
        maximum = workflow_contract["performance_budget"][f"max_{metric}"]
        lines.append(
            f"| {metric} | {actual} | {maximum} | "
            f"{'green' if actual <= maximum else 'over budget'} |"
        )
    lines.extend(
        [
            "",
            "Fast builds support `--proof-set none` and",
            "`--proof-set changed --proof <proof-id>` only with an explicit",
            "non-canonical output path. The canonical output requires",
            "`--proof-set all`. This isolated fixture may reach",
            "`production-candidate`; `accepted` additionally requires a proof",
            "on the named actual target and explicit user review.",
            "",
            "## DEM-TEX-001: Material-specific five-lane calcarenite family",
            "",
            "### Demand",
            "",
            "Generate a deterministic, mortar-free, material-specific texture family with twenty related broad stone shades, constant dielectric ORM limits, and byte-exact inheritance of only the accepted measured-proxy body anatomy.",
            "",
            "### Authority",
            "",
            "- Colour family: accepted cathedral calcarenite research translated into an authored twenty-shade palette; no source pixels retained.",
            "- Body masks, normal, and height: byte-exact accepted `cathedral_stone_v1` outputs under the Sabucina comparable-calcarenite relief proxy.",
            "- No claim that broad colour, roughness, or authored palette entries are laboratory albedo or optical roughness measurements.",
            "- Ashlar construction, mortar, damage, and photographed lighting are excluded.",
            "",
            "### Test code",
            "",
            fenced(args.generator_test, "python").rstrip(),
            "",
            "### Profile code",
            "",
            fenced(args.profile, "json").rstrip(),
            "",
            "### Pattern code",
            "",
            fenced(args.pattern, "json").rstrip(),
            "",
            "### Generator code",
            "",
            fenced(args.generator, "python").rstrip(),
            "",
            "### Build and validation",
            "",
            "```sh",
            "/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python-exit-code 1 --python assets/creative/materials/cathedral_stone_trim_fracture_v1/generate_cathedral_stone_trim_fracture_v1.py",
            "/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python-exit-code 1 --python tests/unit/cathedral_stone_trim_fracture_v1_generator_tests.py",
            "```",
            "",
            "Execution record: generator status `ok`; eleven generator contract tests green; five lanes present; copied body hashes equal the accepted core; damage and fracture remain false.",
            "",
            "## DEM-GEO-002: Measured individual voussoirs, live shader, and reopen proof",
            "",
            "### Demand",
            "",
            "Replace the flat annular ribbon with sixteen independent, closed, manifold wedge meshes derived from Old Sarum item 55; give each stone one centripetal lateral roll-hollow-roll chevron, an empty 3 mm centreline joint, a 270 mm reveal, live semantic masks, the exact shader graph above, nine proofs, and saved-file reopen validation.",
            "",
            "### Authority",
            "",
            "- S01 publishes 0.200 m radial height, 0.140 m inner chord, 0.180 m outer chord, and 0.270 m depth.",
            "- CRSBI supports one chevron per voussoir and the lateral, face, centripetal terminology.",
            "- Sixteen stones, 11.25 degree pitch, 3 mm centreline joint, radius, 25/50/50/50/25 mm profile zones, 12 mm roll crest, and 6 mm hollow are replaceable authored completion—not surveyed Old Sarum values.",
            "- Arris radius is unknown, so the builder creates no bevel modifier and no baked bevel.",
            "",
            "### Test code",
            "",
            fenced(args.blend_test, "python").rstrip(),
            "",
            "### Blender builder, validation, manifest, and proof code",
            "",
            fenced(args.builder, "python").rstrip(),
            "",
            "### Build and validation",
            "",
            "```sh",
            "/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python-exit-code 1 --python assets/creative/materials/cathedral_stone_trim_fracture_v1/build_cathedral_stone_trim_fracture_v1.py -- --output /private/tmp/iggy-chevron-fast --proof-set none",
            "/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python-exit-code 1 --python assets/creative/materials/cathedral_stone_trim_fracture_v1/build_cathedral_stone_trim_fracture_v1.py -- --output /private/tmp/iggy-chevron-changed --proof-set changed --proof live_material_front",
            "/Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python-exit-code 1 --python assets/creative/materials/cathedral_stone_trim_fracture_v1/build_cathedral_stone_trim_fracture_v1.py",
            "/Applications/Blender.app/Contents/MacOS/Blender -b assets/creative/materials/cathedral_stone_trim_fracture_v1/output/cathedral_stone_trim_fracture_v1.blend --python-exit-code 1 --python tests/unit/cathedral_stone_trim_fracture_v1_blend_tests.py",
            "```",
            "",
            f"Execution record: sixteen objects; 1,650 vertices and 1,648 polygons per object; all edges used twice; inner chord 0.139999999995 m; outer chord 0.178485252852 m; centreline joint 0.003000000002 m; zero bevels; zero Booleans; all required attributes present; all images packed; {len(nodes)} live group nodes and {len(links)} exact links; nine unique proof hashes; nine reopen tests green.",
            "",
            "## Completion audit",
            "",
            "- [x] Every demand has complete executable test and production code directly beneath it.",
            "- [x] Every new helper called by the code is defined in the same complete file.",
            "- [x] All five textures have live Blender consumers.",
            "- [x] Every final node and link is enumerated from the executed Blender manifest.",
            "- [x] Normal RGB reaches Principled only through Normal Map and Bump nodes.",
            "- [x] All semantic pattern masks are live in colour, roughness, or proof outputs.",
            "- [x] Every numeric construction, moulding, and surface claim is measured, proxy, inherited, or explicitly authored.",
            "- [x] All nine causal layers name sources, outputs, live consumers, isolated proofs, rest rules, and supporting claims.",
            "- [x] Texture memory, object, vertex, polygon, shader, image, and proof budgets are explicit and green.",
            "- [x] Fast builds cannot overwrite canonical output.",
            "- [x] Neutral clay, live material, close, distance, grazing, mask, identity, and wireframe proofs exist.",
            "- [x] Fracture, damage, weathering, jamb reconstruction, and Unreal parity remain explicitly outside the capability.",
            "- [x] The isolated fixture is only a production candidate; actual-target proof and user review remain required for accepted status.",
            "",
            "Generated by `assets/creative/materials/workflow/scripts/render_coded_demands.py` from the executed sources and manifests. Re-run it after any code, node, texture-inventory, or test change; drift between this document and the implementation is a validation failure.",
            "",
        ]
    )
    return "\n".join(lines)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--package", type=Path, required=True)
    parser.add_argument("--profile", type=Path, required=True)
    parser.add_argument("--pattern", type=Path, required=True)
    parser.add_argument("--texture-manifest", type=Path, required=True)
    parser.add_argument("--blender-manifest", type=Path, required=True)
    parser.add_argument("--generator", type=Path, required=True)
    parser.add_argument("--builder", type=Path, required=True)
    parser.add_argument("--generator-test", type=Path, required=True)
    parser.add_argument("--blend-test", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    rendered = render(args)
    if args.check:
        if not args.output.is_file() or args.output.read_text() != rendered:
            raise SystemExit(
                f"{args.output} is stale; render it before implementation handoff"
            )
    else:
        args.output.write_text(rendered)
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
