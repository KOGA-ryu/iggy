#!/usr/bin/env python3
"""Build a bounded teaching-family example through the existing six-role gate.

Run from Paths: PYTHONPATH=tools python3 -B content/authoring/learning/linear_family/generate.py
The generator knows seed solutions; certificates derive answers from original
equations. This file is also the explicit Python certificate provider loaded by
check_authoring_pilot.check_assignment. It is never loaded by the game.
"""
import argparse
from fractions import Fraction
import json
from pathlib import Path
import sys

import build_question_batch as batch
import check_authoring_pilot as pilot
import export_learning as export

SOURCE = batch.ROOT / 'content/authoring/learning/linear_family'
SETS = ('sample', 'practice', 'fresh_check')
SOURCES = ('recipe.json', 'DESIGN.md', 'lesson.md.in', 'questions.paths.md.in', 'generate.py', 'tests.py')
PREFIX = 'linear_family_v1'


def require(ok, message):
    export.require(ok, 'family.linear', message)


def equation(a, b, c, reverse=False):
    text = batch.linear.equation(dict(a=a, b=b, c=str(c)))
    return '='.join(reversed(text.split('='))) if reverse else text


def original(q):
    case = batch.case_fields(q, ('a', 'b', 'c', 'reversed'))
    a, b, c = (case[k] for k in ('a', 'b', 'c'))
    require(all(type(v) is int for v in (a, b, c)) and type(case['reversed']) is bool,
            'Original coefficients must be integers; reversed must be boolean')
    require(2 <= abs(a) <= 8 and abs(b) <= 9 and abs(c) <= 100,
            'Expected 2 <= |a| <= 8, |b| <= 9 and |c| <= 100')
    # This calculation never receives the generator's intended solution.
    solution = Fraction(c - b, a)
    require(a * solution + b == c, 'Derived solution must satisfy the original equality')
    return a, b, c, solution


def original_context(q):
    a, b, c, solution = original(q)
    role = q['role']; reverse = q['case']['reversed']
    conditions = {
        'read_notation': a > 0 and b > 0 and a != b and not reverse,
        'worked_check': a > 0 and b < 0 and not reverse,
        'choose_next_step': a < 0 and b > 0 and not reverse,
        'explain_step': a < 0 and b == 0 and c != 0 and not reverse,
        'repair_error': a > 0 and b < 0 and not reverse,
        'independent': a > 0 and b > 0 and reverse and solution.denominator == 2,
    }
    require(conditions.get(role, False), f'{role}: original case misses its planned variation')
    given = equation(a, b, c, reverse)
    reached = equation(a, 0, c-b)
    facts = dict(original=q['case'], solution=str(solution), residual='0',
                 uniqueness='For two solutions u,v, subtraction gives a(u-v)=0; nonzero a implies u=v.')
    return a, b, c, solution, given, reached, facts


def read_notation(q):
    a, b, c, solution, given, reached, facts = original_context(q)
    labels = [f'({a},{b})', f'({a},{-b})', f'({b},{a})']
    after = ['(a,b)='+labels[0]]
    steps = [(labels, labels[0])]
    facts['ordered_pair'] = [a, b]
    return given, after, steps, facts


def worked_check(q):
    a, b, c, solution, given, reached, facts = original_context(q)
    labels = list(map(str, (c-b, c, c+b)))
    after = [reached]; steps = [(labels, labels[0])]
    facts['right_side_errors'] = {'one_side_only': c, 'add_constant_again': c+b}
    return given, after, steps, facts


def choose_next_step(q):
    a, b, c, solution, given, reached, facts = original_context(q)
    triples = [(a, 0, c-b), (a, 2*b, c+b), (a, 0, c)]
    labels = [equation(*t) for t in triples]
    residuals = [p*solution+offset-rhs for p, offset, rhs in triples]
    require(residuals == [0, 0, -b], 'Goal contrast must separate equivalence from cancellation')
    require(triples[1][1] != 0, 'Valid distractor must miss the zero-constant goal')
    after = [reached]; steps = [(labels, labels[0])]
    facts['candidate_residuals'] = list(map(str, residuals))
    facts['valid_but_misses_goal'] = labels[1]
    return given, after, steps, facts


def explain_step(q):
    a, b, c, solution, given, reached, facts = original_context(q)
    given += rf'\quad\Longrightarrow\quad x={batch.tex(solution)}'
    labels = [rf'\times({a})', rf'\div({a})', r'\times0']
    factors = (Fraction(a), Fraction(1, a), Fraction(0))
    recovered = [[str(f), str(f*solution)] for f in factors]
    require(recovered[0] == [str(a), str(c)] and recovered[1] != recovered[0],
            'Only multiplication by the divisor recovers the original coefficients')
    after = [equation(a, 0, c)]; steps = [(labels, labels[0])]
    facts['recovered_coefficient_and_constant'] = recovered
    return given, after, steps, facts


def repair_error(q):
    a, b, c, solution, given, reached, facts = original_context(q)
    wrong_rhs = c+b
    wrong_solution = Fraction(wrong_rhs, a)
    require(a*wrong_solution+b-c == 2*b != 0, 'Incorrect attempt must fail the original equation')
    given = (rf'\begin{{gathered}}{given}\\\begin{{aligned}}'
             rf'L_1 &: {a}x={c}+({b})\\L_2 &: {a}x={wrong_rhs}\\'
             rf'L_3 &: x={batch.tex(wrong_solution)}\end{{aligned}}\end{{gathered}}')
    after = [r'L_1', reached]
    steps = [([r'L_1', r'L_2', r'L_3'], r'L_1'),
             (list(map(str, (c-b, c+b, c))), str(c-b))]
    facts.update(first_error='L_1', subsequent_arithmetic_valid=True,
                 wrong_solution=str(wrong_solution), wrong_residual=str(2*b))
    return given, after, steps, facts


def independent(q):
    a, b, c, solution, given, reached, facts = original_context(q)
    candidates = (solution, -solution, Fraction(c-b))
    residuals = [a*x+b-c for x in candidates]
    require(residuals[0] == 0 and all(v != 0 for v in residuals[1:]),
            'Each fresh-problem distractor must fail substitution')
    labels = ['x='+batch.tex(v) for v in candidates]
    after = [labels[0]]; steps = [(labels, labels[0])]
    facts.update(candidate_values=list(map(str, candidates)), candidate_residuals=list(map(str, residuals)))
    return given, after, steps, facts


CHECKERS = {
    'read_notation': read_notation,
    'worked_check': worked_check,
    'choose_next_step': choose_next_step,
    'explain_step': explain_step,
    'repair_error': repair_error,
    'independent': independent,
}


def make_sequence(recipe, selected):
    require(set(recipe) == {'format', 'version', 'sets', 'roles'} and
            recipe['format'] == 'paths_linear_family_example' and type(recipe['version']) is int
            and recipe['version'] == 1, 'Unsupported family recipe fields/version')
    require(type(recipe['sets']) is list and all(type(s) is dict for s in recipe['sets'])
            and [s.get('id') for s in recipe['sets']] == list(SETS), 'Supply the three named sets in order')
    require(type(recipe['roles']) is list and all(type(r) is dict for r in recipe['roles'])
            and [r.get('role') for r in recipe['roles']] == list(batch.EXERCISE_ROLES), 'Supply all six ordered roles')
    for seed in recipe['sets']:
        require(set(seed) == {'id', 'a', 'b', 'solution', 'fresh_solution'}, 'Unexpected seed field')
        a, b, s = (seed[k] for k in ('a', 'b', 'solution'))
        require(all(type(v) is int for v in (a, b, s)) and a in (2, 4, 6, 8)
                and 1 <= b <= 9 and a != b and 2 <= s <= 5, 'Seed outside the bounded family')
        require(type(seed['fresh_solution']) is str and seed['fresh_solution'] in ('-3/2', '-1/2', '1/2', '3/2'),
                'Fresh solution must be a nonzero half integer in the supported range')
    seed = next((s for s in recipe['sets'] if s['id'] == selected), None)
    require(seed is not None, 'Unknown set')
    a, b, s = (seed[k] for k in ('a', 'b', 'solution'))
    # Construct from a known solution. Only the derived original equation is
    # passed across the independent certificate boundary.
    cases = {
        'read_notation': (a, b, s, False),
        'worked_check': (a, -b, s, False),
        'choose_next_step': (-a, b, s, False),
        'explain_step': (-a, 0, -s, False),
        'repair_error': (a, -b, s, False),
        'independent': (a, b, Fraction(seed['fresh_solution']), True),
    }
    questions = []
    for meta in recipe['roles']:
        require(set(meta) == {'role', 'title', 'objective', 'prerequisites', 'variation'}
                and all(type(v) is str and v.strip() for v in meta.values()), 'Incomplete role teaching metadata')
        coefficient, offset, wanted, reverse = cases[meta['role']]
        rhs = coefficient*wanted+offset
        require(Fraction(rhs).denominator == 1, 'This recipe requires integral original constants')
        case = dict(a=coefficient, b=offset, c=int(rhs), reversed=reverse)
        identity = PREFIX+'_'+selected+'_'+meta['role']+'_'+export.sha(export.encoded(case))[:12]
        questions.append(dict(id=identity, role=meta['role'], title=meta['title'], objective=meta['objective'],
                              prerequisites=meta['prerequisites'], case=case))
    sequence = dict(format='paths_exercise_roles', format_version=1, questions=questions)
    batch.validate_role_sequence(sequence, SOURCE/'recipe.json')
    return sequence


def presentation(sequence, selected):
    questions = sequence['questions']
    key = export.sha(export.encoded([q['case'] for q in questions]))[:12]
    values = dict(subject='algebra', subject_title='Algebra', chapter='worked_linear_practice',
                  chapter_title='Worked linear practice', reading_id=PREFIX+'_'+selected+'_'+key+'_reading',
                  reading_title='Balanced equations: '+selected.replace('_', ' '))
    for q in questions:
        role = q['role']; a, b, c, s = original(q)
        checked = batch.reasoning_certificate(q, CHECKERS)['expected']
        fields = dict(given=checked['given'], a=str(a), b=str(b), c=str(c),
                      opposite_b=str(-b), twice_b=str(2*b), c_plus_b=str(c+b),
                      rhs=str(c-b), solution=str(s),
                      neg_candidate_lhs=str(a*(-s)+b), undivided_lhs=str(a*(c-b)+b))
        for number, (after, step) in enumerate(zip(checked['after'], checked['steps']), 1):
            # Semantic IDs stay attached to answer/feedback. Only presentation
            # order is shuffled; a question's original case fixes that order.
            options = [(number*10+i+1, label) for i, label in enumerate(step['choices'])]
            require(step['answer'] == options[0][1], 'Family option 1 must be the certified target')
            options.sort(key=lambda option: export.sha(export.encoded([q['id'], number, option[0]])))
            fields['choices_'+str(number)] = '\n'.join(f'@choice {i} | {label}' for i, label in options)
            fields['choices_'+str(number)] += f'\n@answer {number*10+1}'
            fields['after_'+str(number)] = after
        values.update({role+'_'+name: value for name, value in fields.items()})
    return values


def build(selected, target, model):
    pilot.packet()  # Preserve the existing writers' frozen reference contract.
    inputs = {name: export.read_bytes(SOURCE/name) for name in SOURCES}
    recipe = export.decoded(inputs['recipe.json'])
    sequence = make_sequence(recipe, selected)
    values = presentation(sequence, selected)
    author = dict(format='paths_learning_authoring', format_version=1,
                  package_id=PREFIX+'_'+selected, package_version=1,
                  sources=[dict(id=PREFIX+'_'+selected+'_source', kind='generated',
                      title='Original balanced-equation teaching family', uri='paths:generated/'+PREFIX,
                      revision='1', attribution='Original Paths questions, lesson, variation plan and exact arithmetic certificates.',
                      reuse='Original project material; no external exercise text copied.',
                      content_ids=[values['reading_id'], *[q['id'] for q in sequence['questions']]])])
    source_hashes = {name: export.sha(data) for name, data in inputs.items()}
    digest = export.sha(export.encoded(dict(inputs=source_hashes, selected=selected)))
    root = batch.ROOT/'build/linear-family-example'/selected/digest
    source = root/'source'
    export.immutable_directory(source, {'sequence.json': export.encoded(sequence),
        'lesson.md.in': inputs['lesson.md.in'], 'questions.paths.md.in': inputs['questions.paths.md.in'],
        'certificates.py': inputs['generate.py'], 'authoring.json': export.encoded(author)})
    assignment = dict(subject='algebra', folder=str(source.relative_to(batch.ROOT)),
                      package_id=author['package_id'], values=values)
    # Reuse the complete existing candidate path: compiler, provenance,
    # certificates, all wrong choices, retained working and save replay.
    result = pilot.check_assignment(assignment, target, model)
    require(source_hashes == {name: export.sha(export.read_bytes(SOURCE/name)) for name in SOURCES},
            'Family source changed during verification; regenerate before using this candidate')
    report = dict(result, family_source_sha256=source_hashes, set=selected,
                  variation_plan=[dict(question_id=q['id'], role=q['role'], variation=meta['variation'])
                                  for q, meta in zip(sequence['questions'], recipe['roles'])],
                  teaching_review='Automated gates passed; see DESIGN.md for prose review and limits.',
                  review_schedule='not_implemented', learner_evidence='not_collected')
    receipt = export.encoded(report)
    evidence = root/'checks'/export.sha(receipt)
    export.immutable_directory(evidence, {'verification.json': receipt})
    return dict(report, verification=str(evidence/'verification.json'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--set', choices=SETS, default='sample')
    parser.add_argument('--target', type=Path, default=batch.ROOT/'b/sorter')
    parser.add_argument('--model', type=Path, default=batch.ROOT/'b/paths_learning_document_tests')
    args = parser.parse_args()
    try:
        print(json.dumps(build(args.set, args.target, args.model), indent=2))
        return 0
    except (export.ExportError, ValueError, OSError, KeyError, TypeError) as error:
        print(json.dumps(dict(accepted=False, code=getattr(error, 'code', 'family.invalid'), message=str(error)), indent=2))
        return 1


if __name__ == '__main__':
    sys.exit(main())
