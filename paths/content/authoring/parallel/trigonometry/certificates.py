"""Exact, bounded certificates for the trigonometry sine pilot; no runtime solver."""
from fractions import Fraction

from build_question_batch import require


def _fields(question, names):
    case = question['case']
    require(set(case) == set(names), f"{question['id']}: unexpected original case fields")
    return case


def _fraction(value, label):
    require(type(value) is str and len(value) <= 20, f'{label} must be bounded exact rational text')
    try:
        result = Fraction(value)
    except (TypeError, ValueError, ZeroDivisionError) as error:
        raise ValueError(f'{label} must be an exact rational string') from error
    return result


def _tex(value):
    value = Fraction(value)
    sign = '-' if value < 0 else ''
    value = abs(value)
    if value.denominator == 1:
        return sign + str(value.numerator)
    return sign + rf'\frac{{{value.numerator}}}{{{value.denominator}}}'


def _pi(value):
    value = Fraction(value)
    if value == 0:
        return '0'
    sign = '-' if value < 0 else ''
    value = abs(value)
    if value == 1:
        return sign + r'\pi'
    if value.denominator == 1:
        return sign + str(value.numerator) + r'\pi'
    if value.numerator == 1:
        return sign + rf'\frac{{\pi}}{{{value.denominator}}}'
    return sign + rf'\frac{{{value.numerator}\pi}}{{{value.denominator}}}'


def _set_pi(values):
    return r'\left\{'+','.join(_pi(value) for value in values)+r'\right\}'


def _bounds(question, case):
    lower = _fraction(case['lower_pi'], f"{question['id']} lower_pi")
    upper = _fraction(case['upper_pi'], f"{question['id']} upper_pi")
    require(lower == 0 and upper == 2 and case['lower_closed'] is True and case['upper_closed'] is False,
            f"{question['id']}: this pilot is exactly the radian interval [0,2pi)")
    return lower, upper


def _checked_interval(question, *, sine=None):
    names = ('lower_pi', 'upper_pi', 'lower_closed', 'upper_closed')
    if sine is not None:
        names = ('sine',) + names
    case = _fields(question, names)
    lower, upper = _bounds(question, case)
    level = _fraction(case['sine'], f"{question['id']} sine") if sine is not None else None
    return case, lower, upper, level


def _branches(level):
    """Exact one-turn angles theta/pi, established special values for this pilot."""
    table = {
        Fraction(1, 2): (Fraction(1, 6), Fraction(5, 6)),
        Fraction(-1, 2): (Fraction(7, 6), Fraction(11, 6)),
        # Zero has the two axis points in one turn; together they are theta=k*pi.
        Fraction(0): (Fraction(0), Fraction(1)),
        Fraction(1): (Fraction(1, 2),),
    }
    require(level in table, f'Unsupported sine level {_tex(level)} in this bounded pilot')
    return table[level]


def _inside(value, lower, upper):
    return lower <= value < upper


def _solutions(level, lower, upper):
    branches = _branches(level)
    # Integer shifts are derived from the actual interval; this finite range covers
    # every shift that can enter the fixed two-pi-wide interval.
    candidates = [branch + 2 * integer for branch in branches for integer in range(-2, 3)]
    answers = sorted({value for value in candidates if _inside(value, lower, upper)})
    require(answers, 'The bounded interval should retain at least one branch')
    for value in answers:
        reduced = value % 2
        require(reduced in branches, 'A retained angle must reduce to a proved one-turn branch')
    return answers


def _special_sine(angle):
    values = {Fraction(0): Fraction(0), Fraction(1, 6): Fraction(1, 2),
              Fraction(1, 2): Fraction(1), Fraction(5, 6): Fraction(1, 2),
              Fraction(1): Fraction(0), Fraction(7, 6): Fraction(-1, 2),
              Fraction(11, 6): Fraction(-1, 2)}
    reduced = Fraction(angle) % 2
    require(reduced in values, f'No proved special sine value for theta/pi={angle}')
    return values[reduced]


def _verify_membership(level, answers):
    require(len(answers) == len(set(answers)) and all(0 <= angle < 2 for angle in answers),
            'Solution set must have distinct angles inside [0,2pi)')
    require(all(_special_sine(angle) == level for angle in answers),
            'Exact special-angle membership failed')
    # The horizontal line meets the unit circle twice for -1<level<1,
    # and once at level 1. Membership plus that proved cardinality is complete.
    require(len(answers) == (1 if level == 1 else 2), 'Solution set is missing a valid branch')


def _completeness(level, branches, answers):
    return dict(
        level=str(level), one_turn_branches=[str(branch) for branch in branches],
        interval_solutions=[str(answer) for answer in answers],
        argument='The unit-circle horizontal line at this special height has exactly the stated one-turn intersections; adding 2k*pi lists every repeated turn, then [0,2pi) filters and deduplicates them.',
    )


def read_notation(question):
    _, lower, upper, _ = _checked_interval(question)
    given = rf'\theta\in[{_tex(lower)},{_tex(upper)}\pi)'
    answer = rf'{_tex(lower)}\le\theta<{_tex(upper)}\pi'
    choices = [answer, rf'{_tex(lower)}<\theta\le{_tex(upper)}\pi', rf'{_tex(lower)}<\theta<{_tex(upper)}\pi']
    return given, [answer], [(choices, answer)], dict(
        radians=True, lower_included=True, upper_excluded=True,
        interval_units='theta/pi', endpoint_direction_deduplicated=True)


def worked_check(question):
    # The worked case has one additional original input: its supplied first angle.
    case = _fields(question, ('sine', 'known_pi', 'lower_pi', 'upper_pi', 'lower_closed', 'upper_closed'))
    lower, upper = _bounds(question, case)
    level = _fraction(case['sine'], f"{question['id']} sine")
    known = _fraction(case['known_pi'], f"{question['id']} known_pi")
    branches = _branches(level); answers = _solutions(level, lower, upper)
    require(level == Fraction(1, 2) and known == Fraction(1, 6) and known in answers,
            f"{question['id']}: expected the pi/6, sine one-half worked case")
    _verify_membership(level, answers)
    other = next(value for value in answers if value != known)
    given = rf'\sin\theta={_tex(level)},\quad\theta={_pi(known)}'
    choices = [_pi(known), _pi(other), _pi(Fraction(7, 6))]
    return given, [rf'\theta\in{_set_pi(answers)}'], [(choices, _pi(other))], dict(
        known_angle=str(known), other_angle=str(other), membership=[str(_special_sine(v)) for v in answers],
        completeness=_completeness(level, branches, answers))


def choose_next_step(question):
    case = _fields(question, ('coefficient', 'constant', 'rhs', 'lower_pi', 'upper_pi', 'lower_closed', 'upper_closed'))
    _bounds(question, case)
    require(all(type(case[name]) is int for name in ('coefficient', 'constant', 'rhs')), 'Expected integer equation coefficients')
    a, b, rhs = (Fraction(case[name]) for name in ('coefficient', 'constant', 'rhs'))
    require(a == 2 and b == -1 and rhs == 0, f"{question['id']}: expected 2sin(theta)-1=0")
    isolated = (rhs - b) / a
    require(a != 0 and isolated == Fraction(1, 2), 'Exact rational isolation failed')
    given = r'2\sin\theta-1=0'; answer = rf'\sin\theta={_tex(isolated)}'
    choices = [r'\sin\theta=1', r'\sin\theta=-\frac{1}{2}', answer]
    return given, [answer], [(choices, answer)], dict(coefficient=str(a), constant=str(b), rhs=str(rhs), isolated_sine=str(isolated), reversible_divisor_nonzero=True)


def explain_step(question):
    case = _fields(question, ('alpha_pi', 'sine'))
    alpha = _fraction(case['alpha_pi'], f"{question['id']} alpha_pi")
    level = _fraction(case['sine'], f"{question['id']} sine")
    reflected, opposite, horizontal = 1 - alpha, alpha + 1, 2 - alpha
    require(alpha == Fraction(1, 6) and level == Fraction(1, 2), f"{question['id']}: expected alpha=pi/6, sine=1/2")
    require(_special_sine(alpha) == _special_sine(reflected) == level,
            'Vertical reflection must preserve the exact height')
    require(_special_sine(opposite) == _special_sine(horizontal) == -level,
            'Both requested distractors must have the opposite height')
    choices = [r'\alpha+\pi', r'2\pi-\alpha', r'\pi-\alpha']
    return rf'\alpha={_pi(alpha)},\quad\sin\alpha={_tex(level)}', [r'\sin(\pi-\alpha)=\sin(\alpha)=\frac{1}{2}'], [(choices, choices[2])], dict(
        alpha=str(alpha), reflected=str(reflected), sine_preserved=str(_special_sine(reflected)),
        opposite_sine=str(_special_sine(opposite)), horizontal_reflection_sine=str(_special_sine(horizontal)),
        reason='Reflection across the vertical axis changes horizontal coordinate only; sine is vertical coordinate.')


def repair_error(question):
    _, lower, upper, level = _checked_interval(question, sine=True)
    require(level == 0, f"{question['id']}: repair case must use sine zero")
    branches = _branches(level); answers = _solutions(level, lower, upper)
    _verify_membership(level, answers)
    require(answers == [Fraction(0), Fraction(1)], 'Zero branch must retain zero and pi only')
    k_values = [0, 1]
    given = (r'\begin{gathered}\sin\theta=0,\quad0\le\theta<2\pi\\'
             r'\text{Deliberately incorrect attempt:}\\'
             r'L_1:\ \theta=k\pi,\ k\text{ an integer}\\'
             r'L_2:\ k\in\{0,1,2\}\\L_3:\ \theta\in\{0,\pi,2\pi\}\end{gathered}')
    return given, [r'L_2:\quad k\in\{0,1\}', rf'\theta\in{_set_pi(answers)}'], [
        (['L_2', 'L_1', 'L_3'], 'L_2'),
        ([_set_pi([Fraction(1)]), _set_pi([Fraction(0), Fraction(1), Fraction(2)]), _set_pi(answers)], _set_pi(answers))], dict(
        first_error='L_2', accepted_k=k_values, excludes_upper_endpoint=True, retains_zero=True,
        completeness=_completeness(level, branches, answers))


def independent(question):
    case = _fields(question, ('coefficient', 'constant', 'rhs', 'lower_pi', 'upper_pi', 'lower_closed', 'upper_closed'))
    lower, upper = _bounds(question, case)
    require(all(type(case[name]) is int for name in ('coefficient', 'constant', 'rhs')), 'Expected integer equation coefficients')
    a, b, rhs = (Fraction(case[name]) for name in ('coefficient', 'constant', 'rhs'))
    require(a == 2 and b == 1 and rhs == 0 and lower == 0 and upper == 2 and case['lower_closed'] is True and case['upper_closed'] is False,
            f"{question['id']}: expected the fresh 2sin(theta)+1=0 case on [0,2pi)")
    level = (rhs - b) / a; branches = _branches(level); answers = _solutions(level, lower, upper)
    _verify_membership(level, answers)
    require(level == Fraction(-1, 2) and len(answers) == 2, 'Fresh case must have both negative-half branches')
    wrong_positive = _set_pi(_solutions(Fraction(1, 2), lower, upper))
    correct = _set_pi(answers); missing = _set_pi([answers[0]])
    given = r'2\sin\theta+1=0,\quad0\le\theta<2\pi'
    return given, [rf'\theta\in{correct}'], [([wrong_positive, correct, missing], correct)], dict(
        isolated_sine=str(level), substitution=[str(2 * _special_sine(angle) + 1) for angle in answers],
        completeness=_completeness(level, branches, answers))


CHECKERS = {
    'read_notation': read_notation,
    'worked_check': worked_check,
    'choose_next_step': choose_next_step,
    'explain_step': explain_step,
    'repair_error': repair_error,
    'independent': independent,
}
