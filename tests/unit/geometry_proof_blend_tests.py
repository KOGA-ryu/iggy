"""Blend-level acceptance tests for the SINC_GeometryProof_Profile pilot.

Follows the forged_fasteners pattern: skip when Blender is absent, otherwise
reopen the built artifact headlessly and interrogate it. The strongest gate
here rebuilds the proof INSIDE the reopened .blend from its embedded spec
text with an arc direction flipped - proving the saved file's inputs are
editable and that proof and asset geometry share one source.
"""

import hashlib
import json
import subprocess
import sys
import unittest
from pathlib import Path

sys.dont_write_bytecode = True  # keep __pycache__ out of the asset package

ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / "assets" / "creative" / "geometry_proof"
OUT = PACKAGE / "output" / "paley_pl1_fig6_v1"
BLEND_PATH = OUT / "paley_pl1_fig6_v1_proof.blend"
MANIFEST_PATH = OUT / "paley_pl1_fig6_v1_manifest.json"
SPEC_PATH = PACKAGE / "profiles" / "paley_pl1_fig6_v1.json"
BLENDER = Path("/Applications/Blender.app/Contents/MacOS/Blender")

sys.path.insert(0, str(PACKAGE))


def run_in_blend(blend, script):
    completed = subprocess.run(
        [str(BLENDER), "--background", str(blend), "--python-expr", script],
        capture_output=True, text=True, timeout=300)
    for line in completed.stdout.splitlines():
        if line.startswith("SINC_JSON="):
            return json.loads(line[len("SINC_JSON="):])
    raise AssertionError(
        f"no SINC_JSON payload in Blender output:\n{completed.stdout}\n{completed.stderr}")


@unittest.skipUnless(BLENDER.is_file(), "Blender is not installed")
class GeometryProofBlendTests(unittest.TestCase):
    def test_artifact_exists_with_truthful_manifest(self):
        self.assertTrue(BLEND_PATH.is_file())
        manifest = json.loads(MANIFEST_PATH.read_text())
        self.assertEqual(manifest["status"], "SINC_GEOMETRY_PROOF_BUILT")
        self.assertEqual(manifest["winding"], "COUNTERCLOCKWISE")
        self.assertEqual(manifest["single_source"], "profile_compiler.compile_profile")
        self.assertTrue(manifest["reference_placed"])
        for view in ("front", "opposite", "three_quarter", "wire"):
            render = OUT / "renders" / f"{view}.png"
            self.assertTrue(render.is_file(), f"missing proof render {view}")
            self.assertGreater(render.stat().st_size, 20_000,
                               f"suspiciously small proof render {view}")

    def test_artifact_is_bound_to_the_committed_spec(self):
        # The stale-green hole: editing profiles/<id>.json without rebuilding
        # must fail this suite, not silently pass against the old artifact.
        manifest = json.loads(MANIFEST_PATH.read_text())
        spec = json.loads(SPEC_PATH.read_text())
        expected = hashlib.sha256(
            json.dumps(spec, sort_keys=True).encode()).hexdigest()
        self.assertEqual(manifest["spec_sha256"], expected,
                         "built artifact is stale relative to the committed spec - rebuild")

    def test_master_curve_matches_compiler_output(self):
        # Binds the saved geometry to the repo compiler, not just to itself:
        # a rebuild that dropped the unit scale (or any divergence) fails here.
        from profile_compiler import compile_profile
        spec = json.loads(SPEC_PATH.read_text())
        compiled = compile_profile(spec)
        s = spec["construction_plane"]["scale_m_per_unit"]
        expected = [(x * s, y * s) for x, y in compiled.outline]
        probe = json.dumps(expected[:5] + expected[-2:])
        script = f'''
import bpy, json
master = bpy.data.objects["SINC_Master_paley_pl1_fig6_v1"]
pts = [tuple(p.co)[:2] for p in master.data.splines[0].points]
probe = json.loads('{probe}')
checked = pts[:5] + pts[-2:]
ok = len(pts) == {len(expected)} and all(
    abs(a[0] - b[0]) < 1e-6 and abs(a[1] - b[1]) < 1e-6
    for a, b in zip(checked, probe))  # 1e-6: above float32 storage noise, far below any real defect
print("SINC_JSON=" + json.dumps({{"ok": ok, "count": len(pts)}}))
'''
        payload = run_in_blend(BLEND_PATH, script)
        self.assertTrue(payload["ok"],
                        "master curve diverges from compile_profile output")

    def test_reopened_blend_is_editable_and_single_sourced(self):
        manifest = json.loads(MANIFEST_PATH.read_text())
        script = r'''
import bpy, json
texts = sorted(t.name for t in bpy.data.texts)
master = bpy.data.objects["SINC_Master_paley_pl1_fig6_v1"]
spline = master.data.splines[0]
real_curves = [o.name for o in bpy.data.objects
               if o.type == "CURVE" and o.name.startswith("SINC_")]
spec = json.loads(bpy.data.texts["profile_spec.json"].as_string())
before = [tuple(p.co) for p in spline.points]
spline_type = spline.type
cyclic = spline.use_cyclic_u
generated_from = master.get("sinc_generated_from")
extrude_positive = master.data.extrude > 0

# Edit the embedded input: flip the bowtell, rebuild inside this file.
seg = next(s for s in spec["segments"] if s["name"] == "bowtell")
seg["direction"] = "CLOCKWISE"
t = bpy.data.texts["profile_spec.json"]; t.clear(); t.write(json.dumps(spec))
exec(bpy.data.texts["rebuild_proof.py"].as_string())
master2 = bpy.data.objects["SINC_Master_paley_pl1_fig6_v1"]
after = [tuple(p.co) for p in master2.data.splines[0].points]

payload = {
    "texts": texts,
    "spline_type": spline_type,
    "cyclic": cyclic,
    "point_count": len(before),
    "real_curve_objects": real_curves,
    "cameras": sorted(o.name for o in bpy.data.objects if o.type == "CAMERA"),
    "packed_images": [i.name for i in bpy.data.images if i.packed_file],
    "generated_from": generated_from,
    "extrude_positive": extrude_positive,
    "rebuild_changed_geometry": before != after,
    "rebuild_point_count": len(after),
}
print("SINC_JSON=" + json.dumps(payload))
'''
        payload = run_in_blend(BLEND_PATH, script)
        for text in ("profile_spec.json", "rebuild_proof.py", "proof_scene.py",
                     "profile_compiler.py", "profile_spec.py"):
            self.assertIn(text, payload["texts"], f"embedded input {text} missing")
        self.assertEqual(payload["spline_type"], "POLY")
        self.assertTrue(payload["cyclic"], "master outline must be closed")
        self.assertEqual(payload["point_count"], manifest["outline_vertices"])
        # No duplicate independently authored curve: the master is the ONLY
        # real curve object (labels are FONT objects, not curves).
        self.assertEqual(payload["real_curve_objects"],
                         ["SINC_Master_paley_pl1_fig6_v1"])
        self.assertEqual(payload["cameras"],
                         ["SINC_Cam_Front", "SINC_Cam_Opposite",
                          "SINC_Cam_ThreeQuarter", "SINC_Cam_Wire"])
        self.assertTrue(payload["packed_images"], "reference image must be packed")
        self.assertEqual(payload["generated_from"], "profile_compiler.compile_profile")
        self.assertTrue(payload["extrude_positive"], "proof extrusion missing")
        # THE flip gate, inside the reopened artifact: editing the embedded
        # spec and rerunning the embedded rebuild changes the master geometry.
        self.assertTrue(payload["rebuild_changed_geometry"])
        self.assertLess(payload["rebuild_point_count"], payload["point_count"],
                        "CW bowtell sweeps 93 deg, so the outline must lose samples")


if __name__ == "__main__":
    unittest.main()
