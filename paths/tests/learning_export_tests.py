#!/usr/bin/env python3
"""Headless publication regressions against the built app, not a mock compiler."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import export_learning as export


class PublicationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="paths-publish-test-")
        self.root = Path(self.temporary.name).resolve()
        self.author = self.root / "author"
        shutil.copytree(ROOT / "content/authoring/learning/matrix_foundations", self.author)
        self.store = self.root / "store"
        self.baseline = ROOT / "content/write"

    def tearDown(self):
        self.temporary.cleanup()

    def authoring(self, root=None):
        return json.loads(((root or self.author) / "authoring.json").read_text())

    def save_authoring(self, data, root=None):
        ((root or self.author) / "authoring.json").write_bytes(export.encoded(data))

    def arguments(self, command="publish", source=None, store=None):
        source = source or self.author
        metadata = self.authoring(source) if command != "install" else None
        output = self.root / "exports" / metadata["package_id"] / str(metadata["package_version"]) if metadata else None
        return argparse.Namespace(command=command, source=source, target=OPTIONS.target,
                                  store=store or self.store, base_documents=self.baseline,
                                  output=output, library=None)

    def run_publish(self, **kwargs):
        return export.run(self.arguments(**kwargs))

    def active(self):
        return (self.store / "active.json").read_bytes()

    def target_report(self, store=None):
        return export.Target(OPTIONS.target).inspect(store=store or self.store)

    def revise(self):
        author = self.authoring()
        author["package_version"] += 1
        self.save_authoring(author)

    def rejected(self, action, code=None):
        before = self.active() if (self.store / "active.json").exists() else None
        with self.assertRaises(export.ExportError) as caught:
            action()
        if code:
            self.assertEqual(caught.exception.code, code)
        self.assertEqual(self.active() if (self.store / "active.json").exists() else None, before)
        return caught.exception

    def test_publish_reproduce_and_reinstall(self):
        first = self.run_publish()
        self.assertEqual(first["questions"], 316)
        self.assertFalse(first["unchanged"])
        before, modified = self.active(), (self.store / "active.json").stat().st_mtime_ns
        repeated = self.run_publish()
        self.assertTrue(repeated["unchanged"])
        self.assertEqual(self.active(), before)
        self.assertEqual((self.store / "active.json").stat().st_mtime_ns, modified)
        pack = Path(first["package"])
        manifest = json.loads((pack / "bundle.json").read_text())
        self.assertEqual(manifest["entry_documents"], ["documents/chapter.paths.md"])
        self.assertEqual([r["path"] for r in manifest["files"]],
                         ["documents/chapter.paths.md", "documents/shared/row_rules.inc.md", "provenance.json"])
        self.assertEqual(manifest["requires"]["templates"], ["lesson.v1", "matrix.v1"])
        self.assertEqual(manifest["requires"]["figures"], [])
        other = self.root / "other-store"
        installed = self.run_publish(command="install", source=pack, store=other)
        self.assertEqual(installed["generation"], first["generation"])
        self.assertEqual(self.target_report(other)["catalogue"], self.target_report()["catalogue"])

    def test_export_does_not_activate_or_create_store(self):
        result = self.run_publish(command="export")
        self.assertFalse(result["published"])
        self.assertFalse(self.store.exists())
        self.assertTrue((Path(result["package"]) / "receipt.json").exists())

    def test_source_capture_not_replaced_after_validation(self):
        source = self.author / "documents/chapter.paths.md"
        original = source.read_bytes()
        inspect = export.Target.inspect_bytes
        changed = False
        def inspect_then_edit(target, documents):
            nonlocal changed
            result = inspect(target, documents)
            if not changed and "matrix_foundations/chapter.paths.md" in documents:
                source.write_bytes(original.replace(b"@after [1, 1 | 5] [0, -3 | -9]", b"@after [1, 1 | 5] [0, -3 | -8]"))
                changed = True
            return result
        with patch.object(export.Target, "inspect_bytes", inspect_then_edit):
            result = self.run_publish()
        self.assertTrue(changed)
        self.assertEqual((Path(result["package"]) / "documents/chapter.paths.md").read_bytes(), original)
        self.assertNotEqual(source.read_bytes(), original)
        self.assertTrue(self.target_report()["accepted"])

    def test_frozen_teaching_changes_rejected(self):
        self.run_publish()
        self.revise()
        source = self.author / "documents/shared/row_rules.inc.md"
        source.write_text(source.read_text().replace("nonzero number", "nonzero real number"))
        error = self.rejected(self.run_publish, "question.changed")
        self.assertIn("published_matrix_question", str(error))
        self.assertTrue(self.target_report()["accepted"])

    def test_removed_question_rejected(self):
        self.run_publish()
        self.revise()
        source = self.author / "documents/chapter.paths.md"
        source.write_text(source.read_text().split("@question ")[0].replace("@practice published_matrix_question\n", ""))
        author = self.authoring()
        author["sources"][0]["content_ids"] = ["published_matrix_reading"]
        self.save_authoring(author)
        self.rejected(self.run_publish, "question.changed")

    def test_reading_update_and_filename_change_preserve_stamps(self):
        self.run_publish()
        before = self.target_report()["catalogue"]["questions"]
        self.revise()
        source = self.author / "documents/chapter.paths.md"
        source.write_text(source.read_text().replace("We will eliminate x", "In this chapter, we will eliminate x"))
        source.rename(source.with_name("z_chapter.paths.md"))
        self.run_publish()
        self.assertEqual(before, self.target_report()["catalogue"]["questions"])

    def test_immutable_release_identity(self):
        self.run_publish()
        source = self.author / "documents/chapter.paths.md"
        source.write_text(source.read_text().replace("We will eliminate x", "First we eliminate x"))
        self.rejected(self.run_publish, "release.immutable")

    def test_bad_math_and_capabilities_have_source_diagnostics(self):
        source = self.author / "documents/chapter.paths.md"
        original = source.read_text()
        mutations = [("@template matrix.v1", "@template matrix.v99"),
                     ("@after [1, 1 | 5] [0, -3 | -9]", "@after [1, 1 | 5] [0, -3 | -8]"),
                     ("@include shared/row_rules.inc.md", "@include shared/missing.inc.md"),
                     ("@template lesson.v1", "@template lesson.v1\n@figure unknown_provider 0\n@caption Unknown geometry.")]
        for old, new in mutations:
            with self.subTest(new=new):
                source.write_text(original.replace(old, new))
                error = self.rejected(self.run_publish, "target.rejected")
                self.assertTrue(error.diagnostics)
                self.assertTrue(error.diagnostics[0]["file"].endswith("chapter.paths.md"))
                self.assertGreater(error.diagnostics[0]["line"], 0)
        source.write_text(original)

    def test_provenance_coverage_required(self):
        author = self.authoring()
        author["sources"][0]["content_ids"] = ["published_matrix_reading"]
        self.save_authoring(author)
        self.rejected(self.run_publish, "source.missing")

    def test_tampered_and_unlisted_package_files_rejected(self):
        result = self.run_publish(command="export")
        pack = Path(result["package"])
        for kind in ("bytes", "missing", "extra", "forged"):
            with self.subTest(kind=kind):
                candidate = self.root / kind
                shutil.copytree(pack, candidate)
                chapter = candidate / "documents/chapter.paths.md"
                if kind == "bytes":
                    chapter.write_bytes(chapter.read_bytes() + b"\n")
                elif kind == "missing":
                    (candidate / "documents/shared/row_rules.inc.md").unlink()
                elif kind == "extra":
                    (candidate / "documents/extra.paths.md").write_text("@paths 1\n")
                else:
                    manifest = json.loads((candidate / "bundle.json").read_text())
                    next(row for row in manifest["provides"] if row["kind"] == "question")["stamp_sha256"] = "0" * 64
                    (candidate / "bundle.json").write_bytes(export.encoded(manifest))
                self.rejected(lambda: self.run_publish(command="install", source=candidate),
                              "package.contract" if kind == "forged" else "inventory.changed" if kind == "bytes" else "inventory.files")

    def test_symlinks_and_escaping_paths_rejected(self):
        shared = self.author / "documents/shared/row_rules.inc.md"
        raw = shared.read_bytes()
        outside = self.root / "outside.md"
        outside.write_bytes(raw)
        shared.unlink()
        shared.symlink_to(outside)
        self.rejected(self.run_publish, "path.inventory")
        shared.unlink()
        shared.write_bytes(raw)
        alias = self.root / "alias"
        alias.symlink_to(self.author, target_is_directory=True)
        self.rejected(lambda: self.run_publish(source=alias), "path.symlink")
        source = self.author / "documents/chapter.paths.md"
        source.write_text(source.read_text().replace("@include shared/row_rules.inc.md", "@include ../outside.md"))
        self.rejected(self.run_publish, "target.rejected")

    def test_cross_package_references_and_collision(self):
        self.run_publish()
        author = self.authoring()
        author.update(package_id="linked_question", package_version=1)
        author["sources"][0]["content_ids"] = ["linked_sum"]
        self.save_authoring(author)
        source = self.author / "documents/chapter.paths.md"
        source.write_text("@paths 1\n@subject linear_algebra | Linear Algebra\n@chapter published_matrix_foundations | Matrix foundations\n"
                          "@question linked_sum | Add the constants\n@template choices.v1\n@version 1\n@goal Add.\n@given 2+3\n@domain Integers.\n"
                          "@read published_matrix_reading\n@step 10 | Add.\n@choice 11 | 5\n@choice 12 | 6\n@answer 11\n@after 5\n@why Two plus three is five.\n@wrong Count again.\n@end\n")
        result = self.run_publish()
        manifest = json.loads((Path(result["package"]) / "bundle.json").read_text())
        self.assertIn(dict(kind="reading", id="published_matrix_reading"), manifest["requires"]["catalogue"])
        self.rejected(lambda: self.run_publish(command="install", source=Path(result["package"]), store=self.root / "isolated"), "target.rejected")
        author["package_id"] = "collision"
        self.save_authoring(author)
        self.rejected(self.run_publish, "target.rejected")

    def test_combined_question_limit(self):
        author = self.authoring()
        author["package_id"] = "capacity"
        self.save_authoring(author)
        shutil.rmtree(self.author / "documents")
        (self.author / "documents").mkdir()
        for part in range(8):
            text = "@paths 1\n@subject linear_algebra | Linear Algebra\n@chapter capacity_chapter | Capacity test\n"
            for index in range(part * 100, (part + 1) * 100):
                text += (f"@question capacity_{index} | Sum {index}\n@template choices.v1\n@version 1\n@goal Add.\n@given 2+3\n@domain Integers.\n"
                         "@step 10 | Add.\n@choice 11 | 5\n@choice 12 | 6\n@answer 11\n@after 5\n@why Two plus three is five.\n@wrong Count again.\n@end\n")
            (self.author / "documents" / f"part_{part}.paths.md").write_text(text)
        error = self.rejected(self.run_publish, "target.rejected")
        self.assertIn("Combined question collection", str(error))

    def test_interruption_before_activation_preserves_live_library(self):
        self.run_publish()
        before = self.active()
        self.revise()
        source = self.author / "documents/chapter.paths.md"
        source.write_text(source.read_text().replace("We will eliminate x", "First we eliminate x"))
        args = self.arguments()
        child = ("import os,sys; from pathlib import Path; from argparse import Namespace; "
                 f"sys.path.insert(0,{str(ROOT / 'tools')!r}); import export_learning as e; "
                 "e.replace_active=lambda *a,**k: os._exit(73); "
                 f"e.run(Namespace(**{str({k: str(v) if isinstance(v, Path) else v for k, v in vars(args).items()})}))")
        result = subprocess.run([sys.executable, "-c", child], capture_output=True, env={**os.environ, "PYTHONDONTWRITEBYTECODE": "1"}, timeout=30)
        self.assertEqual(result.returncode, 73, result.stderr.decode())
        self.assertEqual(before, self.active())
        self.assertTrue(self.target_report()["accepted"])
        self.run_publish()
        self.assertNotEqual(before, self.active())

    def test_competing_publishers_keep_both_additions(self):
        processes = []
        for name in ("worker_a", "worker_b"):
            author_root = self.root / name
            shutil.copytree(self.author, author_root)
            author = self.authoring(author_root)
            author["package_id"] = name
            author["sources"][0]["content_ids"] = [f"{name}_reading", f"{name}_question"]
            self.save_authoring(author, author_root)
            source = author_root / "documents/chapter.paths.md"
            source.write_text(source.read_text().replace("published_matrix", name))
            command = [sys.executable, str(ROOT / "tools/export_learning.py"), "publish", str(author_root),
                       "--target", str(OPTIONS.target), "--store", str(self.store),
                       "--base-documents", str(self.baseline), "--output", str(self.root / f"export_{name}")]
            processes.append(subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE))
        for process in processes:
            stdout, stderr = process.communicate(timeout=30)
            self.assertEqual(process.returncode, 0, stderr.decode())
            self.assertTrue(json.loads(stdout)["published"])
        ids = {row["id"] for row in self.target_report()["catalogue"]["questions"]}
        self.assertTrue({"worker_a_question", "worker_b_question"} <= ids)

    def test_runtime_save_replay_and_actual_ui(self):
        self.run_publish()
        for binary in (OPTIONS.model, OPTIONS.ui):
            result = subprocess.run([str(binary), "--published-store", str(self.store)], capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            print(result.stdout.strip())
        active = self.active()
        (self.store / "active.json").write_bytes(active.replace(b'"generation": "', b'"generation": "../'))
        with self.assertRaises(export.ExportError):
            self.target_report()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", type=Path, required=True)
    parser.add_argument("--model", type=Path, required=True)
    parser.add_argument("--ui", type=Path, required=True)
    OPTIONS, remaining = parser.parse_known_args()
    unittest.main(argv=[sys.argv[0], *remaining], verbosity=2)
