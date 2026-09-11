#!/usr/bin/env python3
"""Exact finite 2x2 inverse certificates over actual compiled choices.v1 routes."""
import argparse
import copy
from fractions import Fraction as F
import json
from pathlib import Path
import re
import subprocess

import build_question_batch as shared
import export_learning as export
from question_workflow import exact

ROOT = Path(__file__).resolve().parent
WORKSPACE = shared.ROOT
EVIDENCE = WORKSPACE / 'build/production/wave03/linear_algebra'
MODEL = WORKSPACE / 'b/paths_learning_document_tests'
PACKAGE = 'prod03_linear_matrix_inverse'
I = ((F(1), F(0)), (F(0), F(1)))
ZERO = ((F(0),), (F(0),))
DOMAIN = ('All entries and vectors are real. Rows precede columns. I is the 2 by 2 '
          'identity, B is a candidate inverse, and R1 and R2 denote complete rows.')
GOALS = {
    'inverse': 'Construct the inverse of A and verify both AB=I and BA=I against the original matrix.',
    'application': 'Construct the inverse of A, verify both products, then solve Ax=b and check the original equation.',
    'singular': 'Determine whether A has an inverse and prove the conclusion using its determinant and a nonzero vector v with Av=0.',
}


def need(ok, message):
    export.require(ok, 'wave03.inverse', message)


def frozen(rows):
    return tuple(tuple(F(v) for v in row) for row in rows)


def mul(a, b):
    need(len(a) == len(b) == 2 and all(len(row) == 2 for row in a)
         and len(b[0]) == len(b[1]), 'bounded matrix product dimensions')
    return tuple(tuple(sum(a[i][k]*b[k][j] for k in range(2))
                       for j in range(len(b[0]))) for i in range(2))


def scaled(a, factor):
    return tuple(tuple(factor*v for v in row) for row in a)


def display_matrix(rows, bar=False):
    """Format computed numbers only to bind the separate worked-example source."""
    body = r'\\'.join('&'.join(shared.tex(v) for v in row) for row in rows)
    if bar:
        return r'\left[\begin{array}{cc|cc}'+body+r'\end{array}\right]'
    return r'\begin{bmatrix}'+body+r'\end{bmatrix}'


def inverse(a):
    (u,v),(w,z) = a
    determinant = u*z-v*w
    need(determinant != 0, 'zero determinant forbids inverse division')
    adj = ((z,-v),(-w,u))
    result = scaled(adj, 1/determinant)
    need(mul(a,result) == mul(result,a) == I, 'two-sided original inverse multiplication')
    return result


def original(case):
    a = case['A']
    need(type(a) is list and len(a) == 2
         and all(type(row) is list and len(row) == 2 for row in a), 'original A dimensions')
    need(all(type(v) is int and abs(v) <= 5 for row in a for v in row), 'original integer bound')
    need(any(v for row in a for v in row), 'zero original excluded')
    a = frozen(a)
    (u,v),(w,z) = a
    determinant = u*z-v*w
    rhs = None
    if case['kind'] == 'application':
        rhs = case.get('b')
        need(type(rhs) is list and len(rhs) == 2
             and all(type(v) is int and abs(v) <= 5 for v in rhs), 'original rhs bound')
        rhs = frozen([[v] for v in rhs])
    else:
        need('b' not in case, 'unexpected rhs')
    facts = dict(original=a, determinant=determinant, adjugate=((z,-v),(-w,u)))
    if determinant:
        need(case['kind'] != 'singular', 'singular route on nonsingular original')
        b = inverse(a)
        facts.update(inverse=b, AB=mul(a,b), BA=mul(b,a))
        if rhs is not None:
            x = mul(b,rhs)
            need(mul(a,x) == rhs, 'original Ax=b')
            # Cramer's rule uses original columns, separately from B*b.
            c1,c2 = rhs[0][0],rhs[1][0]
            need(x == ((F(c1*z-v*c2)/determinant,), (F(u*c2-c1*w)/determinant,)),
                 'independent original-column solution')
            facts.update(rhs=rhs, solution=x, original_Ax=mul(a,x))
    else:
        need(case['kind'] == 'singular', 'inverse route on zero determinant')
        row = next(row for row in a if any(row))
        need(row[0] != 0, 'finite witness normalization requires nonzero first coefficient')
        witness = ((-row[1]/row[0],),(F(1),))
        need(witness != ZERO and mul(a,witness) == ZERO, 'nonzero original null witness')
        facts.update(witness=witness, Av=mul(a,witness),
                     contradiction='If CA=I, v=(CA)v=C(Av)=0 contradicts v!=0.')
    return a, facts


# This recognizes only bounded numeric fields, not Markdown, arbitrary TeX or
# expressions. A scalar outside a matrix is evaluated for semantic equivalence
# but retained as a shape flag so an explicit entrywise-evaluation goal can fail.
def number(text):
    text = text.strip()
    match = re.fullmatch(r'([+-]?)\\frac\{([+-]?\d+)\}\{([+-]?\d+)\}', text)
    if match:
        sign,n,d = match.groups()
        n = int(n)*(-1 if sign == '-' else 1)
        d = int(d)
        need(d != 0, 'display denominator zero')
        need(abs(n) <= 10000 and abs(d) <= 10000, 'display rational input bound')
        return exact(f'{n}/{d}')
    need(re.fullmatch(r'[+-]?\d+(?:/[+-]?\d+)?',text) is not None, 'unsupported numeric field')
    value = exact(text)
    need(abs(value.numerator) <= 10000 and value.denominator <= 10000, 'display numeric bound')
    return value


def matrix_field(text):
    text = text.strip().replace(r'\,','').replace(' ','')
    marker = text.find(r'\begin')
    need(marker >= 0, 'numeric matrix expected')
    lead = text[:marker]
    # The left bracket belongs to the matrix wrapper, not a scalar.
    if lead.endswith(r'\left['):
        lead = lead[:-len(r'\left[')]
        body = r'\left[' + text[marker:]
    else:
        body = text[marker:]
    factor = number(lead) if lead else F(1)
    augmented = False
    if body.startswith(r'\begin{bmatrix}') and body.endswith(r'\end{bmatrix}'):
        inner = body[len(r'\begin{bmatrix}'):-len(r'\end{bmatrix}')]
        columns = None
    else:
        match = re.fullmatch(r'\\left\[\\begin\{array\}\{(cc\|cc|cc|c)\}(.*?)\\end\{array\}\\right\]',body)
        need(match is not None, 'unsupported bounded matrix wrapper')
        spec,inner = match.groups()
        augmented = spec == 'cc|cc'
        columns = len(spec.replace('|',''))
    rows = tuple(tuple(number(v) for v in row.split('&')) for row in inner.split(r'\\'))
    need(len(rows) == 2 and len(rows[0]) == len(rows[1]) and len(rows[0]) in (1,2,4),
         'numeric matrix dimensions')
    need(columns is None or columns == len(rows[0]), 'array column specification mismatch')
    need(len(rows[0]) != 4 or augmented, 'four columns require augmented bar')
    return scaled(rows,factor), not bool(lead), augmented


def named_matrix(text, prefix):
    need(text.startswith(prefix+'='), 'matrix label/prefix mismatch')
    return matrix_field(text[len(prefix)+1:])


def formula_method(text):
    pattern = r'B=\\frac\{(A|\\operatorname\{adj\}\(A\))\}\{\\det\(A\)\},\\quad\\det\(A\)(\\ne0|=0)'
    match = re.fullmatch(pattern,text.replace(' ',''))
    need(match is not None, 'unsupported bounded formula method')
    numerator,condition = match.groups()
    return ('adj' if numerator != 'A' else 'original', 'nonzero' if condition == r'\ne0' else 'zero')


def swap_method(text):
    # Three explicit elementary operations, not a row-command interpreter.
    forms = {
        r'R_1\leftrightarrow R_2':'swap', r'R_2\leftrightarrow R_1':'swap',
        r'R_1\leftarrow 0R_1':'erase', r'R_1\leftarrow R_1+R_2':'add',
    }
    key = text.strip()
    need(key in forms, 'unsupported bounded pivot method')
    return forms[key]


def equality(text):
    pieces = text.split('=')
    need(len(pieces) == 2, 'numeric vector equality expected')
    left,right = [matrix_field(p) for p in pieces]
    need(all(len(m[0][0]) == 1 and m[1] and not m[2] for m in (left,right)),
         'evaluated vector equality shape')
    # An equality and its reversed spelling are the same mathematical claim.
    return tuple(sorted((left[0],right[0])))


def augmented(a):
    return tuple(a[i]+I[i] for i in range(2))


def row_move(current, spec):
    kind = spec[0]
    if kind in ('swap','method_swap'):
        result = (current[1],current[0])
        need((result[1],result[0]) == current, 'swap inverse')
    elif kind == 'add':
        _,target,source,k = spec
        need(type(target) is int and type(source) is int and {target,source} == {0,1},
             'two distinct row indices')
        k = exact(k)
        need(k != 0, 'finite route excludes vacuous replacement')
        operation = 'add_row_1_to_2' if target == 1 else 'add_row_2_to_1'
        result = frozen(shared.operate(current,operation,k))
        need(frozen(shared.operate(result,operation,-k)) == current, 'complete row replacement inverse')
    else:
        need(kind == 'scale', 'unknown row operation')
        _,target,d = spec
        need(type(target) is int and target in (0,1), 'scaling row index')
        d = exact(d)
        need(d != 0, 'row division by zero')
        # Existing helper divides row 2 and handles all columns; reorder locally.
        order = [1-target,target]
        ordered = [current[i] for i in order]
        divided = shared.operate(ordered,'divide_row_2',d)
        result_list = [None,None]
        for i,row in zip(order,divided):
            result_list[i] = row
        result = frozen(result_list)
        restored = list(result)
        restored[target] = tuple(v*d for v in result[target])
        need(tuple(restored) == current, 'complete row division inverse')
    return result


def decoded(text, kind):
    if kind in ('add','scale','swap'):
        values,evaluated,bar = matrix_field(text)
        return values, evaluated and bar and len(values[0]) == 4
    if kind == 'method_swap':
        return swap_method(text), True
    if kind == 'method_formula':
        return formula_method(text), True
    if kind == 'det':
        need(text.startswith(r'\det(A)='), 'determinant label')
        return number(text[len(r'\det(A)='):]), True
    if kind == 'contradiction':
        return equality(text), True
    prefix = {'inverse':'B','formula':'B','adj':'J','solve':'x','check':'Ax',
              'witness':'v','null':'Av','AB':'AB','BA':'BA'}[kind]
    values,evaluated,bar = named_matrix(text,prefix)
    width = 1 if kind in ('solve','check','witness','null') else 2
    return values, evaluated and not bar and len(values[0]) == width


def verify(data, routes):
    need(data['format'] == 'paths_finite_inverse_cases' and data['format_version'] == 1, 'finite case format')
    need(data['domain'] == DOMAIN and data['goals'] == GOALS
         and data['bound'] == dict(A_integer_magnitude=5,rhs_integer_magnitude=5,nonzero_original=True),
         'declared finite domain changed')
    cases = data['cases']
    ids = [f'{PACKAGE}_q{i:02}' for i in range(1,13)]
    need([c['id'] for c in cases] == ids and routes['question_ids'] == ids
         and routes['accepted'] and len(routes['questions']) == 12, 'ordered complete question identities')
    need([c['group'] for c in cases] == ['introductory']*4+['practice']*4+['mixed']*4, 'groups')
    need([c['kind'] for c in cases] == ['inverse']*8+['application']*2+['singular']*2, 'route coverage')
    records,positions = [],[]
    for case,row in zip(cases,routes['questions']):
        a,facts = original(case)
        q = row['question']
        need(row['id'] == q['id'] == case['id'], 'compiled identity')
        need(q['description'] == GOALS[case['kind']]+' '+DOMAIN, 'compiled goal/domain')
        given = q['equation']
        if case['kind'] == 'application':
            parts = given.split(r',\quad ')
            need(len(parts) == 2, 'original matrix/rhs given')
            ga,af,ab = named_matrix(parts[0],'A')
            gb,bf,bb = named_matrix(parts[1],'b')
            need(ga == a and gb == facts['rhs'] and af and bf and not ab and not bb,
                 'compiled original A/rhs')
        else:
            ga,af,ab = named_matrix(given,'A')
            need(ga == a and af and not ab, 'compiled original A')
        steps = q['steps']
        plan = case['route']
        need(4 <= len(steps) <= 8 and len(steps) == len(plan)
             and len(q['working_states']) == len(steps)+1, 'complete route length')
        states = {s['id']:s['display'] for s in q['working_states']}
        need(len(states) == len(steps)+1, 'distinct reached state IDs')
        current,previous = augmented(a),given
        decisions = []
        seen = []
        for index,(step,spec) in enumerate(zip(steps,plan)):
            kind = spec[0]
            need(step['id'] == (index+1)*10, 'stable step identity')
            need(states[step['semantics']['before']] == previous, 'before-state linkage')
            after = states[step['semantics']['after']]
            before = current
            if kind in ('add','scale','swap','method_swap'):
                current = row_move(current,spec)
                left = tuple(r[:2] for r in current)
                transform = tuple(r[2:] for r in current)
                need(mul(transform,a) == left, 'all four augmented columns: E*A=L')
                want = current
                if kind == 'method_swap':
                    need(before[0][0] == 0 and before[1][0] != 0, 'existing-row pivot goal')
                    want = 'swap'
                    erased = ((F(0),)*4,before[1])
                    combined = frozen(shared.operate(before,'add_row_2_to_1',1))
                    need(erased[0][0] == 0 and combined[0][0] != 0
                         and combined[0] not in before, 'method distractor analysis')
                    need(frozen(shared.operate(combined,'add_row_2_to_1',-1)) == before,
                         'goal-missing alternative is reversible')
                    actual,shape = decoded(after,'swap')
                    need(actual == current and shape, 'method reached full swap')
                else:
                    actual,shape = decoded(after,kind)
                    need(actual == current and shape, 'false whole-row reached matrix')
            else:
                if kind == 'det':
                    want = facts['determinant']
                elif kind == 'adj':
                    need('det' in seen and facts['determinant'] != 0, 'adjugate stage needs nonzero determinant')
                    want = facts['adjugate']
                elif kind in ('formula','inverse'):
                    need(facts['determinant'] != 0, 'zero-determinant misuse')
                    if kind == 'formula':
                        need('det' in seen and 'adj' in seen, 'formula construction must be complete')
                    else:
                        need(tuple(r[:2] for r in current) == I, 'cannot extract inverse before left block is I')
                        need(tuple(r[2:] for r in current) == facts['inverse'], 'row route/formula independence')
                    want = facts['inverse']
                elif kind in ('AB','BA'):
                    need('formula' in seen or 'inverse' in seen, 'product check requires constructed B')
                    want = mul(a,facts['inverse']) if kind == 'AB' else mul(facts['inverse'],a)
                    need(want == I, 'original inverse product')
                elif kind == 'method_formula':
                    need(index == 0 and facts['determinant'] != 0, 'formula method original condition')
                    want = ('adj','nonzero')
                    wrong_b = scaled(a,1/facts['determinant'])
                    need(mul(a,wrong_b) != I, 'original-over-determinant distractor is not an inverse')
                elif kind == 'solve':
                    need('AB' in seen and 'BA' in seen, 'application requires both verified products')
                    want = facts['solution']
                elif kind == 'check':
                    need('solve' in seen, 'original check follows solve')
                    want = mul(a,facts['solution'])
                    need(want == facts['rhs'], 'original vector substitution')
                elif kind == 'witness':
                    need('det' in seen and facts['determinant'] == 0, 'singular determinant evidence')
                    want = facts['witness']
                    need(want[1][0] == 1 and want != ZERO and mul(a,want) == ZERO, 'normalized nonzero null vector')
                elif kind == 'null':
                    need('witness' in seen, 'null product needs witness')
                    want = mul(a,facts['witness'])
                else:
                    need(kind == 'contradiction' and 'null' in seen and facts['determinant'] == 0,
                         'singular original contradiction order')
                    want = tuple(sorted((facts['witness'],ZERO)))
                actual_text = after
                if kind == 'contradiction':
                    suffix = r',\quad\nexists A^{-1}'
                    need(after.endswith(suffix), 'singular conclusion missing')
                    actual_text = after[:-len(suffix)]
                actual,shape = decoded(actual_text,kind)
                need(actual == want and shape, f'{case["id"]}/{step["id"]}: false or goal-missing reached state')
            options = step['options']
            need(len(options) == 3, 'three symbolic choices')
            values = [decoded(o['label'],kind) for o in options]
            need(len({repr(v) for v,shape in values}) == 3, 'semantically equivalent duplicate options')
            correct = [o['id'] for o,(v,shape) in zip(options,values) if v == want and shape]
            need(len(correct) == 1 and step['accepted_option_ids'] == correct,
                 f'{case["id"]}/{step["id"]}: false key or missing requested form/operation')
            need(all(step.get(k) for k in ('prompt','wrong_hint','explanation')), 'missing teaching')
            need(all(bool(o.get('wrong_feedback')) == (o['id'] not in correct) for o in options),
                 'feedback must stay on actual wrong option')
            for o in options:
                need(o['id'] in ((index+1)*10+1,(index+1)*10+2,(index+1)*10+3), 'stable choice identity')
            need(len({o['id'] for o in options}) == 3, 'unique choice IDs')
            position = next(i for i,o in enumerate(options) if o['id'] == correct[0])
            need(position == (case['first_position']+index)%3, 'deterministic later choice positions')
            if index == 0:
                need(position == case['first_position'], 'first-position contract')
                positions.append(position)
            decisions.append(dict(step_id=step['id'],goal=spec,expected=want,
                                  after=after,correct_ids=correct,
                                  options=[dict(id=o['id'],label=o['label'],value=v,requested_shape=shape,
                                                goal_correct=o['id'] in correct)
                                           for o,(v,shape) in zip(options,values)]))
            previous = after
            seen.append(kind)
        if case['kind'] == 'singular':
            need(seen == ['det','witness','null','contradiction'], 'complete singular proof')
        else:
            need('AB' in seen and 'BA' in seen, 'two-sided check required')
            need(seen[-1] == ('check' if case['kind'] == 'application' else 'BA'), 'original check must finish route')
        records.append(dict(id=case['id'],facts=facts,steps=decisions))
    steps = sum(len(r['steps']) for r in records)
    need(steps == 64 and sorted(positions) == [0]*4+[1]*4+[2]*4, 'finite depth/balance')
    need(routes['routes'] == 12 and routes['wrong_choices'] == 2*steps, 'actual model replay counts')
    return dict(accepted=True,questions=12,readings=1,steps=steps,options=3*steps,wrong_choices=2*steps,
                first_positions=[p+1 for p in positions],checks=records)


def refused(name, call, outcomes):
    try:
        call()
    except (export.ExportError, ValueError, ZeroDivisionError) as error:
        outcomes.append(dict(name=name,rejected=True,reason=str(error)))
        return
    raise AssertionError('unrejected negative probe: '+name)


def regressions(data,routes):
    outcomes = []
    def changed(qi,si,*,label=None,after=None,key=None):
        value = copy.deepcopy(routes)
        q = value['questions'][qi]['question']
        step = q['steps'][si]
        if label is not None:
            next(o for o in step['options'] if o['id'] in step['accepted_option_ids'])['label'] = label
        if after is not None:
            next(s for s in q['working_states'] if s['id'] == step['semantics']['after'])['display'] = after
        if key is not None:
            step['accepted_option_ids'] = [key]
        return value
    refused('false_accepted_key',lambda:verify(data,changed(0,0,key=12)),outcomes)
    partial = r'\left[\begin{array}{cc|cc}1&0&1&0\\0&-3&0&1\end{array}\right]'
    refused('partial_augmented_row_accepted_and_reached',lambda:verify(data,changed(0,0,label=partial,after=partial)),outcomes)
    second_column = r'\left[\begin{array}{cc|cc}1&0&\frac{1}{2}&0\\0&1&0&1\end{array}\right]'
    refused('second_augmented_column_left_unchanged',lambda:verify(data,changed(0,1,after=second_column)),outcomes)
    wrong_adj = r'J=\begin{bmatrix}1&-2\\-3&1\end{bmatrix}'
    refused('adjugate_order_error',lambda:verify(data,changed(5,1,label=wrong_adj,after=wrong_adj)),outcomes)
    wrong_sign = r'J=\begin{bmatrix}2&-2\\1&3\end{bmatrix}'
    refused('adjugate_sign_error',lambda:verify(data,changed(7,1,label=wrong_sign,after=wrong_sign)),outcomes)
    outside = r'B=\frac{1}{5}\begin{bmatrix}3&-1\\-1&2\end{bmatrix}'
    need(decoded(outside,'formula')[0] == original(data['cases'][4])[1]['inverse'],
         'goal-missing control must be mathematically true')
    refused('true_unevaluated_formula_misses_goal',lambda:verify(data,changed(4,2,label=outside,after=outside)),outcomes)
    refused('true_unevaluated_option_only_misses_goal',lambda:verify(data,changed(4,2,label=outside)),outcomes)
    refused('true_unevaluated_reached_only_misses_goal',lambda:verify(data,changed(4,2,after=outside)),outcomes)
    bad_swap = r'R_1\leftarrow R_1+R_2'
    bad_swap_after = r'\left[\begin{array}{cc|cc}2&2&1&1\\2&1&0&1\end{array}\right]'
    refused('reversible_addition_misses_existing_row_goal',lambda:verify(data,changed(9,0,label=bad_swap,after=bad_swap_after)),outcomes)
    duplicate = copy.deepcopy(routes)
    s = duplicate['questions'][0]['question']['steps'][2]
    next(o for o in s['options'] if o['id'] == 32)['label'] = r'B=\begin{bmatrix}\frac{2}{4}&0\\0&-\frac{2}{6}\end{bmatrix}'
    refused('equivalent_fraction_matrix_duplicate',lambda:verify(data,duplicate),outcomes)
    duplicate = copy.deepcopy(routes)
    s = duplicate['questions'][4]['question']['steps'][2]
    next(o for o in s['options'] if o['id'] == 32)['label'] = outside
    refused('outside_scalar_equivalent_duplicate',lambda:verify(data,duplicate),outcomes)
    invalid = copy.deepcopy(data)
    invalid['cases'][0]['A'][0][0] = 6
    refused('original_integer_out_of_bounds',lambda:verify(invalid,routes),outcomes)
    invalid['cases'][0]['A'][0][0] = True
    refused('boolean_original_not_integer',lambda:verify(invalid,routes),outcomes)
    invalid = copy.deepcopy(data)
    invalid['cases'][0]['route'][0][2] = '0'
    refused('zero_row_divisor',lambda:verify(invalid,routes),outcomes)
    refused('zero_determinant_inverse_misuse',lambda:inverse(frozen([[1,2],[2,4]])),outcomes)
    invalid = copy.deepcopy(data)
    invalid['cases'][10]['route'][1] = ['formula']
    refused('singular_route_attempts_formula',lambda:verify(invalid,routes),outcomes)
    invalid = copy.deepcopy(routes)
    invalid['questions'][8]['question']['equation'] = invalid['questions'][8]['question']['equation'].replace('1\\\\4','1\\\\5')
    refused('compiled_original_rhs_changed',lambda:verify(data,invalid),outcomes)
    invalid = copy.deepcopy(routes)
    invalid['questions'][0]['question']['description'] = invalid['questions'][0]['question']['description'].replace('real','integer')
    refused('compiled_domain_changed',lambda:verify(data,invalid),outcomes)
    zero_v = r'v=\begin{bmatrix}0\\0\end{bmatrix}'
    refused('true_zero_null_vector_misses_witness_goal',lambda:verify(data,changed(10,1,label=zero_v,after=zero_v)),outcomes)
    tautology = r'\begin{bmatrix}-2\\1\end{bmatrix}=\begin{bmatrix}-2\\1\end{bmatrix}'
    refused('true_tautology_misses_conditional_consequence',lambda:verify(data,changed(10,3,label=tautology,after=tautology+r',\quad\nexists A^{-1}')),outcomes)
    wrong_product = r'BA=\begin{bmatrix}\frac{9}{5}&-\frac{3}{5}\\-\frac{2}{5}&\frac{4}{5}\end{bmatrix}'
    refused('false_reverse_product',lambda:verify(data,changed(4,4,label=wrong_product,after=wrong_product)),outcomes)
    controls = []
    for name,qi,si,label in [
        ('equivalent_fraction_entries',0,2,r'B=\begin{bmatrix}\frac{2}{4}&0\\0&-\frac{2}{6}\end{bmatrix}'),
        ('alternate_array_wrapper',4,2,r'B=\left[\begin{array}{cc}\frac{6}{10}&-\frac{2}{10}\\-\frac{2}{10}&\frac{4}{10}\end{array}\right]'),
        ('equivalent_augmented_fraction',0,0,r'\left[\begin{array}{cc|cc}1&0&\frac{2}{4}&0\\0&-3&0&1\end{array}\right]'),
        ('equivalent_determinant_fraction',4,0,r'\det(A)=\frac{10}{2}'),
        ('reversed_swap_notation',9,0,r'R_2\leftrightarrow R_1'),
    ]:
        adjusted = changed(qi,si,label=label,after=None if si == 0 and qi == 9 else label)
        verify(data,adjusted)
        controls.append(dict(name=name,accepted=True))
    # Separate lesson example: exact independent inverse and every displayed row.
    h = frozen([[1,-1],[0,2]])
    need(all(h != frozen(c['A']) for c in data['cases']), 'lesson original must be distinct')
    h0 = augmented(h)
    h1 = row_move(h0,['scale',1,'2'])
    h2 = row_move(h1,['add',0,1,'1'])
    hb = inverse(h)
    need(tuple(r[:2] for r in h2) == I and tuple(r[2:] for r in h2) == hb, 'lesson reduction')
    lesson_source = (ROOT/'authoring/documents/chapter.paths.md').read_text().split('@question ',1)[0]
    need('H='+display_matrix(h) in lesson_source and r'H^{-1}='+display_matrix(hb) in lesson_source
         and all(display_matrix(state,True) in lesson_source for state in (h0,h1,h2)),
         'computed worked-example matrices must occur in the actual lesson source')
    return dict(negative_probes=outcomes,valid_alternative_controls=controls,
                separate_worked_example=dict(A=h,augmented_states=[h0,h1,h2],inverse=hb,
                                            AB=mul(h,hb),BA=mul(hb,h),actual_source_matrices_bound=True))


def markdown_edit(data,routes):
    source = ROOT/'authoring/documents/chapter.paths.md'
    before = source.read_bytes()
    q = routes['questions'][0]['question']
    step = q['steps'][0]
    wrong = next(o for o in step['options'] if o.get('wrong_feedback'))
    new_prompt = step['prompt']+' Keep the second row unchanged.'
    new_feedback = wrong['wrong_feedback']+' The same divisor applies on both sides of the bar.'
    text = before.decode()
    need(text.count('@step 10 | '+step['prompt']) == 1
         and text.count('@feedback '+str(wrong['id'])+' | '+wrong['wrong_feedback']) == 1, 'unique real Markdown edit target')
    text = text.replace('@step 10 | '+step['prompt'],'@step 10 | '+new_prompt,1)
    text = text.replace('@feedback '+str(wrong['id'])+' | '+wrong['wrong_feedback'],
                        '@feedback '+str(wrong['id'])+' | '+new_feedback,1)
    # Keep prior edits immutable when the live source changes. A deterministic
    # content-keyed scratch folder permits exact reruns without overwriting it.
    scratch_root = EVIDENCE/'markdown-edit'/export.sha(text.encode())
    scratch = scratch_root/'documents'
    edited_source = scratch/'chapter.paths.md'
    if edited_source.exists():
        need(edited_source.read_bytes() == text.encode(), 'existing scratch edit differs; preserve and inspect it')
    else:
        export.write_tree(scratch,{'chapter.paths.md':text.encode()})
    run = subprocess.run([str(MODEL),'--question-batch',str(scratch)],capture_output=True,text=True,timeout=60)
    need(run.returncode == 0, 'real Markdown edit compile failed: '+run.stderr+run.stdout)
    edited = export.decoded(run.stdout)
    verify(data,edited)
    expected = copy.deepcopy(routes)
    es = expected['questions'][0]['question']['steps'][0]
    es['prompt'] = new_prompt
    next(o for o in es['options'] if o['id'] == wrong['id'])['wrong_feedback'] = new_feedback
    need(edited == expected, 'Markdown edit changed fields beyond prompt and wrong feedback')
    edited_routes = scratch_root/'routes.json'
    if edited_routes.exists():
        need(edited_routes.read_bytes() == export.encoded(edited), 'existing scratch routes differ')
    else:
        export.write_tree(scratch_root,{'routes.json':export.encoded(edited)})
    need(source.read_bytes() == before, 'live source changed during scratch edit')
    return dict(accepted=True,model_exit_code=run.returncode,source_unchanged=True,
                question_id=q['id'],step_id=step['id'],wrong_option_id=wrong['id'],
                prompt_before=step['prompt'],prompt_after=new_prompt,
                feedback_before=wrong['wrong_feedback'],feedback_after=new_feedback,
                exact_two_field_delta=True,
                documents=str(scratch),routes=str(edited_routes),
                source_sha256=export.sha(before),edited_source_sha256=export.sha(text.encode()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--routes',required=True,type=Path)
    args = parser.parse_args()
    ready = export.decoded((WORKSPACE/'build/production/wave01/build-ready.json').read_bytes())
    hashes = {name:export.sha((WORKSPACE/'b'/name).read_bytes()) for name in ready['sha256']}
    need(hashes == ready['sha256'], 'Release binary hash mismatch; do not rebuild')
    data = export.decoded((ROOT/'cases.json').read_bytes())
    routes = export.decoded(args.routes.read_bytes())
    result = verify(data,routes)
    result.update(regressions=regressions(data,routes),markdown_edit=markdown_edit(data,routes),
                  executable_sha256=hashes,routes_sha256=export.sha(args.routes.read_bytes()),
                  finite_scope='12 fixed real 2x2 integer originals; exact numeric matrix fields and listed method forms only; no prose or general CAS verifier',
                  windows=0,published=False)
    print(json.dumps(result,indent=2,default=str))


if __name__ == '__main__':
    main()
