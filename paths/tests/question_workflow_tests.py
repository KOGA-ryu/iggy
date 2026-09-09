"""Authoring defects, exact arithmetic and support disclosure for the bounded pilot."""
import copy
from fractions import Fraction
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import question_workflow as workflow


class QuestionWorkflow(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.spec = json.loads(workflow.SPEC.read_text())
        cls.bank = workflow.generate(cls.spec)
        cls.golden = workflow.make_question(cls.spec, cls.spec['family']['golden_parameters'])

    def test_golden_math_and_teaching_links(self):
        workflow.validate_spec(self.spec)
        workflow.verify_question(self.golden)
        self.assertEqual(self.golden['problem']['equation_tex'], '3x+5=20')
        self.assertEqual([s['c'] for s in self.golden['states']], ['20', '15', '5'])
        self.assertEqual(self.golden['answer']['value'], '5')
        self.assertEqual(self.golden['verification']['left'], '20')
        broken = copy.deepcopy(self.spec)
        del broken['definitions']['eq.nonzero_division']
        with self.assertRaisesRegex(ValueError, 'definition'):
            workflow.validate_spec(broken)

    def test_exact_generated_instances_and_distractors(self):
        self.assertEqual(len(self.bank), 24)
        self.assertEqual({q['stratum'] for q in self.bank}, set(workflow.STRATA))
        self.assertEqual(sum(q['cohort'] == 'fresh_check' for q in self.bank), 6)
        identities, normalized, correct_positions = set(), set(), set()
        for q in self.bank:
            workflow.verify_question(q)
            p = q['parameters']
            # Solve from the published givens, not the generator's chosen root.
            s = (Fraction(p['c']) - p['b']) / p['a']
            self.assertEqual(Fraction(q['answer']['value']), s)
            self.assertEqual(p['a']*s+p['b'], Fraction(p['c']))
            identities.add(q['id'])
            normalized.add((Fraction(p['b'], p['a']), Fraction(p['c'])/p['a']))
            for i, step in enumerate(q['steps']):
                expected = Fraction(p['c'])-p['b'] if i == 0 else s
                values = [Fraction(o['value']) for o in step['choices']]
                self.assertEqual(values.count(expected), 1)
                self.assertEqual(len(set(values)), 3)
                correct_positions.add(values.index(expected))
        self.assertEqual(len(identities), 24)
        self.assertEqual(len(normalized), 24)
        self.assertEqual(correct_positions, {0, 1, 2})

    def test_support_projection_cannot_leak_authoring_key(self):
        for q in self.bank:
            for level in workflow.PROFILE_IDS:
                for step in (0, 1):
                    view = workflow.project(self.spec, q, level, step)
                    self.assertEqual(view['problem'], q['problem'])
                    self.assertFalse({'answer', 'states', 'verification', 'parameters', 'steps'} & view.keys())
                    if level == 'learn':
                        self.assertIn('definitions', view)
                        self.assertIn('why', view)
                        self.assertEqual([b['symbol'] for b in view['bindings']], ['a', 'b', 'c'])
                        self.assertEqual([o['id'] for o in view['choices']], [1, 2, 3])
                        self.assertTrue(all(set(o) == {'id', 'tex'} for o in view['choices']))
                    elif level == 'practice':
                        self.assertIn('blank_tex', view)
                        self.assertEqual(view['input'], 'symbol_choices_optional_blank')
                        self.assertEqual(view['choices'], workflow.project(self.spec, q, 'learn', step)['choices'])
                        self.assertNotIn('definitions', view)
                        self.assertNotIn('why', view)
                    elif level == 'solve':
                        self.assertEqual(set(view), {'question_id', 'support', 'problem', 'input', 'working', 'checkpoint'})
                    else:
                        self.assertEqual(set(view), {'question_id', 'support', 'problem', 'input', 'working'})
                        self.assertEqual(view['working'], '')
        broken = copy.deepcopy(self.spec)
        broken['profiles'][3]['show_why'] = True
        with self.assertRaisesRegex(ValueError, 'disclosure'):
            workflow.validate_spec(broken)
        broken = copy.deepcopy(self.spec)
        broken['schema_version'] = True
        with self.assertRaisesRegex(ValueError, 'contract'):
            workflow.validate_spec(broken)

    def test_runtime_publication_preserves_checked_instance_and_teaching(self):
        published = workflow.runtime_bank(self.spec)
        self.assertEqual(len(published['questions']), 25)
        self.assertEqual(published, json.loads((ROOT/'content/corpus/linear_support.json').read_text()))
        instances = {q['id']: q for q in [self.golden, *self.bank]}
        for row in published['questions']:
            q, authored = row['question'], instances[row['id']]
            self.assertEqual(q['equation'], authored['problem']['equation_tex'])
            self.assertEqual(q['support']['domain'], authored['problem']['domain'])
            self.assertEqual(q['support']['family'], 'linear_balance_ax_b_v1')
            for i, step in enumerate(q['steps']):
                support = q['support']['steps'][i]
                accepted = [j for j, option in enumerate(step['options']) if option['id'] in step['accepted_option_ids']]
                self.assertEqual(len(accepted), 1)
                self.assertEqual(Fraction(support['responses'][accepted[0]]), Fraction(authored['states'][i+1]['c']))
                self.assertEqual(len({Fraction(x) for x in support['responses']}), 3)
                self.assertIn(self.spec['teaching'][authored['steps'][i]['id']]['why'], support['teaching'])
                self.assertTrue(support['definitions'])

    def test_wrong_math_and_broken_routes_are_rejected(self):
        mutations = [
            lambda q: q['states'][1].update(c='25'),
            lambda q: q['states'][1].update(a='0', b='0', c='0'),
            lambda q: q['steps'][0].update(after=2),
            lambda q: q['steps'][0]['operation'].update(kind='add_both'),
            lambda q: q['answer'].update(value='6'),
            lambda q: q['verification'].update(substitute_into='last_line'),
            lambda q: q['problem'].update(equation_tex='3x+5=21'),
            lambda q: q['steps'][1]['choices'][0].update(tex='0'),
            lambda q: q.update(id='order_based_question_001'),
        ]
        for mutate in mutations:
            broken = copy.deepcopy(self.golden)
            mutate(broken)
            with self.subTest(mutation=mutations.index(mutate)), self.assertRaises(ValueError):
                workflow.verify_question(broken)
        broken = copy.deepcopy(self.golden)
        options = broken['steps'][1]['choices']
        wrong = next(o for o in options if o['id'] != 'correct')
        wrong.update(value='10/2', tex='5')
        with self.assertRaisesRegex(ValueError, 'Duplicate'):
            workflow.verify_question(broken)

    def test_bounds_signs_zero_and_fractions(self):
        for a in (-9, -2, 2, 9):
            for b in (-9, -1, 1, 9):
                for c in ('-81', '-1/3', '0', '1/3', '81', str(b)):
                    q = workflow.make_question(self.spec, dict(a=a, b=b, c=c))
                    workflow.verify_question(q)
        for p in [dict(a=0, b=5, c='20'), dict(a=1, b=5, c='20'), dict(a=3, b=0, c='20'),
                  dict(a=True, b=5, c='20'), dict(a=3, b=5, c='1/0'), dict(a=3, b=5, c=0.1)]:
            with self.subTest(parameters=p), self.assertRaises(ValueError):
                workflow.make_question(self.spec, p)
        largest = workflow.generate(self.spec, 10)
        self.assertEqual(len(largest), 60)
        for q in largest:
            workflow.verify_question(q)

    def test_reproducibility_and_identity_ignore_presentation(self):
        self.assertEqual(workflow.generate(self.spec), self.bank)
        amended = copy.deepcopy(self.spec)
        amended['definitions']['eq.balance']['meaning'] += ' Reading revision.'
        self.assertEqual(workflow.generate(amended), self.bank)
        one_per = workflow.generate(self.spec, 1)
        self.assertEqual({q['id'] for q in one_per}, {self.bank[i*4]['id'] for i in range(6)})
        reversed_bank = list(reversed(self.bank))
        self.assertEqual({q['id']: q for q in self.bank}, {q['id']: q for q in reversed_bank})

    def test_coverage_counts_leaves_not_duplicate_parent_totals(self):
        rows = workflow.coverage()
        self.assertEqual(len(rows), 229)
        self.assertEqual(sum(r['kind']=='named_subcategory' for r in rows), 94)
        self.assertEqual(sum(r['kind']=='terminal_chapter' for r in rows), 135)
        self.assertEqual(len({r['subject'] for r in rows}), 6)
        self.assertEqual(sum(r['integrated_four_level_reps'] for r in rows), 0)
        self.assertTrue(all(r['taxonomy_review'] == 'pending' and r['gaps'] for r in rows))
        parents = {r['topic'] for r in rows if r['kind'] == 'named_subcategory'}
        self.assertFalse(parents & {r['id'] for r in rows if r['kind'] == 'terminal_chapter'})


if __name__ == '__main__':
    unittest.main(verbosity=2)
