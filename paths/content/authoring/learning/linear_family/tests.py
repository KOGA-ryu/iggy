#!/usr/bin/env python3
"""Exact family cases plus compiled false-key/false-working regression checks."""
import argparse
import copy
from fractions import Fraction
import itertools
import json
from pathlib import Path
import subprocess
import tempfile
import sys
from unittest.mock import patch
import unittest

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / 'tools'))

import author_question_family as runner
import generate as family
import build_question_batch as batch
import export_learning as export


class FamilyTests(unittest.TestCase):
    def setUp(self):
        self.recipe = json.loads((family.SOURCE/'recipe.json').read_text())['parameters']

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

    def test_complete_family_compiles_replays_and_rejects_changed_keys_or_working(self):
        report = runner.check(family.SOURCE, TARGET, MODEL)
        self.assertEqual((report['questions'], report['readings'], report['decisions'], report['wrong_choices']), (18, 1, 21, 42))
        self.assertFalse(report['published'])
        self.assertEqual(report['lesson_checks']['independent_worked_disclosures'], 3)
        recipe = runner.recipe_from((family.SOURCE/'recipe.json').read_bytes(), family.SOURCE)
        questions, checks, _, _, _ = runner.assemble(recipe, runner.inputs(family.SOURCE), family.SOURCE, family)
        positions = [next(i for i, option in enumerate(q['question']['steps'][0]['options'])
                          if option['id'] in q['question']['steps'][0]['accepted_option_ids'])
                     for q in report['route_checks']['questions']]
        self.assertEqual([positions.count(i) for i in range(3)], [6, 6, 6])
        titles = [e['title'] for e in report['inspection']['entities'] if e['kind'] == 'question']
        self.assertEqual(len(set(titles)), 18)
        self.assertTrue(titles[6].startswith('07 ')); self.assertTrue(titles[6].endswith('practice'))
        original_text = (Path(report['authoring'])/'documents/chapter.paths.md').read_text()
        wrong_key = original_text.replace('@answer 11', '@answer 12', 1).replace('@feedback 12 |', '@feedback 11 |', 1)
        for damaged in (wrong_key, original_text.replace('@after (a,b)=', '@after (b,a)=', 1)):
            with tempfile.TemporaryDirectory(prefix='paths-family-mutation-') as temporary:
                folder = Path(temporary).resolve(); (folder/'chapter.paths.md').write_text(damaged)
                compiled = runner.model_gate(MODEL, '--question-batch', folder)['questions']
                with self.assertRaises(export.ExportError) as caught:
                    batch.verify_role_content(questions, compiled, checks)
                self.assertIn(questions[0]['id'], str(caught.exception))


class RecipePipelineTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='paths-family-recipe-')
        self.addCleanup(self.temporary.cleanup)
        self.source = Path(self.temporary.name).resolve()/'authoring'
        self.created = runner.initialize(self.source, 'linear_balance_v1', 'recipe_test')
        self.active = (ROOT/'b/learning-store/active.json').read_bytes()

    def tearDown(self):
        self.assertEqual((ROOT/'b/learning-store/active.json').read_bytes(), self.active)

    def check(self):
        return runner.check(self.source, TARGET, MODEL)

    def change_recipe(self, edit):
        p = self.source/'recipe.json'; value = export.decoded(p.read_bytes()); edit(value)
        p.write_bytes(export.encoded(value))

    def test_scaffold_is_data_only_repeatable_and_source_bound(self):
        self.assertEqual({p.name for p in self.source.iterdir()}, set(runner.INPUTS))
        self.assertFalse(any(p.suffix == '.py' for p in self.source.iterdir()))
        with self.assertRaises(export.ExportError):
            runner.initialize(self.source, 'linear_balance_v1', 'replacement')
        a, b = self.check(), self.check()
        self.assertEqual(a, b)
        self.assertEqual(a['source_sha256'], runner.hashes(runner.inputs(self.source)))
        receipt = Path(a['verification']); self.assertEqual(export.decoded(receipt.read_bytes())['document_sha256'], a['document_sha256'])
        for name, data in runner.inputs(self.source).items():
            self.assertEqual((receipt.parent/'inputs'/name).read_bytes(), data)
        author = export.decoded((Path(a['authoring'])/'authoring.json').read_bytes())
        self.assertEqual(author['package_id'], 'recipe_test')
        self.assertEqual(len(author['sources'][0]['content_ids']), 19)

    def test_number_edit_changes_only_affected_originals_and_derives_answers(self):
        before = self.check()
        self.change_recipe(lambda r: r['parameters']['sets'][0].update(a=8, b=9))
        after = self.check()
        old = before['route_checks']['questions']; new = after['route_checks']['questions']
        self.assertEqual(old[6:], new[6:])
        self.assertTrue(all(a['id'] != b['id'] for a, b in zip(old[:6], new[:6])))
        self.assertEqual([q['question']['equation'] for q in new[:3]], ['8x+9=33', '8x-9=15', '-8x+9=-15'])
        self.assertEqual(after['mathematical_checks'][1]['expected']['steps'][0]['answer'], '24')
        self.assertEqual(after['mathematical_checks'][5]['expected']['steps'][0]['answer'], r'x=\frac{1}{2}')
        self.assertNotEqual(before['document_sha256'], after['document_sha256'])

    def test_real_markdown_prompt_and_feedback_edits_keep_all_other_fields(self):
        before = self.check(); path = self.source/'questions.paths.md.in'; original = path.read_text()
        for fragment, field in [('Which pair gives (a,b), in that order?', 'prompt'),
                                ('This pair swaps the two jobs.', 'wrong_feedback')]:
            path.write_text(original.replace(fragment, 'Identify each term first. '+fragment, 1))
            after = self.check()
            expected = copy.deepcopy(before['route_checks'])
            for q in expected['questions'][::6]:
                step = q['question']['steps'][0]
                if field == 'prompt': step['prompt'] = 'Identify each term first. '+step['prompt']
                else:
                    option = next(o for o in step['options'] if o['id'] == 13)
                    option['wrong_feedback'] = 'Identify each term first. '+option['wrong_feedback']
            self.assertEqual(after['route_checks'], expected)
            self.assertEqual(after['mathematical_checks'], before['mathematical_checks'])
        path.write_text(original)

    def test_rejects_bad_recipe_and_reports_input_location(self):
        original = (self.source/'recipe.json').read_bytes()
        changes = [lambda r: r.update(family='../unreviewed.py'),
                   lambda r: r.update(format_version=True),
                   lambda r: r['placement'].update(subject='calculus'),
                   lambda r: r['parameters']['sets'][0].update(a=0),
                   lambda r: r['parameters']['sets'][0].update(b=6),
                   lambda r: r['parameters']['sets'][1].update(dict(r['parameters']['sets'][0], id='practice'))]
        for change in changes:
            (self.source/'recipe.json').write_bytes(original); self.change_recipe(change)
            with self.assertRaises(export.ExportError) as caught: self.check()
            self.assertIn(str(self.source/'recipe.json'), str(caught.exception))
        (self.source/'recipe.json').write_text('{"format":1,"format":2}')
        with self.assertRaises(export.ExportError) as caught: self.check()
        self.assertIn('Repeated JSON key', str(caught.exception))

    def test_unknown_field_missing_feedback_and_broken_disclosure_reject(self):
        question = self.source/'questions.paths.md.in'; original = question.read_text()
        question.write_text(original.replace('{{read_notation_a}}', '{{read_notation_typo}}', 1))
        with self.assertRaises(export.ExportError) as caught: self.check()
        self.assertIn('questions.paths.md.in:', str(caught.exception)); self.assertIn('read_notation_typo', str(caught.exception))
        question.write_text('\n'.join(line for line in original.splitlines() if not line.startswith('@feedback 13 | This pair'))+'\n')
        with self.assertRaises(export.ExportError) as caught: self.check()
        self.assertIn('distractor', str(caught.exception).lower())
        self.assertIn(str(question), str(caught.exception))
        question.write_text(original)
        lesson = self.source/'lesson.md.in'
        lesson.write_text(lesson.read_text().replace('@help hint', '@help proof', 1))
        with self.assertRaises(export.ExportError): self.check()

    def test_source_change_during_native_check_cannot_produce_acceptance(self):
        actual = runner.model_gate
        def mutate(model, flag, documents):
            result = actual(model, flag, documents)
            if flag == '--question-batch':
                p = self.source/'DESIGN.md'; p.write_text(p.read_text()+'\nChanged during check.\n')
            return result
        with patch.object(runner, 'model_gate', side_effect=mutate):
            with self.assertRaises(export.ExportError) as caught: self.check()
        self.assertEqual(caught.exception.code, 'family.source_changed')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--target', type=Path, default=ROOT/'b/sorter')
    parser.add_argument('--model', type=Path, default=ROOT/'b/paths_learning_document_tests')
    args, rest = parser.parse_known_args()
    TARGET, MODEL = args.target, args.model
    unittest.main(argv=[sys.argv[0], *rest])
