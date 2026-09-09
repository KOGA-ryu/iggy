#!/usr/bin/env python3
"""Bounded batch mathematics and publication through the actual app/model."""
import argparse
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import build_question_batch as batch
import export_learning as export


class BatchTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='paths-question-batch-')
        self.root = Path(self.temporary.name).resolve()
        self.store = self.root / 'store'

    def tearDown(self):
        self.temporary.cleanup()

    def args(self, count=12, version=1, publish=False):
        return argparse.Namespace(count=count, version=version, publish=publish,
            target=OPTIONS.target, model=OPTIONS.model, output=self.root / 'batches' / str(version),
            store=self.store, base_documents=ROOT / 'content/write')

    def test_arithmetic_diversity_and_stable_extension(self):
        questions = batch.generate(12)
        self.assertEqual(batch.generate(24)[:12], questions)
        self.assertEqual(len({q['id'] for q in questions}), 12)
        self.assertEqual({g: sum(q['group'] == g for q in questions) for g in batch.GROUPS}, dict.fromkeys(batch.GROUPS, 4))
        positions = set()
        for q in batch.generate(36):
            self.assertTrue(batch.verify(q)['accepted'])
            positions.update(s['choices'].index(s['answer']) for s in q['steps'])
        self.assertEqual(positions, {0, 1, 2})
        for count in (0, -3, 4, 39, True):
            with self.assertRaises(export.ExportError):
                batch.generate(count)
        for defect in range(4):
            q = copy.deepcopy(questions[0])
            if defect == 0:
                q['answer'][0] = '999'
            elif defect == 1:
                q['states'][1][1][2] = '999'
            elif defect == 2:
                q['steps'][0]['choices'][1] = q['steps'][0]['choices'][0]
            else:
                q['steps'][1]['answer'] = '0'
            with self.assertRaises(export.ExportError):
                batch.verify(q)

    def test_export_publish_repeat_and_append(self):
        first = batch.run(self.args())
        self.assertFalse(first['publication']['published'])
        self.assertFalse((self.store / 'active.json').exists())
        self.assertEqual(first['route_checks']['routes'], 60)
        self.assertEqual(first['route_checks']['wrong_choices'], 216)
        author = Path(first['generated_authoring'])
        generated = export.capture(author)
        published = batch.run(self.args(publish=True))
        self.assertTrue(published['publication']['published'])
        active = (self.store / 'active.json').read_bytes()
        stamp = (self.store / 'active.json').stat().st_mtime_ns
        before = export.Target(OPTIONS.target).inspect(store=self.store)['catalogue']['questions']
        again = batch.run(self.args(publish=True))
        self.assertTrue(again['publication']['unchanged'])
        self.assertEqual((self.store / 'active.json').read_bytes(), active)
        self.assertEqual((self.store / 'active.json').stat().st_mtime_ns, stamp)
        self.assertEqual(export.capture(author), generated)
        more = batch.run(self.args(count=24, version=2, publish=True))
        after = export.Target(OPTIONS.target).inspect(store=self.store)['catalogue']['questions']
        stamps = {q['id']: q['stamp_sha256'] for q in after}
        self.assertTrue(all(stamps[q['id']] == q['stamp_sha256'] for q in before))
        self.assertEqual(len(after), len(before) + 12)
        active = (self.store / 'active.json').read_bytes()
        with self.assertRaisesRegex(export.ExportError, 'Existing question must remain unchanged'):
            batch.run(self.args(count=6, version=3, publish=True))
        self.assertEqual((self.store / 'active.json').read_bytes(), active)
        self.assertEqual(more['route_checks']['routes'], 120)

    def test_bad_fresh_batch_cannot_publish_a_previous_good_export(self):
        result = batch.run(self.args(count=3, publish=True))
        active = (self.store / 'active.json').read_bytes()
        bad = batch.generate(3);bad[0]['answer'][0] = '999'
        with patch.object(batch, 'generate', return_value=bad), self.assertRaises(export.ExportError):
            batch.run(self.args(count=3, publish=True))
        self.assertEqual((self.store / 'active.json').read_bytes(), active)
        source = Path(result['generated_authoring']) / 'documents/integers.paths.md'
        source.write_text(source.read_text() + '\nChanged after verification.\n')
        with self.assertRaises(export.ExportError):
            batch.run(self.args(count=3, publish=True))
        self.assertEqual((self.store / 'active.json').read_bytes(), active)

    def test_compiler_failure_has_source_and_no_publication(self):
        original = batch.documents
        def damaged(questions):
            docs = original(questions)
            q = next(q for q in questions if q['group'] == 'integers')
            old = ('@after ' + batch.matrix(q['states'][1])).encode()
            docs['integers.paths.md'] = docs['integers.paths.md'].replace(old, b'@after [1, 1 | 1] [0, 1 | 999]', 1)
            return docs
        with patch.object(batch, 'documents', side_effect=damaged), self.assertRaises(export.ExportError) as caught:
            batch.run(self.args(count=3, publish=True))
        d = caught.exception.diagnostics[0]
        self.assertEqual(d['field'], 'after')
        self.assertEqual(d['file'], 'integers.paths.md')
        self.assertGreater(d['line'], 0)
        self.assertIn('accepted choice', d['message'])
        self.assertFalse((self.store / 'active.json').exists())

    def test_command_reports_rejection_as_json(self):
        result = subprocess.run([sys.executable, '-B', str(ROOT / 'tools/build_question_batch.py'), '--count', '4'],
                                text=True, capture_output=True, timeout=30)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(json.loads(result.stderr)['code'], 'batch.count')
        self.assertEqual(result.stdout, '')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--target', type=Path, required=True)
    parser.add_argument('--model', type=Path, required=True)
    OPTIONS, remaining = parser.parse_known_args()
    OPTIONS.target = OPTIONS.target.resolve();OPTIONS.model = OPTIONS.model.resolve()
    unittest.main(argv=[sys.argv[0], *remaining])
