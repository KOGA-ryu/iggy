#!/usr/bin/env python3
"""Check a reserved subject pilot through the shared compiler and model; never publish."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile

import build_question_batch as batch
import export_learning as export

ROOT = Path(__file__).resolve().parents[1]
PLAN = ROOT / 'content/authoring/parallel/assignments.json'
SUBJECTS = ('algebra', 'trigonometry', 'calculus', 'linear_algebra')
INPUTS = ('sequence.json', 'lesson.md.in', 'questions.paths.md.in', 'certificates.py', 'authoring.json')


def local_file(name):
    path = ROOT / name
    export.require(not Path(name).is_absolute() and '..' not in Path(name).parts
                   and path.resolve().is_relative_to(ROOT), 'pilot.path', f'Expected a repository path: {name}')
    export.require(path.is_file() and not any(p.is_symlink() for p in (path, *path.parents)),
                   'pilot.file', f'Missing or symlinked input: {name}')
    return path


def packet():
    plan = export.decoded(export.read_bytes(PLAN))
    export.require(plan['format'] == 'paths_parallel_authoring' and plan['format_version'] == 1,
                   'pilot.plan', 'Unsupported assignment plan')
    assignments = plan['assignments']
    export.require([a['subject'] for a in assignments] == list(SUBJECTS), 'pilot.plan', 'Expected four ordered subject assignments')
    identities = set()
    for a in assignments:
        subject = a['subject']
        export.require(a['folder'] == f'content/authoring/parallel/{subject}', 'pilot.path', f'{subject}: wrong authoring folder')
        source = local_file(a['folder'] + '/sequence.json')
        questions = batch.validate_role_sequence(export.decoded(export.read_bytes(source)), source)
        expected = [a['question_prefix'] + '_' + role for role in batch.EXERCISE_ROLES]
        export.require([q['id'] for q in questions] == expected, 'pilot.identity', f'{subject}: reserved question identities differ')
        for identity in [a['package_id'], a['values']['reading_id'], *expected]:
            export.identity(identity)
            export.require(identity not in identities, 'pilot.identity', f'Repeated reservation: {identity}')
            identities.add(identity)
        export.require(a['values']['subject'] == subject, 'pilot.identity', f'{subject}: wrong subject binding')
    for name, digest in plan['reference_sha256'].items():
        export.require(export.sha(export.read_bytes(local_file(name))) == digest,
                       'pilot.reference_changed', f'{name}: pinned reference changed; coordinator must reconcile the packet')
    return plan


def fingerprint(folder):
    return {name: export.sha(export.read_bytes(folder / name)) for name in INPUTS}


def check_assignment(a, target_path, model_path):
    folder = ROOT / a['folder']
    for name in INPUTS:
        export.require((folder / name).is_file(), 'pilot.incomplete', f"{a['subject']}: author {a['folder']}/{name} before running the content gate")
        local_file(a['folder'] + '/' + name)
    before = fingerprint(folder)
    # An explicitly run authoring check loads only this assignment's reviewed Python provider.
    # No provider code is copied into runtime documents or executed by the importer.
    spec = importlib.util.spec_from_file_location('paths_pilot_' + a['subject'], folder / 'certificates.py')
    provider = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(provider)
    export.require(set(provider.CHECKERS) == set(batch.EXERCISE_ROLES), 'pilot.checkers', 'Supply exactly the six role certificate functions')
    questions, certificates, documents, reading, _ = batch.reasoning_documents(
        folder, provider.CHECKERS, dict(a['values']), 'chapter.paths.md')
    author = export.decoded(export.read_bytes(folder / 'authoring.json'))
    export.require(author['format'] == 'paths_learning_authoring' and type(author['format_version']) is int
                   and author['format_version'] == 1 and author['package_id'] == a['package_id']
                   and type(author['package_version']) is int and author['package_version'] == 1,
                   'pilot.identity', 'Use the reserved package identity and version 1')
    target = export.Target(target_path)
    inspection = target.inspect_bytes(documents)
    entities = inspection['entities']
    export.require({e['id'] for e in entities if e['kind'] == 'lesson'} == {reading}
                   and {e['id'] for e in entities if e['kind'] == 'question'} == {q['id'] for q in questions}
                   and {e['id'] for e in entities if e['kind'] == 'chapter'} == {a['values']['chapter']}
                   and {e['id'] for e in entities if e['kind'] == 'subject'} == {a['subject']}
                   and {e['template'] for e in entities if 'template' in e} == {'lesson.v2', 'choices.v1'}
                   and not any('figure' in e for e in entities),
                   'pilot.scope', 'Pilot must contain the reserved reading and six choice questions, with no figure or extra chapter')
    export.provenance(author, entities)
    model = export.real_path(model_path)
    model_hash = export.sha(export.read_bytes(model, 64 * 1024 * 1024))
    with tempfile.TemporaryDirectory(prefix='paths-pilot-check-') as temporary:
        staged = Path(temporary).resolve()
        export.write_tree(staged, documents)
        played = subprocess.run([str(model), '--question-batch', str(staged)], capture_output=True, text=True, timeout=60)
        export.require(played.returncode == 0, 'pilot.routes', played.stderr or played.stdout)
        routes = export.decoded(played.stdout)
        export.require(routes['accepted'] is True and routes['routes'] == 6
                       and set(routes['question_ids']) == {q['id'] for q in questions},
                       'pilot.routes', 'The model must replay all six prepared questions')
        batch.verify_role_content(questions, routes['questions'], certificates)
    export.require(before == fingerprint(folder), 'pilot.source_changed', 'Authoring inputs changed during verification; rerun')
    export.require(target.fingerprint == export.sha(export.read_bytes(target.path, 64 * 1024 * 1024))
                   and model_hash == export.sha(export.read_bytes(model, 64 * 1024 * 1024)),
                   'pilot.target_changed', 'A checked executable changed during verification; rerun')
    packet()  # Recheck shared inputs as well as the subject's source files.
    files = {'authoring.json': export.encoded(author), **{'documents/' + name: data for name, data in documents.items()}}
    digest = export.sha(export.encoded({name: export.sha(data) for name, data in files.items()}))
    destination = ROOT / 'build/parallel-authoring' / a['subject'] / digest
    report = dict(accepted=True, stage='content_checked', subject=a['subject'], package_id=a['package_id'],
                  source_sha256=before, target_sha256=target.fingerprint, model_sha256=model_hash,
                  mathematical_checks=certificates, route_checks=routes,
                  authoring=str(destination / 'authoring'), publication='not_performed', teaching_review='pending',
                  visual_acceptance='pending_user')
    with export.locked(destination.parent / '.check.lock'):
        export.immutable_directory(destination / 'authoring', files)
        # Evidence is addressed separately: identical content may be checked with new tools.
        receipt = export.encoded(report)
        export.immutable_directory(destination / 'checks' / export.sha(receipt), {'verification.json': receipt})
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--packet-only', action='store_true', help='check instructions, reserved IDs and reference bytes; does not check unwritten content')
    mode.add_argument('--subject', choices=SUBJECTS)
    parser.add_argument('--target', type=Path, default=ROOT / 'b/sorter')
    parser.add_argument('--model', type=Path, default=ROOT / 'b/paths_learning_document_tests')
    args = parser.parse_args()
    try:
        plan = packet()
        result = (dict(accepted=True, stage='packet_ready', subjects=list(SUBJECTS), questions_reserved=24,
                       readings_reserved=4, content_checked=False, workers_started=False)
                  if args.packet_only else check_assignment(next(a for a in plan['assignments'] if a['subject'] == args.subject), args.target, args.model))
        print(json.dumps(result, indent=2))
        return 0
    except (export.ExportError, OSError, ValueError, KeyError, AttributeError, TypeError) as error:
        print(json.dumps(dict(accepted=False, code=getattr(error, 'code', 'pilot.invalid'), message=str(error),
                              diagnostics=getattr(error, 'diagnostics', [])), indent=2))
        return 1


if __name__ == '__main__':
    sys.exit(main())
