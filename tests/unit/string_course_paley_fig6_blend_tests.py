"""Blend tests for the first production consumer of a profile donor.

The load-bearing assertions: the built asset's recorded donor sha must match
the CURRENT donor spec (edit the donor without rebuilding the consumer and
this suite goes red), and the saved mesh must be exactly the compiled donor
outline swept - two rings of the outline's vertices, nothing re-authored.
"""

import hashlib
import json
import subprocess
import sys
import unittest
from pathlib import Path

sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parents[2]
PKG = ROOT / "assets" / "creative" / "architecture" / "masonry" / "string_course_paley_fig6_v1"
OUT = PKG / "output"
BLEND = OUT / "string_course_paley_fig6_v1.blend"
MANIFEST = OUT / "string_course_paley_fig6_v1_manifest.json"
DONOR = ROOT / "assets" / "creative" / "geometry_proof" / "profiles" / "paley_pl1_fig6_v1.json"
BLENDER = Path("/Applications/Blender.app/Contents/MacOS/Blender")

sys.path.insert(0, str(ROOT / "assets" / "creative" / "geometry_proof"))


@unittest.skipUnless(BLENDER.is_file(), "Blender is not installed")
class StringCourseTests(unittest.TestCase):
    def test_provenance_chain_is_live(self):
        manifest = json.loads(MANIFEST.read_text())
        self.assertEqual(manifest["status"], "SINC_STRING_COURSE_BUILT")
        donor = json.loads(DONOR.read_text())
        current_sha = hashlib.sha256(
            json.dumps(donor, sort_keys=True).encode()).hexdigest()
        self.assertEqual(manifest["consumed_donor"]["spec_sha256"], current_sha,
                         "consumer is stale relative to the donor spec - rebuild")
        self.assertEqual(manifest["consumed_donor"]["profile_id"], "paley_pl1_fig6_v1")

    def test_mesh_is_the_compiled_outline_swept(self):
        from profile_compiler import compile_profile
        donor = json.loads(DONOR.read_text())
        compiled = compile_profile(donor)
        s = donor["construction_plane"]["scale_m_per_unit"]
        manifest = json.loads(MANIFEST.read_text())
        expected_ring = len(compiled.outline)
        probe = json.dumps([[x * s, y * s] for x, y in compiled.outline[:4]])
        script = f'''
import bpy, json
obj = bpy.data.objects["SINC_StringCourse_PaleyFig6"]
mesh = obj.data
ring = {expected_ring}
# local XY are the section coordinates; local Z is the sweep axis
zs = sorted({{round(v.co.z, 6) for v in mesh.vertices}})
first_ring = [v for v in mesh.vertices if abs(v.co.z - zs[0]) < 1e-9]
by_xy = {{(round(v.co.x, 6), round(v.co.y, 6)) for v in first_ring}}
probe = json.loads('{probe}')
payload = {{
    "verts": len(mesh.vertices),
    "ring_expected": ring,
    "two_rings": len(mesh.vertices) == 2 * ring,
    "z_levels": len(zs),
    "sweep_span": zs[-1] - zs[0],
    "probe_present": all(any(abs(px - x) < 1e-6 and abs(py - y) < 1e-6
                             for (x, y) in by_xy) for px, py in probe),
    "donor_prop": obj.get("sinc_donor_profile"),
    "sha_prop": obj.get("sinc_donor_sha256"),
}}
print("SINC_JSON=" + json.dumps(payload))
'''
        completed = subprocess.run(
            [str(BLENDER), "--background", str(BLEND), "--python-expr", script],
            capture_output=True, text=True, timeout=300)
        payload = None
        for line in completed.stdout.splitlines():
            if line.startswith("SINC_JSON="):
                payload = json.loads(line[len("SINC_JSON="):])
        self.assertIsNotNone(payload, completed.stdout + completed.stderr)
        self.assertTrue(payload["two_rings"],
                        f"expected exactly two outline rings, got {payload['verts']} verts "
                        f"for ring {payload['ring_expected']}")
        self.assertEqual(payload["z_levels"], 2)
        self.assertAlmostEqual(payload["sweep_span"], manifest["length_m"], places=5)
        self.assertTrue(payload["probe_present"],
                        "section ring does not contain the compiled outline points")
        self.assertEqual(payload["donor_prop"], "paley_pl1_fig6_v1")
        self.assertEqual(payload["sha_prop"], manifest["consumed_donor"]["spec_sha256"])


if __name__ == "__main__":
    unittest.main()
