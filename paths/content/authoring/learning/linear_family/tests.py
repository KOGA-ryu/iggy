#!/usr/bin/env python3
"""Exact family cases plus compiled false-key/false-working regression checks."""
import copy
from fractions import Fraction
import itertools
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

import generate as family
import build_question_batch as batch
import export_learning as export


class FamilyTests(unittest.TestCase):
    def setUp(self):
        self.recipe = json.loads((family.SOURCE/'recipe.json').read_text())

    def test_sample_answers_from_original_equations(self):
        sequence = family.make_sequence(self.recipe, 'sample')
        checks = [batch.reasoning_certificate(q, family.CHECKERS) for q in sequence['questions']]
        self.assertEqual([c['expected']['steps'][0]['answer'] for c in checks],
                         ['(6,8)', '18', '-6x=-18', r'\times(-6)', r'L_1', r'x=\frac{1}{2}'])
        self.assertEqual(checks[4]['expected']['steps'][1]['answer'], '18')
        self.assertEqual(checks[2]['evidence']['facts']['candidate_residuals'], ['0', '0', '-8'])
        self.assertEqual(checks[4]['evidence']['facts']['wrong_residual'], '-16')
        self.assertEqual(checks[5]['evidence']['facts']['candidate_residuals'], ['0', '-6', '15'])
        self.assertEqual(sum(c['steps_checked'] for c in checks), 7)
        self.assertEqual(sum(c['wrong_choices_checked'] for c in checks), 14)

    def test_entire_declared_seed_domain_has_no_degenerate_decisions(self):
        seeds = 0
        for a, b, s, fresh in itertools.product((2,4,6,8), range(1,10), range(2,6), ('-3/2','-1/2','1/2','3/2')):
            if a == b:
                continue
            recipe = copy.deepcopy(self.recipe)
            recipe['sets'][0].update(a=a, b=b, solution=s, fresh_solution=fresh)
            sequence = family.make_sequence(recipe, 'sample')
            for q in sequence['questions']:
                certificate = batch.reasoning_certificate(q, family.CHECKERS)
                case = q['case']; answer = Fraction(certificate['evidence']['facts']['solution'])
                self.assertEqual(case['a']*answer+case['b'], case['c'])
                self.assertEqual(certificate['evidence']['kind'], batch.EXERCISE_ROLES[q['role']])
            seeds += 1
        self.assertEqual(seeds, 512)

    def test_rejects_outside_domain_and_ambiguous_presentation(self):
        for key, bad in [('a',0), ('a',True), ('a',1), ('a',3), ('b',0), ('b',6),
                         ('solution',0), ('fresh_solution','0'), ('fresh_solution','1/3')]:
            recipe = copy.deepcopy(self.recipe); recipe['sets'][0][key] = bad
            with self.assertRaises(export.ExportError, msg=(key,bad)):
                family.make_sequence(recipe, 'sample')
        original = family.make_sequence(self.recipe, 'sample')['questions']
        for index, field, value in [(0,'a',0),(0,'a',True),(0,'b',6),(1,'b',8),
                                    (2,'a',6),(3,'c',0),(4,'b',0),(5,'reversed',False),(5,'c',14)]:
            q = copy.deepcopy(original[index]); q['case'][field] = value
            with self.assertRaises(export.ExportError, msg=(index,field,value)):
                batch.reasoning_certificate(q, family.CHECKERS)

    def test_identity_is_stable_for_prose_and_changes_for_new_mathematics(self):
        before = family.make_sequence(self.recipe, 'sample')
        self.recipe['roles'][0]['title'] = 'Read the two terms'
        after = family.make_sequence(self.recipe, 'sample')
        self.assertEqual(before['questions'][0]['id'], after['questions'][0]['id'])
        self.recipe['sets'][0]['b'] = 9
        changed = family.make_sequence(self.recipe, 'sample')
        self.assertNotEqual(before['questions'][0]['id'], changed['questions'][0]['id'])

    def test_three_candidates_compile_replay_and_reject_changed_keys_or_working(self):
        all_ids = set()
        for name in family.SETS:
            report = family.build(name, batch.ROOT/'b/sorter', batch.ROOT/'b/paths_learning_document_tests')
            self.assertEqual(report['route_checks']['routes'], 6)
            self.assertEqual(report['publication'], 'not_performed')
            ids = set(report['route_checks']['question_ids'])
            self.assertFalse(all_ids & ids); all_ids |= ids
            sequence = family.make_sequence(self.recipe, name)
            original_text = (Path(report['authoring'])/'documents/chapter.paths.md').read_text()
            checks = report['mathematical_checks']
            # Each broken document is valid grammar and fully playable against
            # its false key; the independent certificate must still refuse it.
            wrong_key = original_text.replace('@answer 11', '@answer 12', 1)
            wrong_key = wrong_key.replace('@feedback 12 |', '@feedback 11 |', 1)
            for damaged in (wrong_key,
                            original_text.replace('@after (a,b)=', '@after (b,a)=', 1)):
                with tempfile.TemporaryDirectory(prefix='paths-family-mutation-') as temporary:
                    folder = Path(temporary).resolve()
                    (folder/'chapter.paths.md').write_text(damaged)
                    run = subprocess.run([str(batch.ROOT/'b/paths_learning_document_tests'),
                                          '--question-batch', str(folder)], capture_output=True, text=True, timeout=60)
                    self.assertEqual(run.returncode, 0, run.stderr or run.stdout)
                    compiled = json.loads(run.stdout)['questions']
                    with self.assertRaises(export.ExportError) as caught:
                        batch.verify_role_content(sequence['questions'], compiled, checks)
                    self.assertIn(sequence['questions'][0]['id'], str(caught.exception))
        self.assertEqual(len(all_ids), 18)


if __name__ == '__main__':
    unittest.main()
