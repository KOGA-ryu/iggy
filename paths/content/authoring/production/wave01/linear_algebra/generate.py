#!/usr/bin/env python3
"""Build and independently certify both canonical Wave 01 Linear Algebra families."""
import argparse
from fractions import Fraction
import json
from pathlib import Path
import subprocess
import tempfile

import build_question_batch as batch
import check_authoring_pilot as pilot
import export_learning as export


WORKER = batch.ROOT / 'content/authoring/production/wave01/linear_algebra'
OUT = batch.ROOT / 'build/production/wave01/linear_algebra'
SETS = ('teaching', 'practice', 'fresh_check')
ROLES = tuple(batch.EXERCISE_ROLES)
ROLE_CODES = dict(zip(ROLES, ('rn', 'wc', 'ns', 'es', 're', 'in')))
POSITIONS = {
    'row_operations': {
        'teaching': (0, 1, 2, 2, 0, 1),
        'practice': (1, 2, 0, 0, 1, 2),
        'fresh_check': (2, 0, 1, 1, 2, 0),
    },
    'determinants_2x2': {
        'teaching': (1, 0, 2, 0, 2, 1),
        'practice': (2, 1, 0, 1, 0, 2),
        'fresh_check': (0, 2, 1, 2, 1, 0),
    },
}


def require(ok, message):
    export.require(ok, 'production.linear_algebra', message)


def tex(value):
    return batch.tex(Fraction(value))


def mat(a, b, c, d):
    return rf'\left[\begin{{array}}{{cc}}{a}&{b}\\{c}&{d}\end{{array}}\right]'


# A staged provider receives this metadata inside the immutable sequence.  It
# never infers a family or set from an ID or consults the mutable recipe.
def declared_question(question):
    require(question['role'] in ROLES, 'unknown exercise role')
    case = question['case']
    require(
        type(case) is dict and set(case) == {'family', 'set', 'inputs'},
        f'{question["id"]}: case must declare family, set, and inputs',
    )
    family = case['family']
    selected = case['set']
    inputs = case['inputs']
    require(family in ('row_operations', 'determinants_2x2'), 'unknown declared family')
    require(selected in SETS, 'unknown declared set')
    require(type(inputs) is dict and bool(inputs), 'declared inputs must be a nonempty object')
    original = dict(question)
    original['case'] = inputs
    return family, selected, original


def order(question, selected, choices, answer, step=1):
    require(len(choices) == 3 and len(set(choices)) == 3 and answer in choices,
            'ambiguous options')
    target = POSITIONS[question['case']['family']][selected][ROLES.index(question['role'])] if step == 1 else 2
    rest = sorted(
        (choice for choice in choices if choice != answer),
        key=lambda choice: export.sha(export.encoded([question['id'], step, choice])),
    )
    rest.insert(target, answer)
    return rest


def numeric(answer, *candidates):
    result = [answer]
    for item in candidates:
        if item not in result:
            result.append(item)
        if len(result) == 3:
            break
    require(len(result) == 3, 'role requires two distinct computed misconceptions')
    return list(map(str, result))


def determinant_statement(value):
    state = 'invertible' if value != '0' else 'singular'
    return rf'\det(A)={value};\ A\ \mathrm{{is\ {state}}}'


def row_certificate(question, selected=None, original=None):
    if original is None:
        family, selected, original = declared_question(question)
        require(family == 'row_operations', 'row certificate requires row-operations inputs')
    given, after, decisions, facts = batch.MATRIX_ROLE_CHECKERS[question['role']](original)
    return (
        given,
        after,
        [(order(question, selected, list(choices), answer, index), answer)
         for index, (choices, answer) in enumerate(decisions, 1)],
        facts,
    )


def determinant_facts(original):
    case = batch.case_fields(original, ('a', 'b', 'c', 'd'))
    require(
        all(type(value) is int and -12 <= value <= 12 for value in case.values()),
        'bounded integer matrix required',
    )
    a, b, c, d = (case[key] for key in ('a', 'b', 'c', 'd'))
    determinant = a * d - b * c
    facts = {
        'entries': [a, b, c, d],
        'determinant': str(determinant),
        'formula': 'ad-bc',
    }
    if determinant:
        inverse = [
            [Fraction(d, determinant), Fraction(-b, determinant)],
            [Fraction(-c, determinant), Fraction(a, determinant)],
        ]
        product = [
            [Fraction(a) * inverse[0][column] + Fraction(b) * inverse[1][column]
             for column in range(2)],
            [Fraction(c) * inverse[0][column] + Fraction(d) * inverse[1][column]
             for column in range(2)],
        ]
        require(product == [[1, 0], [0, 1]], 'inverse product failed')
        facts['inverse'] = [[str(value) for value in row] for row in inverse]
    else:
        vector = (
            (b, -a) if (a, b) != (0, 0)
            else ((d, -c) if (c, d) != (0, 0) else (1, 0))
        )
        require(
            vector != (0, 0)
            and a * vector[0] + b * vector[1] == c * vector[0] + d * vector[1] == 0,
            'null-vector witness failed',
        )
        facts['null_vector'] = list(vector)
    return facts


def determinant_certificate(question, selected=None, original=None):
    if original is None:
        family, selected, original = declared_question(question)
        require(family == 'determinants_2x2', 'determinant certificate requires determinant inputs')
    facts = determinant_facts(original)
    a, b, c, d = facts['entries']
    determinant = int(facts['determinant'])
    role = question['role']
    shown = mat(a, b, c, d)

    if role == 'read_notation':
        answer = str(determinant)
        choices = numeric(determinant, a * d + b * c, b * c - a * d, a * d)
        after = [rf'\det(A)={determinant}']
    elif role == 'worked_check':
        answer = str(determinant)
        choices = numeric(determinant, a * d + b * c, a * d)
        shown = rf'\det({shown})=({a})({d})-({b})({c})'
        after = [rf'\det(A)={determinant}']
    elif role == 'choose_next_step':
        values = numeric(determinant, a * d + b * c, b * c - a * d, a * d)
        answer = determinant_statement(values[0])
        choices = [answer, determinant_statement(values[1]), determinant_statement(values[2])]
        after = [rf'\det(A)={determinant}']
    elif role == 'explain_step':
        require(determinant != 0, 'row-swap contrast requires distinct nonzero determinants')
        shown = rf'A={shown},\quad B={mat(c, d, a, b)}\ \text{{(rows swapped)}}'
        choices = [r'\det(B)=-\det(A)', r'\det(B)=\det(A)', r'\det(B)=0']
        answer = choices[0]
        after = [rf'\det(B)={-determinant}']
    elif role == 'repair_error':
        shown = (
            rf'\begin{{gathered}}A={shown}\\'
            rf'\begin{{aligned}}L_1 &: \det(A)=({a})({d})+({b})({c})\\'
            rf'L_2 &: \det(A)={a * d + b * c}\\'
            rf'L_3 &: A\ \mathrm{{is\ invertible}}\end{{aligned}}\end{{gathered}}'
        )
        answer = str(determinant)
        return (
            shown,
            [r'L_1', rf'\det(A)=({a})({d})-({b})({c})={determinant}'],
            [
                (order(question, selected, [r'L_1', r'L_2', r'L_3'], r'L_1'), r'L_1'),
                (order(question, selected, numeric(determinant, a * d + b * c, a * d), answer, 2), answer),
            ],
            facts,
        )
    else:
        values = numeric(determinant, a * d + b * c, b * c - a * d, a * d)
        answer = determinant_statement(values[0])
        choices = [answer, determinant_statement(values[1]), determinant_statement(values[2])]
        after = [rf'\det(A)={determinant}']
    return shown, after, [(order(question, selected, choices, answer), answer)], facts


def certificate_for(question):
    family, selected, original = declared_question(question)
    if family == 'row_operations':
        return row_certificate(question, selected, original)
    return determinant_certificate(question, selected, original)


CHECKERS = {role: certificate_for for role in ROLES}


def load_recipe():
    data = export.decoded(export.read_bytes(WORKER / 'recipe.json'))
    require(
        data['format'] == 'paths_production_linear_algebra_wave01'
        and data['format_version'] == 1
        and tuple(data['sets']) == SETS,
        'unsupported recipe',
    )
    return data


def sequence(data, family, selected):
    require(family in ('row_operations', 'determinants_2x2') and selected in SETS,
            'unknown family or set')
    if family == 'row_operations':
        source = data['families'][family]
        index = SETS.index(selected)
        inputs = {
            'read_notation': {'row': source['read_rows'][index]},
            'worked_check': {
                'rows': source['worked'][index],
                'multiplier': -source['worked'][index][1][0],
            },
            'choose_next_step': {'rows': source['method'][index]},
            'explain_step': {
                'rows': source['explain'][index],
                'multiplier': -source['explain'][index][1][0],
            },
            'repair_error': {
                'rows': source['repair'][index],
                'multiplier': -source['repair'][index][1][0],
            },
            'independent': {'rows': source['independent'][index]},
        }
    else:
        inputs = {
            role: dict(zip(('a', 'b', 'c', 'd'), value))
            for role, value in zip(ROLES, data['families'][family]['matrices'][selected])
        }
    package = f'prod01_linear_algebra_{family}_{selected}'
    objectives = {
        'row_operations': {
            'read_notation': 'Translate an augmented row into its x-and-y equation.',
            'worked_check': 'Complete the constant in a whole-row replacement.',
            'choose_next_step': 'Eliminate the row-2 x coefficient while retaining row 1.',
            'explain_step': 'Reverse a row-2 replacement with its opposite multiple.',
            'repair_error': 'Find the first omitted constant update and repair it.',
            'independent': 'Verify one ordered pair in both original equations.',
        },
        'determinants_2x2': {
            'read_notation': 'Evaluate ad-bc from the four matrix entries.',
            'worked_check': 'Subtract the two diagonal products to complete the determinant.',
            'choose_next_step': 'Classify this matrix from its calculated determinant.',
            'explain_step': 'Use the row-swap sign rule for a determinant.',
            'repair_error': 'Find the first plus-for-minus determinant error and repair it.',
            'independent': 'Compute and classify a fresh two-by-two determinant.',
        },
    }[family]
    questions = [
        {
            'id': f'{package}_{ROLE_CODES[role]}_{export.sha(export.encoded(inputs[role]))[:6]}',
            'role': role,
            'title': f'{family.replace("_", " ")}: {role.replace("_", " ")}',
            'objective': objectives[role],
            'prerequisites': 'Signed arithmetic, exact fractions, and the linked definitions.',
            'case': {'family': family, 'set': selected, 'inputs': inputs[role]},
        }
        for role in ROLES
    ]
    result = {
        'format': 'paths_exercise_roles',
        'format_version': 1,
        'questions': questions,
    }
    batch.validate_role_sequence(result, f'{family}/{selected}')
    return result
def row_rows(original):
    return [[Fraction(value) for value in row] for row in original['case']['rows']]


def option_id(decisions, step, label):
    return str(step * 10 + decisions[step - 1][0].index(label) + 1)


def row_fields(question, decisions):
    family, selected, original = declared_question(question)
    require(family == 'row_operations', 'row fields require row-operations inputs')
    role = question['role']
    _, _, raw_decisions, facts = batch.MATRIX_ROLE_CHECKERS[role](original)
    fields = {'answer_1': decisions[0][1]}
    # Raw semantic order belongs to the shared checker; presentation order is
    # independent. Templates own the prose attached to these semantic choices.
    for step, (choices, answer) in enumerate(raw_decisions, 1):
        for index, label in enumerate(choices):
            if label != answer:
                fields[f'wrong_{step}_{index}_id'] = option_id(decisions, step, label)
                fields[f'wrong_{step}_{index}_label'] = label
    if role == 'read_notation':
        a, b, c = map(Fraction, original['case']['row'])
        fields.update(a=tex(a), b=tex(b), c=tex(c), negative_b=tex(-b), negative_c=tex(-c))
        fields['calculation_1'] = rf'\left[{tex(a)},{tex(b)}\mid{tex(c)}\right]\Longrightarrow {decisions[0][1]}.'
        return fields

    rows = row_rows(original)
    first, second = rows
    fields.update({f'first_{j}': tex(v) for j, v in enumerate(first)})
    fields.update({f'second_{j}': tex(v) for j, v in enumerate(second)})
    if role != 'independent':
        k = (-second[0] / first[0] if role == 'choose_next_step'
             else Fraction(original['case']['multiplier']))
        changed = batch.row_addition(rows, k)
        fields.update(k=tex(k), opposite_k=tex(-k), changed_constant=tex(changed[1][2]),
                      repeated_constant=tex(changed[1][2] + k * first[2]),
                      wrong_multiple_x=tex(second[0] - k * first[0]))
        fields['constant_expression'] = rf'{tex(second[2])}+({tex(k)})({tex(first[2])})'
        fields['constant_calculation'] = fields['constant_expression'] + '=' + tex(changed[1][2])
        fields['changed_row'] = batch.matrix_tex(changed)
        fields['original_row'] = batch.matrix_tex(rows)
        fields['forward_calculation'] = r',\quad '.join(
            rf'{tex(second[j])}+({tex(k)})({tex(first[j])})={tex(changed[1][j])}'
            for j in range(3))
        restored = batch.row_addition(changed, -k)
        require(restored == rows, 'complete inverse restoration failed')
        fields['inverse_calculation'] = r',\quad '.join(
            rf'{tex(changed[1][j])}+({tex(-k)})({tex(first[j])})={tex(restored[1][j])}'
            for j in range(3))
    else:
        x, y = map(Fraction, facts['solution'])
        fields.update(x=str(x), y=str(y), first_constant=str(first[2]), second_constant=str(second[2]))
        fields['calculation_1'] = ',\\quad '.join(
            rf'({tex(a)})({tex(x)})+({tex(b)})({tex(y)})={tex(c)}'
            for a, b, c in rows) + '.'
        for index in (0, 2):
            residuals = map(Fraction, facts['choice_residuals'][index])
            for j, (row, residual) in enumerate(zip(rows, residuals)):
                fields[f'wrong_{index}_left_{j}'] = str(row[2] + residual)
    return fields


def determinant_option_label(role, value):
    return determinant_statement(str(value)) if role in ('choose_next_step', 'independent') else str(value)


def determinant_fields(question, decisions, facts):
    a, b, c, d = facts['entries']
    determinant = int(facts['determinant'])
    role = question['role']
    fields = dict(a=str(a), b=str(b), c=str(c), d=str(d),
                  determinant=str(determinant), swapped_determinant=str(-determinant),
                  classification='invertible' if determinant else 'singular',
                  sum_value=str(a*d+b*c), main_product=str(a*d), other_product=str(b*c),
                  sum_expression=f'({a})({d})+({b})({c})',
                  determinant_expression=f'({a})({d})-({b})({c})',
                  swapped_expression=f'({c})({b})-({d})({a})')
    if role == 'explain_step':
        fields['same_id'] = option_id(decisions, 1, r'\det(B)=\det(A)')
        fields['zero_id'] = option_id(decisions, 1, r'\det(B)=0')
        return fields
    step = 2 if role == 'repair_error' else 1
    if role == 'repair_error':
        fields['l2_id'] = option_id(decisions, 1, 'L_2')
        fields['l3_id'] = option_id(decisions, 1, 'L_3')
    # Selection is arithmetic, not a prose route. The same ordered candidate
    # computations used by numeric() identify the actual displayed error.
    candidates = [('sum', a*d+b*c, 'ad+bc', f'({a})({d})+({b})({c})')]
    if role not in ('worked_check', 'repair_error'):
        candidates.append(('reverse', b*c-a*d, 'bc-ad', f'({b})({c})-({a})({d})'))
    candidates.append(('omit', a*d, 'ad', f'({a})({d})'))
    selected_labels = set()
    for semantic, value, rule, expression in candidates:
        label = determinant_option_label(role, value)
        if label == decisions[step-1][1] or label in selected_labels:
            continue
        if label not in decisions[step-1][0]:
            continue
        prefix = 'sum' if semantic == 'sum' else 'other'
        fields.update({prefix+'_id': option_id(decisions, step, label),
                       prefix+'_label': label, prefix+'_rule': rule,
                       prefix+'_expression': expression, prefix+'_value': str(value)})
        selected_labels.add(label)
    require(len(selected_labels) == 2, 'unidentified determinant misconception')
    return fields


def choice_fields(question, decisions, after):
    family, selected, original = declared_question(question)
    fields = (row_fields(question, decisions) if family == 'row_operations'
              else determinant_fields(question, decisions, determinant_facts(original)))
    for index, ((choices, answer), reached) in enumerate(zip(decisions, after), 1):
        fields[f'choices_{index}'] = batch.choices_text(choices, choices.index(answer), index)
        fields[f'after_{index}'] = reached
    return fields


def render(question, reading, family):
    given, after, decisions, unused_facts = CHECKERS[question['role']](question)
    fields = {
        'id': question['id'],
        'title': question['title'],
        'objective': question['objective'],
        'given': given,
        'reading_id': reading,
    }
    fields.update(choice_fields(question, decisions, after))
    template = WORKER / family / 'roles' / (question['role'] + '.md.in')
    return batch.fill_template(export.read_bytes(template).decode(), fields, template)


def source_attribution():
    return (
        'Original Paths material. Matrix, determinant, and inverse conventions checked against '
        'OpenStax College Algebra 2e §§7.6 and 7.7 '
        '(https://openstax.org/books/college-algebra-2e/pages/7-6-solving-systems-with-gaussian-elimination ; '
        'https://openstax.org/books/college-algebra-2e/pages/7-7-solving-systems-with-inverses) and '
        'OpenStax Intermediate Algebra 2e §4.6 '
        '(https://openstax.org/books/intermediate-algebra-2e/pages/4-6-solve-systems-of-equations-using-determinants), '
        'accessed 2026-09-10.'
    )


def source_record(package, reading, question_ids, title):
    return {
        'format': 'paths_learning_authoring',
        'format_version': 1,
        'package_id': package,
        'package_version': 1,
        'sources': [{
            'id': package + '_original',
            'kind': 'original',
            'title': title,
            'uri': 'paths:original/' + package + '/v1',
            'revision': '1',
            'attribution': source_attribution(),
            'reuse': 'Original prose, examples, questions, and certificates; no external exercise text copied.',
            'content_ids': [reading, *question_ids],
        }],
    }


def family_input_paths(family):
    return [
        WORKER / 'recipe.json',
        WORKER / 'generate.py',
        WORKER / 'certificate_tests.py',
        WORKER / 'DESIGN.md',
        WORKER / family / 'lesson.md.in',
        *sorted((WORKER / family / 'roles').glob('*.md.in')),
    ]


def family_source_hashes(family):
    return {
        str(path.relative_to(WORKER)): export.sha(export.read_bytes(path))
        for path in family_input_paths(family)
    }


def family_material(data, family):
    questions = []
    for selected in SETS:
        questions.extend(sequence(data, family, selected)['questions'])
    certificates = [batch.reasoning_certificate(question, CHECKERS) for question in questions]
    package = f'prod01_linear_algebra_{family}'
    reading = package + '_r'
    source = data['families'][family]
    reading_title = (
        'Complete and reversible row operations'
        if family == 'row_operations'
        else 'Two by two determinants and invertibility'
    )
    values = {
        'subject': 'linear_algebra',
        'subject_title': 'Linear Algebra',
        'chapter': source['chapter'],
        'chapter_title': source['chapter_title'],
        'reading_id': reading,
        'reading_title': reading_title,
        'lesson_blocks': export.read_bytes(WORKER / family / 'lesson.md.in').decode(),
        'practice_links': ''.join(f'@practice {question["id"]}\n' for question in questions),
        'questions': '\n'.join(render(question, reading, family) for question in questions),
    }
    documents = {f'{family}.paths.md': batch.chapter_text(values).encode()}
    author = source_record(package, reading, [question['id'] for question in questions], reading_title)
    hashes = family_source_hashes(family)
    return questions, certificates, documents, author, hashes


def verify_family_pool(questions, certificates):
    require(len(questions) == len(certificates) == 18, 'each canonical family needs eighteen questions')
    decisions = sum(certificate['steps_checked'] for certificate in certificates)
    wrong_choices = sum(certificate['wrong_choices_checked'] for certificate in certificates)
    require(decisions == 21 and wrong_choices == 42, 'canonical decision or distractor count is wrong')
    by_id = {certificate['id']: certificate for certificate in certificates}
    for selected in SETS:
        positions = [
            by_id[question['id']]['expected']['steps'][0]['choices'].index(
                by_id[question['id']]['expected']['steps'][0]['answer']
            )
            for question in questions
            if declared_question(question)[1] == selected
        ]
        require(sorted(positions) == [0, 0, 1, 1, 2, 2],
                f'{selected} first-decision positions are not balanced')


def staging(data, family, selected, target, model):
    sequence_record = sequence(data, family, selected)
    package = f'prod01_linear_algebra_{family}_{selected}'
    reading = f'prod01_linear_algebra_{family}_r'
    source = data['families'][family]
    role_hashes = [
        export.sha(export.read_bytes(path))
        for path in sorted((WORKER / family / 'roles').glob('*.md.in'))
    ]
    digest = export.sha(export.encoded([
        family,
        selected,
        export.sha(export.read_bytes(WORKER / 'recipe.json')),
        export.sha(export.read_bytes(WORKER / 'generate.py')),
        export.sha(export.read_bytes(WORKER / family / 'lesson.md.in')),
        role_hashes,
    ]))
    folder = OUT / 'staging' / family / selected / digest / 'source'
    author = source_record(
        package,
        reading,
        [question['id'] for question in sequence_record['questions']],
        f'{family.replace("_", " ")}: {selected.replace("_", " ")}',
    )
    export.immutable_directory(
        folder,
        {
            'sequence.json': export.encoded(sequence_record),
            'lesson.md.in': export.read_bytes(WORKER / family / 'lesson.md.in'),
            'questions.paths.md.in': '\n'.join(
                render(question, reading, family) for question in sequence_record['questions']
            ).encode(),
            'certificates.py': export.read_bytes(WORKER / 'generate.py'),
            'authoring.json': export.encoded(author),
        },
    )
    report = pilot.check_assignment(
        {
            'subject': 'linear_algebra',
            'folder': str(folder.relative_to(batch.ROOT)),
            'package_id': package,
            'values': {
                'subject': 'linear_algebra',
                'subject_title': 'Linear Algebra',
                'chapter': source['chapter'],
                'chapter_title': source['chapter_title'],
                'reading_id': reading,
                'reading_title': f'{family.replace("_", " ")}: {selected.replace("_", " ")}',
            },
        },
        target,
        model,
    )
    verification = (
        Path(report['authoring']).parent
        / 'checks'
        / export.sha(export.encoded(report))
        / 'verification.json'
    )
    require(verification.is_file(), 'shared candidate receipt was not written at its exact content address')
    return report, str(verification)


def canonical(data, family, target, model):
    questions, certificates, documents, author, hashes = family_material(data, family)
    verify_family_pool(questions, certificates)
    package = f'prod01_linear_algebra_{family}'
    destination = OUT / 'families' / family / export.sha(export.encoded(hashes))
    target_object = export.Target(target)
    model_path = export.real_path(model)
    target_hash = target_object.fingerprint
    model_hash = export.sha(export.read_bytes(model_path, 64 * 1024 * 1024))
    inspection = target_object.inspect_bytes(documents)

    with tempfile.TemporaryDirectory(prefix='paths-production-linear-') as temporary:
        stage = Path(temporary).resolve()
        export.write_tree(stage, documents)
        replay = subprocess.run(
            [str(model_path), '--question-batch', str(stage)],
            capture_output=True,
            text=True,
            timeout=60,
        )
        require(replay.returncode == 0, replay.stderr or replay.stdout)
        routes = export.decoded(replay.stdout)
        require(
            routes['accepted'] is True
            and routes['routes'] == 18
            and set(routes['question_ids']) == {question['id'] for question in questions},
            'all eighteen routes required',
        )
        batch.verify_role_content(questions, routes['questions'], certificates)
        lesson_replay = subprocess.run(
            [str(model_path), '--family-lessons', str(stage)],
            capture_output=True,
            text=True,
            timeout=60,
        )
        require(lesson_replay.returncode == 0, lesson_replay.stderr or lesson_replay.stdout)
        lesson_checks = export.decoded(lesson_replay.stdout)
        require(lesson_checks.get('accepted') is True, 'family lesson disclosure gate rejected the family')

    export.provenance(author, inspection['entities'])
    require(hashes == family_source_hashes(family), 'authoring source changed during final verification')
    require(
        target_hash == export.sha(export.read_bytes(target_object.path, 64 * 1024 * 1024)),
        'target executable changed during final verification',
    )
    require(
        model_hash == export.sha(export.read_bytes(model_path, 64 * 1024 * 1024)),
        'model executable changed during final verification',
    )

    files = {
        'authoring.json': export.encoded(author),
        **{'documents/' + name: value for name, value in documents.items()},
    }
    export.immutable_directory(destination / 'authoring', files)
    report = {
        'accepted': True,
        'stage': 'family_content_checked',
        'contract_revision': 2,
        'family': family,
        'package_id': package,
        'questions': 18,
        'reading': package + '_r',
        'decisions': 21,
        'wrong_choices': 42,
        'source_sha256': hashes,
        'source_manifest_sha256': export.sha(export.encoded(hashes)),
        'package_sha256': export.sha(export.encoded(export.inventory(files))),
        'package_inventory': export.inventory(files),
        'target_sha256': target_hash,
        'model_sha256': model_hash,
        'mathematical_checks': certificates,
        'route_checks': routes,
        'family_lesson_checks': lesson_checks,
        'authoring': str(destination / 'authoring'),
        'publication': 'not_performed',
        'teaching_review': 'coordinator_pending',
        'visual_acceptance': 'not_observed',
    }
    receipt = export.encoded(report)
    evidence = destination / 'checks' / export.sha(receipt)
    export.immutable_directory(evidence, {'verification.json': receipt})
    return str(destination / 'authoring'), str(evidence / 'verification.json')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--target', type=Path, default=batch.ROOT / 'b/sorter')
    parser.add_argument('--model', type=Path, default=batch.ROOT / 'b/paths_learning_document_tests')
    args = parser.parse_args()
    try:
        data = load_recipe()
        candidates = []
        for family in ('row_operations', 'determinants_2x2'):
            for selected in SETS:
                report, verification = staging(data, family, selected, args.target, args.model)
                candidates.append({
                    'family': family,
                    'set': selected,
                    'authoring': report['authoring'],
                    'verification': verification,
                })
        families = []
        for family in ('row_operations', 'determinants_2x2'):
            authoring, verification = canonical(data, family, args.target, args.model)
            families.append({
                'family': family,
                'authoring': authoring,
                'verification': verification,
            })
        result = {
            'format': 'paths_production_subject',
            'format_version': 1,
            'wave': 'wave01',
            'contract_revision': 2,
            'subject': 'linear_algebra',
            'accepted': True,
            'published': False,
            'candidates': candidates,
            'families': families,
        }
        OUT.mkdir(parents=True, exist_ok=True)
        (OUT / 'production.json').write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps(result, indent=2))
        return 0
    except (export.ExportError, ValueError, OSError, KeyError, TypeError) as error:
        print(json.dumps({
            'accepted': False,
            'code': getattr(error, 'code', 'production.invalid'),
            'message': str(error),
        }, indent=2))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
