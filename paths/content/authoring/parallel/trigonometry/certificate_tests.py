"""Independent rejection tests for the bounded trigonometry pilot certificates."""
from copy import deepcopy
from fractions import Fraction
import json
from pathlib import Path
import sys
from unittest.mock import patch

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
    for answers in ([Fraction(7,6)], [Fraction(7,6),Fraction(7,6)], [Fraction(7,6),Fraction(23,6)]):
        with patch('certificates._solutions',return_value=answers):
            try: certified(by_role('independent'))
            except ExportError: pass
            else: raise AssertionError('Missing, duplicated or out-of-interval solution accepted')
    for field,bad in (('lower_pi','1'),('upper_pi','3'),('lower_closed',False),('upper_closed',True)):
        q=deepcopy(by_role('choose_next_step'));q['case'][field]=bad
        rejects(q,'exactly the radian interval')
    q=deepcopy(by_role('independent'));q['case']['constant']=True
    rejects(q,'integer equation coefficients')
    top=certificates._solutions(Fraction(1),Fraction(0),Fraction(2))
    certificates._verify_membership(Fraction(1),top)
    assert top==[Fraction(1,2)], 'Separate worked example has one permitted angle'

    print('trigonometry certificate tests: 6 certificates, 7 decisions, 14 wrong choices, branch/endpoint/type mutations and top-point boundary passed')


if __name__ == '__main__':
    main()
