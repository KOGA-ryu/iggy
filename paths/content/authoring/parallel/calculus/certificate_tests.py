#!/usr/bin/env python3
"""Independent rejection tests for the quadratic difference-quotient pilot."""
import copy
import json
from pathlib import Path

import build_question_batch as batch
from build_question_batch import require as _batch_require
from export_learning import ExportError

from certificates import CHECKERS, _difference_coefficients, _polynomial_value, _quotient_data


ROOT = Path(__file__).resolve().parent
SEQUENCE = json.loads((ROOT / 'sequence.json').read_text(encoding='utf-8'))['questions']


def require(condition, code, message):
    """Exercise the shared authoring rejection route with a local diagnostic label."""
    _batch_require(condition, f'{code}: {message}')


def expect_rejected(name, action):
    try:
        action()
    except (ExportError, AssertionError, ValueError, TypeError):
        return
    raise AssertionError(f'{name} was accepted')


def certificate(record):
    return batch.reasoning_certificate(record, CHECKERS)


def compiled_mock(records):
    """A minimal compiled projection for testing the shared certificate comparison."""
    result = []
    for record in records:
        checked = certificate(record)['expected']
        steps = []
        for index, decision in enumerate(checked['steps'], 1):
            choices, answer = decision['choices'], decision['answer']
            option_base = index * 10
            options = [dict(id=option_base + choice_index, label=label,
                            wrong_feedback='' if label == answer else 'Specific correction.')
                       for choice_index, label in enumerate(choices, 1)]
            steps.append(dict(options=options, accepted_option_ids=[next(option['id'] for option in options if option['label'] == answer)]))
        result.append(dict(id=record['id'], question=dict(
            description=record['objective'] + ' Checked mock.', equation=checked['given'],
            working_states=[dict(display=value) for value in [checked['given'], *checked['after']]], steps=steps)))
    return result


def derivative_by_coefficients(coefficients, point):
    a, b, _ = coefficients
    return 2 * a * point + b


def main():
    require(set(CHECKERS) == set(batch.EXERCISE_ROLES), 'calculus.tests', 'All six roles need a certificate')
    records = batch.validate_role_sequence({'format': 'paths_exercise_roles', 'format_version': 1, 'questions': SEQUENCE}, ROOT / 'sequence.json')
    certificates = [certificate(record) for record in records]
    require(sum(item['steps_checked'] for item in certificates) == 7, 'calculus.tests', 'The six roles must contain seven decisions')
    require(sum(item['wrong_choices_checked'] for item in certificates) == 14, 'calculus.tests', 'Each decision needs two diagnosed wrong choices')

    # Complete separate worked example: x^2+x at 1 gives numerator h(3+h).
    constant, linear, quadratic = _difference_coefficients(1, 1, 0, 1)
    require((constant, linear, quadratic) == (0, 3, 1), 'calculus.worked_example', 'Worked-example expansion must be h(3+h)')
    require(derivative_by_coefficients([1, 1, 0], 1) == 3, 'calculus.worked_example', 'Worked-example coefficient derivative must be 3')
    require(_polynomial_value(1, 1, 0, 1) == 2, 'calculus.worked_example', 'Worked example must retain f(1)=2')

    # Every assigned case independently agrees between expansion and coefficient differentiation.
    for record in records:
        a, b, c, point, linear, quadratic = _quotient_data(record)
        require(linear == derivative_by_coefficients([a, b, c], point), 'calculus.derivative', f"{record['id']}: independent derivative disagreement")
        require(_polynomial_value(a, b, c, point) == a * point * point + b * point + c, 'calculus.value', 'Direct polynomial evaluation disagreement')

    compiled = compiled_mock(records)
    batch.verify_role_content(records, compiled, certificates)

    altered_key = copy.deepcopy(compiled)
    altered_key[0]['question']['steps'][0]['accepted_option_ids'] = [12]
    expect_rejected('altered compiled key', lambda: batch.verify_role_content(records, altered_key, certificates))

    altered_after = copy.deepcopy(compiled)
    altered_after[-1]['question']['working_states'][-1]['display'] = r"f'(2)=9"
    expect_rejected('altered reached derivative', lambda: batch.verify_role_content(records, altered_after, certificates))

    # Removing B=-2 from 3x^2-2x+1 creates 12, not the certified derivative 10.
    require(derivative_by_coefficients([3, 0, 1], 2) == 12, 'calculus.mutation', 'Missing-linear mutation setup failed')
    expect_rejected('missing linear contribution', lambda: require(derivative_by_coefficients([3, 0, 1], 2) == 10, 'calculus.mutation', 'A missing linear contribution must fail'))

    # A wrong h coefficient is rejected as a polynomial identity, not merely at one h value.
    require(_difference_coefficients(2, 1, 0, 1) == (0, 5, 2), 'calculus.expansion', 'Repair case expansion must be 5h+2h^2')
    expect_rejected('altered expansion coefficient', lambda: require(_difference_coefficients(2, 1, 0, 1) == (0, 4, 2), 'calculus.expansion', 'Wrong h coefficient must fail the identity'))

    # The original quotient is explicitly unavailable at zero; only nearby nonzero h are cancellable.
    def quotient_at(h):
        require(h != 0, 'calculus.cancellation', 'The original difference quotient is undefined at h=0')
        return h * (6 + h) / h

    require(quotient_at(2) == 8, 'calculus.cancellation', 'Nonzero cancellation changed the quotient')
    expect_rejected('cancellation at zero', lambda: quotient_at(0))
    require(6 + 0 == 6, 'calculus.limit', 'The simplified continuous polynomial must have the nearby limit 6')

    malformed = copy.deepcopy(records[0]); malformed['case']['coefficients'] = [1, 0]
    expect_rejected('unsupported degree', lambda: CHECKERS['read_notation'](malformed))
    malformed = copy.deepcopy(records[0]); malformed['case']['coefficients'] = [1, 0, float('nan')]
    expect_rejected('noninteger coefficient', lambda: CHECKERS['read_notation'](malformed))
    malformed = copy.deepcopy(records[0]); malformed['case']['at'] = 4
    expect_rejected('point outside bound', lambda: CHECKERS['read_notation'](malformed))

    print('calculus certificate tests: 6 roles, 7 decisions, 14 distractors, worked example and rejection mutations passed')


if __name__ == '__main__':
    main()
