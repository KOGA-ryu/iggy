#!/usr/bin/env python3
"""Create or check an editable family recipe; emit ordinary Paths Markdown.

Only registered, reviewed providers supply mathematics. Writers supply JSON
data and Markdown, never Python. Checking does not activate the live library.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile

import build_question_batch as batch
import export_learning as export

ROOT = batch.ROOT
PROVIDERS = {
    'linear_balance_v1': ROOT / 'content/authoring/learning/linear_family',
    'sine_turn_v1': ROOT / 'content/authoring/learning/sine_family',
}
# Reviewed code only. The sine adapter reuses the frozen Wave 01 mathematics;
# include that imported implementation and its local imports in the receipt.
PROVIDER_DEPENDENCIES = {'sine_turn_v1': (
    ROOT / 'content/authoring/production/wave01/trigonometry/generate.py',
    ROOT / 'tools/check_authoring_pilot.py',
)}
INPUTS = ('recipe.json', 'lesson.md.in', 'questions.paths.md.in', 'DESIGN.md')


def inputs(source):
    return {name: export.read_bytes(source / name, export.MAX_FILE) for name in INPUTS}


def hashes(files):
    return {name: export.sha(data) for name, data in files.items()}


def recipe_from(raw, source):
    label = str(source / 'recipe.json')
    def require(ok, pointer, message):
        export.require(ok, 'family.recipe', f'{label}:{pointer}: {message}')
    def fields(value, names, pointer):
        require(type(value) is dict and set(value) == set(names), pointer,
                'Expected exactly: ' + ', '.join(names))
    def text(value, pointer):
        require(type(value) is str and 0 < len(value.strip()) <= 2048
                and '\n' not in value and '\r' not in value, pointer, 'Expected nonempty single-line text')
    try:
        recipe = export.decoded(raw)
    except ValueError as error:
        raise export.ExportError('family.recipe', f'{label}: {error}') from error
    fields(recipe, ('format', 'format_version', 'family', 'package', 'placement', 'source', 'parameters'), '/')
    require(recipe['format'] == 'paths_question_family' and type(recipe['format_version']) is int
            and recipe['format_version'] == 1, '/format_version', 'Use paths_question_family version 1')
    require(type(recipe['family']) is str and recipe['family'] in PROVIDERS,
            '/family', 'Supported families: ' + ', '.join(PROVIDERS))
    package, placement, credit = recipe['package'], recipe['placement'], recipe['source']
    fields(package, ('id', 'version', 'title'), '/package')
    fields(placement, ('subject', 'subject_title', 'chapter', 'chapter_title'), '/placement')
    fields(credit, ('kind', 'title', 'uri', 'revision', 'attribution', 'reuse'), '/source')
    for pointer, value in (('/package/id', package['id']), ('/placement/subject', placement['subject']),
                           ('/placement/chapter', placement['chapter'])):
        try:
            export.identity(value)
        except export.ExportError as error:
            require(False, pointer, str(error))
    require(type(package['version']) is int and 0 < package['version'] <= 2**32-1,
            '/package/version', 'Expected a positive 32-bit integer')
    text(package['title'], '/package/title')
    for name in ('subject_title', 'chapter_title'):
        text(placement[name], '/placement/' + name)
    for name, value in credit.items():
        text(value, '/source/' + name)
    require(type(recipe['parameters']) is dict, '/parameters', 'Expected the selected family parameters')
    return recipe


def load_provider(family):
    path = PROVIDERS[family] / 'generate.py'
    spec = importlib.util.spec_from_file_location('paths_family_' + family, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def tool_hashes(family):
    paths = (Path(__file__), PROVIDERS[family] / 'generate.py', Path(batch.__file__),
             Path(export.__file__), Path(batch.linear.__file__), batch.CHAPTER_TEMPLATE,
             *PROVIDER_DEPENDENCIES.get(family, ()))
    return {str(p.relative_to(ROOT)): export.sha(export.read_bytes(p)) for p in paths}


def assemble(recipe, captured, source, provider):
    export.require(recipe['placement']['subject'] == provider.SUBJECT, 'family.placement',
                   f'{source}/recipe.json:/placement/subject: {recipe["family"]} belongs to {provider.SUBJECT}')
    namespace = 'qf_' + export.sha(export.encoded([recipe['family'], recipe['package']['id']]))[:12]
    try:
        groups = provider.prepare(recipe['parameters'], namespace)
    except (export.ExportError, ValueError, KeyError, TypeError) as error:
        detail = str(error)
        location = '/parameters' + (detail if detail.startswith('/') else ': ' + detail)
        raise export.ExportError('family.parameters', f'{source}/recipe.json:{location}') from error
    questions = [q for group, _ in groups for q in group]
    export.require(questions and len({q['id'] for q in questions}) == len(questions),
                   'family.identity', 'The provider must supply distinct question identities')
    certificates = [batch.reasoning_certificate(q, provider.CHECKERS) for q in questions]
    certified = {c['id']: c for c in certificates}
    reading = namespace + '_reading'
    common = dict(recipe['placement'], reading_id=reading, reading_title=recipe['package']['title'])
    blocks = []
    ordinal = 0
    for group_index, (group, calculated) in enumerate(groups):
        values = dict(calculated, **common)
        for index, q in enumerate(group):
            ordinal += 1
            expected = certified[q['id']]['expected']
            title = f'{ordinal:02} {q["title"]} · {calculated["set_title"]}'
            fields = dict(id=q['id'], title=title, objective=q['objective'], given=expected['given'])
            for number, (after, step) in enumerate(zip(expected['after'], expected['steps']), 1):
                accepted = step['choices'].index(step['answer'])
                lines = batch.choices_text(step['choices'], accepted, number).splitlines()
                choices, answer = lines[:-1], lines[-1]
                # Keep semantic option IDs attached to their feedback. The first
                # correct position cycles across both roles and sets; the same
                # role cannot teach a fixed answer position across its repetitions.
                target = (index + group_index + number - 1) % len(choices)
                rotation = (accepted - target) % len(choices)
                ordered = choices[rotation:] + choices[:rotation]
                fields['choices_' + str(number)] = '\n'.join([*ordered, answer])
                fields['after_' + str(number)] = after
            values.update({q['role'] + '_' + name: value for name, value in fields.items()})
        path = source / 'questions.paths.md.in'
        blocks.append(batch.fill_template(captured[path.name].decode(), values, path))
    values = dict(common, questions='\n\n'.join(blocks),
                  practice_links=''.join('@practice ' + q['id'] + '\n' for q in questions),
                  lesson_blocks=batch.fill_template(captured['lesson.md.in'].decode(), common, source / 'lesson.md.in'))
    documents = {'chapter.paths.md': batch.chapter_text(values).encode()}
    author = dict(format='paths_learning_authoring', format_version=1,
                  package_id=recipe['package']['id'], package_version=recipe['package']['version'],
                  sources=[dict(recipe['source'], id=namespace + '_source',
                                content_ids=[reading, *[q['id'] for q in questions]])])
    return questions, certificates, documents, author, reading


def model_gate(model, flag, documents):
    process = subprocess.run([str(model), flag, str(documents)], capture_output=True, text=True, timeout=60)
    export.require(process.returncode == 0, 'family.model', process.stderr or process.stdout)
    result = export.decoded(process.stdout)
    export.require(result.get('accepted') is True and result.get('windows') == 0,
                   'family.model', f'{flag}: model did not accept the candidate headlessly')
    return result


def check(source, target_path, model_path):
    source = export.real_path(source)
    captured = inputs(source)
    recipe = recipe_from(captured['recipe.json'], source)
    before = tool_hashes(recipe['family'])
    provider = load_provider(recipe['family'])
    questions, certificates, documents, author, reading = assemble(recipe, captured, source, provider)
    target = export.Target(target_path)
    model = export.real_path(model_path)
    model_hash = export.sha(export.read_bytes(model, 64 * 1024 * 1024))
    inspection = target.inspect_bytes(documents)
    entities = inspection['entities']
    expected_ids = [q['id'] for q in questions]
    expected = {
        'question': set(expected_ids), 'lesson': {reading},
        'chapter': {recipe['placement']['chapter']}, 'subject': {recipe['placement']['subject']}}
    for kind, identities in expected.items():
        actual = [e['id'] for e in entities if e['kind'] == kind]
        export.require(len(actual) == len(identities) and set(actual) == identities,
                       'family.scope', f'Compiled {kind} records differ from the recipe')
    export.require(all(e['links'] == [reading] for e in entities if e['kind'] == 'question')
                   and next(e['links'] for e in entities if e['kind'] == 'lesson') == expected_ids
                   and all(e['template'] in ('choices.v1', 'lesson.v2') for e in entities if 'template' in e)
                   and not any('figure' in e for e in entities),
                   'family.scope', 'Use one shared lesson, ordered practice links and the registered choice format')
    provenance = export.provenance(author, entities)
    with tempfile.TemporaryDirectory(prefix='paths-family-check-') as temporary:
        folder = Path(temporary).resolve()
        export.write_tree(folder, documents)
        try:
            routes = model_gate(model, '--question-batch', folder)
        except export.ExportError as error:
            raise export.ExportError(error.code, f'{source}/questions.paths.md.in: {error}', error.diagnostics) from error
        try:
            batch.verify_role_content(questions, routes['questions'], certificates)
        except export.ExportError as error:
            raise export.ExportError(error.code, f'{source}/questions.paths.md.in: {error}', error.diagnostics) from error
        try:
            lessons = model_gate(model, '--family-lessons', folder)
        except export.ExportError as error:
            raise export.ExportError(error.code, f'{source}/lesson.md.in: {error}', error.diagnostics) from error
    export.require(routes['routes'] == len(questions) and routes['save_replay']
                   and routes['wrong_choices'] == sum(c['wrong_choices_checked'] for c in certificates)
                   and lessons['questions'] == len(questions) and lessons['readings'] == 1
                   and lessons['independent_worked_disclosures'] == 3,
                   'family.coverage', 'Check every route, wrong choice, save replay and independent worked disclosure')
    export.require(captured == inputs(source) and before == tool_hashes(recipe['family']),
                   'family.source_changed', f'{source}: authoring or tools changed during checking; rerun')
    export.require(model_hash == export.sha(export.read_bytes(model, 64 * 1024 * 1024))
                   and target.fingerprint == export.sha(export.read_bytes(target.path, 64 * 1024 * 1024)),
                   'family.target_changed', 'A checked executable changed; rerun')
    files = {'authoring.json': export.encoded(author), **{'documents/' + name: data for name, data in documents.items()}}
    digest = export.sha(export.encoded(hashes(files)))
    output = ROOT / 'build/question-families' / author['package_id'] / digest
    report = dict(format='paths_question_family_check', format_version=1, accepted=True,
                  stage='ready_for_coordinator_review', family=recipe['family'], package_id=author['package_id'],
                  package_version=author['package_version'], questions=len(questions), readings=1,
                  decisions=sum(c['steps_checked'] for c in certificates), wrong_choices=routes['wrong_choices'],
                  authoring=str(output / 'authoring'), source=str(source), source_sha256=hashes(captured),
                  tools_sha256=before, target_sha256=target.fingerprint, model_sha256=model_hash,
                  document_sha256=hashes(documents), mathematical_checks=certificates,
                  inspection=inspection, route_checks=routes, lesson_checks=lessons,
                  published=False, teaching_review='pending_coordinator', visual_acceptance='unobserved',
                  learner_evidence='not_collected', windows=0)
    evidence = export.encoded(report)
    checks = output / 'checks' / export.sha(evidence)
    with export.locked(output.parent / '.check.lock'):
        export.immutable_directory(output / 'authoring', files)
        export.immutable_directory(checks, {'verification.json': evidence, 'provenance.json': provenance,
                                           **{'inputs/' + name: data for name, data in captured.items()}})
    return dict(report, verification=str(checks / 'verification.json'))


def initialize(destination, family, package_id):
    destination = export.real_path(destination)
    export.require(not destination.exists(), 'family.exists', f'Choose a new authoring folder: {destination}')
    export.identity(package_id)
    files = inputs(PROVIDERS[family])
    recipe = recipe_from(files['recipe.json'], PROVIDERS[family])
    recipe['package']['id'] = package_id
    files['recipe.json'] = export.encoded(recipe)
    export.immutable_directory(destination, files)
    return dict(created=True, source=str(destination), editable_files=list(INPUTS), family=family,
                check_command=['python3', '-B', str(Path(__file__).resolve()), 'check', str(destination)], published=False)


def main(argv=None):
    cli = argparse.ArgumentParser(description=__doc__)
    commands = cli.add_subparsers(dest='command', required=True)
    create = commands.add_parser('init', help='create the editable recipe and reviewed teaching templates')
    create.add_argument('source', type=Path)
    create.add_argument('--family', choices=tuple(PROVIDERS), required=True)
    create.add_argument('--package', required=True, help='new stable package identity')
    verify = commands.add_parser('check', help='generate and check one complete family; never publish')
    verify.add_argument('source', type=Path)
    verify.add_argument('--target', type=Path, default=ROOT / 'b/sorter')
    verify.add_argument('--model', type=Path, default=ROOT / 'b/paths_learning_document_tests')
    args = cli.parse_args(argv)
    try:
        result = (initialize(args.source, args.family, args.package) if args.command == 'init'
                  else check(args.source, args.target, args.model))
        if args.command == 'check':
            result = {name: result[name] for name in (
                'format', 'format_version', 'accepted', 'stage', 'family', 'package_id', 'package_version',
                'questions', 'readings', 'decisions', 'wrong_choices', 'authoring', 'verification',
                'published', 'teaching_review', 'visual_acceptance')}
        print(json.dumps(result, indent=2))
        return 0
    except (export.ExportError, ValueError, OSError, KeyError, TypeError, subprocess.TimeoutExpired) as error:
        print(json.dumps(dict(accepted=False, code=getattr(error, 'code', 'family.invalid'), message=str(error),
                              diagnostics=getattr(error, 'diagnostics', [])), indent=2))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
