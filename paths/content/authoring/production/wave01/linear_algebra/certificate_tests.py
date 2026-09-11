#!/usr/bin/env python3
"""Exercise the complete declared pool and refuse mutations after real compilation."""
import argparse
import copy
import importlib.util
from fractions import Fraction
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate
import build_question_batch as batch
import export_learning as export


ROLE_PROMPTS = {
    'row_operations': {
        'read_notation': 'Which equation is represented by this augmented row?',
        'worked_check': 'What is the missing constant after the complete replacement of row 2?',
        'choose_next_step': 'Which operation eliminates x from row 2 while leaving row 1 unchanged?',
        'explain_step': 'Which inverse operation restores the original second row?',
        'repair_error': 'Which line first fails to apply the row operation to the complete row?',
        'independent': 'Which ordered pair solves both original equations?',
    },
    'determinants_2x2': {
        'read_notation': 'What is det(A)=ad-bc for these four entries?',
        'worked_check': 'What is the completed value after subtracting the diagonal products?',
        'choose_next_step': 'Which statement gives a valid determinant criterion for this matrix?',
        'explain_step': 'How does a row swap change the determinant?',
        'repair_error': 'Which line first uses the determinant rule incorrectly?',
        'independent': 'Which exact determinant statement correctly classifies this fresh matrix?',
    },
}


def rejects(call):
    try:
        call()
    except export.ExportError:
        return
    raise AssertionError('expected certificate refusal')


def compiled_routes(documents, model):
    with tempfile.TemporaryDirectory(prefix='paths-production-linear-test-') as temporary:
        stage = Path(temporary).resolve()
        export.write_tree(stage, documents)
        replay = subprocess.run(
            [str(model), '--question-batch', str(stage)],
            capture_output=True,
            text=True,
            timeout=60,
        )
        if replay.returncode:
            raise AssertionError(replay.stderr or replay.stdout)
        result = export.decoded(replay.stdout)
        if result.get('accepted') is not True:
            raise AssertionError('compiler/model did not accept the mutation')
        return result


def first_question_block(document):
    start = document.index('@question ')
    end = document.index('\n@end', start) + len('\n@end')
    return start, end, document[start:end]


def mutate_first_answer_key(documents):
    name, original = next(iter(documents.items()))
    text = original.decode()
    start, end, block = first_question_block(text)
    choices = re.findall(r'^@choice (\d+) \|', block, re.MULTILINE)
    answer = re.search(r'^@answer (\d+)$', block, re.MULTILINE)
    assert len(choices) == 3 and answer is not None
    wrong = next(choice for choice in choices if choice != answer.group(1))
    changed = block.replace(
        f'@answer {answer.group(1)}',
        f'@answer {wrong}',
        1,
    ).replace(
        f'@feedback {wrong} |',
        f'@feedback {answer.group(1)} |',
        1,
    )
    assert changed != block and f'@feedback {answer.group(1)} |' in changed
    result = dict(documents)
    result[name] = (text[:start] + changed + text[end:]).encode()
    return result


def mutate_first_reached_work(documents):
    name, original = next(iter(documents.items()))
    text = original.decode()
    start, end, block = first_question_block(text)
    changed, count = re.subn(r'^@after .+$', '@after 0', block, count=1, flags=re.MULTILINE)
    assert count == 1 and changed != block
    result = dict(documents)
    result[name] = (text[:start] + changed + text[end:]).encode()
    return result


def assert_role_templates_are_consumed(data, model):
    live = generate.WORKER
    inputs = {path for family in data['families'] for path in generate.family_input_paths(family)}
    captured = {str(path.relative_to(live)): path.read_bytes() for path in inputs}
    with tempfile.TemporaryDirectory(prefix='paths-linear-markdown-edit-') as temporary:
        scratch = Path(temporary).resolve()
        export.write_tree(scratch, captured)
        for family, prompts in ROLE_PROMPTS.items():
            questions, certificates, documents, _, _ = generate.family_material(data, family)
            baseline = compiled_routes(documents, model)
            for role, prompt in prompts.items():
                path = scratch / family / 'roles' / (role + '.md.in')
                text = path.read_text()
                assert prompt in text and '@feedback ' in text
                text = text.replace(prompt, prompt + f' [scratch-prompt-{role}]', 1)
                text, count = re.subn(
                    r'^(@feedback .+)$',
                    lambda match: match[1] + f' [scratch-feedback-{role}]',
                    text, count=1, flags=re.MULTILINE)
                assert count == 1
                path.write_text(text)
            with patch.object(generate, 'WORKER', scratch):
                changed_documents = generate.family_material(data, family)[2]
            changed = compiled_routes(changed_documents, model)
            batch.verify_role_content(questions, changed['questions'], certificates)
            assert len(changed['questions']) == 18
            for question, before, after in zip(questions, baseline['questions'], changed['questions']):
                role = question['role']
                old_step = before['question']['steps'][0]
                new_step = after['question']['steps'][0]
                assert new_step['prompt'] == old_step['prompt'] + f' [scratch-prompt-{role}]'
                changed_options = [
                    (old, new) for old, new in zip(old_step['options'], new_step['options'])
                    if old != new]
                assert len(changed_options) == 1
                old, new = changed_options[0]
                assert new['wrong_feedback'] == old['wrong_feedback'] + f' [scratch-feedback-{role}]'
                # No mathematical or other compiled field may change.
                new_step['prompt'] = old_step['prompt']
                new['wrong_feedback'] = old['wrong_feedback']
                assert before == after
    assert {name: (live / name).read_bytes() for name in captured} == captured


def assert_determinant_diagnostics_are_distinct(data):
    observed = set()
    for selected in generate.SETS:
        for question in generate.sequence(data, 'determinants_2x2', selected)['questions']:
            _, _, decisions, facts = generate.CHECKERS[question['role']](question)
            if question['role'] == 'explain_step':
                continue
            fields = generate.determinant_fields(question, decisions, facts)
            a, b, c, d = facts['entries']
            computed = {'ad+bc': a*d+b*c, 'bc-ad': b*c-a*d, 'ad': a*d}
            step = 2 if question['role'] == 'repair_error' else 1
            for prefix in ('sum', 'other'):
                rule = fields[prefix + '_rule']
                observed.add(rule)
                value = computed[rule]
                label = generate.determinant_option_label(question['role'], value)
                assert fields[prefix + '_label'] == label
                assert fields[prefix + '_value'] == str(value)
                assert fields[prefix + '_id'] == generate.option_id(decisions, step, label)
                assert label != decisions[step-1][1]
    assert observed == {'ad+bc', 'bc-ad', 'ad'}


def assert_complete_explanations(data, model):
    for family in data['families']:
        questions, _, documents, _, _ = generate.family_material(data, family)
        compiled = compiled_routes(documents, model)
        for question, record in zip(questions, compiled['questions']):
            role = question['role']
            explanation = record['question']['steps'][0]['explanation']
            original = generate.declared_question(question)[2]
            if family == 'row_operations' and role in ('worked_check', 'choose_next_step', 'explain_step'):
                rows = generate.row_rows(original)
                k = (-rows[1][0] / rows[0][0] if role == 'choose_next_step'
                     else Fraction(original['case']['multiplier']))
                changed = batch.row_addition(rows, k)
                before, after, multiple = (changed, rows, -k) if role == 'explain_step' else (rows, changed, k)
                for j in range(3):
                    calculation = (rf'{generate.tex(before[1][j])}+({generate.tex(multiple)})'
                                   rf'({generate.tex(before[0][j])})={generate.tex(after[1][j])}')
                    assert calculation in explanation
                assert batch.matrix_tex(after) in explanation
            if family == 'determinants_2x2' and role == 'explain_step':
                a, b, c, d = (original['case'][key] for key in ('a', 'b', 'c', 'd'))
                assert rf'\det(A)=({a})({d})-({b})({c})={a*d-b*c}' in explanation
                assert rf'\det(B)=({c})({b})-({d})({a})={c*b-d*a}' in explanation


def assert_provider_is_sequence_self_contained(data):
    questions = [
        question for family in ('row_operations', 'determinants_2x2')
        for selected in generate.SETS
        for question in generate.sequence(data, family, selected)['questions']
    ]
    expected = [batch.reasoning_certificate(q, generate.CHECKERS) for q in questions]
    with tempfile.TemporaryDirectory(prefix='paths-staged-provider-') as temporary:
        folder = Path(temporary)
        export.write_tree(folder, {
            'certificates.py': (generate.WORKER / 'generate.py').read_bytes(),
            'sequence.json': export.encoded(questions),
        })
        spec = importlib.util.spec_from_file_location('isolated_linear_provider', folder / 'certificates.py')
        provider = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(provider)
        staged = export.decoded((folder / 'sequence.json').read_bytes())
        # Both all file reads and the recipe entry point fail during replay.
        failure = AssertionError('staged certificates must use only their captured sequence')
        with patch.object(provider, 'load_recipe', side_effect=failure), \
                patch.object(export, 'read_bytes', side_effect=failure), \
                patch('builtins.open', side_effect=failure), \
                patch.object(Path, 'open', side_effect=failure):
            actual = [batch.reasoning_certificate(q, provider.CHECKERS) for q in staged]
        assert actual == expected
        renamed = copy.deepcopy(staged[0])
        renamed['id'] = 'opaque_identity_with_no_family_or_set'
        assert batch.reasoning_certificate(renamed, provider.CHECKERS)['evidence'] == expected[0]['evidence']


def assert_compiled_mutations_are_refused(data, model):
    for family in ('row_operations', 'determinants_2x2'):
        questions, certificates, documents, author, hashes = generate.family_material(data, family)
        routes = compiled_routes(documents, model)
        batch.verify_role_content(questions, routes['questions'], certificates)
        key_routes = compiled_routes(mutate_first_answer_key(documents), model)
        rejects(lambda: batch.verify_role_content(questions, key_routes['questions'], certificates))
        reached_routes = compiled_routes(mutate_first_reached_work(documents), model)
        rejects(lambda: batch.verify_role_content(questions, reached_routes['questions'], certificates))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--model', type=Path, default=batch.ROOT / 'b/paths_learning_document_tests')
    args = parser.parse_args()
    model = export.real_path(args.model)
    data = generate.load_recipe()
    seen = set()
    total = 0
    patterns = set()

    for family in ('row_operations', 'determinants_2x2'):
        for selected in generate.SETS:
            sequence = generate.sequence(data, family, selected)
            assert len(sequence['questions']) == 6
            positions = []
            for question in sequence['questions']:
                assert set(question['case']) == {'family', 'set', 'inputs'}
                assert question['case']['family'] == family and question['case']['set'] == selected
                certificate = batch.reasoning_certificate(question, generate.CHECKERS)
                assert certificate['accepted'] and certificate['evidence']['kind'] == batch.EXERCISE_ROLES[question['role']]
                assert question['id'] not in seen
                seen.add(question['id'])
                positions.append(
                    certificate['expected']['steps'][0]['choices'].index(
                        certificate['expected']['steps'][0]['answer']
                    )
                )
                assert all(
                    step['choices'].count(step['answer']) == 1
                    for step in certificate['expected']['steps']
                )
                if question['role'] == 'repair_error':
                    expected = certificate['expected']
                    template = generate.render(question, 'test_reading', family)
                    first = '\n'.join(line for line in template.split('@step 20')[0].splitlines()
                                      if line.startswith(('@feedback ', '@why ', '@after ')))
                    assert '=' + expected['steps'][1]['answer'] not in first
                    assert '@domain L1 is' not in template
                total += 1
            assert sorted(positions) == [0, 0, 1, 1, 2, 2]
            assert tuple(positions) not in patterns
            patterns.add(tuple(positions))
    assert total == 36

    for selected in generate.SETS:
        for question in generate.sequence(data, 'determinants_2x2', selected)['questions']:
            a, b, c, d = (question['case']['inputs'][key] for key in ('a', 'b', 'c', 'd'))
            determinant = a * d - b * c
            facts = generate.determinant_certificate(question)[3]
            assert Fraction(facts['determinant']) == determinant
            if question['role'] == 'explain_step':
                swapped = c * b - d * a
                assert swapped == -determinant
                assert generate.determinant_certificate(question)[1] == [rf'\det(B)={swapped}']
            if determinant:
                inverse = [[Fraction(value) for value in row] for row in facts['inverse']]
                matrix = [[a, b], [c, d]]
                for left, right in ((matrix, inverse), (inverse, matrix)):
                    assert [[sum(left[i][k] * right[k][j] for k in range(2))
                             for j in range(2)] for i in range(2)] == [[1, 0], [0, 1]]
            else:
                x, y = facts['null_vector']
                assert (x, y) != (0, 0) and a * x + b * y == c * x + d * y == 0

    all_zero = copy.deepcopy(
        generate.sequence(data, 'determinants_2x2', 'teaching')['questions'][0]
    )
    all_zero['case']['inputs'] = {'a': 0, 'b': 0, 'c': 0, 'd': 0}
    assert generate.determinant_facts(generate.declared_question(all_zero)[2])['null_vector'] == [1, 0]
    rejects(lambda: generate.determinant_certificate(all_zero))
    zero_first_row = copy.deepcopy(all_zero)
    zero_first_row['case']['inputs'] = {'a': 0, 'b': 0, 'c': 2, 'd': -3}
    witness = generate.determinant_facts(generate.declared_question(zero_first_row)[2])['null_vector']
    assert witness != [0, 0] and 2 * witness[0] - 3 * witness[1] == 0
    rejects(lambda: generate.determinant_certificate(zero_first_row))
    # Numeric roles require two distinct, mathematically motivated errors.
    # No answer+1 filler is permitted when these computations collide.
    rejects(lambda: generate.numeric(0, 0, 0))
    rejects(lambda: generate.numeric(4, 4, -4, 4))
    assert generate.numeric(0, 8, 0, 4) == ['0', '8', '4']
    for role in generate.ROLES:
        degenerate = copy.deepcopy(all_zero)
        degenerate['role'] = role
        rejects(lambda: generate.determinant_certificate(degenerate))
    ambiguous_swap = copy.deepcopy(all_zero)
    ambiguous_swap['role'] = 'explain_step'
    rejects(lambda: generate.determinant_certificate(ambiguous_swap))

    for field, value in (('family', 'unknown'), ('set', 'unknown'), ('inputs', {})):
        malformed = copy.deepcopy(all_zero)
        malformed['case'][field] = value
        rejects(lambda: batch.reasoning_certificate(malformed, generate.CHECKERS))

    # Check the separate lesson examples and every original system with the
    # existing exact matrix owner. No local alternative solver is introduced.
    worked_rows = [[1, 2, 6], [3, -1, 5]]
    worked_pair = batch.system_solution([[Fraction(v) for v in row] for row in worked_rows])
    assert worked_pair == [Fraction(16, 7), Fraction(13, 7)]
    for selected in generate.SETS:
        for question in generate.sequence(data, 'row_operations', selected)['questions']:
            inputs = question['case']['inputs']
            if 'rows' not in inputs:
                continue
            assert inputs['rows'] != worked_rows
            rows = [[Fraction(v) for v in row] for row in inputs['rows']]
            x, y = batch.system_solution(rows)
            assert all(a * x + b * y == c for a, b, c in rows)
            if 'multiplier' in inputs or question['role'] == 'choose_next_step':
                k = Fraction(inputs['multiplier']) if 'multiplier' in inputs else -rows[1][0] / rows[0][0]
                changed = batch.row_addition(rows, k)
                assert changed[0] == rows[0]
                assert all(changed[1][j] == rows[1][j] + k * rows[0][j] for j in range(3))
                assert batch.row_addition(changed, -k) == rows
    fresh = generate.sequence(data, 'row_operations', 'fresh_check')['questions'][2]
    assert generate.CHECKERS[fresh['role']](fresh)[3]['multiplier'] == '-3/2'

    out_of_bounds = copy.deepcopy(
        generate.sequence(data, 'determinants_2x2', 'teaching')['questions'][0]
    )
    out_of_bounds['case']['inputs']['a'] = 13
    rejects(lambda: generate.determinant_certificate(out_of_bounds))

    assert_role_templates_are_consumed(data, model)
    assert_complete_explanations(data, model)
    assert_determinant_diagnostics_are_distinct(data)
    assert_provider_is_sequence_self_contained(data)
    assert_compiled_mutations_are_refused(data, model)
    print(
        'production linear algebra certificate tests: passed '
        '(36 questions, two canonical families; 12 scratch Markdown templates: prompt and '
        'wrong-feedback edits verified in 36 compiled questions; complete row and swapped '
        'determinant explanations; numeric-collision refusal; isolated provider replay; '
        'compiled false-key and false-working refusal)'
    )


if __name__ == '__main__':
    main()
