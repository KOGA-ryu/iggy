"""Adapt editable sine seeds to the reviewed Wave 01 exact mathematics."""
from fractions import Fraction
import importlib.util
from pathlib import Path

import build_question_batch as batch
import export_learning as export

SOURCE = Path(__file__).resolve().parent
SUBJECT = 'trigonometry'
DOMAIN = dict(units='radians', lower='0', upper='2', include_lower=True, include_upper=False)
LEVELS = ('-1', '-1/2', '0', '1/2', '1')
ROLE_LEVELS = {
    'read_notation': ('-1', '-1/2', '1/2', '1'),
    'worked_check': ('-1/2', '0', '1/2'),
    'choose_next_step': LEVELS,
    'explain_step': ('-1/2', '1/2'),
    'repair_error': LEVELS,
    'independent': LEVELS,
}

# The published source stays frozen. Use its pure mathematical functions;
# never call its recipe reader, presentation, packaging or publication routes.
spec = importlib.util.spec_from_file_location('paths_reviewed_sine_math',
    batch.ROOT / 'content/authoring/production/wave01/trigonometry/generate.py')
math = importlib.util.module_from_spec(spec)
spec.loader.exec_module(math)
CHECKERS = math.CHECKERS


def require(ok, pointer, message):
    export.require(ok, 'sine.recipe', pointer + ': ' + message)


def original_case(seed, role, pointer):
    require(type(seed) is dict and set(seed) == {'level', 'coefficient_sign', 'offset', 'known_branch'},
            pointer, 'Supply level, coefficient_sign, offset and known_branch')
    raw, sign, b, branch = (seed[k] for k in ('level', 'coefficient_sign', 'offset', 'known_branch'))
    require(type(raw) is str and raw in ROLE_LEVELS[role], pointer+'/level',
            'For '+role+' use one of '+', '.join(ROLE_LEVELS[role])+' as exact text')
    require(type(sign) is int and sign in (-1, 1), pointer+'/coefficient_sign', 'Use integer -1 or 1')
    require(type(b) is int and -2 <= b <= 2, pointer+'/offset', 'Use an integer from -2 through 2')
    level = Fraction(raw)
    a = (2 if level.denominator == 2 else 1) * sign
    require(role != 'choose_next_step' or a != 1, pointer+'/coefficient_sign',
            'Use -1 at integer levels so removing the constant still leaves a multiplier to isolate')
    branches = math.solutions('sine_turn', level)
    require(type(branch) is int and 0 <= branch < len(branches), pointer+'/known_branch',
            'Select a zero-based branch index below '+str(len(branches)))
    require(role in ('worked_check', 'explain_step') or branch == 0, pointer+'/known_branch',
            'Use 0 where no known angle is displayed')
    # Construct from the seed height; certificates receive only A/B/C and the
    # known angle, and derive the height again from the original equation.
    return dict(family='sine_turn', a=a, b=b, c=int(a*level+b), alpha_pi=str(branches[branch]))


def presentation(questions):
    """Numeric fields for the unchanged reviewed teaching; no prose or keys."""
    values = {}
    for q in questions:
        f, a, b, c, level, alpha, branches, symbol, name, coord, reflection, given, answers = math.context(q)
        reflected = 1-alpha
        quarter = (alpha+Fraction(1,2))%2
        contrast = -level if level else Fraction(1,2)
        fields = dict(a=str(a), b=str(b), c=str(c), rhs=str(c-b), level=str(level),
            opposite=str(-level), shifted=str(level+Fraction(1,2)),
            answers=math.set_plain(answers), count=str(len(answers)), known=math.angle_plain(alpha),
            other=math.angle_plain(next((v for v in answers if v != alpha), alpha)),
            omitted=math.angle_plain(answers[-1]), wrong_angle=math.angle_plain(quarter),
            wrong_coordinate=math.coordinate_plain(f, quarter), reflected=math.angle_plain(reflected%2),
            reflected_raw=math.angle_plain(reflected), reflection_shift=math.angle_plain(reflected%2-reflected),
            halfturn=math.angle_plain((alpha+1)%2), other_reflection=math.angle_plain((2-alpha)%2),
            contrast=str(contrast), contrast_lhs=str(a*contrast+b), wrong_sign_lhs=str(a*(-level)+b),
            shifted_lhs=str(a*(level+Fraction(1,2))+b), endpoint_lhs=str(a*math.membership(f, Fraction(2))+b))
        values.update({q['role']+'_'+name: value for name, value in fields.items()})
    return values

