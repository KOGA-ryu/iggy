"""Adapt editable augmented rows to the reviewed exact row-operation mathematics."""
import importlib.util
from pathlib import Path

import build_question_batch as batch
import export_learning as export

SOURCE = Path(__file__).resolve().parent
SUBJECT = 'linear_algebra'
OPERATIONS = ('worked_check', 'explain_step', 'repair_error')
WORKED_ROWS = [[1,2,6], [3,-1,5]]
CHECKERS = batch.MATRIX_ROLE_CHECKERS

# Reuse calculated corrections without entering the frozen recipe, answer
# ordering or packaging routes. The shared runner owns all three sets.
spec = importlib.util.spec_from_file_location('paths_reviewed_row_math',
    batch.ROOT / 'content/authoring/production/wave01/linear_algebra/generate.py')
math = importlib.util.module_from_spec(spec)
spec.loader.exec_module(math)


def require(ok, pointer, message):
    export.require(ok, 'row.recipe', pointer + ': ' + message)


def original_case(seed, role, pointer):
    key = 'row' if role == 'read_notation' else 'rows'
    fields = {key, 'multiplier'} if role in OPERATIONS else {key}
    require(type(seed) is dict and set(seed) == fields, pointer, 'Supply exactly '+', '.join(sorted(fields)))
    rows = [seed[key]] if key == 'row' else seed[key]
    require(type(rows) is list and len(rows) == (1 if key == 'row' else 2), pointer+'/'+key,
            'Supply one augmented row' if key == 'row' else 'Supply two augmented rows')
    for i, row in enumerate(rows):
        location = pointer+'/'+key+('' if key == 'row' else '/'+str(i))
        require(type(row) is list and len(row) == 3, location, 'Each row has three entries [x coefficient, y coefficient, constant]')
        for j, value in enumerate(row):
            require(type(value) is int and -20 <= value <= 20, location+'/'+str(j), 'Use an integer from -20 through 20')
    if role in OPERATIONS:
        require(type(seed['multiplier']) is int and 0 < abs(seed['multiplier']) <= 6,
                pointer+'/multiplier', 'Use a nonzero integer multiplier from -6 through 6')
        require(rows[0][2] != 0, pointer+'/rows/0/2',
                'Use a nonzero source constant so the stated constant correction distinguishes the wrong operation')
    if key == 'row':
        b, c = rows[0][1:]
        require(b not in (0, c, -c), pointer+'/row',
                'Use a nonzero y coefficient different from both the constant and its opposite; keep each stated correction true')
    else:
        require(rows != WORKED_ROWS, pointer+'/rows', 'Keep the lesson worked example separate; choose another original system')
    case = {key: list(rows[0]) if key == 'row' else [list(row) for row in rows]}
    if role in OPERATIONS: case['multiplier'] = seed['multiplier']
    try:
        batch.reasoning_certificate(dict(id=role, role=role, case=case), CHECKERS)
    except export.ExportError as error:
        raise export.ExportError('row.recipe', pointer+': '+str(error)) from error
    return case


def presentation(questions):
    values = {}
    for q in questions:
        # row_fields accepts the frozen envelope; its set field is validation
        # metadata only. Raw checker order keeps feedback IDs semantic.
        wrapped = dict(q, case=dict(family='row_operations', set='teaching', inputs=q['case']))
        _, _, decisions, _ = CHECKERS[q['role']](q)
        fields = math.row_fields(wrapped, decisions)
        values.update({q['role']+'_'+name:value for name,value in fields.items()})
    return values
