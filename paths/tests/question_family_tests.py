#!/usr/bin/env python3
"""Registered family mathematics and the shared data-to-compiled-card boundary."""
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

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))

import author_question_family as runner
import build_question_batch as batch
import export_learning as export

family = runner.load_provider('linear_balance_v1')
sine = runner.load_provider('sine_turn_v1')


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
        self.assertTrue(all(sorted(positions[i::6]) == [0,1,2] for i in range(6)))
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


class RecipeFixture(unittest.TestCase):
    family_name = 'linear_balance_v1'

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='paths-family-recipe-')
        self.addCleanup(self.temporary.cleanup)
        self.source = Path(self.temporary.name).resolve()/'authoring'
        self.created = runner.initialize(self.source, self.family_name, 'recipe_test')
        self.active = (ROOT/'b/learning-store/active.json').read_bytes()

    def tearDown(self):
        self.assertEqual((ROOT/'b/learning-store/active.json').read_bytes(), self.active)

    def check(self):
        return runner.check(self.source, TARGET, MODEL)

    def change_recipe(self, edit):
        p = self.source/'recipe.json'; value = export.decoded(p.read_bytes()); edit(value)
        p.write_bytes(export.encoded(value))


class RecipePipelineTests(RecipeFixture):
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


class SineRecipeTests(RecipeFixture):
    family_name = 'sine_turn_v1'

    def test_reviewed_math_teaching_and_feedback_survive_the_shared_route(self):
        report = self.check()
        self.assertEqual((report['questions'], report['readings'], report['decisions'], report['wrong_choices']), (18, 1, 21, 42))
        self.assertTrue(report['route_checks']['save_replay'])
        self.assertEqual(report['lesson_checks']['independent_worked_disclosures'], 3)
        self.assertEqual({p.name for p in self.source.iterdir()}, set(runner.INPUTS))
        self.assertFalse(report['published'])
        self.assertEqual(report, self.check())
        for path in runner.PROVIDER_DEPENDENCIES[self.family_name]:
            self.assertEqual(report['tools_sha256'][str(path.relative_to(ROOT))], export.sha(path.read_bytes()))
        for name in ('lesson.md.in', 'questions.paths.md.in'):
            self.assertEqual((self.source/name).read_bytes(), (sine.math.SOURCE/'families/sine_turn'/name).read_bytes())
        recipe = export.decoded((self.source/'recipe.json').read_bytes())
        groups = sine.prepare(recipe['parameters'], 'reuse_test')
        for (questions, fields), old_set in zip(groups, sine.math.SETS):
            old = sine.math.sequence('sine_turn', old_set)['questions']
            for q, previous in zip(questions, old):
                self.assertEqual(q['case'], previous['case'])
                actual = batch.reasoning_certificate(q, sine.CHECKERS)
                prior = batch.reasoning_certificate(previous, sine.math.CHECKERS)
                self.assertEqual(actual['expected'], prior['expected'])
                self.assertEqual(actual['evidence'], prior['evidence'])
            prior_fields = sine.math.presentation('sine_turn', old_set)
            self.assertEqual({k:v for k,v in fields.items() if k != 'set_title'},
                             {k:prior_fields[k] for k in fields if k != 'set_title'})
        first_positions = []
        for item in report['route_checks']['questions']:
            step = item['question']['steps'][0]
            first_positions.append(next(i for i,o in enumerate(step['options']) if o['id'] in step['accepted_option_ids']))
            for step in item['question']['steps']:
                for o in step['options']:
                    self.assertEqual(bool(o.get('wrong_feedback')), o['id'] not in step['accepted_option_ids'])
        self.assertEqual([first_positions.count(i) for i in range(3)], [6, 6, 6])
        self.assertTrue(all(sorted(first_positions[i::6]) == [0,1,2] for i in range(6)))

    def test_all_permitted_role_seeds_against_triangle_and_quadrant_oracle(self):
        # Reference-triangle folding, independent of the producer's inverse
        # branch table and repeated exact rotations (the Wave 01 test method).
        def sine_coordinate(theta):
            t = theta%2; sign = 1
            if t > 1: t -= 1; sign = -1
            if t > Fraction(1,2): t = 1-t
            rational, radical = {Fraction(0):(0,0), Fraction(1,6):(Fraction(1,2),0),
                                 Fraction(1,3):(0,Fraction(1,2)), Fraction(1,2):(1,0)}[t]
            return sign*rational, sign*radical
        parameters = export.decoded((self.source/'recipe.json').read_bytes())['parameters']
        counts = {}
        for meta in parameters['roles']:
            role = meta['role']; counts[role] = 0
            for level, sign, offset, branch in itertools.product(sine.ROLE_LEVELS[role], (-1,1), range(-2,3), (0,1)):
                if branch and (role not in ('worked_check','explain_step') or abs(Fraction(level)) == 1): continue
                if role == 'choose_next_step' and sign == 1 and Fraction(level).denominator == 1: continue
                seed = dict(level=level, coefficient_sign=sign, offset=offset, known_branch=branch)
                case = sine.original_case(seed, role, '/seed')
                q = dict(meta, id='seed_'+role, case=case)
                check = batch.reasoning_certificate(q, sine.CHECKERS)
                derived = Fraction(case['c']-case['b'], case['a'])
                expected = [Fraction(i,6) for i in range(12) if sine_coordinate(Fraction(i,6)) == (derived,0)]
                self.assertEqual(len(expected), 1 if abs(derived) == 1 else 2)
                self.assertEqual(list(map(str, expected)), check['evidence']['facts']['interval_solutions_theta_over_pi'])
                self.assertTrue(all(case['a']*sine_coordinate(t)[0]+case['b'] == case['c'] for t in expected))
                self.assertNotEqual((case['a'],case['b'],case['c']), (2,2,4))
                counts[role] += 1
        self.assertEqual(counts, dict(read_notation=40, worked_check=60, choose_next_step=35,
                                     explain_step=40, repair_error=50, independent=50))

    def test_number_edit_reaches_compiled_given_feedback_and_only_one_question(self):
        before = self.check()
        self.change_recipe(lambda r: r['parameters']['sets'][0]['cases']['read_notation'].update(offset=1))
        after = self.check()
        old, new = before['route_checks']['questions'], after['route_checks']['questions']
        self.assertEqual(old[1:], new[1:])
        self.assertNotEqual(old[0]['id'], new[0]['id'])
        self.assertEqual(new[0]['question']['equation'], r'2\sin\theta+1=2')
        wrong = next(o for o in new[0]['question']['steps'][0]['options'] if o['id'] == 12)
        self.assertIn('(2)(-1/2)+(1)=0, not 2', wrong['wrong_feedback'])
        self.assertEqual(after['mathematical_checks'][0]['expected']['steps'][0]['answer'], r'\sin\theta=\frac{1}{2}')

    def test_markdown_changes_only_the_requested_prompts_and_feedback(self):
        before = self.check(); path = self.source/'questions.paths.md.in'; original = path.read_text()
        for fragment, field in [('Which other permitted angle has the same sine coordinate?', 'prompt'),
                                ('is the known angle already supplied.', 'wrong_feedback')]:
            path.write_text(original.replace(fragment, 'Consider the known angle. '+fragment, 1))
            after = self.check(); expected = copy.deepcopy(before['route_checks'])
            for q in expected['questions'][1::6]:
                step = q['question']['steps'][0]
                if field == 'prompt': step['prompt'] = 'Consider the known angle. '+step['prompt']
                else:
                    option = next(o for o in step['options'] if o['id'] == 11)
                    option['wrong_feedback'] = option['wrong_feedback'].replace(fragment, 'Consider the known angle. '+fragment, 1)
            self.assertEqual(after['route_checks'], expected)
            self.assertEqual(after['mathematical_checks'], before['mathematical_checks'])

    def test_bad_seeds_domains_and_repeated_displayed_questions_identify_the_field(self):
        original = (self.source/'recipe.json').read_bytes()
        invalid = [('read_notation','level','0'), ('read_notation','level','0.5'),
                   ('worked_check','level','1'), ('explain_step','level','-1'),
                   ('worked_check','known_branch',2), ('worked_check','known_branch',True),
                   ('independent','known_branch',1), ('independent','offset',3),
                   ('independent','offset',False), ('independent','coefficient_sign',0),
                   ('independent','coefficient_sign',True)]
        for role, field, bad in invalid:
            with self.subTest(role=role, field=field, bad=bad):
                (self.source/'recipe.json').write_bytes(original)
                self.change_recipe(lambda r: r['parameters']['sets'][0]['cases'][role].update({field:bad}))
                with self.assertRaises(export.ExportError) as caught: self.check()
                self.assertIn(str(self.source/'recipe.json')+f':/parameters/sets/0/cases/{role}/{field}', str(caught.exception))
        for field, bad in [('units','degrees'), ('include_lower',1), ('include_upper',True), ('upper','4')]:
            (self.source/'recipe.json').write_bytes(original)
            self.change_recipe(lambda r: r['parameters']['domain'].update({field:bad}))
            with self.assertRaises(export.ExportError) as caught: self.check()
            self.assertIn('/parameters/domain', str(caught.exception))
        (self.source/'recipe.json').write_bytes(original)
        self.change_recipe(lambda r: r['parameters']['sets'][0]['cases']['choose_next_step'].update(level='1', coefficient_sign=1))
        with self.assertRaises(export.ExportError): self.check()
        (self.source/'recipe.json').write_bytes(original)
        def repeat(r):
            cases = r['parameters']['sets']
            cases[2]['cases']['explain_step'] = dict(cases[0]['cases']['explain_step'], offset=1)
        self.change_recipe(repeat)
        with self.assertRaises(export.ExportError) as caught: self.check()
        self.assertIn('different displayed original', str(caught.exception))

    def test_compiled_false_keys_and_reached_working_fail_certificate_comparison(self):
        report = self.check(); recipe = export.decoded((self.source/'recipe.json').read_bytes())
        questions, checks, _, _, _ = runner.assemble(recipe, runner.inputs(self.source), self.source, sine)
        original = (Path(report['authoring'])/'documents/chapter.paths.md').read_text()
        false_key = original.replace('@answer 11', '@answer 12', 1).replace('@feedback 12 |', '@feedback 11 |', 1)
        false_working = original.replace(r'@after \sin\theta=\frac{1}{2}', '@after 0=1', 1)
        for damaged in (false_key, false_working):
            self.assertNotEqual(damaged, original)
            with tempfile.TemporaryDirectory(prefix='paths-sine-mutation-') as temporary:
                folder = Path(temporary).resolve(); (folder/'chapter.paths.md').write_text(damaged)
                compiled = runner.model_gate(MODEL, '--question-batch', folder)['questions']
                with self.assertRaises(export.ExportError) as caught: batch.verify_role_content(questions, compiled, checks)
                self.assertIn(questions[0]['id'], str(caught.exception))

    def test_imported_mathematical_source_change_invalidates_check(self):
        actual_gate, actual_read = runner.model_gate, export.read_bytes
        changed = False
        def gate(model, flag, documents):
            nonlocal changed
            result = actual_gate(model, flag, documents); changed = True
            return result
        def read(path, *args):
            data = actual_read(path, *args)
            return data+b'\n' if changed and path == Path(sine.math.__file__) else data
        # Simulate changed captured bytes without writing to the frozen file.
        with patch.object(runner, 'model_gate', side_effect=gate), patch.object(export, 'read_bytes', side_effect=read):
            with self.assertRaises(export.ExportError) as caught: self.check()
        self.assertEqual(caught.exception.code, 'family.source_changed')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--target', type=Path, default=ROOT/'b/sorter')
    parser.add_argument('--model', type=Path, default=ROOT/'b/paths_learning_document_tests')
    args, rest = parser.parse_known_args()
    TARGET, MODEL = args.target, args.model
    unittest.main(argv=[sys.argv[0], *rest])
