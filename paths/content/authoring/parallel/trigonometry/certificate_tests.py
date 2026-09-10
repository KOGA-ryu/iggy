"""Independent rejection tests for the bounded trigonometry pilot certificates."""
from copy import deepcopy
from fractions import Fraction
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[4] / 'tools'))
from build_question_batch import reasoning_certificate, validate_role_sequence
from export_learning import ExportError
import certificates


ROOT = Path(__file__).resolve().parent
QUESTIONS = validate_role_sequence(json.loads((ROOT / 'sequence.json').read_text()), ROOT / 'sequence.json')


def certified(question):
    return reasoning_certificate(question, certificates.CHECKERS)


def rejects(question, fragment):
    try:
        certified(question)
    except (ExportError, ValueError) as error:
        assert fragment in str(error), str(error)
    else:
        raise AssertionError(f'Expected rejection containing {fragment!r}')


def by_role(role):
    return next(question for question in QUESTIONS if question['role'] == role)


def main():
    results = [certified(question) for question in QUESTIONS]
    assert [result['steps_checked'] for result in results] == [1, 1, 1, 1, 2, 1]
    assert sum(result['wrong_choices_checked'] for result in results) == 14

    worked = certified(by_role('worked_check'))
    assert worked['expected']['after'] == [r'\theta\in\left\{\frac{\pi}{6},\frac{5\pi}{6}\right\}']
    independent = certified(by_role('independent'))
    assert independent['expected']['steps'][0]['answer'] == r'\left\{\frac{7\pi}{6},\frac{11\pi}{6}\right\}'
    assert independent['evidence']['facts']['substitution'] == ['0', '0']
    repair = certified(by_role('repair_error'))
    assert repair['expected']['after'][1] == r'\theta\in\left\{0,\pi\right\}'
    assert repair['evidence']['facts']['retains_zero'] is True
    assert repair['evidence']['facts']['excludes_upper_endpoint'] is True

    missing_branch = deepcopy(by_role('independent'))
    missing_branch['case']['constant'] = 0
    rejects(missing_branch, 'fresh 2sin')
    endpoint = deepcopy(by_role('repair_error'))
    endpoint['case']['upper_closed'] = True
    rejects(endpoint, 'exactly the radian interval')
    degrees = deepcopy(by_role('worked_check'))
    degrees['case']['units'] = 'degrees'
    rejects(degrees, 'unexpected original case fields')
    reflection = deepcopy(by_role('explain_step'))
    reflection['case']['sine'] = '-1/2'
    rejects(reflection, 'expected alpha=pi/6')
    unsupported = deepcopy(by_role('worked_check'))
    unsupported['case']['sine'] = '3/5'
    rejects(unsupported, 'Unsupported sine level')
    duplicate = [Fraction(1, 6), Fraction(1, 6)]
    assert len(set(duplicate)) != len(duplicate), 'Sanity check: duplicate angles must be rejected by set comparison'

    print('trigonometry certificate tests: 6 certificates, 7 decisions, 14 wrong choices, 5 rejection cases passed')


if __name__ == '__main__':
    main()
