"""Independent certificates for the bounded quadratic difference-quotient pilot."""
from math import isfinite

from build_question_batch import require as _batch_require


def require(condition, code, message):
    """Keep family diagnostics specific while using the shared math rejection route."""
    _batch_require(condition, f'{code}: {message}')


def _case(record):
    case = record['case']
    require(set(case) == {'coefficients', 'at'}, 'calculus.case', 'Expected coefficients and at in the original case')
    coefficients, at = case['coefficients'], case['at']
    require(type(coefficients) is list and len(coefficients) == 3, 'calculus.degree', 'Expected exactly [A, B, C] for a quadratic')
    require(all(type(value) is int and abs(value) <= 9 for value in coefficients), 'calculus.coefficients', 'Quadratic coefficients must be bounded integers')
    require(type(at) is int and abs(at) <= 3 and isfinite(at), 'calculus.at', 'The real evaluation point must be a bounded finite integer')
    return coefficients[0], coefficients[1], coefficients[2], at


def _polynomial_value(a, b, c, x):
    return a * x * x + b * x + c


def _difference_coefficients(a, b, c, point):
    """Expand f(point+h)-f(point) as constant, h, h^2 coefficients."""
    shift = (point, 1)  # The polynomial point+h, in ascending powers of h.
    expanded = [0, 0, 0]
    for i, left in enumerate(shift):
        for j, right in enumerate(shift):
            expanded[i+j] += a * left * right
    for i, value in enumerate(shift):
        expanded[i] += b * value
    expanded[0] += c - _polynomial_value(a, b, c, point)
    return tuple(expanded)


def _quotient_data(record):
    a, b, c, point = _case(record)
    constant, linear, quadratic = _difference_coefficients(a, b, c, point)
    require(constant == 0, 'calculus.expansion', 'The difference must factor by h')
    derivative_from_expansion = linear
    # Differentiate [C,B,A] by degree, then evaluate. This does not use the
    # shifted-product expansion above, so either path can catch the other's error.
    derivative_from_coefficients = sum(degree * coefficient * point**(degree-1)
        for degree, coefficient in enumerate((c, b, a)) if degree)
    require(derivative_from_expansion == derivative_from_coefficients and quadratic == a,
            'calculus.derivative', 'Expansion and coefficient differentiation disagree')
    return a, b, c, point, linear, quadratic


def _definition(record):
    a, b, c, point = _case(record)
    require((a, b, c, point) == (1, 0, 0, 3), 'calculus.read_notation', 'This role is reserved for f(x)=x^2 at 3')
    given = r'f(x)=x^2'
    answer = r'\lim_{h\to0}\frac{f(3+h)-f(3)}{h}'
    return given, [r"f'(3)=" + answer], [([answer, r'\lim_{h\to0}\frac{h}{f(3+h)-f(3)}', r'f(3)'], answer)], {
        'function_value': 9,
        'increment_domain': 'real h, h != 0 in quotient',
        'definition': 'output-change divided by input-increment limit',
    }


def _worked_limit(record):
    a, b, c, point, linear, quadratic = _quotient_data(record)
    require((a, b, c, point, linear, quadratic) == (1, 0, 0, 2, 4, 1), 'calculus.worked_check', 'This role is reserved for x^2 at 2')
    given = r'\begin{gathered}f(x)=x^2,\quad a=2\\\frac{f(2+h)-f(2)}{h}=4+h\quad(h\ne0)\end{gathered}'
    return given, [r"f'(2)=4"], [(['0', '4', '4+h'], '4')], {
        'quotient_polynomial': '4+h',
        'limit': 4,
        'coefficient_derivative': '2x at x=2',
    }


def _factor_method(record):
    a, b, c, point, linear, quadratic = _quotient_data(record)
    require((a, b, c, point, linear, quadratic) == (1, 0, 0, 3, 6, 1), 'calculus.choose_next_step', 'This role is reserved for x^2 at 3')
    given = r'\begin{gathered}f(x)=x^2,\quad a=3\\\frac{f(3+h)-f(3)}{h}=\frac{(3+h)^2-9}{h}\end{gathered}'
    factor = r'\frac{h(6+h)}{h}'
    return given, [r'\frac{f(3+h)-f(3)}{h}=6+h,\quad h\ne0'], [([r'\frac{6+h}{h}', r'6h+h^2', factor], factor)], {
        'expanded_numerator': '6h+h^2',
        'factored_numerator': 'h(6+h)',
        'cancellation_condition': 'h != 0',
    }


def _cancellation_reason(record):
    a, b, c, point, linear, quadratic = _quotient_data(record)
    require((a, b, c, point, linear, quadratic) == (1, 0, 0, 3, 6, 1), 'calculus.explain_step', 'This role is reserved for x^2 at 3')
    given = r'\frac{h(6+h)}{h}=6+h'
    answer = r'h\ne0'
    return given, [r'\frac{h(6+h)}{h}=6+h,\quad h\ne0'], [([r'h=0', r'\text{all real }h', answer], answer)], {
        'condition': 'h != 0',
        'at_zero': 'original quotient undefined',
        'nearby_limit': 6,
    }


def _repair(record):
    a, b, c, point, linear, quadratic = _quotient_data(record)
    require((a, b, c, point, linear, quadratic) == (2, 1, 0, 1, 5, 2), 'calculus.repair_error', 'This role is reserved for 2x^2+x at 1')
    given = (r'\begin{gathered}f(x)=2x^2+x,\quad a=1\\'
             r'\begin{aligned}L_1 &: f(1+h)=2(1+2h+h^2)+1+h\\'
             r'L_2 &: f(1+h)=3+4h+2h^2\\'
             r"L_3 &: f'(1)=\lim_{h\to0}\frac{4h+2h^2}{h}=4\end{aligned}\end{gathered}")
    return given, [r'L_2:\quad f(1+h)=3+5h+2h^2', r"f'(1)=5"], [(['L2', 'L1', 'L3'], 'L2'), (['4', '3', '5'], '5')], {
        'first_error': 'L2',
        'corrected_expansion': '3+5h+2h^2',
        'corrected_quotient': '5+2h',
        'derivative': 5,
    }


def _independent(record):
    a, b, c, point, linear, quadratic = _quotient_data(record)
    require((a, b, c, point, linear, quadratic) == (3, -2, 1, 2, 10, 3), 'calculus.independent', 'This role is reserved for 3x^2-2x+1 at 2')
    given = r'f(x)=3x^2-2x+1'
    return given, [r"f'(2)=10"], [(['9', '10', '12'], '10')], {
        'function_value': _polynomial_value(a, b, c, point),
        'difference_quotient': '3h+10 for h != 0',
        'coefficient_derivative': '6x-2 at x=2',
    }


CHECKERS = {
    'read_notation': _definition,
    'worked_check': _worked_limit,
    'choose_next_step': _factor_method,
    'explain_step': _cancellation_reason,
    'repair_error': _repair,
    'independent': _independent,
}
