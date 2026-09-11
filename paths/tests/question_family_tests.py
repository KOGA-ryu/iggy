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
polynomial = runner.load_provider('polynomial_derivative_v1')
rows = runner.load_provider('row_operations_v1')


def semantic_question(row):
    """Compare full compiled teaching while allowing identity/position changes."""
    q = copy.deepcopy(row['question']); q.pop('id')
    for step in q['steps']:
        labels = {o['id']:o['label'] for o in step['options']}
        step['accepted_option_ids'] = [labels[i] for i in step['accepted_option_ids']]
        step['options'] = sorted([{k:v for k,v in o.items() if k != 'id'} for o in step['options']], key=lambda o:o['label'])
    return q


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
        groups = runner.prepare_groups(sine, recipe['parameters'], 'reuse_test')
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


class PolynomialRecipeTests(RecipeFixture):
    family_name = 'polynomial_derivative_v1'

    def test_reviewed_math_and_complete_compiled_teaching_survive_common_format(self):
        report = self.check(); g = polynomial.math
        self.assertEqual((report['questions'], report['readings'], report['decisions'], report['wrong_choices']), (18,1,21,42))
        self.assertEqual(report['lesson_checks']['independent_worked_disclosures'], 3)
        self.assertTrue(report['route_checks']['save_replay']); self.assertFalse(report['published'])
        self.assertEqual({p.name for p in self.source.iterdir()}, set(runner.INPUTS))
        self.assertEqual(report, self.check())
        self.assertEqual((self.source/'lesson.md.in').read_bytes(), (g.ROOT/'families/polynomial_rules/lesson.md.in').read_bytes())
        recipe = export.decoded((g.ROOT/'recipe.json').read_bytes()); old_questions, blocks = [], []
        for selected in g.SETS:
            group = g.sequence(recipe, 'polynomial_rules', selected)['questions']; old_questions.extend(group)
            blocks.extend(g.question_text(q, pos) for q,pos in zip(group, g.POSITIONS['polynomial_rules',selected]))
        with tempfile.TemporaryDirectory(prefix='paths-calculus-reference-') as temporary:
            folder = Path(temporary).resolve()
            (folder/'chapter.paths.md').write_text(batch.chapter_text(g.values('polynomial_rules',old_questions,'\n\n'.join(blocks))))
            old = runner.model_gate(MODEL, '--question-batch', folder)['questions']
        self.assertEqual(list(map(semantic_question,report['route_checks']['questions'])), list(map(semantic_question,old)))
        for current, previous in zip(report['mathematical_checks'], old_questions):
            expected = batch.reasoning_certificate(previous, g.CHECKERS)
            self.assertEqual(current['expected'], expected['expected'])
            self.assertEqual(current['evidence'], expected['evidence'])
        positions = [next(i for i,o in enumerate(q['question']['steps'][0]['options'])
                          if o['id'] in q['question']['steps'][0]['accepted_option_ids']) for q in report['route_checks']['questions']]
        self.assertTrue(all(sorted(positions[i::6]) == [0,1,2] for i in range(6)))
        for path in runner.PROVIDER_DEPENDENCIES[self.family_name]:
            self.assertEqual(report['tools_sha256'][str(path.relative_to(ROOT))], export.sha(path.read_bytes()))

    def test_full_bounded_polynomial_pool_against_independent_horner_expansion(self):
        # Repeated multiplication by (a+h); no binomial or derivative helper.
        def shifted_horner(c, a):
            result = [0]
            for coefficient in c:
                product = [0]*(len(result)+1)
                for i,value in enumerate(result): product[i] += a*value; product[i+1] += value
                product[0] += coefficient; result = product
            return result[:4]
        checked, numeric, degenerate = 0, 0, 0
        for coefficients in itertools.product(range(-5,6), repeat=4):
            if not any(coefficients[:3]): continue
            for a in range(-3,4):
                seed = dict(coefficients=list(coefficients), at=a)
                if dict(family='polynomial_rules', **seed) == polynomial.WORKED_CASE: continue
                case = polynomial.original_case(seed, 'read_notation', '/seed')
                t = polynomial.math.context(dict(id='oracle', role='read_notation', case=case))
                e = shifted_horner(coefficients, a)
                self.assertEqual(t['e'], e); self.assertEqual(t['d'], e[1]); self.assertEqual(t['function_value'], e[0])
                # These are the three named misconceptions, evaluated from the
                # independently expanded original; select distinct wrong values.
                wrong = list(dict.fromkeys(v for v in (e[0],e[1]+coefficients[-1],-e[1]) if v != e[1]))
                if len(wrong) >= 2:
                    self.assertEqual([e['value'] for e in polynomial.math.numeric_errors(t)], wrong[:2]); numeric += 1
                else:
                    with self.assertRaises(export.ExportError): polynomial.math.numeric_errors(t)
                    degenerate += 1
                checked += 1
        self.assertEqual(checked, 102409)
        self.assertEqual(numeric+degenerate, checked)
        print(json.dumps(dict(polynomial_originals_checked=checked,numeric_role_cases=numeric,rejected_numeric_cases=degenerate)))

    def test_point_edit_reaches_answer_working_and_feedback_without_changing_other_questions(self):
        before = self.check()
        self.change_recipe(lambda r:r['parameters']['sets'][0]['cases']['worked_check'].update(at=1))
        after = self.check(); old, new = before['route_checks']['questions'], after['route_checks']['questions']
        self.assertEqual(old[:1]+old[2:], new[:1]+new[2:]); self.assertNotEqual(old[1]['id'], new[1]['id'])
        self.assertIn(r'\quad a=1', new[1]['question']['equation'])
        self.assertEqual(after['mathematical_checks'][1]['expected']['steps'][0]['answer'], '5')
        self.assertEqual(new[1]['question']['working_states'][-1]['display'], "f'(1)=5")
        wrong = next(o for o in new[1]['question']['steps'][0]['options'] if o['id']==12)
        self.assertIn('(-1)(1)^3+(3)(1)^2+(2)(1)+(-5)=-1', wrong['wrong_feedback'])
        self.assertIn('(-1)(3)(1)^2+(3)(2)(1)+(2)(1)=5', wrong['wrong_feedback'])

    def test_markdown_prompt_and_specific_correction_edits_reach_all_three_sets(self):
        before = self.check(); path = self.source/'questions.paths.md.in'; original = path.read_text()
        for fragment, field in [('After applying the power and sum rules, what value does the derivative expression give?', 'prompt'),
                                ('This option computes', 'wrong_feedback')]:
            path.write_text(original.replace(fragment, 'Check the signed terms. '+fragment, 1))
            after = self.check(); expected = copy.deepcopy(before['route_checks'])
            for q in expected['questions'][1::6]:
                step = q['question']['steps'][0]
                if field == 'prompt': step['prompt'] = 'Check the signed terms. '+step['prompt']
                else:
                    option = next(o for o in step['options'] if o['id']==12)
                    option['wrong_feedback'] = 'Check the signed terms. '+option['wrong_feedback']
            self.assertEqual(after['route_checks'], expected)
            self.assertEqual(after['mathematical_checks'], before['mathematical_checks'])

    def test_invalid_inputs_and_degenerate_numeric_cases_identify_exact_locations(self):
        original = (self.source/'recipe.json').read_bytes()
        cases = [({'coefficients':[1,2,3]},'/coefficients'), ({'coefficients':[1,2,3,4,5]},'/coefficients'),
                 ({'coefficients':[6,2,3,4]},'/coefficients/0'), ({'coefficients':[True,2,3,4]},'/coefficients/0'),
                 ({'coefficients':[1,2,'3',4]},'/coefficients/2'), ({'coefficients':[0,0,0,4]},'/coefficients'),
                 ({'at':4},'/at'), ({'at':False},'/at'), ({'at':'1'},'/at'),
                 ({'coefficients':[1,-2,1,4],'at':1},''), ({'coefficients':[0,1,0,0],'at':0},'')]
        for mutation, suffix in cases:
            (self.source/'recipe.json').write_bytes(original)
            self.change_recipe(lambda r:r['parameters']['sets'][0]['cases']['independent'].update(mutation))
            with self.assertRaises(export.ExportError) as caught: self.check()
            self.assertIn(str(self.source/'recipe.json')+':/parameters/sets/0/cases/independent'+suffix, str(caught.exception))
        (self.source/'recipe.json').write_bytes(original)
        self.change_recipe(lambda r:r['parameters']['sets'][1]['cases'].update(r['parameters']['sets'][0]['cases']))
        with self.assertRaises(export.ExportError) as caught: self.check()
        self.assertIn('different displayed original', str(caught.exception))
        for mutation in (lambda r:r['parameters'].update(domain='complex'),
                         lambda r:r['parameters']['sets'][0]['cases']['independent'].update(answer=0),
                         lambda r:r['parameters']['roles'][0].update(title='too long '*200)):
            (self.source/'recipe.json').write_bytes(original); self.change_recipe(mutation)
            with self.assertRaises(export.ExportError): self.check()

    def test_actual_false_key_missing_point_and_invalid_cancellation_are_rejected(self):
        # Mutate the authored template and send it through the real compiler;
        # the common certificate boundary must reject otherwise playable cards.
        path = self.source/'questions.paths.md.in'; original = path.read_text()
        defects = [original.replace('@after {{explain_step_after_1}}', '@after h=0', 1),
                   original.replace('@given {{independent_given}}', '@given f(x)={{independent_f}}', 1)]
        for damaged in defects:
            path.write_text(damaged)
            with self.assertRaises(export.ExportError) as caught: self.check()
            self.assertIn('given/after', str(caught.exception)); self.assertIn(str(path), str(caught.exception))
        path.write_text(original)
        report = self.check(); recipe = export.decoded((self.source/'recipe.json').read_bytes())
        qs, checks, _, _, _ = runner.assemble(recipe, runner.inputs(self.source), self.source, polynomial)
        text = (Path(report['authoring'])/'documents/chapter.paths.md').read_text()
        text = text.replace('@answer 11', '@answer 12', 1).replace('@feedback 12 |', '@feedback 11 |', 1)
        with tempfile.TemporaryDirectory(prefix='paths-polynomial-key-') as temporary:
            folder = Path(temporary).resolve(); (folder/'chapter.paths.md').write_text(text)
            compiled = runner.model_gate(MODEL, '--question-batch', folder)['questions']
            with self.assertRaises(export.ExportError): batch.verify_role_content(qs, compiled, checks)


class RowRecipeTests(RecipeFixture):
    family_name = 'row_operations_v1'

    def test_reviewed_math_and_complete_compiled_teaching_survive_common_format(self):
        report = self.check(); g = rows.math
        self.assertEqual((report['questions'],report['readings'],report['decisions'],report['wrong_choices']), (18,1,21,42))
        self.assertEqual(report['lesson_checks']['independent_worked_disclosures'], 3)
        self.assertTrue(report['route_checks']['save_replay']); self.assertFalse(report['published'])
        self.assertEqual({p.name for p in self.source.iterdir()}, set(runner.INPUTS))
        self.assertEqual(report, self.check())
        self.assertEqual((self.source/'lesson.md.in').read_bytes(), (g.WORKER/'row_operations/lesson.md.in').read_bytes())
        old_qs, old_checks, documents, _, _ = g.family_material(export.decoded((g.WORKER/'recipe.json').read_bytes()),'row_operations')
        with tempfile.TemporaryDirectory(prefix='paths-row-reference-') as temporary:
            folder = Path(temporary).resolve(); export.write_tree(folder, documents)
            old = runner.model_gate(MODEL,'--question-batch',folder)['questions']
        self.assertEqual(list(map(semantic_question,report['route_checks']['questions'])), list(map(semantic_question,old)))
        for current, previous in zip(report['mathematical_checks'], old_checks):
            self.assertEqual(current['evidence'], previous['evidence'])
            a, b = copy.deepcopy(current['expected']), copy.deepcopy(previous['expected'])
            for expected in (a,b):
                for step in expected['steps']: step['choices'].sort()
            self.assertEqual(a, b)
        positions = [next(i for i,o in enumerate(q['question']['steps'][0]['options'])
                          if o['id'] in q['question']['steps'][0]['accepted_option_ids']) for q in report['route_checks']['questions']]
        self.assertTrue(all(sorted(positions[i::6]) == [0,1,2] for i in range(6)))
        self.assertEqual(report['mathematical_checks'][14]['evidence']['facts']['multiplier'], '-3/2')
        for path in runner.PROVIDER_DEPENDENCIES[self.family_name]:
            self.assertEqual(report['tools_sha256'][str(path.relative_to(ROOT))], export.sha(path.read_bytes()))

    def test_finite_sample_pool_against_independent_gaussian_elimination(self):
        # Pivot, normalize and eliminate; do not use the production Cramer
        # solver, row-addition helper or certificate to derive expectations.
        def solve(original):
            a = [[Fraction(v) for v in row] for row in original]
            for column in range(2):
                pivot = next((i for i in range(column,2) if a[i][column]), None)
                if pivot is None: return None
                a[column], a[pivot] = a[pivot], a[column]
                scale = a[column][column]; a[column] = [v/scale for v in a[column]]
                for i in range(2):
                    if i != column:
                        factor = a[i][column]; a[i] = [v-factor*w for v,w in zip(a[i],a[column])]
            return [a[0][2],a[1][2]]
        pool = [list(row) for row in itertools.product(range(-2,3),range(-2,3),(-1,1))]
        accepted = dict.fromkeys(batch.EXERCISE_ROLES,0); rejected = 0
        for first, second in itertools.product(pool,repeat=2):
            original = [first,second]; xy = solve(original)
            candidates = [('choose_next_step',None),('independent',None)] + [
                (role,k) for role,k in itertools.product(rows.OPERATIONS,(-2,-1,1,2))]
            for role,k in candidates:
                case = dict(rows=original)
                if k is not None: case['multiplier'] = k
                possible = xy is not None
                if role == 'choose_next_step': possible = possible and first[0] != 0 and second[0] != 0
                if role == 'independent': possible = possible and xy[0] != 0 and xy[0] != xy[1]
                if role in ('worked_check','repair_error'):
                    possible = possible and second[0]+k*first[0] == 0 and second[1]+k*first[1] == 1
                if role == 'repair_error': possible = possible and k <= -2
                if not possible:
                    with self.assertRaises(export.ExportError, msg=(role,case)): rows.original_case(case,role,'/sample')
                    rejected += 1; continue
                q = dict(id='sample',role=role,case=rows.original_case(case,role,'/sample'))
                cert = batch.reasoning_certificate(q,rows.CHECKERS); facts = cert['evidence']['facts']
                solution = list(map(Fraction,facts['solution' if role == 'independent' else 'original_solution']))
                self.assertEqual(solution,xy)
                if role == 'independent':
                    pairs = [(xy[1],xy[0]),xy,(2*xy[0],xy[1])]
                    residuals = [[a*x+b*y-c for a,b,c in original] for x,y in pairs]
                    self.assertEqual([list(map(Fraction,r)) for r in facts['choice_residuals']],residuals)
                else:
                    if role == 'choose_next_step': k = -Fraction(second[0],first[0]); self.assertEqual(Fraction(facts['multiplier']),k)
                    changed = [first,[v+k*w for v,w in zip(second,first)]]
                    self.assertEqual(solve(changed),xy)
                    if role in ('worked_check','repair_error'):
                        self.assertEqual(list(map(Fraction,facts['new_row' if role == 'worked_check' else 'corrected_row'])),changed[1])
                        self.assertEqual(Fraction(cert['expected']['steps'][-1]['answer']),changed[1][2])
                    if role == 'explain_step': self.assertEqual([[Fraction(v) for v in r] for r in facts['recovered_rows']],original)
                    fields = rows.presentation([q]); prefix = role+'_'
                    for j in range(3):
                        self.assertIn(f'{second[j]}+({batch.tex(k)})({first[j]})={batch.tex(changed[1][j])}',fields[prefix+'forward_calculation'])
                accepted[role] += 1
        self.assertEqual(solve(rows.WORKED_ROWS),[Fraction(16,7),Fraction(13,7)])
        self.assertEqual(sum(accepted.values())+rejected,35000)
        self.assertTrue(all(accepted[r] > 0 for r in batch.EXERCISE_ROLES if r != 'read_notation'))
        print(json.dumps(dict(row_system_role_cases=35000,row_cases_accepted=accepted,row_cases_rejected=rejected)))

    def test_original_row_edit_updates_all_columns_answer_and_specific_feedback(self):
        before = self.check()
        self.change_recipe(lambda r:r['parameters']['sets'][0]['cases']['worked_check'].update(rows=[[1,-1,3],[2,-1,11]]))
        after = self.check(); old, new = before['route_checks']['questions'],after['route_checks']['questions']
        self.assertEqual(old[:1]+old[2:],new[:1]+new[2:]); self.assertNotEqual(old[1]['id'],new[1]['id'])
        self.assertEqual(after['mathematical_checks'][1]['expected']['steps'][0]['answer'],'5')
        self.assertIn(r'0&1&5\end{array}',new[1]['question']['working_states'][-1]['display'])
        step = new[1]['question']['steps'][0]
        for calculation in ('2+(-2)(1)=0','-1+(-2)(-1)=1','11+(-2)(3)=5'):
            self.assertIn(calculation,step['explanation'])
        wrong = next(o for o in step['options'] if o['id'] == 11)
        self.assertEqual(wrong['label'],'11'); self.assertIn('11+(-2)(3)=5',wrong['wrong_feedback'])

    def test_markdown_prompt_and_correction_edits_reach_all_three_sets(self):
        before = self.check(); path = self.source/'questions.paths.md.in'; original = path.read_text()
        for fragment,field in [('Which inverse operation restores the original second row?','prompt'),
                               ('Repeating the original multiple gives constant','wrong_feedback')]:
            path.write_text(original.replace(fragment,'Retain the complete row. '+fragment,1))
            after = self.check(); expected = copy.deepcopy(before['route_checks'])
            for q in expected['questions'][3::6]:
                step = q['question']['steps'][0]
                if field == 'prompt': step['prompt'] = 'Retain the complete row. '+step['prompt']
                else:
                    option = next(o for o in step['options'] if o['id'] == 11)
                    option['wrong_feedback'] = 'Retain the complete row. '+option['wrong_feedback']
            self.assertEqual(after['route_checks'],expected)
            self.assertEqual(after['mathematical_checks'],before['mathematical_checks'])

    def test_invalid_shapes_singular_systems_and_misleading_corrections_report_recipe_location(self):
        original = (self.source/'recipe.json').read_bytes()
        failures = [('read_notation',dict(row=[1,2]),'/row'),('read_notation',dict(row=[1,True,3]),'/row/1'),
            ('read_notation',dict(row=[1,2,21]),'/row/2'),('read_notation',dict(row=[1,'2',3]),'/row/1'),
            *[('read_notation',dict(row=[1,b,c]),'/row') for b,c in ((0,3),(2,2),(2,-2))],
            ('independent',dict(rows=[[1,2,3]]),'/rows'),('independent',dict(rows=[[1,2,3],[2,4,6]]),''),
            ('independent',dict(rows=[[1,0,0],[0,1,2]]),''),('independent',dict(rows=[[1,0,2],[0,1,2]]),''),
            ('independent',dict(rows=rows.WORKED_ROWS),'/rows'),('independent',dict(answer=[1,2]),''),
            ('choose_next_step',dict(rows=[[0,1,2],[1,1,3]]),''),
            ('worked_check',dict(multiplier=0),'/multiplier'),('worked_check',dict(multiplier=True),'/multiplier'),
            ('worked_check',dict(multiplier=7),'/multiplier'),('worked_check',dict(multiplier=1),''),
            ('worked_check',dict(rows=[[1,2,0],[2,5,8]]),'/rows/0/2'),
            ('explain_step',dict(rows=[[1,2,0],[2,5,8]]),'/rows/0/2'),
            ('repair_error',dict(rows=[[1,1,2],[1,2,4]],multiplier=-1),'')]
        for role,mutation,suffix in failures:
            (self.source/'recipe.json').write_bytes(original)
            self.change_recipe(lambda r:r['parameters']['sets'][0]['cases'][role].update(mutation))
            with self.assertRaises(export.ExportError) as caught: self.check()
            self.assertIn(str(self.source/'recipe.json')+':/parameters/sets/0/cases/'+role+suffix,str(caught.exception))
        (self.source/'recipe.json').write_bytes(original)
        self.change_recipe(lambda r:r['parameters']['sets'][1]['cases'].update(r['parameters']['sets'][0]['cases']))
        with self.assertRaises(export.ExportError) as caught: self.check()
        self.assertIn('different displayed original',str(caught.exception))

    def test_compiled_false_keys_and_working_reject_and_repair_keeps_two_decisions(self):
        report = self.check(); recipe = export.decoded((self.source/'recipe.json').read_bytes())
        qs,checks,_,_,_ = runner.assemble(recipe,runner.inputs(self.source),self.source,rows)
        for index in (4,10,16):
            first,second = report['route_checks']['questions'][index]['question']['steps']
            self.assertIn('Which line first',first['prompt']); self.assertIn('corrected consequence',second['prompt'])
            answer = checks[index]['expected']['steps'][1]['answer']
            self.assertNotIn('='+answer,checks[index]['expected']['after'][0]+first['explanation'])
        original = (Path(report['authoring'])/'documents/chapter.paths.md').read_text()
        false_key = original.replace('@answer 11','@answer 12',1).replace('@feedback 12 |','@feedback 11 |',1)
        false_working = original.replace('@after '+checks[1]['expected']['after'][0],'@after 0=1',1)
        for damaged in (false_key,false_working):
            self.assertNotEqual(damaged,original)
            with tempfile.TemporaryDirectory(prefix='paths-row-mutation-') as temporary:
                folder = Path(temporary).resolve(); (folder/'chapter.paths.md').write_text(damaged)
                compiled = runner.model_gate(MODEL,'--question-batch',folder)['questions']
                with self.assertRaises(export.ExportError): batch.verify_role_content(qs,compiled,checks)

    def test_calculated_fields_use_captured_inputs_without_frozen_assembly_or_file_reads(self):
        parameters = export.decoded((self.source/'recipe.json').read_bytes())['parameters']
        expected = runner.prepare_groups(rows,parameters,'sample')
        self.assertIs(rows.CHECKERS,batch.MATRIX_ROLE_CHECKERS)
        failure = AssertionError('The recipe adapter must not call frozen assembly or read author inputs')
        with patch.object(rows.math,'sequence',side_effect=failure), patch.object(rows.math,'render',side_effect=failure), \
                patch.object(rows.math,'family_material',side_effect=failure), patch.object(rows.math,'order',side_effect=failure), \
                patch.object(export,'read_bytes',side_effect=failure), patch.object(Path,'open',side_effect=failure):
            self.assertEqual(runner.prepare_groups(rows,parameters,'sample'),expected)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--target', type=Path, default=ROOT/'b/sorter')
    parser.add_argument('--model', type=Path, default=ROOT/'b/paths_learning_document_tests')
    args, rest = parser.parse_known_args()
    TARGET, MODEL = args.target, args.model
    unittest.main(argv=[sys.argv[0], *rest])
