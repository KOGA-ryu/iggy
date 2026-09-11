#!/usr/bin/env python3
"""Certify the finite full-system pool against actual compiler/model output."""
import argparse
import copy
from fractions import Fraction as F
import json
from pathlib import Path
import subprocess
import tempfile

import build_question_batch as shared
import export_learning as export

ROOT = Path(__file__).resolve().parent
MODEL = shared.ROOT / 'b/paths_learning_document_tests'
tex = shared.tex
matrix = shared.matrix_tex


def require(condition, message):
    export.require(condition, 'depth.linear_algebra', message)


def rank(rows):
    if any(rows[0][i]*rows[1][j] != rows[0][j]*rows[1][i]
           for i in range(len(rows[0])) for j in range(i+1, len(rows[0]))):
        return 2
    return int(any(value for row in rows for value in row))


def originals(case):
    rows = case['rows']
    require(type(rows) is list and len(rows) == 2
            and all(type(row) is list and len(row) == 3 for row in rows),
            'two augmented rows required')
    require(all(type(value) is int and abs(value) <= 12 for row in rows for value in row),
            'original entries must be bounded integers')
    rows = [list(map(F, row)) for row in rows]
    coefficient_rank = rank([row[:2] for row in rows])
    augmented_rank = rank(rows)
    require(coefficient_rank >= 1, 'rank-zero coefficient matrix is excluded')
    (a,b,c),(d,e,f) = rows
    determinant = a*e-b*d
    facts = dict(determinant=str(determinant), coefficient_rank=coefficient_rank,
                 augmented_rank=augmented_rank)
    if determinant:
        # Cramer's rule on ORIGINAL equations, independently of elimination.
        pair = [(c*e-b*f)/determinant, (a*f-c*d)/determinant]
        require(all(u*pair[0]+v*pair[1] == w for u,v,w in rows), 'Cramer substitution')
        facts.update(kind='unique', solution=list(map(str,pair)))
    elif augmented_rank > coefficient_rank:
        pivot = next(j for j in range(2) if rows[0][j])
        ratio = rows[1][pivot]/rows[0][pivot]
        require(all(rows[1][j] == ratio*rows[0][j] for j in range(2))
                and f != ratio*c, 'original contradiction')
        facts.update(kind='empty', original_ratio=str(ratio),
                     incompatible_constants=[str(ratio*c),str(f)])
    else:
        pivot = next(j for j in range(2) if rows[0][j])
        free = 1-pivot
        affine = [[F(0),F(0)],[F(0),F(0)]]
        affine[free] = [F(0),F(1)]
        affine[pivot] = [c/rows[0][pivot], -rows[0][free]/rows[0][pivot]]
        # Identity in t: separately check constant and parameter coefficient.
        for u,v,w in rows:
            require(u*affine[0][0]+v*affine[1][0] == w
                    and u*affine[0][1]+v*affine[1][1] == 0, 'affine residual')
        require(affine[free] == [0,1], 'free coordinate must cover every real value')
        facts.update(kind='infinite', free_coordinate=free,
                     affine=[[str(v) for v in coordinate] for coordinate in affine])
    return rows, facts


def pair_tex(values):
    return r'\left(' + ','.join(map(tex,values)) + r'\right)'


def singleton(values):
    return r'S=\{' + pair_tex(values) + r'\}'


def ranks(a,b):
    return rf'\left(\operatorname{{rank}}(A),\operatorname{{rank}}([A|b])\right)=({a},{b})'


def affine_tex(coordinate):
    constant, slope = coordinate
    if not slope:
        return tex(constant)
    term = ('-' if slope == -1 else '' if slope == 1 else tex(slope)) + 't'
    if not constant:
        return term
    return tex(constant) + ('+' if slope > 0 else '') + term


def family_tex(affine):
    return r'S=\{\left(' + ','.join(map(affine_tex,affine)) + r'\right):t\in\mathbb{R}\}'


def evaluated(rows, affine):
    return [[a*affine[0][i]+b*affine[1][i] for i in range(2)] for a,b,_ in rows]


def values_tex(values):
    return r'\left(' + ','.join(map(affine_tex,values)) + r'\right)'


def wrong_family(affine, free):
    changed = copy.deepcopy(affine)
    pivot = 1-free
    index = 1 if changed[pivot][1] else 0
    changed[pivot][index] *= -1
    return changed


def mathematics(case):
    original, facts = originals(case)
    current = copy.deepcopy(original)
    expected = []
    for number, specification in enumerate(case['steps'],1):
        kind = specification[0]
        before = copy.deepcopy(current)
        if kind == 'add':
            _, target, source, operand, _ = specification
            require({target,source} == {0,1}, 'distinct row indices')
            k = F(operand)
            # Existing exact row owner handles either direction by swapping its view.
            ordered = [before[source],before[target]]
            changed = shared.operate(ordered,'add_row_1_to_2',k)[1]
            unchanged_constant = changed[:2]+[before[target][2]]
            reversed_sign = shared.operate(ordered,'add_row_1_to_2',-k)[1]
            candidates = [changed,unchanged_constant,reversed_sign]
            current[target] = changed
            require(shared.operate([current[source],current[target]],'add_row_1_to_2',-k)[1] == before[target],
                    'row replacement inverse')
            labels = [matrix([row]) for row in candidates]
            after = matrix(current)
        elif kind == 'scale':
            _, target, divisor, _ = specification
            divisor = F(divisor)
            require(divisor != 0, 'division by zero')
            changed = [value/divisor for value in before[target]]
            candidates = [changed,changed[:2]+[before[target][2]],[-v for v in changed]]
            current[target] = changed
            require([v*divisor for v in changed] == before[target], 'scaling inverse')
            labels = [matrix([row]) for row in candidates]
            after = matrix(current)
        elif kind in ('swap','method_swap'):
            current = [before[1],before[0]]
            require([current[1],current[0]] == before, 'swap inverse')
            if kind == 'swap':
                partial = [before[1][:2]+[before[0][2]], before[0][:2]+[before[1][2]]]
                candidates = [current,partial,before]
                labels = [matrix(rows) for rows in candidates]
            else:
                mode = specification[1]
                if mode == 'existing':
                    require(before[0][0] == 0 and before[1][0] != 0, 'existing pivot method')
                    erased = [[F(0)]*3,before[1]]
                    combined = shared.operate(before,'add_row_2_to_1',1)
                    candidates = [current,erased,combined]
                    satisfies_goal = [rows[0][0] != 0 and rows[0] in before
                                      and rows[1] in before for rows in candidates]
                    require(satisfies_goal == [True,False,False] and rank(erased) < rank(before),
                            'existing-row goal or irreversible zero scaling')
                    require(shared.operate(combined,'add_row_2_to_1',-1) == before,
                            'goal-missing addition must remain reversible')
                    labels = [r'R_1\leftrightarrow R_2',r'R_1\leftarrow 0R_1',
                              r'R_1\leftarrow R_1+R_2']
                else:
                    require(mode == 'unit' and before[1][0] == 1 and before[0][0] == 2
                            and any((v/2).denominator != 1 for v in before[0]),
                            'unit integral pivot method')
                    divided = [[v/2 for v in before[0]],before[1]]
                    replaced = shared.operate(before,'add_row_1_to_2',-1)
                    candidates = [current,divided,replaced]
                    satisfies_goal = [rows[0][0] == 1 and all(v.denominator == 1 for row in rows for v in row)
                                      for rows in candidates]
                    require(satisfies_goal == [True,False,False], 'unit-integral goal')
                    require([v*2 for v in divided[0]] == before[0]
                            and shared.operate(replaced,'add_row_1_to_2',1) == before,
                            'goal-missing alternatives must remain reversible')
                    labels = [r'R_1\leftrightarrow R_2',r'R_1\leftarrow \frac{1}{2}R_1',
                              r'R_2\leftarrow R_2-R_1']
                facts['method_choice_results'] = {
                    label:[[str(v) for v in row] for row in rows]
                    for label,rows in zip(labels,candidates)}
            after = matrix(current)
        elif kind == 'unique':
            require(facts['kind'] == 'unique', 'unique classification on singular case')
            x,y = map(F,facts['solution'])
            require(current == [[1,0,x],[0,1,y]], 'complete reduction required before reading pair')
            candidates = [[x,y],[-x,y],[x,-y]]
            labels = [singleton(pair) for pair in candidates]
            after = labels[0] + r',\quad ' + ranks(2,2)
        elif kind == 'check':
            require(facts['kind'] == 'unique', 'pair check requires unique case')
            x,y = map(F,facts['solution'])
            (a,b,c),(d,e,f) = original
            candidates = [[a*x+b*y,d*x+e*y],[a*x-b*y,f],[c,d*x-e*y]]
            require(candidates[0] == [c,f], 'original left sides')
            labels = [pair_tex(pair) for pair in candidates]
            equations = [rf'({tex(u)})({tex(x)})+({tex(v)})({tex(y)})={tex(w)}'
                         for u,v,w in original]
            after = r'\begin{gathered}' + r'\\'.join(equations+[singleton([x,y])]) + r'\end{gathered}'
        elif kind == 'empty':
            require(facts['kind'] == 'empty' and current[1][:2] == [0,0] and current[1][2] != 0,
                    'explicit contradictory augmented row required')
            labels = [ranks(1,2)+r',\ S=\varnothing',
                      ranks(1,1)+r',\ |S|=\infty',
                      ranks(2,2)+r',\ |S|=1']
            candidates = [(1,2,'empty'),(1,1,'infinite'),(2,2,'unique')]
            after = rf'0={tex(current[1][2])},\quad ' + labels[0]
        elif kind == 'incompatible':
            require(facts['kind'] == 'empty', 'original incompatibility required')
            ratio = F(facts['original_ratio'])
            c,f = original[0][2],original[1][2]
            candidates = [[ratio*c,f],[c,f],[ratio*c,ratio*c]]
            labels = [pair_tex(pair) for pair in candidates]
            first = [v*ratio for v in original[0]]
            after = r'\begin{gathered}' + shared.row_equation(first) + r'\\' + shared.row_equation(original[1])
            after += rf'\\0={tex(f-ratio*c)}\\S=\varnothing\end{{gathered}}'
        elif kind in ('parameter','parameter_check'):
            require(facts['kind'] == 'infinite' and current[1] == [0,0,0], 'one free parameter')
            affine = [list(map(F,coordinate)) for coordinate in facts['affine']]
            free = facts['free_coordinate']
            pivot = 1-free
            require(current[0][pivot] == 1, 'normalized parameter equation required')
            wrong = wrong_family(affine,free)
            if kind == 'parameter':
                candidates = [affine,wrong,[[v[0],F(0)] for v in affine]]
                labels = [family_tex(affine),family_tex(wrong),singleton([v[0] for v in affine])]
                after = labels[0] + r',\quad ' + ranks(1,1)
            else:
                correct = evaluated(original,affine)
                mistaken = evaluated(original,wrong)
                omitted = copy.deepcopy(correct)
                omitted[0] = [original[0][0]*v for v in affine[0]]
                candidates = [correct,mistaken,omitted]
                labels = [values_tex(values) for values in candidates]
                require(correct == [[row[2],0] for row in original], 'identity for every parameter')
                equations = [rf'({tex(a)})({affine_tex(affine[0])})+({tex(b)})({affine_tex(affine[1])})={tex(c)}'
                             for a,b,c in original]
                after = r'\begin{gathered}' + r'\\'.join(equations+[family_tex(affine)]) + r'\end{gathered}'
        else:
            raise export.ExportError('depth.linear_algebra','unknown subject decision')
        require(len({repr(value) for value in candidates}) == 3 and len(set(labels)) == 3,
                f'{case["id"]} step {number}: equivalent options')
        require(rank(current) == facts['augmented_rank']
                and rank([row[:2] for row in current]) == facts['coefficient_rank'],
                'row operation changed ranks')
        if facts['kind'] == 'unique':
            x,y = map(F,facts['solution'])
            require(all(a*x+b*y == c for a,b,c in current), 'intermediate solution preservation')
        elif facts['kind'] == 'infinite':
            affine = [list(map(F,coordinate)) for coordinate in facts['affine']]
            require(evaluated(current,affine) == [[row[2],0] for row in current],
                    'intermediate affine preservation')
        position = (case['first_position']+number-1)%3
        order = [1,2]
        order.insert(position,0)
        expected.append(dict(kind=kind, choices=[labels[index] for index in order],
                             answer=labels[0], after=after, position=position))
    require(case['steps'][-1][0] in ('check','incompatible','parameter_check'), 'final original check')
    return matrix(original), expected, facts


def verify(data, routes):
    require(routes['accepted'] is True and len(routes['questions']) == 12, 'actual full pool required')
    require(routes['question_ids'] == [case['id'] for case in data['cases']], 'ordered stable IDs')
    checked = []
    counts = {'unique':0,'empty':0,'infinite':0}
    positions = [0,0,0]
    for case, record in zip(data['cases'],routes['questions']):
        given, expected, facts = mathematics(case)
        q = record['question']
        require(record['id'] == q['id'] == case['id'], 'compiled identity')
        require(q['equation'] == given, 'compiled original given mismatch')
        require(q['description'] == data['goal']+' '+data['domain'], 'compiled original domain mismatch')
        require(len(q['steps']) == len(expected) and len(q['working_states']) == len(expected)+1,
                'complete route/state count')
        states = {state['id']:state['display'] for state in q['working_states']}
        previous = given
        for i,(step,wanted) in enumerate(zip(q['steps'],expected),1):
            labels = [option['label'] for option in step['options']]
            require(labels == wanted['choices'], f'{case["id"]} step {i}: actual options mismatch')
            matches = [option['id'] for option in step['options'] if option['label'] == wanted['answer']]
            require(len(matches) == 1 and step['accepted_option_ids'] == matches, 'false accepted key')
            require(states[step['semantics']['before']] == previous
                    and states[step['semantics']['after']] == wanted['after'], 'false intermediate working')
            require(bool(step['prompt']) and bool(step['explanation']) and bool(step['wrong_hint']),
                    'missing teaching field')
            for option in step['options']:
                semantic_labels = [wanted['answer']] + [
                    label for label in wanted['choices'] if label != wanted['answer']]
                require(option['id'] == i*10+semantic_labels.index(option['label'])+1,
                        'semantic choice identity moved away from its feedback')
                require((option['id'] in matches) == ('wrong_feedback' not in option), 'feedback attachment')
                if option['id'] not in matches:
                    require(bool(option['wrong_feedback'].strip()), 'empty wrong feedback')
            previous = wanted['after']
        counts[facts['kind']] += 1
        positions[expected[0]['position']] += 1
        checked.append(dict(id=case['id'],accepted=True,steps=len(expected),
                            wrong_choices=2*len(expected),given=given,decisions=expected,evidence=facts))
    require(counts == {'unique':8,'empty':2,'infinite':2} and positions == [4,4,4], 'coverage/balance')
    require(routes['routes'] == 12 and routes['wrong_choices'] == 118, 'model routes/wrong choices')
    require([case['group'] for case in data['cases']] == ['introductory']*4+['practice']*4+['mixed']*4,
            'four cases per curriculum group')
    required_swaps = [case['id'] for case in data['cases'] if case['rows'][0][0] == 0
                      and case['rows'][1][0] != 0 and case['steps'][0][0] in ('swap','method_swap')]
    fractional = [entry['id'] for entry in checked if entry['evidence']['kind'] == 'unique'
                  and any(F(v).denominator != 1 for v in entry['evidence']['solution'])]
    methods = [case['id'] for case in data['cases'][8:] if case['steps'][0][0] == 'method_swap']
    require(len(required_swaps) >= 2 and len(fractional) >= 2 and len(methods) >= 2,
            'swap, fractional-coordinate and mixed-method coverage')
    return dict(accepted=True,questions=12,steps=59,wrong_choices=118,classifications=counts,checks=checked)


def refused(call):
    try:
        call()
    except export.ExportError:
        return
    raise AssertionError('mutation was not refused')


def compile_scratch(text):
    with tempfile.TemporaryDirectory(prefix='markdown-', dir=shared.ROOT/'build/production/wave02/linear_algebra') as temp:
        folder = Path(temp).resolve()
        export.write_tree(folder, {'chapter.paths.md':text.encode()})
        result = subprocess.run([str(MODEL),'--question-batch',str(folder)],
                                capture_output=True,text=True,timeout=60)
        require(result.returncode == 0, result.stderr or result.stdout)
        return export.decoded(result.stdout)


def regressions(data, routes):
    path = ROOT/'authoring/documents/chapter.paths.md'
    original = path.read_bytes()
    source = original.decode()
    first = routes['questions'][0]['question']['steps'][0]
    edits = source.replace(data['cases'][0]['steps'][0][-1],
                           data['cases'][0]['steps'][0][-1]+' Scratch prompt edit.',1)
    wrong = next(option for option in first['options'] if 'wrong_feedback' in option)
    edits = edits.replace(wrong['wrong_feedback'],wrong['wrong_feedback']+' Scratch feedback edit.',1)
    changed = compile_scratch(edits)
    verify(data,changed)
    expected = copy.deepcopy(routes['questions'])
    expected[0]['question']['steps'][0]['prompt'] += ' Scratch prompt edit.'
    next(option for option in expected[0]['question']['steps'][0]['options']
         if option['id'] == wrong['id'])['wrong_feedback'] += ' Scratch feedback edit.'
    require(changed['questions'] == expected, 'real Markdown edit did not reach exactly the two compiled fields')

    false_key = source.replace('@answer 11','@answer 12',1).replace('@feedback 12 |','@feedback 11 |',1)
    key_routes = compile_scratch(false_key)
    refused(lambda:verify(data,key_routes))
    given, steps, _ = mathematics(data['cases'][0])
    false_work = source.replace('@after '+steps[0]['after'],'@after '+given,1)
    work_routes = compile_scratch(false_work)
    refused(lambda:verify(data,work_routes))
    duplicate = source.replace('@choice 12 | '+wrong['label'],
                               '@choice 12 | '+steps[0]['answer']+r'\,',1)
    require(duplicate != source, 'duplicate mutation missed its source')
    duplicate_routes = compile_scratch(duplicate)
    refused(lambda:verify(data,duplicate_routes))
    invalid = copy.deepcopy(data['cases'][0])
    invalid['rows'][0][0] = 13
    refused(lambda:mathematics(invalid))
    invalid['rows'] = [[0,0,0],[0,0,0]]
    refused(lambda:mathematics(invalid))
    invalid = copy.deepcopy(data['cases'][0])
    invalid['steps'][1][2] = '0'
    refused(lambda:mathematics(invalid))
    domain = copy.deepcopy(routes)
    domain['questions'][0]['question']['description'] = domain['questions'][0]['question']['description'].replace('x and y are real','x and y are integers')
    refused(lambda:verify(data,domain))
    require(path.read_bytes() == original, 'live Markdown changed during scratch tests')
    # Separate reading example: original equations, exact answer and all operations.
    example = [[F(1),F(-1),F(2)],[F(2),F(1),F(7)]]
    require(all(case['rows'] != example for case in data['cases']), 'worked example overlaps pool')
    require(shared.system_solution(example) == [F(3),F(1)], 'reading example independent solve')
    require(shared.row_addition(example,-2) == [[1,-1,2],[0,3,3]], 'reading example full row')
    return ['actual_markdown_prompt_and_feedback_edits','compiled_false_key_refused',
            'compiled_false_working_refused','compiled_equivalent_option_refused',
            'invalid_integer_bound_rank_zero_divisor_and_domain_refused',
            'separate_worked_example_checked','live_source_unchanged']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--routes',type=Path,required=True)
    args = parser.parse_args()
    data = export.decoded((ROOT/'cases.json').read_bytes())
    routes = export.decoded(args.routes.read_bytes())
    result = verify(data,routes)
    result['regressions'] = regressions(data,routes)
    result['routes_sha256'] = export.sha(args.routes.read_bytes())
    result['model_sha256'] = export.sha(MODEL.read_bytes())
    result['source_sha256'] = {
        str(path.relative_to(ROOT)):export.sha(path.read_bytes())
        for path in [ROOT/'cases.json',Path(__file__).resolve(),ROOT/'DESIGN.md',
                     ROOT/'authoring/authoring.json',ROOT/'authoring/documents/chapter.paths.md']}
    print(json.dumps(result,indent=2))


if __name__ == '__main__':
    main()
