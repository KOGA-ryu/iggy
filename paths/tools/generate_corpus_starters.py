#!/usr/bin/env python3
"""Publish explicitly authored starters; never edit the pinned corpus sources."""
import argparse
import collections
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
AUTHOR = ROOT / 'content/authoring/corpus_starters'
OUTPUT = ROOT / 'content/corpus/starters.json'
SHEET = ROOT / 'docs/CORPUS_STARTER_COVERAGE.md'


def build():
    toc = json.loads((ROOT / 'content/corpus/toc.json').read_text())
    mapping = json.loads((ROOT / 'content/authoring/math_corpus_map.json').read_text())
    subcategories = json.loads((AUTHOR / 'subcategories.json').read_text())
    topics = {t['id']: t for t in toc['topics']}
    expected_subs = {(e['topic'], e['heading_path'][2]) for e in mapping['entries']
                     if len(e['heading_path']) == 4 and e['heading_path'][1] == topics[e['topic']]['title']}
    assert {(s['topic'], s['title']) for s in subcategories} == expected_subs, 'Subcategory coverage changed'
    nodes = {}
    for subject in toc['subjects']:
        nodes[subject['id']] = dict(subject=subject['id'], title=subject['title'], level='subject')
    for topic in toc['topics']:
        nodes[topic['id']] = dict(subject=topic['subject'], topic=topic['id'], title=topic['title'], level='chapter')
    for sub in subcategories:
        assert sub['id'] not in nodes
        nodes[sub['id']] = dict(subject=topics[sub['topic']]['subject'], topic=sub['topic'], title=sub['title'], level='subcategory')
    questions = {}
    for path in sorted(AUTHOR.glob('*.txt')):
        for line_number, line in enumerate(path.read_text().splitlines(), 1):
            if not line or line.startswith('#'):
                continue
            fields = line.split('|')
            assert len(fields) == 10, f'{path.name}:{line_number}: expected 10 fields, got {len(fields)}'
            key, prompt, given, setup, misstep, answer, wrong1, wrong2, explanation, check = fields
            assert key in nodes and key not in questions, f'{path.name}:{line_number}: unknown or repeated {key}'
            assert all(fields), f'{key}: empty field'
            assert len({setup, misstep}) == 2 and len({answer, wrong1, wrong2}) == 3, f'{key}: repeated tile'
            states = [{'id': i + 1, 'display': text} for i, text in enumerate((given, setup, answer))]
            steps = []
            for i, choices in enumerate(((setup, misstep), (answer, wrong1, wrong2))):
                # Stable non-uniform answer positions; identities do not depend on row order.
                rotation = int(hashlib.sha256(f'{key}/{i}'.encode()).hexdigest()[:8], 16) % len(choices)
                labels = list(choices[rotation:] + choices[:rotation])
                steps.append(dict(id=i + 1, layer_name=('Setup', 'Result')[i],
                    prompt=('Choose the setup.', 'Complete the result.')[i],
                    options=[dict(id=j + 1, label=label) for j, label in enumerate(labels)],
                    accepted_option_ids=[labels.index(choices[0]) + 1], wrong_hint='Try another tile; the working is unchanged.',
                    explanation=explanation if i else 'The selected setup is now in the working.',
                    hint=prompt, next_move=choices[0],
                    semantics=dict(purpose='calculation', completion='any_accepted', before=i + 1, after=i + 2)))
            questions[key] = dict(id='starter_' + key, **nodes[key], authoring=f'{path.name}:{line_number}',
                check=check, question=dict(schema_version=1, id='starter_' + key, content_version=1,
                    equation=given, skill=nodes[key]['title'], description=prompt, working_states=states, steps=steps))
    assert questions.keys() == nodes.keys(), f'Missing: {sorted(nodes.keys() - questions.keys())}'
    ordered = []
    for subject in toc['subjects']:
        ordered.append(questions[subject['id']])
        for topic in toc['topics']:
            if topic['subject'] != subject['id']:
                continue
            ordered.append(questions[topic['id']])
            ordered.extend(questions[s['id']] for s in subcategories if s['topic'] == topic['id'])
    return dict(schema_version=1, difficulty='representative_start', questions=ordered)


def coverage(bank):
    lines = ['# Starting question coverage', '',
        'Generated from the six subject authoring files. One separate question is assigned to each subject, chapter and named subcategory.', '',
        'This inventories the Library TOC, not the separate math_lab textbook or all 930 individual definitions. Drafting-round headings are not mathematical subcategories.', '',
        'Every question has a setup decision, a result decision, and a retained worked explanation. Answers below are LaTeX source; the game typesets them natively.', '']
    subject = None
    for q in bank['questions']:
        if q['subject'] != subject:
            subject = q['subject']; lines += ['', '## '+q['title'], '',
                '| ID | Level | Category | Starting problem | Result |', '| --- | --- | --- | --- | --- |']
        content = q['question']
        lines.append(f"| {q['id']} | {q['level']} | {q['title']} | {content['description']} | `{content['working_states'][-1]['display']}` |")
    return '\n'.join(lines)+'\n'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    result = build()
    text = json.dumps(result, ensure_ascii=False, indent=2) + '\n'
    for path, contents in ((OUTPUT, text), (SHEET, coverage(result))):
        if args.check:
            assert path.read_text() == contents, f'{path.name} differs from its authoring'
        else:
            temporary = path.with_suffix('.tmp')
            temporary.write_text(contents)
            temporary.replace(path)
    print(f"{len(result['questions'])} starters: {dict(collections.Counter(q['level'] for q in result['questions']))}")
