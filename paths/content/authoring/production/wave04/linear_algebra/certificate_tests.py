#!/usr/bin/env python3
"""Finite R2/R3 span/basis certificates; original minors, not authored keys."""
import argparse
import copy
import importlib.util
import itertools
import json
from fractions import Fraction as F
from pathlib import Path
import re
import subprocess

import build_question_batch as shared
import export_learning as export
from question_workflow import exact

ROOT = Path(__file__).resolve().parent
WORKSPACE = shared.ROOT
OUT = WORKSPACE / 'build/production/wave04/linear_algebra'
MODEL = WORKSPACE / 'b/paths_learning_document_tests'
PACKAGE = 'prod04_linear_span_basis'
# Reuse the already bounded rational-TeX atom reader, not its 2x2 matrix
# recognizer or inverse solver. The published dependency remains read-only.
ATOM = WORKSPACE / 'content/authoring/production/wave03/linear_algebra/certificate_tests.py'
spec = importlib.util.spec_from_file_location('published_numeric_atom', ATOM)
atoms = importlib.util.module_from_spec(spec)
spec.loader.exec_module(atoms)
number = atoms.number


def need(ok, message):
    export.require(ok, 'wave04.span_basis', message)


def frozen(rows):
    return tuple(tuple(F(v) for v in row) for row in rows)


def transpose(a):
    return tuple(zip(*a))


def column(v):
    return tuple((F(x),) for x in v)


def mul(a, b):
    need(len(a[0]) == len(b), 'product dimensions')
    return tuple(tuple(sum(x*y for x,y in zip(row,col)) for col in transpose(b)) for row in a)


def det(a):
    """Leibniz minors of order <=3, independent of any authored elimination."""
    n = len(a)
    need(1 <= n <= 3 and all(len(row) == n for row in a), 'minor dimensions')
    result = F(0)
    for p in itertools.permutations(range(n)):
        term = F((-1)**sum(p[i] > p[j] for i in range(n) for j in range(i+1,n)))
        for i in range(n):
            term *= a[i][p[i]]
        result += term
    return result


def rank(a):
    for size in range(min(len(a),len(a[0])),0,-1):
        for rows in itertools.combinations(range(len(a)),size):
            for cols in itertools.combinations(range(len(a[0])),size):
                if det(tuple(tuple(a[i][j] for j in cols) for i in rows)):
                    return size
    return 0


def solve(a, b):
    """Unique coefficients from an original full-column-rank minor; all rows checked."""
    n = len(a[0])
    need(rank(a) == n, 'unique coefficient system required')
    for indices in itertools.combinations(range(len(a)),n):
        square = tuple(a[i] for i in indices)
        divisor = det(square)
        if not divisor:
            continue
        values = []
        for j in range(n):
            replaced = tuple(tuple(b[i][0] if k == j else a[i][k] for k in range(n))
                             for i in indices)
            values.append(det(replaced)/divisor)
        c = column(values)
        need(mul(a,c) == b, 'original system inconsistent')
        return c
    raise AssertionError('full rank but no nonzero minor')


def pivots(a):
    selected = []
    for j in range(len(a[0])):
        test = tuple(tuple(row[k] for k in selected+[j]) for row in a)
        if rank(test) > len(selected):
            selected.append(j)
    return selected


def basis_valid(b, a):
    if len(b) != len(a) or rank(b) != len(b[0]) or rank(b) != rank(a):
        return False
    return rank(tuple(x+y for x,y in zip(b,a))) == rank(a)


def relation_valid(a, c):
    return len(c) == len(a[0]) and any(row[0] for row in c) and mul(a,c) == column([0]*len(a))


def augment(a, b):
    return tuple(row+(b[i][0],) for i,row in enumerate(a))


def row_op(rows, op):
    name = op[0]
    result = [list(row) for row in rows]
    if name == 'swap':
        i,j = op[1:]
        result[i],result[j] = result[j],result[i]
    elif name == 'add':
        i,j,k = op[1:]
        need(i != j, 'distinct source/destination required')
        result[i] = shared.operate([rows[j],rows[i]],'add_row_1_to_2',k)[1]
    elif name == 'scale':
        i,k = op[1:]
        result[i] = shared.operate([rows[(i+1)%len(rows)],rows[i]],'divide_row_2',k)[1]
    else:
        raise AssertionError('unsupported finite operation')
    result = frozen(result)
    need(all(abs(v) <= 1000 for row in result for v in row), 'working magnitude bound')
    return result


def inverse_op(op):
    if op[0] == 'swap':
        return op
    if op[0] == 'add':
        return ['add',op[1],op[2],str(-exact(op[3]))]
    return ['scale',op[1],str(1/exact(op[2]))]


def matrix_value(text, prefix='', bar=False):
    """Only this family's bounded numeric rectangles, using the shared atom reader."""
    compact = text.replace(' ','').strip()
    need(compact.startswith(prefix), 'field prefix')
    body = compact[len(prefix):]
    if bar:
        match = re.fullmatch(r'\\left\[\\begin\{array\}\{(c{2,4}\|c)\}(.*?)\\end\{array\}\\right\]',body)
        need(match is not None, 'complete augmented matrix wrapper required')
        columns = len(match[1])-1
        body = match[2]
    else:
        match = re.fullmatch(r'\\begin\{bmatrix\}(.*?)\\end\{bmatrix\}',body)
        columns = None
        if match is None:
            # Equivalent unaugmented array is allowed; no scalar outside.
            match = re.fullmatch(r'\\left\[\\begin\{array\}\{(c{1,4})\}(.*?)\\end\{array\}\\right\]',body)
            need(match is not None, 'numeric matrix/vector required')
            columns,body = len(match[1]),match[2]
        else:
            body = match[1]
        need(match is not None, 'numeric matrix/vector required')
    rows = tuple(tuple(number(v) for v in row.split('&')) for row in body.split(r'\\'))
    need(1 <= len(rows) <= 3 and 1 <= len(rows[0]) <= 5
         and all(len(row) == len(rows[0]) for row in rows), 'finite rectangle dimensions')
    need(columns is None or columns == len(rows[0]), 'augmented column specification')
    return rows


def display(a):
    return r'\begin{bmatrix}'+r'\\'.join('&'.join(shared.tex(v) for v in row) for row in a)+r'\end{bmatrix}'


def original(case):
    kind = case['kind']
    if kind == 'plane_basis':
        normal = case['normal']
        need(len(normal) == 3 and all(type(v) is int and abs(v) <= 3 for v in normal)
             and normal[1] != 0, 'plane normal bounds')
        a = frozen([[1,0],[-F(normal[0],normal[1]),-F(normal[2],normal[1])],[0,1]])
        need(mul((tuple(normal),),a) == ((0,0),), 'symbolic plane parametrization')
    else:
        cols = case['columns']
        need(2 <= len(cols) <= 4 and len(cols[0]) in (2,3)
             and all(len(col) == len(cols[0]) for col in cols), 'original dimensions')
        need(all(type(v) is int and abs(v) <= 7 for col in cols for v in col), 'original integer bounds')
        need(len(set(map(tuple,cols))) == len(cols), 'finite original columns distinct')
        a = frozen(transpose(cols))
    m,n = len(a),len(a[0])
    target = case.get('target',[0]*m)
    need(len(target) == m and all(type(v) is int and abs(v) <= 7 for v in target), 'original target bounds')
    w = column(target)
    indices = pivots(a)
    b = tuple(tuple(row[j] for j in indices) for row in a)
    facts = {'A':a,'w':w,'rank':rank(a),'pivot_indices':[i+1 for i in indices],'B':b}
    if kind == 'outside':
        j = case['ell_anchor']
        others = [i for i in range(m) if i != j]
        equations = transpose(a)
        unknown = solve(tuple(tuple(row[i] for i in others) for row in equations),
                        column([-row[j] for row in equations]))
        ell = [F(1) if i == j else unknown[others.index(i)][0] for i in range(m)]
        ell = column(ell)
        obstruction = mul(transpose(ell),augment(a,w))
        need(obstruction[0][:-1] == (0,)*n and obstruction[0][-1] != 0, 'non-membership proof')
        need(rank(augment(a,w)) > rank(a), 'inconsistent original augmented rank')
        facts.update(ell=ell,obstruction=obstruction)
    elif kind == 'dependent':
        anchor = case['witness_anchor']
        others = [j for j in range(n) if j != anchor]
        c0 = solve(tuple(tuple(row[j] for j in others) for row in a),column([-row[anchor] for row in a]))
        c = column([1 if j == anchor else c0[others.index(j)][0] for j in range(n)])
        need(relation_valid(a,c) and rank(a) < n, 'nontrivial original relation')
        facts['c'] = c
    else:
        c = solve(b,w)
        need(basis_valid(b,a), 'basis independence and spanning of original columns')
        generators = [solve(b,column(col)) for col in transpose(a)]
        facts.update(c=c,generator_coordinates=generators)
        if kind == 'independent':
            need(rank(a) == n < m and c == column([0]*n), 'independent proper span')
        if kind == 'basis':
            need(rank(a) == n == m, 'ambient basis proof')
        if kind == 'plane_basis':
            need(rank(a) == 2 and len(a) == 3, 'proper plane basis')
        if kind == 'two_bases':
            need(len(case['second_columns']) == 2
                 and all(len(col) == 2 for col in case['second_columns']), 'second original dimensions')
            second = frozen(transpose(case['second_columns']))
            need(len(second) == m == 2 and rank(second) == 2, 'second ambient basis')
            need(all(type(v) is int and abs(v) <= 7 for col in case['second_columns'] for v in col),
                 'second original integer bounds')
            facts.update(second=second,c_second=solve(second,w))
    for key in ('c','ell','c_second'):
        if key in facts:
            need(all(abs(v.numerator) <= 12 and v.denominator <= 12 for row in facts[key] for v in row),
                 'witness rational bounds')
    return facts


def given(case, f):
    if case['kind'] == 'plane_basis':
        need(case['normal'] == [1,1,0], 'finite plane source identity')
        return r'W=\{(x,y,z)^T:x+y=0\},\quad w='+display(f['w'])
    if case['kind'] == 'two_bases':
        return 'A_1='+display(f['A'])+r',\quad A_2='+display(f['second'])+r',\quad w='+display(f['w'])
    return 'A='+display(f['A'])+(r',\quad w='+display(f['w']) if 'target' in case else '')


def normalized(v):
    flat = tuple(x for row in v for x in row)
    first = next((v for v in flat if v),None)
    return tuple(v/first for v in flat) if first else flat


def signature(value, op):
    if op == 'relation':
        return normalized(value)
    if op == 'basis_columns':
        return tuple(sorted(normalized(column(col)) for col in transpose(value)))
    return value


def route_certificate(case):
    f = original(case)
    kind = case['kind']
    rows = augment(f['A'],column([0]*len(f['A'])) if kind == 'plane_basis' else f['w'])
    specs = []
    for operation in case['route']:
        name = operation[0]
        prefix, suffix = '', ''
        parser = 'matrix'
        if name in ('add','scale','swap','method_swap','second_add'):
            if name == 'second_add':
                rows = augment(f['second'],f['w'])
            op = ['swap' if name == 'method_swap' else 'add' if name == 'second_add' else name,*operation[1:]]
            before = rows
            rows = row_op(rows,op)
            need(row_op(rows,inverse_op(op)) == before, 'every entry of inverse row operation')
            need(rank(rows) == rank(before), 'reversible augmented rank invariant')
            value = rows
            parser = 'swap_method' if name == 'method_swap' else 'augmented'
        elif name == 'method_system':
            parser,value = 'system_method',rows
        elif name == 'plane_rule':
            parser,value = 'plane_rule',(-F(case['normal'][0],case['normal'][1]),-F(case['normal'][2],case['normal'][1]))
        elif name == 'rank_pair':
            parser,value = 'rank_pair',(f['rank'],len(f['A']))
        elif name == 'basis_columns':
            prefix,value = 'B=',f['B']
        elif name == 'annihilator':
            prefix,value = r'\ell=',f['ell']
        elif name == 'obstruction':
            prefix,value = r'\ell^T[A\midw]=',f['obstruction']
            suffix = r',\quad w\notin\operatorname{span}(U)'
        elif name in ('coeff','relation','coeff_first','coeff_second'):
            prefix = {'coeff_first':r'c^{(1)}=','coeff_second':r'c^{(2)}='}.get(name,'c=')
            value = f['c_second'] if name == 'coeff_second' else f['c']
        elif name in ('reconstruct','null_check'):
            prefix = 'Bc=' if kind in ('column_basis','plane_basis') else 'Ac='
            value = mul(f['B'] if kind == 'column_basis' else f['A'],f['c'])
            if name == 'null_check':
                suffix = r',\quad c\ne0'
            elif kind == 'member':
                suffix = r',\quad w\in\operatorname{span}(U)'
        elif name == 'both_check':
            prefix = r'A_1c^{(1)}=A_2c^{(2)}='
            value = mul(f['A'],f['c'])
            need(value == mul(f['second'],f['c_second']) == f['w'], 'two original reconstructions')
        else:
            raise AssertionError(name)
        specs.append(dict(name=name,parser=parser,prefix=prefix,value=value,suffix=suffix))
    return f,specs


def value_of(text, spec, after=False):
    parser = spec['parser']
    if after and spec['suffix']:
        need(text.endswith(spec['suffix']), 'missing or wrong mathematical conclusion')
        text = text[:-len(spec['suffix'])]
    if parser in ('augmented','system_method','swap_method') and (after or parser == 'augmented'):
        return matrix_value(text,bar=True)
    if parser == 'system_method':
        forms = {'Ac=w':'original','A^Tc=w':'transposed','Ac=0':'homogeneous'}
        need(text in forms, 'finite coefficient method')
        return forms[text]
    if parser == 'swap_method':
        forms = {r'R_1\leftrightarrow R_2':'swap',r'R_2\leftrightarrow R_1':'swap',
                 r'R_1\leftarrow\frac{1}{2}R_1':'scale',r'R_1\leftarrow R_1+R_2':'add'}
        need(text in forms, 'finite pivot method')
        return forms[text]
    if parser == 'plane_rule':
        forms = {'y=-x':(-F(1),F(0)),'y=x':(F(1),F(0)),'y=0':(F(0),F(0))}
        need(text in forms, 'finite plane parameter rule')
        return forms[text]
    if parser == 'rank_pair':
        match = re.fullmatch(r'\(r,m\)=\(([0-3]),([23])\)',text)
        need(match is not None, 'finite rank/ambient pair')
        return tuple(map(int,match.groups()))
    return matrix_value(text,spec['prefix'])


def check_record(case, record, data):
    f,specs = route_certificate(case)
    q = record['question']
    need(record['id'] == q['id'] == case['id'], 'question identity')
    need(q['equation'] == given(case,f), 'compiled original givens')
    need(q['description'] == data['goals'][case['kind']]+' '+data['domain'], 'original domain and goal')
    need(len(q['steps']) == len(specs) and 3 <= len(specs) <= 8, 'complete bounded route')
    states = {s['id']:s['display'] for s in q['working_states']}
    need(len(states) == len(specs)+1 and states[1] == q['equation'], 'initial working state')
    evidence = []
    for i,(step,s) in enumerate(zip(q['steps'],specs)):
        need(step['id'] == (i+1)*10 and step['semantics']['before'] == i+1
             and step['semantics']['after'] == i+2, 'sequential reached state')
        need(step['prompt'] and step['explanation'] and step['wrong_hint'], 'teaching completeness')
        if s['name'] == 'relation':
            need('normalized' in step['prompt'] and 'c3=1' in step['prompt'], 'declared witness goal')
        if s['name'] == 'basis_columns':
            need(('ORIGINAL' in step['prompt'] and 'increasing' in step['prompt'])
                 or ('s' in step['prompt'] and 't' in step['prompt'] and 'first' in step['prompt']),
                 'ordered original/parameter basis goal')
        options = step['options']
        need(len(options) == 3, 'three symbolic choices')
        values = [value_of(o['label'],s) for o in options]
        keys = [signature(v,s['name']) for v in values]
        need(len(set(keys)) == 3, 'semantic duplicate options')
        expected = {'system_method':'original','swap_method':'swap'}.get(s['parser'],s['value'])
        correct = [o['id'] for o,v in zip(options,values) if v == expected]
        need(len(correct) == 1 and step['accepted_option_ids'] == correct, 'unique mathematical goal key')
        need([o['id'] for o in options].index(correct[0]) == (case['first_position']+i)%3,
             'balanced deterministic position')
        need(all(o.get('wrong_feedback') for o in options if o['id'] != correct[0]), 'specific feedback present')
        after = value_of(states[i+2],s,after=True)
        need(after == s['value'], 'false reached working')
        truths = []
        for v in values:
            truths.append(basis_valid(v,f['A']) if s['name'] == 'basis_columns'
                          else relation_valid(f['A'],v) if s['name'] == 'relation'
                          else v == expected)
        evidence.append(dict(step=step['id'],operation=s['name'],derived=s['value'],correct=correct,
                             option_values=values,mathematically_valid=truths))
    return dict(id=case['id'],facts=f,decisions=evidence)


def check_all(data, routes):
    need(routes.get('accepted') is True and len(routes['questions']) == 12, 'all compiled routes')
    need([q['id'] for q in routes['questions']] == [c['id'] for c in data['cases']], 'ordered coverage')
    need([c['first_position'] for c in data['cases']].count(0) ==
         [c['first_position'] for c in data['cases']].count(1) ==
         [c['first_position'] for c in data['cases']].count(2) == 4, 'first-position balance')
    return [check_record(c,q,data) for c,q in zip(data['cases'],routes['questions'])]


def refuses(call):
    try:
        call()
    except (export.ExportError,ValueError,AssertionError):
        return
    raise AssertionError('negative control was accepted')


def controls(data, routes):
    names = []
    def replace_answer(q, step_index, label):
        step = q['steps'][step_index]
        next(o for o in step['options'] if o['id'] in step['accepted_option_ids'])['label'] = label
    def altered(name, index, edit):
        changed = copy.deepcopy(routes['questions'][index])
        edit(changed['question'])
        refuses(lambda:check_record(data['cases'][index],changed,data))
        names.append(name)
    altered('false_key',0,lambda q:q['steps'][0].update(accepted_option_ids=[12]))
    altered('partial_augmented_row',0,lambda q:q['working_states'][1].update(
        display=q['steps'][0]['options'][1]['label']))
    altered('wrong_original',0,lambda q:q.update(equation=q['equation'].replace('3','4',1)))
    altered('changed_domain',0,lambda q:q.update(description=q['description'].replace('real','complex')))
    altered('swapped_coordinates',8,lambda q:q['steps'][5].update(accepted_option_ids=[62]))
    altered('false_rank',4,lambda q:q['steps'][-1].update(accepted_option_ids=[52]))
    altered('zero_relation',5,lambda q:replace_answer(q,-2,'c='+display(column([0,0,0]))))
    f,specs = route_certificate(data['cases'][5])
    scaled = tuple(tuple(2*v for v in row) for row in f['c'])
    need(relation_valid(f['A'],scaled) and scaled != f['c'], 'valid nonzero witness scaling')
    step_index = len(specs)-2
    correct_index = next(i for i,o in enumerate(routes['questions'][5]['question']['steps'][step_index]['options'])
                         if o['id'] in routes['questions'][5]['question']['steps'][step_index]['accepted_option_ids'])
    altered('valid_relation_missing_normalization',5,lambda q:q['steps'][step_index]['options'][correct_index].update(
        label='c='+display(scaled)))
    altered('proportional_relation_duplicate',5,lambda q:q['steps'][step_index]['options'][(correct_index+1)%3].update(
        label='c='+display(scaled)))
    bf,bs = route_certificate(data['cases'][9])
    reversed_b = tuple(tuple(reversed(row)) for row in bf['B'])
    need(basis_valid(reversed_b,bf['A']), 'reordered basis remains mathematically valid')
    altered('reordered_basis_duplicate',9,lambda q:q['steps'][5]['options'][0].update(label='B='+display(reversed_b)))
    scaled_b = tuple((2*row[0],row[1]) for row in bf['B'])
    altered('rescaled_basis_duplicate',9,lambda q:q['steps'][5]['options'][0].update(label='B='+display(scaled_b)))
    altered('valid_nonpivot_basis_wrong_goal',9,lambda q:q['steps'][5].update(accepted_option_ids=[62]))
    altered('reduced_columns_not_original_basis',9,lambda q:q['steps'][5].update(accepted_option_ids=[63]))
    altered('independent_not_ambient_spanning',4,lambda q:replace_answer(q,-1,'(r,m)=(3,3)'))
    malformed = copy.deepcopy(data['cases'][0])
    malformed['columns'][0][0] = 8
    refuses(lambda:original(malformed))
    names.append('original_bound')
    # Exact atom and matrix-wrapper alternatives preserve meaning without
    # making scalar multiples competing answers in the learner's option list.
    need(matrix_value(r'c=\left[\begin{array}{c}\frac{6}{4}\\1/2\end{array}\right]','c=') ==
         column([F(3,2),F(1,2)]), 'valid rational/wrapper alternative')
    variant = copy.deepcopy(routes['questions'][8])
    for option in variant['question']['steps'][5]['options']:
        option['label'] = option['label'].replace(r'\frac{3}{2}',r'\frac{6}{4}')
    check_record(data['cases'][8],variant,data)
    need(value_of(r'R_2\leftrightarrow R_1',bs[0]) == 'swap', 'equivalent swap notation')
    need(basis_valid(matrix_value(routes['questions'][9]['question']['steps'][5]['options'][0]['label'],'B='),
                     bf['A']), 'genuinely different nonpivot basis is mathematically valid')
    refuses(lambda:matrix_value(r'\left[\begin{array}{cc}1\\2\end{array}\right]'))
    names.append('mismatched_array_specification')
    altered('equivalent_fraction_duplicate',8,lambda q:q['steps'][5]['options'][0].update(
        label=r'c=\begin{bmatrix}\frac{6}{4}\\\frac{2}{4}\end{bmatrix}'))
    # Challenge the key at EVERY decision, not only selected fault categories.
    for case,record in zip(data['cases'],routes['questions']):
        for i,step in enumerate(record['question']['steps']):
            changed = copy.deepcopy(record)
            wrong = next(o for o in step['options'] if o['id'] not in step['accepted_option_ids'])
            changed['question']['steps'][i]['accepted_option_ids'] = [wrong['id']]
            refuses(lambda:check_record(case,changed,data))
    names.append('false_key_at_all_72_decisions')
    return dict(negative=names,valid=['equivalent_rational_compiled_option','equivalent_unaugmented_wrapper',
                                    'equivalent_swap_notation','nonzero_scaled_relation',
                                    'different_valid_basis','reordered_valid_basis'])


def scratch_edit(data, routes):
    source = (ROOT/'authoring/documents/chapter.paths.md').read_bytes()
    q = routes['questions'][0]['question']
    prompt = q['steps'][0]['prompt']
    feedback = next(o['wrong_feedback'] for o in q['steps'][0]['options'] if o.get('wrong_feedback'))
    changed = source.decode().replace(prompt,prompt+' [scratch prompt]',1).replace(
        feedback,feedback+' [scratch feedback]',1).encode()
    need(source != changed, 'real Markdown edit')
    folder = OUT/'markdown-edit'/export.sha(changed)
    export.immutable_directory(folder/'documents',{'chapter.paths.md':changed})
    run = subprocess.run([str(MODEL),'--question-batch',str(folder/'documents')],
                         capture_output=True,text=True,check=True,timeout=60)
    edited = export.decoded(run.stdout)
    need(check_all(data,edited) == check_all(data,routes), 'scratch mathematics unchanged')
    s = edited['questions'][0]['question']['steps'][0]
    need(s['prompt'] == prompt+' [scratch prompt]', 'compiled prompt edit')
    option = next(o for o in s['options'] if o.get('wrong_feedback') == feedback+' [scratch feedback]')
    s['prompt'] = prompt
    option['wrong_feedback'] = feedback
    need(edited == routes, 'exactly two compiled fields changed')
    need((ROOT/'authoring/documents/chapter.paths.md').read_bytes() == source, 'live source unchanged')
    receipt = dict(accepted=True,source_sha256=export.sha(source),scratch_sha256=export.sha(changed),
                   compiled_fields_changed=2,mathematics_unchanged=True)
    export.immutable_directory(folder/'checks',{'edit.json':export.encoded(receipt),
                                               'routes.json':run.stdout.encode()})
    return dict(path=str(folder/'checks/edit.json'),**receipt)


def worked_check():
    b = frozen([[1,0],[0,1],[1,0]])
    w = column([2,3,2])
    c = solve(b,w)
    need(rank(b) == 2 and mul(b,c) == w and mul(((1,0,-1),),b) == ((0,0),),
         'worked plane basis and original reconstruction')
    text = (ROOT/'authoring/documents/chapter.paths.md').read_text()
    worked = text.split('@block example | worked |',1)[1].split('@endblock',1)[0]
    need('B='+display(b) in worked and '[w]_B='+display(c) in worked and 'w='+display(w) in worked,
         'actual worked displays match independent originals')
    reduced = row_op(augment(b,w),['add',2,0,'-1'])
    body = r'\\'.join('&'.join(shared.tex(v) for v in row) for row in reduced)
    need(body in worked, 'actual worked reduction')
    need(text.count('@practice '+PACKAGE+'_q') == 12, 'lesson practice count')
    need('Creative Commons Attribution-ShareAlike 3.0 United States' in text
         and 'Jim Hefferon' in text and 'These adapted lesson and questions' in text, 'public attribution')
    return dict(rank=2,basis=b,coordinates=c,reconstruction=mul(b,c))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--routes',type=Path,required=True)
    args = parser.parse_args()
    data = export.decoded((ROOT/'cases.json').read_bytes())
    routes = export.decoded(args.routes.read_bytes())
    results = check_all(data,routes)
    seeds = [c['source']['locator'] for c in data['cases']]
    need(seeds == ['Two.I.2.23(a)','Two.I.2.22','Two.I.2.26(c)','Two.I.2.28(a)',
                   'Two.II.1.23(a)','Two.II.1.21(b)','Two.II.1.21(c)','Two.II.1.23(b)',
                   'Two.III.1.22(a)','Two.I.2.26(e)','Two.III.1.27(b)','Two.III.1.23'],
         'approved finite seed locators in assignment order')
    need(len(set(seeds)) == 12 and all(c['source']['source_id'] == 'hefferon_span_basis'
         and c['source']['original_givens'] and c['source']['changes'] for c in data['cases']),
         'twelve distinct traced source seeds')
    report = dict(accepted=True,questions=12,readings=1,
                  steps=sum(len(c['route']) for c in data['cases']),
                  wrong_choices=2*sum(len(c['route']) for c in data['cases']),
                  source_questions=12,distinct_seed_prompts=len(set(seeds)),
                  certificates=results,controls=controls(data,routes),
                  worked=worked_check(),markdown_edit=scratch_edit(data,routes),
                  limits='Finite real R2/R3 integer originals; exact minors and normalized witnesses. '
                         'Numeric field recognition is not a general TeX parser. Prose reviewed separately.',
                  published=False)
    print(json.dumps(report,sort_keys=True,indent=2,default=str))


if __name__ == '__main__':
    main()
