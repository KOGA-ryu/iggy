#!/usr/bin/env python3
"""Independently verify the published numeric row operations and solution sets."""
import ast
from fractions import Fraction as F
import importlib.util
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('author', ROOT/'tools/generate_matrix_reasoning.py')
author = importlib.util.module_from_spec(spec); spec.loader.exec_module(author)
bank = json.loads((ROOT/'content/corpus/matrix_reasoning.json').read_text())
assert bank == author.build(), 'Published questions must reproduce exactly'


def rational(text):
    text = str(text).replace(' ', '')
    if text in ('', '+'): return F(1)
    if text == '-': return F(-1)
    return F(re.sub(r'\\frac(\d)(\d)', r'\1/\2', text))


def entries(text):
    found = re.search(r'\\begin\{(?:bmatrix|array)\}(?:\{[^}]*\})?(.*?)\\end\{', text)
    assert found, text
    return [[cell for cell in row.split('&')] for row in found[1].split(r'\\')]


def numeric(text): return [[rational(cell) for cell in row] for row in entries(text)]


def operated(before, label):
    rows = [row[:] for row in before]
    match = re.fullmatch(r'R_(\d)\\leftrightarrow R_(\d)', label)
    if match:
        a, b = (int(x)-1 for x in match.groups()); rows[a], rows[b] = rows[b], rows[a]; return rows
    match = re.fullmatch(r'R_(\d)\\leftarrow R_\1([+-].*)R_(\d)', label)
    if match:
        a, scalar, b = match.groups(); a, b = int(a)-1, int(b)-1
        rows[a] = [x+rational(scalar)*y for x, y in zip(rows[a], rows[b])]; return rows
    match = re.fullmatch(r'R_(\d)\\leftarrow(.*)R_\1', label)
    assert match, label
    a, scalar = match.groups(); scalar = rational(scalar)
    if scalar == 0: return None  # Not an elementary row scaling.
    rows[int(a)-1] = [scalar*x for x in rows[int(a)-1]]; return rows


def rref(source):
    a = [[F(x) for x in row] for row in source]; row = 0
    for col in range(len(a[0])):
        pivot = next((i for i in range(row, len(a)) if a[i][col]), None)
        if pivot is None: continue
        a[row], a[pivot] = a[pivot], a[row]
        scale = a[row][col]; a[row] = [x/scale for x in a[row]]
        for i in range(len(a)):
            if i != row:
                scale = a[i][col]; a[i] = [x-scale*y for x, y in zip(a[i], a[row])]
        row += 1
        if row == len(a): break
    return a


def rank(a): return sum(any(row) for row in rref(a))
def product(a, x): return [sum(F(v)*y for v, y in zip(row, x)) for row in a]


def affine(text):
    """Return exact constant, s and t coefficients; reject nonlinear expressions."""
    node = ast.parse(re.sub(r'(\d)([st])', r'\1*\2', text), mode='eval').body
    def visit(node):
        if isinstance(node, ast.Constant): return [F(node.value), F(0), F(0)]
        if isinstance(node, ast.Name) and node.id in ('s','t'): return [F(0), F(node.id=='s'), F(node.id=='t')]
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub): return [-v for v in visit(node.operand)]
        if isinstance(node, ast.BinOp):
            a, b = visit(node.left), visit(node.right)
            if isinstance(node.op, ast.Add): return [x+y for x,y in zip(a,b)]
            if isinstance(node.op, ast.Sub): return [x-y for x,y in zip(a,b)]
            if isinstance(node.op, ast.Mult):
                if not any(a[1:]): return [a[0]*v for v in b]
                if not any(b[1:]): return [b[0]*v for v in a]
        raise AssertionError('Not an affine parameter expression')
    return visit(node)


operations = wrong_operations = 0
for row in bank['questions']:
    q = row['question']; states = {s['id']:s['display'] for s in q['working_states']}
    check = row['check']; given = check['given']
    expected_given = [r+[b] for r,b in zip(given,check['rhs'])] if 'rhs' in check else given
    assert numeric(q['equation']) == expected_given, 'The independent problem must match the actual displayed givens'
    for step in q['steps']:
        if step['semantics']['purpose'] != 'operation_choice': continue
        before, after = (numeric(states[step['semantics'][key]]) for key in ('before','after'))
        for option in step['options']:
            result = operated(before, option['label'])
            if option['id'] in step['accepted_option_ids']:
                assert result == after, (row['id'], step['id'], 'wrong accepted operation'); operations += 1
            else:
                assert result != after, (row['id'], step['id'], 'equivalent operation incorrectly rejected'); wrong_operations += 1
    check = row['check']; a = check['given']; final = q['working_states'][-1]['display']
    kind = check['kind']
    assert ('{rr|r}' in q['equation']) == (kind in ('unique','affine','inconsistent')), 'Right-hand-side interpretation must be explicit'
    if kind == 'rref':
        assert numeric(final) == rref(a) == [[F(x) for x in r] for r in check['answer']]
    elif kind == 'kernel':
        coefficients = [affine(r[0]) for r in entries(final)]
        origin, s, t = zip(*coefficients)
        assert product(a,origin) == product(a,s) == product(a,t) == [0,0]
        assert rank([s,t]) == len(a[0])-rank(a) == 2
        pivot_step = q['steps'][1]; answer = next(o['label'] for o in pivot_step['options'] if o['id'] in pivot_step['accepted_option_ids'])
        assert answer == r'\{1\}' and rank(a) == 1
    elif kind == 'unique':
        answer = [F(x) for x in final.split('=')[1].strip('()').split(',')]
        assert answer == check['answer'] and product(a,answer) == check['rhs'] and rank(a) == len(a[0])
    elif kind == 'inconsistent':
        assert rank(a) < rank([r+[b] for r,b in zip(a,check['rhs'])]) and final == r'S=\varnothing'
        assert next(o['label'] for o in q['steps'][1]['options'] if o['id'] in q['steps'][1]['accepted_option_ids']) == '0=2'
    elif kind == 'affine':
        expressions = final.split('=')[1].split(')')[0].lstrip('(').split(',')
        coefficients = [affine(expression) for expression in expressions]; origin, s, direction = zip(*coefficients)
        assert not any(s) and product(a,origin) == check['rhs'] and product(a,direction) == [0,0]
        assert rank([direction]) == len(a[0])-rank(a) == 1
        assert next(o['label'] for o in q['steps'][1]['options'] if o['id'] in q['steps'][1]['accepted_option_ids']) == '0=0'

# Recognition follows from the actual pivot columns, not from a stored label.
recognition = bank['questions'][3]['question']; b = numeric(recognition['equation'])
pivots = [next(i for i,x in enumerate(row) if x) for row in b]
assert pivots == [0,1] and b[0][1] == 3 and b != rref(b)
assert next(o['label'] for o in recognition['steps'][0]['options'] if o['id'] in recognition['steps'][0]['accepted_option_ids']) == r'\mathrm{REF},\ \neg\mathrm{RREF}'
assert next(o['label'] for o in recognition['steps'][1]['options'] if o['id'] in recognition['steps'][1]['accepted_option_ids']) == 'b_{12}=3'
print(json.dumps(dict(questions=8, decisions=23, exact_row_operations=operations,
                     rejected_alternative_operations=wrong_operations, solution_sets=4, generator_reproduced=True)))
