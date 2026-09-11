"""Finite identity audit: exact circle reduction, domains, forms and real compiler probes.

This is not a TeX parser or general CAS. Displays decode only through the finite
typed registry in cases.json and a small whitespace/delimiter equivalence rule.
"""
import argparse
import copy
from fractions import Fraction as F
from itertools import combinations_with_replacement
import json
from pathlib import Path
import subprocess
import tempfile

import export_learning as export

SOURCE = Path(__file__).resolve().parent
ROOT = SOURCE.parents[4]
OUT = ROOT / 'build/production/wave03/trigonometry'
AUTHOR = SOURCE / 'authoring'
MODEL = ROOT / 'b/paths_learning_document_tests'
TARGET = ROOT / 'b/sorter'
PACKAGE = 'prod03_trig_identities'
DOMAINS = {'R': set(), 'S': {0, 2}, 'C': {1, 3}, 'SC': {0, 1, 2, 3}}
DTEX = {'R': r'\theta\in\mathbb{R}', 'S': r's\ne0', 'C': r'c\ne0',
        'SC': r's\ne0,\ c\ne0'}


def need(ok, message):
    if not ok:
        raise ValueError(message)


def clean(p):
    return {k: v for k, v in p.items() if v}


def add(a, b, sign=1):
    out = dict(a)
    for k, v in b.items():
        out[k] = out.get(k, F(0)) + sign*v
    return clean(out)


def mul(a, b):
    out = {}
    for (i, j), v in a.items():
        for (k, l), w in b.items():
            need(i+j+k+l <= 16, 'Polynomial degree exceeds finite bound 16')
            # Expand every c^2 as 1-s^2 immediately, using exact coefficients.
            pending = {(i+k, (j+l) % 2): v*w}
            for _ in range((j+l)//2):
                pending = add(pending, {(x+2, y): -z for (x, y), z in pending.items()})
            out = add(out, pending)
    return clean(out)


ONE = {(0, 0): F(1)}
S = {(1, 0): F(1)}
C = {(0, 1): F(1)}


def signature(p):
    need(bool(p), 'Identically zero denominator')
    lead = p[sorted(p)[0]]
    return tuple((k, v/lead) for k, v in sorted(p.items()))


def zero_catalogue():
    factors = [(S, {0, 2}), (C, {1, 3}), (add(ONE, S), {3}),
               (add(ONE, S, -1), {1}), (add(ONE, C), {2}), (add(ONE, C, -1), {0})]
    result = {signature(ONE): set()}
    for n in range(1, 5):
        for indices in combinations_with_replacement(range(6), n):
            p, zeros = ONE, set()
            for i in indices:
                p = mul(p, factors[i][0]); zeros |= factors[i][1]
            key = signature(p)
            need(key not in result or result[key] == zeros, 'Conflicting zero factorization')
            result[key] = zeros
    return result


ZEROS = zero_catalogue()


def zeros(p):
    key = signature(p)
    need(key in ZEROS, 'Unsupported denominator: outside six-factor finite catalogue')
    return set(ZEROS[key])


def validate(x, depth=0, counter=None):
    counter = [0] if counter is None else counter
    counter[0] += 1
    need(depth <= 10 and counter[0] <= 96, 'Expression exceeds depth/node bounds')
    if type(x) is int:
        need(abs(x) <= 3, 'Coefficient outside [-3,3]'); return
    if isinstance(x, str):
        need(x in ('s', 'c', 'sin', 'cos', 'tan', 'cot', 'sec', 'csc'), 'Unsupported leaf'); return
    need(isinstance(x, list) and len(x) == 3 and x[0] in ('add', 'sub', 'mul', 'div', 'pow'), 'Invalid finite AST')
    validate(x[1], depth+1, counter)
    if x[0] == 'pow':
        need(type(x[2]) is int and x[2] == 2, 'Only squares supported')
    else:
        validate(x[2], depth+1, counter)


def value(x):
    """(reduced numerator, reduced denominator, ALL original exclusions)."""
    if type(x) is int:
        return ({(0, 0): F(x)} if x else {}, ONE, set())
    if isinstance(x, str):
        return {'s': (S, ONE, set()), 'sin': (S, ONE, set()),
                'c': (C, ONE, set()), 'cos': (C, ONE, set()),
                'tan': (S, C, {1, 3}), 'cot': (C, S, {0, 2}),
                'sec': (ONE, C, {1, 3}), 'csc': (ONE, S, {0, 2})}[x]
    op, a, b = x
    n, d, excluded = value(a)
    if op == 'pow':
        return mul(n, n), mul(d, d), excluded
    m, e, other = value(b); excluded = excluded | other
    if op in ('add', 'sub'):
        return add(mul(n, e), mul(m, d), -1 if op == 'sub' else 1), mul(d, e), excluded
    if op == 'mul':
        return mul(n, m), mul(d, e), excluded
    return mul(n, e), mul(d, m), excluded | zeros(m)


def equal(a, b):
    n, d, _ = value(a); m, e, _ = value(b)
    return not add(mul(n, e), mul(m, d), -1)


def residual(a, b):
    n, d, _ = value(a); m, e, _ = value(b)
    return [{'s_power': i, 'c_power': j, 'coefficient': str(v)}
            for (i, j), v in sorted(add(mul(n, e), mul(m, d), -1).items())]


def tex(x):
    if type(x) is int:
        return str(x)
    if isinstance(x, str):
        return x if x in ('s', 'c') else '\\'+x+r'\theta'
    op, a, b = x
    if op == 'div': return r'\frac{'+tex(a)+'}{'+tex(b)+'}'
    if op == 'pow':
        if isinstance(a, str) and a not in ('s', 'c'):
            return '\\'+a+r'^{2}\theta'
        return '{'+tex(a)+'}^{2}'
    if op == 'mul': return tex(a)+r'\,'+tex(b)
    return r'\left('+tex(a)+('+' if op == 'add' else '-')+tex(b)+r'\right)'


def normal(s):
    return ''.join(s.replace(r'\dfrac', r'\frac').replace(r'\left', '').replace(r'\right', '').replace(r'\,', '').split())


def isop(x, op): return isinstance(x, list) and x[0] == op
def leaf(x): return isinstance(x, str) and x in ('s', 'c', 'sin', 'cos')
def square(x): return isop(x, 'pow') and leaf(x[1])
def poly(x):
    return type(x) is int or leaf(x) or (isinstance(x, list) and x[0] in ('add', 'sub', 'mul', 'pow') and poly(x[1]) and (x[0] == 'pow' or poly(x[2])))
def monomial(x):
    return type(x) is int or leaf(x) or square(x) or (isop(x, 'mul') and monomial(x[1]) and monomial(x[2]))
def expanded(x):
    return monomial(x) or (isinstance(x, list) and x[0] in ('add', 'sub') and expanded(x[1]) and expanded(x[2]))
def only_coordinates(x):
    return type(x) is int or leaf(x) or (isinstance(x, list) and only_coordinates(x[1]) and (x[0] == 'pow' or only_coordinates(x[2])))
def linear_pair(x): return isop(x, 'add') and ((x[1] == 1 and leaf(x[2])) or (x[2] == 1 and leaf(x[1])))


def form(name, x):
    fraction = isop(x, 'div')
    forms = {
        'single_factor': lambda: leaf(x),
        'squared_numerator': lambda: fraction and square(x[1]) and poly(x[2]),
        'quotient_sum': lambda: isop(x, 'add') and all(isop(y, 'div') and leaf(y[1]) and leaf(y[2]) for y in x[1:]),
        'difference_fractions': lambda: isop(x, 'sub') and all(isop(y, 'div') and poly(y[1]) and poly(y[2]) for y in x[1:]),
        'common_fraction': lambda: fraction and poly(x[1]) and poly(x[2]),
        'expanded_fraction': lambda: fraction and expanded(x[1]) and poly(x[2]),
        'factored_fraction': lambda: fraction and isop(x[1], 'mul') and any(linear_pair(y) for y in x[1][1:]) and poly(x[2]),
        'constant_fraction': lambda: fraction and type(x[1]) is int and poly(x[2]),
        'constant_over_coordinate': lambda: fraction and type(x[1]) is int and leaf(x[2]),
        'constant_over_coordinate_product': lambda: fraction and type(x[1]) is int and isop(x[2], 'mul') and all(leaf(y) for y in x[2][1:]),
        'constant_over_conjugates': lambda: fraction and type(x[1]) is int and isop(x[2], 'mul') and all(isinstance(y, list) and y[0] in ('add','sub') and y[1] == 1 and leaf(y[2]) for y in x[2][1:]),
        'difference_square_fraction': lambda: fraction and isop(x[1], 'sub') and x[1][1] == 1 and square(x[1][2]),
        'conjugate_fraction': lambda: fraction and isop(x[1], 'mul') and all(isinstance(y, list) and y[0] in ('add','sub') for y in x[1][1:]) and poly(x[2]),
        'rationalized_fraction': lambda: fraction and leaf(x[1]) and linear_pair(x[2]),
        'zero_fraction': lambda: fraction and x[1] == 0 and poly(x[2]),
        'constant': lambda: type(x) is int,
        'quotient_rewrite': lambda: fraction and only_coordinates(x) and isop(x[2], 'div'),
        'reciprocal_ratio': lambda: fraction and all(isop(y, 'pow') and y[1] in ('sec','csc') for y in x[1:]),
        'nested_reciprocals': lambda: fraction and all(isop(y, 'div') and y[1] == 1 and square(y[2]) for y in x[1:]),
        'inverted_ratio': lambda: fraction and square(x[1]) and square(x[2]),
        'quotient_square': lambda: isop(x, 'pow') and x[1] in ('tan','cot'),
        'constant_difference': lambda: fraction and type(x[1]) is int and isop(x[2], 'sub') and x[2][1] == 1 and square(x[2][2]),
        'constant_square': lambda: fraction and type(x[1]) is int and square(x[2]),
        'reciprocal_multiple': lambda: isop(x, 'mul') and type(x[1]) is int and isop(x[2], 'pow') and x[2][1] in ('sec','csc'),
        'polynomial': lambda: poly(x),
    }
    need(name in forms, 'Unsupported local goal '+name)
    return bool(forms[name]())


def registry(case):
    entries = [case['original'], case['multiplier'], case['final']]+case.get('controls', [])
    entries += [x for step in case['steps'] if step['form'] != 'domain' for x in step['candidates']]
    result = {}
    for x in entries:
        validate(x)
        try:
            value(x)
        except ValueError as e:
            raise ValueError(f"{case['id']} registry {x}: {e}") from e
        key = normal(tex(x))
        need(key not in result or result[key] == x, 'Ambiguous finite registry')
        result[key] = x
    return result


def audit(payload, cases):
    need(payload['accepted'] and payload['windows'] == 0 and payload['routes'] == 12 and payload['save_replay'] and len(payload['questions']) == 12, 'Wrong route inventory')
    need([c['group'] for c in cases] == ['introductory']*4+['practice']*4+['mixed']*4, 'Wrong 4/4/4 finite coverage')
    need([c['id'] for c in cases] == [PACKAGE+f'_q{i:02}' for i in range(1,13)] == payload['question_ids'], 'Wrong assigned question order')
    records = []; positions = []
    for row, case in zip(payload['questions'], cases):
        q = row['question']; original = case['original']; validate(original)
        excluded = value(original)[2]
        need(excluded == DOMAINS[case['domain']], 'Input original/domain mismatch')
        need(q['id'] == case['id'] == row['id'], 'Question identity mismatch')
        need(q['equation'] == 'E='+tex(original), 'Compiled original differs from finite input')
        need(q['description'] == case['goal']+' '+case['domain_text'], 'Compiled goal/domain changed')
        need(q['content_version'] == 1 and q['schema_version'] == 1, 'Unexpected content version')
        need(len(q['steps']) == len(case['steps']), 'Missing local decision')
        need(len(q['working_states']) == len(q['steps'])+1, 'Unexpected working-state inventory')
        need(q['working_states'][0]['display'] == q['equation'], 'Original working changed')
        states = {s['id']: s['display'] for s in q['working_states']}
        decode = registry(case)
        n, d, _ = value(case['multiplier'])
        need(not zeros(n) - excluded and not value(case['multiplier'])[2] - excluded, 'Check multiplier can vanish on original domain')
        need(equal(original, case['final']) and form(case['final_form'], case['final']), 'Incorrect final certificate')
        need(not value(case['final'])[2] - excluded, 'Final expression loses permitted angles')
        for index, (step, local) in enumerate(zip(q['steps'], case['steps'])):
            need(step['id'] == 10*(index+1), 'Decision identity changed')
            need(step['semantics']['before'] == (q['working_states'][0]['id'] if index == 0 else q['steps'][index-1]['semantics']['after']), 'Broken reached-state chain')
            options = step['options']; need(len(options) == 3, 'Need three choices')
            need(sorted(o['id'] for o in options) == [step['id']+1, step['id']+2, step['id']+3], 'Option identity changed')
            kind = local['form']; decoded = []
            for o in options:
                if kind == 'domain':
                    matches = [k for k, v in DTEX.items() if normal(v) == normal(o['label'])]
                    need(len(matches) == 1, 'Unknown domain display'); decoded.append(matches[0])
                else:
                    need(normal(o['label']) in decode, 'Display outside finite expression registry')
                    decoded.append(decode[normal(o['label'])])
            for i in range(3):
                for j in range(i):
                    need(DOMAINS[decoded[i]] != DOMAINS[decoded[j]] if kind == 'domain' else not equal(decoded[i], decoded[j]), f"Equivalent duplicate options: {case['id']} step {step['id']} options {i}/{j}")
            target = ['mul', case['multiplier'], original] if local.get('check') else original
            good = [i for i, x in enumerate(decoded) if (DOMAINS[x] == excluded if kind == 'domain' else equal(x, target) and not value(x)[2]-excluded and form(kind, x))]
            need(len(good) == 1, 'No unique truth-and-goal-correct choice')
            accepted = options[good[0]]['id']
            need(step['accepted_option_ids'] == [accepted], 'False accepted key')
            if index == 0: positions.append(good[0])
            reached = normal(states[step['semantics']['after']])
            valid_states = []
            if kind == 'domain': valid_states = ['D:'+DTEX[case['domain']]]
            else:
                prefix = tex(case['multiplier'])+'E=' if local.get('check') else 'E='
                for x in decode.values():
                    if equal(x, target) and not value(x)[2]-excluded and form(kind, x):
                        conclusion = r',\quad E='+tex(case['final']) if local.get('check') else ''
                        valid_states.append(prefix+tex(x)+conclusion+r',\quad D:'+DTEX[case['domain']])
            need(reached in [normal(s) for s in valid_states], 'False reached work or lost original domain/form')
            records.append({'question': case['id'], 'step': step['id'], 'goal': kind,
                            'accepted_id': accepted, 'options_checked': 3, 'exact_identity': True,
                            'retained_axis_exclusions': sorted(excluded),
                            'option_evidence': [{'id': o['id'],
                                'axis_exclusions': sorted(DOMAINS[x] if kind == 'domain' else value(x)[2]),
                                'truth': DOMAINS[x] == excluded if kind == 'domain' else equal(x, target),
                                'goal_form': True if kind == 'domain' else form(kind, x),
                                'cross_product_remainder': None if kind == 'domain' else residual(x, target)}
                                for o, x in zip(options, decoded)]})
    need(sorted(positions) == [0]*4+[1]*4+[2]*4, 'Unbalanced first positions')
    return records


def reject(fn):
    try: fn()
    except (ValueError, KeyError, TypeError, ZeroDivisionError) as e: return str(e)
    raise AssertionError('Deliberate corruption passed')


def probes(payload, cases):
    negatives = []; positives = []
    for qi, row in enumerate(payload['questions']):
        for si, step in enumerate(row['question']['steps']):
            bad = copy.deepcopy(payload); bstep = bad['questions'][qi]['question']['steps'][si]
            bstep['accepted_option_ids'] = [next(o['id'] for o in step['options'] if o['id'] not in step['accepted_option_ids'])]
            negatives.append({'probe': 'false_key', 'q': qi+1, 'step': si+1, 'rejected': reject(lambda: audit(bad, cases))})
            bad = copy.deepcopy(payload); q = bad['questions'][qi]['question']
            state = next(s for s in q['working_states'] if s['id'] == step['semantics']['after'])
            local = cases[qi]['steps'][si]
            wrong_label = next(o['label'] for o in step['options'] if o['id'] not in step['accepted_option_ids'])
            if local['form'] == 'domain':
                state['display'] = 'D:'+wrong_label
            else:
                prefix = tex(cases[qi]['multiplier'])+'E=' if local.get('check') else 'E='
                conclusion = r',\quad E='+tex(cases[qi]['final']) if local.get('check') else ''
                state['display'] = prefix+wrong_label+conclusion+r',\quad D:'+DTEX[cases[qi]['domain']]
            negatives.append({'probe': 'false_work', 'q': qi+1, 'step': si+1, 'rejected': reject(lambda: audit(bad, cases))})
    bad = copy.deepcopy(payload); step = bad['questions'][0]['question']['steps'][1]
    correct = next(o for o in step['options'] if o['id'] in step['accepted_option_ids'])
    wrong = next(o for o in step['options'] if o['id'] not in step['accepted_option_ids'])
    wrong['label'] = correct['label'].replace(r'\frac', r'\dfrac')+' '
    negatives.append({'probe': 'different_spelling_equivalent_duplicate', 'rejected': reject(lambda: audit(bad, cases))})
    bad = copy.deepcopy(payload); q = bad['questions'][0]['question']; step = q['steps'][2]
    next(s for s in q['working_states'] if s['id'] == step['semantics']['after'])['display'] = 'E=s'+r',\quad D:'+DTEX['R']
    negatives.append({'probe': 'cancelled_domain_loss', 'rejected': reject(lambda: audit(bad, cases))})
    bad = copy.deepcopy(payload); step = bad['questions'][0]['question']['steps'][2]
    next(o for o in step['options'] if o['id'] in step['accepted_option_ids'])['label'] = tex(cases[0]['original'])
    need(equal(cases[0]['original'], 's') and not form('single_factor', cases[0]['original']), 'Goal probe must be true')
    negatives.append({'probe': 'true_but_not_cancelled_single_factor', 'rejected': reject(lambda: audit(bad, cases))})
    for x in [4, True, ['pow','s',3], ['div',1,0], ['div',1,['add',2,'s']]]:
        negatives.append({'probe': 'invalid_or_unsupported_input', 'input': x, 'rejected': reject(lambda: (validate(x), value(x)))})
    badcases = copy.deepcopy(cases); badcases[0]['domain'] = 'R'
    negatives.append({'probe': 'invalid_input_domain', 'rejected': reject(lambda: audit(payload, badcases))})
    bad = copy.deepcopy(payload); bad['questions'][0]['question']['description'] += ' All real angles allowed.'
    negatives.append({'probe': 'compiled_domain_corruption', 'rejected': reject(lambda: audit(bad, cases))})
    alternate = copy.deepcopy(payload); q = alternate['questions'][0]['question']; step = q['steps'][2]
    next(o for o in step['options'] if o['id'] in step['accepted_option_ids'])['label'] = r'\sin\theta'
    next(s for s in q['working_states'] if s['id'] == step['semantics']['after'])['display'] = r'E=\sin\theta,\quad D:s\ne0'
    audit(alternate, cases); positives.append('sin(theta) instead of s: choice and reached work accepted')
    alternate = copy.deepcopy(payload); step = alternate['questions'][0]['question']['steps'][1]
    next(o for o in step['options'] if o['id'] in step['accepted_option_ids'])['label'] = correct['label'].replace(r'\frac', r'\dfrac')+' '
    audit(alternate, cases); positives.append('Equivalent fraction spelling accepted singly')
    alternate = copy.deepcopy(payload); q = alternate['questions'][4]['question']; step = q['steps'][2]
    reordered = cases[4]['controls'][0]
    next(o for o in step['options'] if o['id'] in step['accepted_option_ids'])['label'] = tex(reordered)
    next(s for s in q['working_states'] if s['id'] == step['semantics']['after'])['display'] = 'E='+tex(reordered)+r',\quad D:'+DTEX['S']
    audit(alternate, cases); positives.append('Reordered expanded polynomial: choice and reached work accepted')
    duplicate = copy.deepcopy(payload); step = duplicate['questions'][4]['question']['steps'][2]
    next(o for o in step['options'] if o['id'] not in step['accepted_option_ids'])['label'] = tex(reordered)
    negatives.append({'probe': 'reordered_polynomial_duplicate', 'rejected': reject(lambda: audit(duplicate, cases))})
    bad = copy.deepcopy(payload); bad['questions'][0]['question']['equation'] = 'E='+tex(cases[1]['original'])
    negatives.append({'probe': 'compiled_original_corruption', 'rejected': reject(lambda: audit(bad, cases))})
    bad = copy.deepcopy(payload); step = bad['questions'][11]['question']['steps'][2]
    early_square = ['div', 2, ['pow', 'c', 2]]
    need(equal(early_square, cases[11]['original']) and not form('constant_over_conjugates', early_square), 'Early form probe must be true but miss the goal')
    next(o for o in step['options'] if o['id'] in step['accepted_option_ids'])['label'] = tex(early_square)
    negatives.append({'probe': 'true_but_requested_denominator_product_missing', 'rejected': reject(lambda: audit(bad, cases))})
    return negatives, positives


def command(args):
    p = subprocess.run(list(map(str, args)), capture_output=True, text=True, timeout=60)
    need(p.returncode == 0, p.stderr or p.stdout)
    return json.loads(p.stdout)


def markdown_probes(payload):
    original = export.read_bytes(AUTHOR/'documents/chapter.paths.md').decode()
    evidence = []
    for directive, field in [('@step 10 | ', 'prompt'), ('@feedback ', 'wrong_feedback')]:
        question_start = original.index('@question ')
        start = original.index(directive, question_start); end = original.index('\n', start)
        old = original[start:end]; changed = original[:end]+' Recheck this decision carefully.'+original[end:]
        with tempfile.TemporaryDirectory(prefix='markdown-probe-', dir=OUT) as temp:
            folder = Path(temp)
            export.write_tree(folder, {'chapter.paths.md': changed.encode()})
            compiled = command([MODEL, '--question-batch', folder])
            expected = copy.deepcopy(payload)
            if field == 'prompt': expected['questions'][0]['question']['steps'][0]['prompt'] += ' Recheck this decision carefully.'
            else:
                option_id = int(old.split(' | ')[0].split()[1])
                step = expected['questions'][0]['question']['steps'][0]
                next(o for o in step['options'] if o['id'] == option_id)[field] += ' Recheck this decision carefully.'
            need(compiled == expected, 'Markdown edit changed fields beyond intended '+field)
            evidence.append({'real_markdown_edit': field, 'compiled_only_intended_field': True,
                             'original_sha256': export.sha(original.encode()), 'edited_sha256': export.sha(changed.encode())})
    return evidence


def source_hashes():
    return {str(p.relative_to(SOURCE)): export.sha(export.read_bytes(p)) for p in sorted(SOURCE.rglob('*'))
            if p.is_file() and p.name != 'BRIEF.md' and '__pycache__' not in p.parts}


def main():
    parser = argparse.ArgumentParser(); parser.add_argument('--routes', required=True); args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    before = source_hashes(); cases = json.loads(export.read_bytes(SOURCE/'cases.json'))['cases']
    need(set(before) == {'DESIGN.md', 'cases.json', 'certificate_tests.py', 'review.md', 'authoring/authoring.json', 'authoring/documents/chapter.paths.md'}, 'Incomplete or extra maintained authoring route')
    ready = json.loads(export.read_bytes(ROOT/'build/production/wave01/build-ready.json'))
    hashes = {p.name: export.sha(export.read_bytes(p, 64*1024*1024)) for p in (TARGET, MODEL)}
    need(ready['status'] == 'ready' and all(ready['sha256'][k] == v for k, v in hashes.items()), 'Shared build pins changed')
    routes_path = Path(args.routes).resolve(); payload = json.loads(export.read_bytes(routes_path))
    fresh = command([MODEL, '--question-batch', AUTHOR/'documents'])
    need(payload == fresh, 'Stale compiled routes')
    records = audit(payload, cases); negatives, positives = probes(payload, cases)
    edits = markdown_probes(payload)
    inspection = export.Target(TARGET).inspect(documents=AUTHOR/'documents')
    supplied_inspection = json.loads(export.read_bytes(OUT/'inspection.json'))
    need(inspection == supplied_inspection, 'Stale inspection')
    author = json.loads(export.read_bytes(AUTHOR/'authoring.json'))
    entities = inspection['entities']
    content = [e for e in entities if e['kind'] in ('lesson','question')]
    need(len(content) == 13 and all(e['parent'] == 'topic_0033' and e['subject'] == 'trigonometry' for e in content), 'Wrong chapter/subject binding')
    reading = next(e for e in content if e['kind'] == 'lesson')
    need(reading['id'] == PACKAGE+'_r' and reading['links'] == [c['id'] for c in cases], 'Canonical reading/practice order mismatch')
    need(all(e['links'] == [PACKAGE+'_r'] for e in content if e['kind'] == 'question'), 'Question has wrong reading')
    need(author['format'] == 'paths_learning_authoring' and author['format_version'] == 1 and author['package_id'] == PACKAGE and author['package_version'] == 1, 'Wrong metadata identity/version')
    provenance = json.loads(export.provenance(author, inspection['entities']))
    lesson = command([MODEL, '--family-lessons', AUTHOR/'documents'])
    need(lesson == json.loads(export.read_bytes(OUT/'lessons.json')) and lesson['accepted'] and lesson['questions'] == 12 and lesson['readings'] == 1, 'Lesson gate mismatch')
    need(source_hashes() == before, 'Canonical source changed during verification')
    need(all(export.sha(export.read_bytes(p,64*1024*1024)) == hashes[p.name] for p in (TARGET,MODEL)), 'Executable changed during verification')
    report = {'accepted': True, 'questions': 12, 'steps': len(records), 'wrong_choices': 2*len(records),
              'finite_scope': 'Twelve explicit originals; exact rational circle reduction; six denominator factors through four factors; registered displays and controlled spellings only',
              'decisions': records, 'negative_probes': negatives, 'positive_controls': positives,
              'real_markdown_probes': edits, 'provenance': provenance, 'source_sha256': before,
              'target_sha256': hashes[TARGET.name], 'model_sha256': hashes[MODEL.name], 'windows': 0}
    (OUT/'mathematics.json').write_bytes(export.encoded(report))
    evidence = {name: export.sha(export.read_bytes(OUT/name)) for name in ('inspection.json','routes.json','lessons.json','mathematics.json')}
    receipt = {'format': 'paths_depth_subject', 'format_version': 1, 'wave': 'wave03', 'subject': 'trigonometry',
               'stage': 'ready_for_coordinator_review', 'published': False, 'source': str(AUTHOR),
               'package_id': PACKAGE, 'reading_id': PACKAGE+'_r', 'question_ids': [c['id'] for c in cases],
               'questions': 12, 'readings': 1, 'steps': len(records), 'wrong_choices': 2*len(records),
               'source_sha256': before, 'evidence_sha256': evidence,
               'target_sha256': hashes[TARGET.name], 'model_sha256': hashes[MODEL.name],
               'inspection': str(OUT/'inspection.json'), 'routes': str(routes_path), 'lessons': str(OUT/'lessons.json'),
               'mathematical_evidence': str(OUT/'mathematics.json'),
               'reused_helpers': {'tools/export_learning.py': export.sha(export.read_bytes(ROOT/'tools/export_learning.py'))},
               'remaining_concerns': ['Coordinator mathematical/prose review and publication remain separate; native appearance and learner outcomes not observed.']}
    (OUT/'production.json').write_bytes(export.encoded(receipt))
    print(json.dumps({'accepted': True, 'questions': 12, 'steps': len(records), 'wrong_choices': 2*len(records),
                      'negative_probes': len(negatives), 'positive_controls': len(positives),
                      'real_markdown_edits': len(edits), 'production': str(OUT/'production.json')}, indent=2))


if __name__ == '__main__': main()
