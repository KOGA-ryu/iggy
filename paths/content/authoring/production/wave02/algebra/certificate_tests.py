"""Finite affine mathematics over the existing compiler's question-batch JSON.

No Markdown/document parser, content generator, runtime checker or publisher.
Python's standard ast parser reads bounded arithmetic expressions after only
TeX fraction/delimiter normalization; a safe affine evaluator rejects nonlinear
products and variable/zero divisors. Original term sums provide a second,
independent expansion/evaluation path. Authored keys never determine truth.
"""
import argparse
import ast
import copy
from fractions import Fraction as F
import json
from math import lcm
from pathlib import Path
import re
import sys

SOURCE = Path(__file__).resolve().parent
ROOT = SOURCE.parents[4]
sys.path.insert(0, str(ROOT / 'tools'))
from question_workflow import exact, tex as shared_tex

# Only this finite prompt explicitly orders the two kinds of term. Other
# simplified-equation goals permit constant-first and variable-first forms.
VARIABLE_FIRST_GOALS = frozenset({('prod02_algebra_full_linear_q12', 'simplify')})
EQUATION_GOALS = frozenset({'simplify', 'clear', 'collect', 'isolate', 'solve'})


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def tex(value):
    return shared_tex(str(value))


def arithmetic_tree(text):
    """One existing arithmetic normalization/AST path for truth and form."""
    text = text.replace(r'\left', '').replace(r'\right', '').replace(' ', '')
    for _ in range(8):
        new = re.sub(r'\\frac\{([^{}]+)\}\{([^{}]+)\}', r'((\1)/(\2))', text)
        if new == text:
            break
        text = new
    need(len(text) < 512 and re.fullmatch(r'[0-9x()+*/.\-]+', text), 'unsupported arithmetic field')
    text = re.sub(r'(?<=[0-9x)])(?=[x(])', '*', text)
    return ast.parse(text, mode='eval').body


def affine(text):
    """Interpret only arithmetic in a compiled math field, never directives."""

    def visit(node):
        if isinstance(node, ast.Constant):
            need(type(node.value) is int, 'only exact integer literals are allowed')
            return F(0), F(node.value)
        if isinstance(node, ast.Name):
            need(node.id == 'x', 'unknown variable')
            return F(1), F(0)
        if isinstance(node, ast.UnaryOp):
            a, b = visit(node.operand)
            need(isinstance(node.op, (ast.UAdd, ast.USub)), 'unsupported unary operation')
            return (-a, -b) if isinstance(node.op, ast.USub) else (a, b)
        need(isinstance(node, ast.BinOp), 'unsupported arithmetic node')
        a, b = visit(node.left)
        c, d = visit(node.right)
        if isinstance(node.op, ast.Add):
            return a+c, b+d
        if isinstance(node.op, ast.Sub):
            return a-c, b-d
        if isinstance(node.op, ast.Mult):
            need(not a*c, 'nonlinear product')
            return a*d+b*c, b*d
        need(isinstance(node.op, ast.Div) and c == 0 and d != 0, 'variable or zero denominator')
        return a/d, b/d

    return visit(arithmetic_tree(text))


def equation(text):
    sides = text.split('=')
    need(len(sides) == 2, 'equation needs exactly two sides')
    return (*affine(sides[0]), *affine(sides[1]))


def equation_form(text, *, variable_first=False, cleared=False):
    """Check the requested displayed form, independently of affine values.

    Each side has at most one variable monomial and one constant atom. The
    AST must already have distributed/combined them: evaluating an arbitrary
    subtree to its affine value here would recreate F1. Rational atoms, signs,
    parentheses and either scalar*x or x*scalar notation remain available.
    """
    def integer(node):
        if isinstance(node, ast.Constant) and type(node.value) is int:
            return node.value
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
            value = integer(node.operand)
            return None if value is None else (-value if isinstance(node.op, ast.USub) else value)
        return None

    def scalar(node):
        value = integer(node)
        if value is not None:
            return F(value)
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
            value = scalar(node.operand)
            return None if value is None else (-value if isinstance(node.op, ast.USub) else value)
        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Div):
            numerator, denominator = integer(node.left), integer(node.right)
            if numerator is not None and denominator not in (None, 0):
                return F(numerator, denominator)
        return None

    def variable(node):
        if isinstance(node, ast.Name) and node.id == 'x':
            return 1
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
            value = variable(node.operand)
            return None if value is None else (-value if isinstance(node.op, ast.USub) else value)
        return None

    def term(node):
        number = scalar(node)
        if number is not None:
            return 0, number
        coefficient = variable(node)
        if coefficient is not None:
            return 1, F(coefficient)
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
            value = term(node.operand)
            return None if value is None else (value[0], -value[1] if isinstance(node.op, ast.USub) else value[1])
        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Mult):
            for factor, name in ((node.left, node.right), (node.right, node.left)):
                number, coefficient = scalar(factor), variable(name)
                if number is not None and coefficient is not None:
                    return 1, number*coefficient
        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Div):
            coefficient, denominator = variable(node.left), scalar(node.right)
            if coefficient is not None and denominator not in (None, 0):
                return 1, F(coefficient)/denominator
        return None

    def summands(node):
        if isinstance(node, ast.BinOp) and isinstance(node.op, (ast.Add, ast.Sub)):
            return summands(node.left) + summands(node.right)
        return [term(node)]

    sides = text.split('=')
    need(len(sides) == 2, 'equation form needs two sides')
    for side in sides:
        tree = arithmetic_tree(side)
        if cleared and any(isinstance(n, ast.Div) for n in ast.walk(tree)):
            return False
        parts = summands(tree)
        if any(p is None for p in parts):
            return False
        degrees = [p[0] for p in parts]
        if len(set(degrees)) != len(degrees):
            return False  # repeated variable terms or constants remain uncombined
        if any(value == 0 and (degree == 1 or len(parts) > 1) for degree, value in parts):
            return False  # an explicit zero term has not been removed
        if variable_first and degrees != sorted(degrees, reverse=True):
            return False
    return True


def expand_terms(terms):
    a = b = F(0)
    for term in terms:
        need(type(term) is list and len(term) == 3, 'original term shape')
        m, c, d = map(exact, term)
        need(all(abs(q.numerator) <= 12 and q.denominator <= 6 for q in (m, c, d)), 'original input outside bound')
        a += m*c
        b += m*d
    return a, b


def evaluate_terms(terms, x):
    return sum((exact(m)*(exact(a)*x+exact(b)) for m, a, b in terms), F(0))


def solution(state):
    a, b, c, d = state
    if a == c:
        return ('all',) if b == d else ('none',)
    return 'one', (d-b)/(a-c)


def set_text(result):
    if result[0] == 'all':
        return r'S=\mathbb{R}'
    if result[0] == 'none':
        return r'S=\varnothing'
    return 'S=\\{' + tex(result[1]) + r'\}'


def set_value(text):
    if text == r'S=\mathbb{R}':
        return ('all',)
    if text == r'S=\varnothing':
        return ('none',)
    need(text.startswith(r'S=\{') and text.endswith(r'\}'), 'unsupported set option')
    a, b = affine(text[4:-2])
    need(a == 0, 'nonconstant singleton')
    return 'one', b


def pair_value(text):
    text = text.replace(r'\left', '').replace(r'\right', '')
    need(text.startswith('(') and text.endswith(')'), 'ordered-pair option expected')
    parts = text[1:-1].split(',')
    need(len(parts) == 2, 'ordered pair needs two values')
    pair = [affine(p) for p in parts]
    need(all(a == 0 for a, _ in pair), 'pair must contain exact numbers')
    return tuple(b for _, b in pair)


def audit(routes, pool):
    need(pool['input_bound'] == dict(numerator_magnitude=12, denominator_max=6, domain='real', variable_denominators=False), 'invalid declared domain')
    cases = pool['cases']
    need([c['id'] for c in cases] == [f'prod02_algebra_full_linear_q{i:02}' for i in range(1, 13)], 'finite case identities')
    need(routes['accepted'] is True and routes['question_ids'] == [c['id'] for c in cases], 'compiled case identities')
    need(len(routes['questions']) == 12, 'compiled question count')
    records = []
    positions = []
    for case, row in zip(cases, routes['questions']):
        q = row['question']
        need(q['id'] == row['id'] == case['id'], 'compiled question identity')
        need(q['equation'] == case['given'], 'compiled original differs from declared given')
        need(q['description'] == 'Solve the original equation completely and verify the solution set. ' + pool['domain'], 'compiled goal/domain differs')
        original = (*expand_terms(case['left']), *expand_terms(case['right']))
        need(equation(q['equation']) == original, 'compiled original expansion differs from independent terms')
        result = solution(original)
        state = original
        k = lcm(*(exact(v).denominator for terms in (case['left'], case['right']) for term in terms for v in term))
        steps = q['steps']
        working = q['working_states']
        need(len(steps) == len(case['goals']) and 4 <= len(steps) <= 8, 'complete route step count')
        need(len(working) == len(steps)+1 and working[0]['display'] == case['given'], 'working route shape')
        record = {'id': case['id'], 'original': original, 'solution_set': result, 'denominator_multiplier': k, 'steps': []}
        for n, (goal, step) in enumerate(zip(case['goals'], steps)):
            a, b, c, d = state
            before = state
            form_policy = {'variable_first': (case['id'], goal) in VARIABLE_FIRST_GOALS,
                           'cleared': goal == 'clear'}
            if goal in EQUATION_GOALS:
                if goal == 'clear':
                    need(k != 0, 'nonzero clearing multiplier')
                    state = tuple(k*v for v in state)
                elif goal == 'collect':
                    state = a-c, b, F(0), d
                elif goal == 'isolate':
                    need(c == 0, 'isolate goal requires variables collected')
                    state = a, F(0), c, d-b
                elif goal == 'solve':
                    need(b == c == 0 and a != 0, 'division requires isolated nonzero coefficient')
                    state = F(1), F(0), F(0), d/a
                expected = state
                decode = equation
                need(solution(state) == result, 'operation changes original solution set')
                need(equation(working[n+1]['display']) == state, 'false intermediate working')
                need(equation_form(working[n+1]['display'], **form_policy), 'reached form violates local goal')
            elif goal == 'method':
                expected = (F(0), F(k))
                def decode(s):
                    need(s.startswith('k='), 'method multiplier expected')
                    return affine(s[2:])
                need(working[n+1]['display'] == f'k={k}', 'false method working')
            elif goal == 'classify':
                need(a == c == 0, 'classify only after cancellation')
                expected, decode = result, set_value
                need(working[n+1]['display'] == set_text(result), 'false solution-set working')
            elif goal == 'check':
                need(result[0] == 'one' and state == (1, 0, 0, result[1]), 'check requires isolated unique candidate')
                x = result[1]
                values = evaluate_terms(case['left'], x), evaluate_terms(case['right'], x)
                need(values[0] == values[1], 'original substitution failed')
                expected, decode = values, pair_value
                r, left, right = tex(x), tex(values[0]), tex(values[1])
                expected_work = set_text(result) + rf',\quad (L({r}),R({r}))=({left},{right})'
                need(working[n+1]['display'] == expected_work, 'false original check working')
                record['unexpanded_substitution'] = {'x': x, 'left': values[0], 'right': values[1]}
            else:
                need(goal == 'difference' and result[0] in ('all', 'none'), 'unsupported local goal')
                expected = original[0]-original[2], original[1]-original[3]
                need(expected[0] == 0, 'degenerate difference must be constant')
                def decode(s):
                    need(s.startswith('L(x)-R(x)='), 'original difference expected')
                    return affine(s[len('L(x)-R(x)='):])
                need(working[n+1]['display'] == set_text(result) + r',\quad L(x)-R(x)=' + tex(expected[1]), 'false original difference working')
                record['all_real_difference'] = expected
            options = step['options']
            need(len(options) == 3, 'three symbolic choices required')
            values = [decode(o['label']) for o in options]
            need(len(set(values)) == 3, 'equivalent duplicate option')
            forms = [equation_form(o['label'], **form_policy) if goal in EQUATION_GOALS else True for o in options]
            correct = [o['id'] for o, v, form in zip(options, values, forms) if v == expected and form]
            need(len(correct) == 1, 'not exactly one goal-correct choice')
            need(step['accepted_option_ids'] == correct, 'false accepted key')
            need(step['semantics']['before'] == working[n]['id'] and step['semantics']['after'] == working[n+1]['id'], 'wrong reached-state linkage')
            need(step['prompt'] and step['explanation'] and step['wrong_hint'], 'missing teaching text')
            for option in options:
                need(bool(option.get('wrong_feedback')) == (option['id'] not in correct), 'wrong-choice feedback attachment')
            if n == 0:
                positions.append(next(i+1 for i, o in enumerate(options) if o['id'] == correct[0]))
            record['steps'].append({'goal': goal, 'before_coefficients': before, 'expected_mathematics': expected,
                                    'representation_policy': {'simplified_sides': goal in EQUATION_GOALS, **form_policy},
                                    'independently_correct_ids': correct, 'reached': working[n+1]['display'],
                                    'options': [{'id': o['id'], 'label': o['label'], 'mathematics': v,
                                                 'mathematical_target_matches': v == expected,
                                                 'representation_meets_goal': form, 'goal_correct': v == expected and form}
                                                for o, v, form in zip(options, values, forms)]})
        records.append(record)
    need(sorted(positions) == [1]*4+[2]*4+[3]*4, 'unbalanced first positions')
    outcomes = [r['solution_set'][0] for r in records]
    need(outcomes.count('one') == 8 and outcomes.count('none') == outcomes.count('all') == 2, 'outcome coverage')
    need(sum(len(r['steps']) for r in records) == 52 and routes['wrong_choices'] == 104, 'route counts')
    return {'questions': 12, 'steps': 52, 'wrong_choices': 104, 'outcomes': {'unique': 8, 'none': 2, 'all_real': 2}, 'first_correct_positions': positions, 'cases': records}


def regressions(routes, pool):
    results = []
    def rejects(name, mutation, pool_mutation=False):
        r, p = copy.deepcopy(routes), copy.deepcopy(pool)
        mutation(p if pool_mutation else r)
        try:
            audit(r, p)
        except (ValueError, KeyError, SyntaxError, ZeroDivisionError) as error:
            results.append({'test': name, 'rejected': True, 'reason': str(error)})
        else:
            raise ValueError('negative test unexpectedly passed: ' + name)
    rejects('false accepted key', lambda r: r['questions'][0]['question']['steps'][0].update(accepted_option_ids=[12]))
    rejects('false intermediate working', lambda r: r['questions'][0]['question']['working_states'][1].update(display='2x-5=8'))
    rejects('equivalent duplicate option with different spelling', lambda r: r['questions'][0]['question']['steps'][0]['options'][1].update(label='2x+(-5)=7'))
    rejects('compiled domain changed', lambda r: r['questions'][0]['question'].update(description='Work over complex numbers.'))
    rejects('compiled original changed', lambda r: r['questions'][0]['question'].update(equation='3x-5=x+8'))
    rejects('invalid finite domain', lambda p: p['input_bound'].update(domain='complex'), True)
    rejects('invalid original constant denominator', lambda p: p['cases'][3]['left'][0].__setitem__(0, '1/0'), True)
    rejects('out-of-bound original coefficient', lambda p: p['cases'][0]['left'][0].__setitem__(0, 13), True)
    rejects('false final solution set', lambda r: r['questions'][6]['question']['working_states'][3].update(display=r'S=\{0\}'))
    # Reviewer demonstrations: preserve the prompt and mathematical values,
    # corrupt the label alone, reached display alone, then both together.
    for qindex, sindex, text in ((2, 0, '2(x-3)+1=9'),
                                (8, 1, '3(x-1)-2(x+2)=6'),
                                (11, 0, '6-x=6-x')):
        for fields in ('label', 'reached', 'both'):
            rejects(f'goal form q{qindex+1:02} step {(sindex+1)*10} {fields}',
                    lambda r, qi=qindex, si=sindex, t=text, f=fields: replace_form(r, qi, si, t, f))
    for qindex, sindex, text in ((2, 0, '2x-6+1=9'),
                                (8, 1, '3x-3-2x-4=6'),
                                (8, 1, r'\frac{2x}{2}-7=6')):
        rejects(f'unfinished simplification {text}',
                lambda r, qi=qindex, si=sindex, t=text: replace_form(r, qi, si, t))
    for text in ('x*x', '1/x', '1/0'):
        try:
            affine(text)
        except ValueError as error:
            results.append({'test': 'excluded arithmetic ' + text, 'rejected': True, 'reason': str(error)})
        else:
            raise ValueError('excluded arithmetic accepted')
    return results


def replace_form(routes, qindex, sindex, text, fields='both'):
    q = routes['questions'][qindex]['question']
    step = q['steps'][sindex]
    accepted = next(o for o in step['options'] if o['id'] in step['accepted_option_ids'])
    need(equation(text) == equation(accepted['label']), 'form probe must preserve the exact affine side values')
    if fields in ('label', 'both'):
        accepted['label'] = text
    if fields in ('reached', 'both'):
        next(w for w in q['working_states'] if w['id'] == step['semantics']['after'])['display'] = text


def representation_controls(routes, pool):
    results = []
    for qindex, sindex, text in ((2, 0, '-5+2x=9'),
                                (8, 1, '-7+x=6'),
                                (11, 0, '(-x)+6=-1*x+6'),
                                (5, 2, 'x=20/7')):
        varied = copy.deepcopy(routes)
        replace_form(varied, qindex, sindex, text)
        proof = audit(varied, pool)
        results.append({'question_id': pool['cases'][qindex]['id'], 'step_id': (sindex+1)*10,
                        'equivalent_label_and_reached': text, 'accepted': True,
                        'independent_target': proof['cases'][qindex]['steps'][sindex]['expected_mathematics']})
    return results


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--routes', type=Path, required=True)
    parser.add_argument('--edited-routes', type=Path)
    args = parser.parse_args()
    pool = json.loads((SOURCE/'cases.json').read_text())
    routes = json.loads(args.routes.read_text())
    result = audit(routes, pool)
    result.update(accepted=True, negative_tests=regressions(routes, pool),
                  representation_controls=representation_controls(routes, pool),
                  scope='all twelve declared compiled cases; no generic runtime or larger-domain claim')
    if args.edited_routes:
        edited = json.loads(args.edited_routes.read_text())
        need(audit(edited, pool) == audit(routes, pool), 'Markdown edit changed mathematics')
        before = routes['questions'][0]['question']['steps'][0]
        after = edited['questions'][0]['question']['steps'][0]
        need(before['prompt'] != after['prompt'], 'real prompt edit did not reach compiled field')
        need(before['options'][1]['wrong_feedback'] != after['options'][1]['wrong_feedback'], 'real feedback edit did not reach compiled field')
        reverted = copy.deepcopy(edited)
        reverted_step = reverted['questions'][0]['question']['steps'][0]
        reverted_step['prompt'] = before['prompt']
        reverted_step['options'][1]['wrong_feedback'] = before['options'][1]['wrong_feedback']
        need(reverted == routes, 'Markdown edit affected unexpected compiled fields')
        result['markdown_edit_tests'] = {
            'question_id': pool['cases'][0]['id'], 'step_id': before['id'],
            'prompt_before': before['prompt'], 'prompt_after': after['prompt'],
            'wrong_feedback_before': before['options'][1]['wrong_feedback'],
            'wrong_feedback_after': after['options'][1]['wrong_feedback'],
            'all_mathematics_unchanged': True, 'only_two_compiled_fields_changed': True}
    print(json.dumps(result, indent=2, default=str))
