#!/usr/bin/env python3
"""Independent arithmetic and refusal checks for pilot_linear_algebra_rows."""
from fractions import Fraction
import json
from pathlib import Path

import build_question_batch as batch
from certificates import CHECKERS


ROOT = Path(__file__).resolve().parent
QUESTIONS = json.loads((ROOT / 'sequence.json').read_text(encoding='utf-8'))['questions']


def solve(rows):
    """Cramer's rule and direct original-row substitution, independent of row operations."""
    (a, b, c), (d, e, f) = [[Fraction(v) for v in row] for row in rows]
    determinant = a * e - b * d
    assert determinant != 0
    pair = ((c * e - b * f) / determinant, (a * f - c * d) / determinant)
    assert a * pair[0] + b * pair[1] == c
    assert d * pair[0] + e * pair[1] == f
    return pair


def changed(question, **case_changes):
    clone = dict(question)
    clone['case'] = dict(question['case'])
    clone['case'].update(case_changes)
    return clone


def rejects(callable_):
    try:
        callable_()
    except (ValueError, ZeroDivisionError):
        return
    raise AssertionError('expected mathematical contract rejection')


def main():
    assert CHECKERS is batch.MATRIX_ROLE_CHECKERS
    assert [q['role'] for q in QUESTIONS] == list(batch.EXERCISE_ROLES)
    expected_pairs = [(5, 3), (2, 1), (5, 2), (4, 3), (Fraction(3, 2), Fraction(1, 2))]
    system_questions = QUESTIONS[1:]
    assert [solve(q['case']['rows']) for q in system_questions] == expected_pairs

    # Every provider derives its display and choice facts from the original case.
    certified = {q['role']: CHECKERS[q['role']](q) for q in QUESTIONS}
    assert certified['read_notation'][2][0][1] == '-3x+2y=7'
    assert certified['worked_check'][2][0][1] == '3'
    assert certified['choose_next_step'][2][0][1] == r'R_2\leftarrow R_2-3R_1'
    assert certified['explain_step'][2][0][1] == r'R_2\leftarrow R_2+2R_1'
    assert [decision[1] for decision in certified['repair_error'][2]] == ['L_1', '3']
    assert certified['independent'][3]['choice_residuals'] == [['-2', '-2'], ['0', '0'], ['9/2', '3/2']]

    # The required separate reading example: complete row arithmetic, inverse,
    # and direct original-equation check of (1,2).
    reading_rows = [[Fraction(1), Fraction(1), Fraction(3)], [Fraction(2), Fraction(3), Fraction(8)]]
    after_first = batch.operate(reading_rows, 'add_row_1_to_2', -2)
    assert after_first == [[1, 1, 3], [0, 1, 2]]
    after_second = batch.operate(after_first, 'add_row_2_to_1', -1)
    assert after_second == [[1, 0, 1], [0, 1, 2]]
    assert batch.operate(after_first, 'add_row_1_to_2', 2) == reading_rows
    assert solve(reading_rows) == (1, 2)

    # Boundary and altered-fact rejections. A changed original constant produces
    # a distinct certificate and therefore cannot match this candidate's given.
    altered_constant = changed(QUESTIONS[1], rows=[[1, -1, 2], [2, -1, 8]])
    assert CHECKERS['worked_check'](altered_constant)[0] != certified['worked_check'][0]
    rejects(lambda: CHECKERS['worked_check'](changed(QUESTIONS[1], rows=[[1, -1, 2], [2, -2, 4]])))
    rejects(lambda: CHECKERS['choose_next_step'](changed(QUESTIONS[2], rows=[[0, 2, 4], [3, 1, 7]])))
    rejects(lambda: CHECKERS['explain_step'](changed(QUESTIONS[3], multiplier=0)))
    rejects(lambda: CHECKERS['repair_error'](changed(QUESTIONS[4], rows=[[1, -1, 0], [2, -1, 5]])))

    # These candidate errors are rejected by the independently derived facts:
    # wrong destination, missing constant operation, altered reached row,
    # wrong key, reversed pair, and an equivalent correct distractor.
    method_choices, method_key = certified['choose_next_step'][2][0]
    assert r'R_1\leftarrow R_1-3R_2' in method_choices and method_key != r'R_1\leftarrow R_1-3R_2'
    repair_after = certified['repair_error'][1]
    assert r'5-2(1)' in repair_after[0] and r'\mid5' not in repair_after[1]
    assert repair_after[1] != r'\left[\begin{array}{cc|c}1&-1&1\\0&1&5\end{array}\right],\quad y=5'
    assert certified['repair_error'][2][1][1] != '5'
    transfer_choices, transfer_key = certified['independent'][2][0]
    assert transfer_choices[0] != transfer_key and transfer_choices[2] != transfer_key
    assert len(set(transfer_choices)) == 3
    print('linear algebra pilot certificate tests: passed')


if __name__ == '__main__':
    main()
