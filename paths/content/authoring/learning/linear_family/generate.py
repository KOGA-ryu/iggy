#!/usr/bin/env python3
"""Bounded linear-family mathematics for tools/author_question_family.py.

Seed solutions construct originals; certificates derive answers from originals.
The shared runner owns templates, choice serialization, checking and packaging.
This reviewed provider is never selected by an author-supplied Python path.
"""
from fractions import Fraction

import build_question_batch as batch
import export_learning as export

SOURCE = batch.ROOT / 'content/authoring/learning/linear_family'
SUBJECT = 'algebra'
SETS = ('sample', 'practice', 'fresh_check')
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


def make_sequence(recipe, selected, prefix=PREFIX):
    require(type(recipe) is dict and set(recipe) == {'sets', 'roles'},
            '/parameters: supply exactly sets and roles')
    require(type(recipe['sets']) is list and all(type(s) is dict for s in recipe['sets'])
            and [s.get('id') for s in recipe['sets']] == list(SETS), 'Supply the three named sets in order')
    require(type(recipe['roles']) is list and all(type(r) is dict for r in recipe['roles'])
            and [r.get('role') for r in recipe['roles']] == list(batch.EXERCISE_ROLES), 'Supply all six ordered roles')
    for index, seed in enumerate(recipe['sets']):
        field = f'/sets/{index}'
        require(set(seed) == {'id', 'a', 'b', 'solution', 'fresh_solution'}, field + ': unexpected seed field')
        a, b, s = (seed[k] for k in ('a', 'b', 'solution'))
        require(type(a) is int and a in (2, 4, 6, 8), field + '/a: use an even integer from 2 through 8')
        require(type(b) is int and 1 <= b <= 9 and a != b,
                field + '/b: use an integer from 1 through 9 different from a')
        require(type(s) is int and 2 <= s <= 5, field + '/solution: use an integer from 2 through 5')
        require(type(seed['fresh_solution']) is str and seed['fresh_solution'] in ('-3/2', '-1/2', '1/2', '3/2'),
                field + '/fresh_solution: use one of -3/2, -1/2, 1/2, 3/2 as a string')
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
        identity = prefix+'_'+selected+'_'+meta['role']+'_'+export.sha(export.encoded(case))[:12]
        questions.append(dict(id=identity, role=meta['role'], title=meta['title'], objective=meta['objective'],
                              prerequisites=meta['prerequisites'], case=case))
    sequence = dict(format='paths_exercise_roles', format_version=1, questions=questions)
    batch.validate_role_sequence(sequence, SOURCE/'recipe.json')
    return sequence


def presentation(sequence):
    questions = sequence['questions']
    values = {}
    for q in questions:
        role = q['role']; a, b, c, s = original(q)
        fields = dict(a=str(a), b=str(b), c=str(c),
                      opposite_b=str(-b), twice_b=str(2*b), c_plus_b=str(c+b),
                      rhs=str(c-b), solution=str(s),
                      neg_candidate_lhs=str(a*(-s)+b), undivided_lhs=str(a*(c-b)+b))
        values.update({role+'_'+name: value for name, value in fields.items()})
    return values


def prepare(parameters, prefix):
    """Three deliberate six-role groups, with exact fields for their teaching."""
    groups = [(sequence['questions'], dict(presentation(sequence), set_title=selected.replace('_', ' ')))
              for selected in SETS
              for sequence in (make_sequence(parameters, selected, prefix),)]
    signatures = [(q['role'], export.encoded(q['case'])) for questions, _ in groups for q in questions]
    require(len(signatures) == len(set(signatures)),
            '/sets: practice and fresh checks must use different originals for the same role')
    return groups
