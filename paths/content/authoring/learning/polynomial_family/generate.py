"""Adapt editable polynomial cases to the reviewed Wave 01 exact mathematics."""
import importlib.util
from pathlib import Path

import build_question_batch as batch
import export_learning as export

SOURCE = Path(__file__).resolve().parent
SUBJECT = 'calculus'
NUMERIC_ROLES = ('worked_check', 'repair_error', 'independent')
WORKED_CASE = dict(family='polynomial_rules', coefficients=[1,-2,1,4], at=1)

# Published reproduction sources stay frozen. This adapter calls their pure
# mathematics, never their recipe, question serializer or packaging functions.
spec = importlib.util.spec_from_file_location('paths_reviewed_polynomial_math',
    batch.ROOT / 'content/authoring/production/wave01/calculus/generate.py')
math = importlib.util.module_from_spec(spec)
spec.loader.exec_module(math)
CHECKERS = math.CHECKERS


def require(ok, pointer, message):
    export.require(ok, 'polynomial.recipe', pointer + ': ' + message)


def original_case(seed, role, pointer):
    require(type(seed) is dict and set(seed) == {'coefficients', 'at'}, pointer,
            'Supply coefficients [A, B, C, D] and the evaluation point at')
    c, a = seed['coefficients'], seed['at']
    require(type(c) is list and len(c) == 4, pointer+'/coefficients', 'Supply four coefficients for Ax^3+Bx^2+Cx+D')
    for index, v in enumerate(c):
        require(type(v) is int and -5 <= v <= 5, pointer+f'/coefficients/{index}', 'Use an integer from -5 through 5')
    require(any(c[:-1]), pointer+'/coefficients', 'Use a nonconstant polynomial; A, B and C cannot all be zero')
    require(type(a) is int and -3 <= a <= 3, pointer+'/at', 'Use an integer from -3 through 3')
    case = dict(family='polynomial_rules', coefficients=list(c), at=a)
    require(case != WORKED_CASE, pointer, 'Keep the lesson worked example separate; choose another polynomial or point')
    q = dict(id=role, role=role, case=case)
    if role in NUMERIC_ROLES:
        try:
            math.numeric_errors(math.context(q))
        except export.ExportError as error:
            raise export.ExportError('polynomial.recipe', pointer+
                ': These inputs do not produce two distinct wrong calculations; change the polynomial or point') from error
    return case


def presentation(questions):
    """Reuse the reviewed calculated fields and named misconception values."""
    values = {}
    for q in questions:
        context = math.context(q)
        fields = {k:str(v) for k,v in context.items() if not isinstance(v, list)}
        if q['role'] in NUMERIC_ROLES:
            for code, error in zip(('a', 'b'), math.numeric_errors(context)):
                fields.update({'error_'+code+'_'+key:str(v) for key,v in error.items()})
        values.update({q['role']+'_'+name:value for name,value in fields.items()})
    return values

