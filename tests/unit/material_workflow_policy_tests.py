#!/usr/bin/env python3
"""Policy tests for the five-phase material workflow enforcement tools."""

from __future__ import annotations

import importlib.util
import json
from pathlib import Path
from types import SimpleNamespace
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = (
    REPO_ROOT
    / "assets"
    / "creative"
    / "materials"
    / "workflow"
    / "scripts"
)


def load_module(name: str, path: Path) -> object:
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


GATE = load_module(
    "material_workflow_gate_under_test",
    SCRIPTS / "material_workflow_gate.py",
)
AUDITOR = load_module(
    "material_package_auditor_under_test",
    SCRIPTS / "audit_material_package.py",
)


def valid_workstream() -> str:
    return (
        "# Workstream\n\n"
        "| Stage | Status | Evidence | Blocking issue |\n"
        "| --- | --- | --- | --- |\n"
        "| 04 Coded implementation blueprint | complete | reviewed | |\n"
    )


def valid_coded_demands() -> str:
    return """# Coded Demands

## DEM-TEX-001: Bounded result

### Test code

```python
def test_result():
    assert 1 + 1 == 2
```

### Generator code

```python
def build_result():
    return 2
```
"""


class MaterialWorkflowGateTests(unittest.TestCase):
    def freeze_args(
        self,
        package: Path,
        target: Path,
    ) -> SimpleNamespace:
        return SimpleNamespace(
            package=package,
            workstream=package / "WORKSTREAM.md",
            coded_demands=package / "CODED_DEMANDS.md",
            tier="hero-master",
            target=[target],
            red_gate_evidence=["expected red: missing capability"],
            revision_note="test freeze",
        )

    def make_package(self, root: Path) -> tuple[Path, Path]:
        package = root / "material"
        package.mkdir()
        (package / "WORKSTREAM.md").write_text(valid_workstream())
        (package / "CODED_DEMANDS.md").write_text(valid_coded_demands())
        target = package / "builder.py"
        target.write_text("VALUE = 1\n")
        return package, target

    def test_freeze_and_open_build_record_verified_order(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            package, target = self.make_package(Path(directory))
            self.assertEqual(
                GATE.command_freeze(self.freeze_args(package, target)),
                0,
            )
            self.assertEqual(
                GATE.command_open_build(SimpleNamespace(package=package)),
                0,
            )
            state = json.loads(
                (package / "WORKFLOW_STATE.json").read_text()
            )
            revision = state["revisions"][0]
            self.assertTrue(
                revision["build_entry"]["target_baselines_verified"]
            )
            self.assertEqual(revision["demand_ids"], ["DEM-TEX-001"])

    def test_open_build_rejects_a_target_edited_after_freeze(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            package, target = self.make_package(Path(directory))
            GATE.command_freeze(self.freeze_args(package, target))
            target.write_text("VALUE = 2\n")
            with self.assertRaisesRegex(
                ValueError,
                "changed before build entry",
            ):
                GATE.command_open_build(SimpleNamespace(package=package))

    def test_verify_entry_rejects_coded_demands_changed_after_open(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            package, target = self.make_package(Path(directory))
            GATE.command_freeze(self.freeze_args(package, target))
            GATE.command_open_build(SimpleNamespace(package=package))
            (package / "CODED_DEMANDS.md").write_text(
                valid_coded_demands().replace("return 2", "return 3")
            )
            with self.assertRaisesRegex(
                ValueError,
                "changed after build entry",
            ):
                GATE.command_verify_entry(SimpleNamespace(package=package))

    def test_freeze_rejects_placeholder_code(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            package, target = self.make_package(Path(directory))
            (package / "CODED_DEMANDS.md").write_text(
                valid_coded_demands().replace("return 2", "pass")
            )
            with self.assertRaisesRegex(ValueError, "placeholder"):
                GATE.command_freeze(self.freeze_args(package, target))

    def test_freeze_rejects_incomplete_stage_four(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            package, target = self.make_package(Path(directory))
            (package / "WORKSTREAM.md").write_text(
                valid_workstream().replace("complete", "pending")
            )
            with self.assertRaisesRegex(ValueError, "Stage 04"):
                GATE.command_freeze(self.freeze_args(package, target))


class MaterialPackageAuditPolicyTests(unittest.TestCase):
    def test_claim_audit_rejects_an_uncovered_numeric_dimension(self) -> None:
        profile = {"dimensions": {"measured_m": 0.2, "mystery_m": 0.4}}
        contract = {
            "claim_roots": [
                {"document": "profile", "pointer": "/dimensions"}
            ],
            "measurement_claims": [
                {
                    "id": "measured",
                    "document": "profile",
                    "pointer": "/dimensions/measured_m",
                    "value": 0.2,
                    "status": "measured",
                    "source_id": "S01",
                }
            ],
        }
        passed, detail = AUDITOR.check_measurement_claims(
            contract,
            {"profile": profile, "pattern": {}},
        )
        self.assertFalse(passed)
        self.assertTrue(
            any("mystery_m" in error for error in detail["errors"])
        )

    def test_route_audit_rejects_dormant_images_and_attributes(self) -> None:
        manifest = {
            "shader": {
                "nodes": [
                    {
                        "name": "DormantImage",
                        "bl_idname": "ShaderNodeTexImage",
                    },
                    {
                        "name": "DormantAttribute",
                        "bl_idname": "ShaderNodeAttribute",
                    },
                ],
                "links": [],
                "attribute_nodes": ["iggy_live"],
            },
            "attributes": {
                "present": ["iggy_live", "iggy_unowned"],
            },
        }
        passed, detail = AUDITOR.check_routes(
            {"storage_only_attributes": []},
            manifest,
        )
        self.assertFalse(passed)
        self.assertEqual(detail["dormant_image_nodes"], ["DormantImage"])
        self.assertEqual(
            detail["dormant_attribute_nodes"],
            ["DormantAttribute"],
        )
        self.assertEqual(
            detail["unowned_geometry_attributes"],
            ["iggy_unowned"],
        )

    def test_performance_audit_rejects_budget_overrun(self) -> None:
        contract = {
            "performance_budget": {
                "max_total_texture_bytes": 10,
                "max_product_objects": 1,
                "max_total_vertices": 10,
                "max_total_polygons": 10,
                "max_shader_nodes": 1,
                "max_shader_links": 1,
                "max_unique_images": 1,
                "max_proof_count": 1,
            }
        }
        texture_manifest = {"files": {"base": {"bytes": 11}}}
        blender_manifest = {
            "geometry": {
                "object_count": 2,
                "vertices_per_object": 6,
                "polygons_per_object": 6,
            },
            "shader": {
                "node_count": 2,
                "link_count": 2,
                "nodes": [
                    {
                        "bl_idname": "ShaderNodeTexImage",
                        "image": "A",
                    },
                    {
                        "bl_idname": "ShaderNodeTexImage",
                        "image": "B",
                    },
                ],
            },
            "proofs": {"one": {}, "two": {}},
        }
        passed, detail = AUDITOR.check_performance(
            contract,
            texture_manifest,
            blender_manifest,
        )
        self.assertFalse(passed)
        self.assertEqual(len(detail["errors"]), 8)

    def test_layer_audit_rejects_missing_bands_and_unknown_proof(self) -> None:
        passed, detail = AUDITOR.check_layer_provenance(
            {
                "tier": "hero-master",
                "measurement_claims": [{"id": "known"}],
                "layer_provenance": [
                    {
                        "id": "construction",
                        "physical_meaning": "separate units",
                        "sources": ["pattern"],
                        "outputs": ["geometry"],
                        "consumers": ["builder"],
                        "proof_ids": ["missing_proof"],
                        "rest_rule": "no surface noise",
                        "claim_ids": ["known"],
                    }
                ],
            },
            {"proofs": {"neutral": {}}},
        )
        self.assertFalse(passed)
        self.assertTrue(
            any("Required layer provenance" in error for error in detail["errors"])
        )
        self.assertTrue(
            any("unknown proof IDs" in error for error in detail["errors"])
        )

    def test_surface_method_audit_rejects_opaque_stack_and_damage(self) -> None:
        passed, detail = AUDITOR.check_surface_method_contract(
            {
                "surface_method_contract": {
                    "version": 1,
                    "effect_stack": [
                        {
                            "id": "brush_a",
                            "order": 20,
                            "physical_role": "directional tooling",
                            "operation": "tangent normal detail blend",
                            "coordinate_frame": "component tangent metres",
                            "quiet_rule": "absent on cast faces",
                            "inputs": ["normal_a"],
                            "mask_sources": ["worked_face"],
                            "outputs": ["normal"],
                            "consumers": ["Principled Normal"],
                            "proof_ids": ["missing"],
                        },
                        {
                            "id": "brush_b",
                            "order": 20,
                            "physical_role": "",
                            "operation": "blend",
                            "coordinate_frame": "",
                            "quiet_rule": "",
                            "inputs": [],
                            "mask_sources": [],
                            "outputs": [],
                            "consumers": [],
                            "proof_ids": [],
                        },
                    ],
                    "geometry_to_map_transfer": [],
                    "damage_placement": {"enabled": True},
                }
            },
            {"proofs": {"grazing": {}}},
        )
        self.assertFalse(passed)
        self.assertTrue(
            any("orders must be unique" in error for error in detail["errors"])
        )
        self.assertTrue(
            any("unknown proof IDs" in error for error in detail["errors"])
        )
        self.assertTrue(
            any("eligible_semantics" in error for error in detail["errors"])
        )

    def test_output_audit_rejects_a_stale_file(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            texture_manifest_path = output / "texture_manifest.json"
            blender_manifest_path = output / "blender_manifest.json"
            for path in (
                texture_manifest_path,
                blender_manifest_path,
                output / "base.png",
                output / "asset.blend",
                output / "stale.png",
            ):
                path.write_bytes(b"x")
            passed, detail = AUDITOR.check_output_inventory(
                output,
                texture_manifest_path,
                {"files": {"base": {"filename": "base.png"}}},
                blender_manifest_path,
                {
                    "blend": {"filename": "asset.blend"},
                    "proofs": {},
                },
            )
        self.assertFalse(passed)
        self.assertTrue(
            any(path.endswith("stale.png") for path in detail["unexpected"])
        )

    def test_acceptance_audit_rejects_missing_actual_target_proof(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            reference_delta = Path(directory) / "REFERENCE_DELTA.md"
            reference_delta.write_text(
                "# Delta\n\n## Unresolved differences\n\n- integration\n"
            )
            passed, detail = AUDITOR.check_acceptance(
                {
                    "tier": "hero-master",
                    "acceptance": {
                        "actual_target_required_for_acceptance": True,
                        "actual_target_identifier": (
                            "pending-selection: production consumer"
                        ),
                    },
                },
                "accepted",
                [],
                [],
                reference_delta,
            )
        self.assertFalse(passed)
        self.assertTrue(
            any("actual-target" in error for error in detail["errors"])
        )
        self.assertTrue(
            any("target identifier" in error for error in detail["errors"])
        )
        self.assertTrue(
            any("manual-review" in error for error in detail["errors"])
        )

    def test_workflow_audit_rejects_an_undeclared_production_target(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            package = root / "material"
            package.mkdir()
            coded_demands = package / "CODED_DEMANDS.md"
            coded_demands.write_text(valid_coded_demands())
            declared = package / "declared.py"
            declared.write_text("VALUE = 1\n")
            omitted = package / "omitted.py"
            omitted.write_text("VALUE = 2\n")
            state = {
                "schema": "iggy3d.material_workflow_state.v1",
                "package": str(package.resolve()),
                "current_revision": 1,
                "revisions": [
                    {
                        "revision": 1,
                        "tier": "hero-master",
                        "coded_demands": {
                            "path": str(coded_demands.resolve())
                        },
                        "coded_demands_sha256": AUDITOR.sha256(
                            coded_demands
                        ),
                        "target_baselines": [
                            {"path": str(declared.resolve())}
                        ],
                        "build_entry": {
                            "coded_demands_sha256": AUDITOR.sha256(
                                coded_demands
                            ),
                            "target_baselines_verified": True,
                        },
                    }
                ],
            }
            workflow_state = package / "WORKFLOW_STATE.json"
            workflow_state.write_text(json.dumps(state))
            passed, detail = AUDITOR.check_workflow_state(
                workflow_state,
                package,
                {"tier": "hero-master"},
                coded_demands,
                [declared, omitted],
            )
        self.assertFalse(passed)
        self.assertEqual(detail["omitted_targets"], [str(omitted.resolve())])


if __name__ == "__main__":
    unittest.main()
