#!/usr/bin/env python3
"""Check the four-level linear family; publish its runtime pack only with --publish."""
import argparse
from collections import Counter
from fractions import Fraction
import hashlib
import itertools
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / 'content/authoring/question_layers_v1.json'
PROFILE_IDS = ('learn', 'practice', 'solve', 'independent')
STRATA = ('positive_integer', 'negative_coefficient', 'negative_offset',
          'negative_solution', 'zero_solution', 'fractional_solution')


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def encoded(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False)


def digest(value):
    return hashlib.sha256(encoded(value).encode()).hexdigest()


def exact(value):
    require(type(value) in (int, str), 'Exact numbers must be integer or rational text')
    require(len(str(value)) <= 40, 'Exact number exceeds authoring bound')
    try:
        result = Fraction(value)
    except (ValueError, ZeroDivisionError) as error:
        raise ValueError('Invalid exact number') from error
    require(abs(result.numerator) <= 1000000 and result.denominator <= 1000000,
            'Exact number exceeds authoring bound')
    return result


def tex(value):
    q = exact(value)
    if q.denominator == 1:
        return str(q.numerator)
    return ('-' if q < 0 else '') + rf'\frac{{{abs(q.numerator)}}}{{{q.denominator}}}'


def equation(state):
    a, b, c = (exact(state[k]) for k in ('a', 'b', 'c'))
    lhs = 'x' if a == 1 else '-x' if a == -1 else tex(str(a)) + 'x'
    if b:
        lhs += ('+' if b > 0 else '-') + tex(str(abs(b)))
    return lhs + '=' + tex(str(c))


def validate_spec(spec):
    require(type(spec['schema_version']) is int and spec['schema_version'] == 1 and spec['artifact_kind'] == 'question_support_authoring_pilot'
            and spec['publication'] == 'authoring_only', 'Not the authoring-only pilot contract')
    profiles = spec['profiles']
    require(tuple(p['id'] for p in profiles) == PROFILE_IDS, 'Exactly four ordered support profiles required')
    inputs = ('symbol_choices', 'symbol_choices_optional_blank', 'checkpoint_and_working', 'written_solution')
    for i, p in enumerate(profiles):
        require(set(p) == {'id', 'label', 'input', 'show_goal', 'show_why', 'show_definitions', 'show_step_prompt'},
                'Unknown profile field; change the contract explicitly')
        require(p['input'] == inputs[i], 'Profile input changed')
        flags = (p['show_goal'], p['show_why'], p['show_definitions'], p['show_step_prompt'])
        require(all(type(x) is bool for x in flags), 'Profile flags must be boolean')
        require(flags == ((True, True, True, True), (True, False, False, True),
                          (False, False, False, False), (False, False, False, False))[i],
                'Support disclosure changed')
    family = spec['family']
    require(family['id'] == 'linear_balance_ax_b' and type(family['version']) is int
            and family['version'] == 1, 'Unsupported family/version; add a separately checked recipe')
    require(family['strata'] == list(STRATA), 'Missing or reordered generation stratum')
    require(family['source']['kind'] == 'original_generated' and family['source']['reuse'] == 'reference_only',
            'Imported content needs a separate provenance review')
    require(set(spec['teaching']) == {'remove_offset', 'divide_coefficient'}, 'Missing teaching step')
    for step in spec['teaching'].values():
        require(all(step[k] for k in ('goal', 'prompt', 'why', 'after', 'definition_ids', 'misconceptions')),
                'Incomplete teaching step')
        require(len(set(step['definition_ids'])) == len(step['definition_ids']), 'Duplicate definition reference')
        for ref in step['definition_ids']:
            definition = spec['definitions'].get(ref)
            require(definition and all(definition[k] for k in ('version', 'term', 'meaning', 'rule_tex')),
                    'Unresolved or empty definition')
    leaf = next((row for row in coverage() if row['id'] == family['subcategory']), None)
    require(leaf and leaf['topic'] == family['topic'] and leaf['subject'] == family['subject'],
            'Family taxonomy does not resolve')


def coverage():
    mapping = json.loads((ROOT / 'content/authoring/math_corpus_map.json').read_text())
    subcategories = json.loads((ROOT / 'content/authoring/corpus_starters/subcategories.json').read_text())
    topics = {t['id']: t for t in mapping['topics']}
    parents = {s['topic'] for s in subcategories}
    leaves = [dict(id=s['id'], topic=s['topic'], title=s['title'], kind='named_subcategory',
                   subject=topics[s['topic']]['subject']) for s in subcategories]
    leaves += [dict(id=t['id'], topic=t['id'], title=t['title'], subject=t['subject'], kind='terminal_chapter')
               for t in mapping['topics'] if t['id'] not in parents]
    require(len({x['id'] for x in leaves}) == len(leaves), 'Duplicate practice leaf')
    for row in leaves:
        row.update(taxonomy_review='pending', family_specs=0, pilot_instances=0,
                   skill_ids=[], family_ids=[], drafted_instances=0,
                   mathematically_checked_instances=0, formatted_instances=0,
                   integrated_four_level_reps=0, initial_target_reps=24, expansion_target_reps=60,
                   initial_target_families=4, expansion_target_families=6,
                   gaps=['taxonomy_review', 'family_specification', 'four_level_runtime'])
    return sorted(leaves, key=lambda x: (x['subject'], x['topic'], x['id']))


def parameters(a, b, c):
    require(type(a) is int and type(b) is int and 2 <= abs(a) <= 9 and 1 <= abs(b) <= 9,
            'Parameters outside the nonzero linear pilot family')
    return dict(a=a, b=b, c=str(exact(c)))


def choices(key, step, correct, alternatives):
    items = [dict(id='correct', value=str(correct), tex=tex(str(correct)))]
    seen = {correct}
    for reason, value in alternatives:
        if value not in seen:
            items.append(dict(id=reason, value=str(value), tex=tex(str(value))))
            seen.add(value)
        if len(items) == 3:
            break
    require(len(items) == 3, 'Fewer than two distinct wrong choices')
    return sorted(items, key=lambda item: digest([key, step, item['id']]))


def make_question(spec, params, stratum='golden', cohort='example'):
    p = parameters(**params)
    a, b, c = p['a'], p['b'], exact(p['c'])
    d, answer = c - b, (c - b) / a
    family = spec['family']
    key = f"{family['id']}_v{family['version']}_{digest(p)[:32]}"
    states = [dict(a=str(a), b=str(b), c=str(c)), dict(a=str(a), b='0', c=str(d)),
              dict(a='1', b='0', c=str(answer))]
    steps = [dict(id='remove_offset', before=0, after=1,
                  operation=dict(kind='subtract_both', operand=str(b)), blank_tex=tex(a)+r'x=\square',
                  choices=choices(key, 0, d, [('wrong_sign', c+b), ('erase_only_left', c)])),
             dict(id='divide_coefficient', before=1, after=2,
                  operation=dict(kind='divide_both', operand=str(a)), blank_tex=r'x=\square',
                  choices=choices(key, 1, answer, [('missing_division', d), ('ignore_offset', c/a),
                                                 ('wrong_sign', -answer), ('subtract_after_dividing', c/a-b)]))]
    return dict(id=key, family_id=family['id'], content_version=1, parameters=p,
                stratum=stratum, cohort=cohort, complexity_band=family['complexity_band'],
                problem=dict(equation_tex=equation(states[0]), goal='Solve for x.', domain='x is real.'),
                states=states, steps=steps, answer=dict(kind='exact_rational', variable='x', value=str(answer)),
                verification=dict(substitute_into='original', left=str(a*answer+b), right=str(c)))


def generate(spec, per_stratum=4):
    validate_spec(spec)
    require(type(per_stratum) is int and 1 <= per_stratum <= 10, 'Use 1..10 instances per stratum')
    result, seen = [], set()
    for stratum in STRATA:
        aa = range(-9, -1) if stratum == 'negative_coefficient' else range(2, 10)
        bb = range(-9, 0) if stratum == 'negative_offset' else range(1, 10)
        roots = ([Fraction(0)] if stratum == 'zero_solution' else
                 [Fraction(-n) for n in range(1, 10)] if stratum == 'negative_solution' else
                 sorted({Fraction(p, q) for p in range(-9, 10) for q in range(2, 8)
                         if p and Fraction(p, q).denominator > 1}) if stratum == 'fractional_solution' else
                 [Fraction(n) for n in range(1, 10)])
        pool = [parameters(a, b, str(a*s+b)) for a, b, s in itertools.product(aa, bb, roots)]
        pool.sort(key=lambda p: digest([spec['family']['seed'], stratum, p]))
        added = 0
        for p in pool:
            # Normalized displayed coefficients reject scalar multiples, not
            # every question sharing a final answer (zero is a useful stratum).
            normalized = (Fraction(p['b'], p['a']), exact(p['c']) / p['a'])
            if normalized in seen:
                continue
            seen.add(normalized)
            cohort = 'fresh_check' if added % 4 == 3 else 'practice'
            result.append(make_question(spec, p, stratum, cohort))
            added += 1
            if added == per_stratum:
                break
        require(added == per_stratum, 'Parameter pool exhausted before target')
    return result


def verify_question(q):
    """Independent balance invariants and substitution, not the construction seed."""
    p = parameters(**q['parameters'])
    require(q['family_id'] == 'linear_balance_ax_b' and q['content_version'] == 1, 'Question family/version mismatch')
    require(q['id'] == f"linear_balance_ax_b_v1_{digest(p)[:32]}", 'Question identity drift')
    states = [tuple(exact(s[k]) for k in ('a', 'b', 'c')) for s in q['states']]
    require(len(states) == 3 and states[0] == (p['a'], p['b'], exact(p['c'])), 'Givens changed')
    require(q['problem'] == dict(equation_tex=equation(q['states'][0]), goal='Solve for x.', domain='x is real.'),
            'Displayed problem disagrees with mathematics')
    require(len(q['steps']) == 2, 'Expected two mathematical steps')
    for i, step in enumerate(q['steps']):
        before, after = states[i:i+2]
        require((step['id'], step['before'], step['after']) ==
                (('remove_offset', 'divide_coefficient')[i], i, i+1), 'Broken route')
        require(step['operation'] == (dict(kind='subtract_both', operand=str(p['b'])) if i == 0 else
                                     dict(kind='divide_both', operand=str(p['a']))), 'Incorrect operation')
        # Cross multiplication proves that the complete linear equations have
        # the same root, with nonzero coefficients excluding vacuous equations.
        require(before[0] != 0 and after[0] != 0 and
                (before[1]-before[2])*after[0] == (after[1]-after[2])*before[0], 'Step changes the solution set')
        require(after[1] == 0 and (after[0] == before[0] if i == 0 else after[0] == 1), 'Step misses its stated goal')
        require(step['blank_tex'] == (tex(p['a'])+r'x=\square' if i == 0 else r'x=\square'), 'Wrong answer slot')
        values = [exact(option['value']) for option in step['choices']]
        require(len(values) == 3 and len(set(values)) == 3, 'Duplicate or missing choices')
        require(len({o['id'] for o in step['choices']}) == 3, 'Duplicate choice identity')
        for option, value in zip(step['choices'], values):
            require(option['tex'] == tex(str(value)), 'Choice display drift')
            require((option['id'] == 'correct') == (value == after[2]), 'Incorrect answer key')
        require(sum(value == after[2] for value in values) == 1, 'Choice set has no unique answer')
    answer = exact(q['answer']['value'])
    require(q['answer']['kind'] == 'exact_rational' and q['answer']['variable'] == 'x' and
            states[-1] == (1, 0, answer), 'Final answer contract mismatch')
    require(p['a'] * answer + p['b'] == exact(p['c']), 'Original substitution fails')
    require(q['verification'] == dict(substitute_into='original', left=str(p['a']*answer+p['b']), right=p['c']),
            'Verification must use original givens')


def project(spec, q, profile_id, step_index=0):
    """A proposed pre-answer view. Allowlisted fields never include the key/solution."""
    require(profile_id in PROFILE_IDS and step_index in (0, 1), 'Unknown support level or step')
    profile = spec['profiles'][PROFILE_IDS.index(profile_id)]
    view = dict(question_id=q['id'], support=profile_id, problem=q['problem'].copy(), input=profile['input'])
    if profile_id in ('solve', 'independent'):
        view['working'] = ''  # A real UI supplies the learner's retained draft.
        if profile_id == 'solve':
            view['checkpoint'] = 'Supply x and keep your written working.'
        return view
    step = q['steps'][step_index]
    teaching = spec['teaching'][step['id']]
    view['working_tex'] = equation(q['states'][step['before']])
    p = q['parameters']
    view['bindings'] = [dict(symbol='a', value_tex=tex(p['a']), meaning='the coefficient multiplying x'),
                        dict(symbol='b', value_tex=tex(p['b']), meaning='the constant added to ax in the original'),
                        dict(symbol='c', value_tex=tex(p['c']), meaning='the original right-hand side')]
    view['goal'] = teaching['goal']
    view['prompt'] = teaching['prompt']
    if profile['show_why']:
        view['why'] = teaching['why']
    if profile['show_definitions']:
        view['definitions'] = [spec['definitions'][key] for key in teaching['definition_ids']]
    if profile_id in ('learn', 'practice'):
        # Opaque input IDs, not the authoring misconception/answer-key labels.
        view['choices'] = [dict(id=i+1, tex=o['tex']) for i, o in enumerate(step['choices'])]
    if profile_id == 'practice':
        view['blank_tex'] = step['blank_tex']
    return view


def runtime_bank(spec):
    """Explicit publication adapter: existing prepared content plus typed support."""
    golden = make_question(spec, spec['family']['golden_parameters'])
    instances = [golden] + [q for q in generate(spec) if q['id'] != golden['id']]
    rows = []
    def plain(state):
        a, b, c = (exact(state[k]) for k in ('a', 'b', 'c'))
        lhs = 'x' if a == 1 else str(a)+'x'
        if b:
            lhs += ('+' if b > 0 else '') + str(b)
        return lhs+'='+str(c)
    for number, q in enumerate(instances, 1):
        verify_question(q)
        prepared, support = [], []
        for i, step in enumerate(q['steps']):
            teaching = spec['teaching'][step['id']]
            definitions = '\n\n'.join(d['term']+'\n\n'+d['meaning']+'\n\n$$'+d['rule_tex']+'$$'
                                       for d in (spec['definitions'][ref] for ref in teaching['definition_ids']))
            a,b,c = (q['parameters'][k] for k in ('a','b','c'))
            binding = f'Here a={a} multiplies x, b={b} is the original added constant, and c={c} is the original right-hand side.'
            prompt = (f'Subtract ({b}) on both sides. Complete {a}x = ?' if i == 0 else
                      f'Divide both sides by ({a}). Complete x = ?')
            prepared.append(dict(id=i+1, layer_name=teaching['goal'], prompt=prompt,
                options=[dict(id=j+1, label=o['tex']) for j,o in enumerate(step['choices'])],
                accepted_option_ids=[j+1 for j,o in enumerate(step['choices']) if o['id']=='correct'],
                wrong_hint='Working retained. Check the sign and apply the operation to both sides.',
                explanation=teaching['after'], hint=teaching['why'], next_move=equation(q['states'][i+1]),
                semantics=dict(purpose='calculation', completion='any_accepted', before=i+1, after=i+2)))
            support.append(dict(equation=plain(q['states'][i+1]), response_prefix=(str(a)+'x=' if i==0 else 'x='),
                definitions=definitions, teaching=teaching['goal']+'\n\n'+binding+'\n\n'+teaching['why']+'\n\n'+definitions,
                responses=[o['value'] for o in step['choices']]))
        rows.append(dict(id=q['id'], title=f'Linear practice {number:02}', level='practice',
            subject=spec['family']['subject'], topic=spec['family']['topic'],
            question=dict(schema_version=1, id=q['id'], content_version=1,
                equation=equation(q['states'][0]), skill=spec['family']['skill_id'], description='Solve for x.',
                working_states=[dict(id=i+1, display=equation(s)) for i,s in enumerate(q['states'])], steps=prepared,
                support=dict(family='linear_balance_ax_b_v1', equation=plain(q['states'][0]),
                             domain='x is real.', steps=support))))
    return dict(schema_version=1, collection='linear_support', questions=rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'build/question-format-evidence')
    parser.add_argument('--per-stratum', type=int, default=4)
    publication = parser.add_mutually_exclusive_group()
    publication.add_argument('--publish', action='store_true')
    publication.add_argument('--check-runtime', action='store_true')
    args = parser.parse_args()
    spec = json.loads(SPEC.read_text())
    if args.publish or args.check_runtime:
        bank = runtime_bank(spec)
        target = ROOT/'content/corpus/linear_support.json'
        contents = json.dumps(bank, ensure_ascii=False, indent=2)+'\n'
        if args.check_runtime:
            require(target.read_text() == contents, 'Runtime pack differs from checked authoring')
        else:
            target.write_text(contents)
        print(f"{len(bank['questions'])} four-level questions: runtime publication reproduced")
        return
    questions = generate(spec, args.per_stratum)
    golden = make_question(spec, spec['family']['golden_parameters'])
    for q in [golden, *questions]:
        verify_question(q)
    require(len({q['id'] for q in questions}) == len(questions), 'Repeated instance ID')
    published = ROOT/'content/corpus/linear_support.json'
    expected = runtime_bank(spec)
    integrated = len(expected['questions']) if published.exists() and json.loads(published.read_text()) == expected else 0
    checked_count = len({q['id'] for q in [golden, *questions]})
    leaves = coverage()
    for row in leaves:
        if row['id'] == spec['family']['subcategory']:
            row.update(family_specs=1, pilot_instances=len(questions),
                       skill_ids=[spec['family']['skill_id']], family_ids=[spec['family']['id']],
                       drafted_instances=checked_count, mathematically_checked_instances=checked_count,
                       integrated_four_level_reps=integrated,
                       gaps=['taxonomy_review', 'additional_task_families', 'teaching_and_format_review']+([] if integrated else ['four_level_runtime']))
    report = dict(status='authoring_verified; runtime_pack_reproduced' if integrated else 'authoring_verified; runtime_pack_missing_or_changed',
                  instances=len(questions), mathematical_steps=len(questions)*2,
                  wrong_choices_checked=len(questions)*4, strata=dict(Counter(q['stratum'] for q in questions)),
                  cohorts=dict(Counter(q['cohort'] for q in questions)), four_support_levels_per_instance=4,
                  practice_leaves=len(leaves), named_subcategories=sum(r['kind']=='named_subcategory' for r in leaves),
                  terminal_chapters=sum(r['kind']=='terminal_chapter' for r in leaves),
                  integrated_four_level_reps=integrated, runtime_gate='See build/four-level-evidence/verification.json',
                  visual_acceptance='pending', windows=0, captures=0,
                  authoring_spec_sha256=hashlib.sha256(SPEC.read_bytes()).hexdigest())
    args.output.mkdir(parents=True, exist_ok=True)
    outputs = {'pilot_questions.json': dict(publication='authoring_only', questions=questions),
               'golden_question.json': golden,
               'golden_views.json': {level: project(spec, golden, level) for level in PROFILE_IDS},
               'coverage.json': leaves, 'verification.json': report}
    for name, data in outputs.items():
        (args.output/name).write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
