#!/usr/bin/env python3
"""Bounded batch mathematics and publication through the actual app/model."""
import argparse
import copy
import json
from fractions import Fraction
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import build_question_batch as batch
import export_learning as export


class BatchTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='paths-question-batch-')
        self.root = Path(self.temporary.name).resolve()
        self.store = self.root / 'store'

    def tearDown(self):
        self.temporary.cleanup()

    def args(self, count=12, version=1, publish=False, format_version=1, family='matrix'):
        return argparse.Namespace(count=count, version=version, publish=publish, format_version=format_version, family=family,
            target=OPTIONS.target, model=OPTIONS.model, output=self.root / 'batches' / str(version),
            store=self.store, base_documents=ROOT / 'content/write')

    def draft_args(self, family='linear'):
        source=self.root/(family+' release')/'authoring'
        if not source.exists():
            version=2 if family=='matrix' else 1
            args=self.args(family=family,version=version,format_version=version)
            _,_,docs,author,audit=batch.BUILDERS[family](args)
            export.write_tree(source,{'authoring.json':export.encoded(author),'audit.json':export.encoded(audit),
                                     **{'documents/'+p:data for p,data in docs.items()}})
        return argparse.Namespace(command='draft',source=source,target=OPTIONS.target,library=None,
                                  output=self.root/(family+' editable draft'))

    def test_drafts_copy_generated_families_and_reach_live_preview(self):
        for family,questions in (('linear',12),('matrix',24)):
            with self.subTest(family=family):
                args=self.draft_args(family)
                # An unused Markdown note and an old audit must not become runtime input or current evidence.
                (args.source/'documents/unused.md').write_text('Not included in this chapter.\n')
                source=export.capture(args.source)
                if family=='linear':
                    command=[sys.executable,'-B',str(ROOT/'tools/export_learning.py'),'draft',str(args.source),
                             '--output',str(args.output),'--target',str(OPTIONS.target)]
                    result=subprocess.run(command,capture_output=True,text=True,timeout=30)
                    self.assertEqual(result.returncode,0,result.stderr)
                    report=json.loads(result.stdout)
                else:
                    report=export.run(args)
                self.assertFalse(report['published'])
                self.assertTrue(report['preview_only'])
                self.assertEqual(report['questions'],questions)
                self.assertEqual(shlex.split(report['preview_command']),report['preview_argv'])
                self.assertEqual(report['preview_argv'],[str(OPTIONS.target.resolve()),'--documents',
                                 str(args.output/'documents'),'--watch-documents'])
                copied=export.capture(args.output)
                self.assertEqual(set(copied),{'draft.json'} | {p for p in source if p.startswith('documents/') and p!='documents/unused.md'})
                for p,data in copied.items():
                    if p!='draft.json':self.assertEqual(data,source[p])
                origin=json.loads(copied['draft.json'])
                self.assertEqual(origin['sources'],json.loads(source['authoring.json'])['sources'])
                self.assertEqual(origin['origin_files'],export.inventory({p:d for p,d in copied.items() if p!='draft.json'}))
                model=subprocess.run([str(OPTIONS.model),'--draft-preview',report['documents']],
                                     capture_output=True,text=True,timeout=30)
                self.assertEqual(model.returncode,0,model.stderr)
                self.assertTrue(json.loads(model.stdout)['hint_reloaded'])
                self.assertEqual(export.capture(args.source),source)
                edited=Path(report['edit_files'][0]);edited.write_bytes(edited.read_bytes()+b'\n')
                kept=export.capture(args.output)
                with self.assertRaises(export.ExportError) as caught:export.run(args)
                self.assertEqual(caught.exception.code,'draft.exists')
                self.assertEqual(export.capture(args.output),kept)
                self.assertFalse(self.store.exists())

    def test_draft_includes_and_rejected_source_leave_inputs_intact(self):
        args=self.draft_args()
        file=args.source/'documents/positive_integer.paths.md';original=file.read_bytes()
        fragment=file.parent/'parts/positive.inc.md';fragment.parent.mkdir()
        fragment.write_bytes(original);file.write_text('@include parts/positive.inc.md\n')
        report=export.run(args)
        self.assertEqual((Path(report['documents'])/'parts/positive.inc.md').read_bytes(),original)
        args.output=self.root/'invalid draft'
        fragment.write_bytes(original.replace(b'@template linear.v1',b'@template linear.v99',1))
        with self.assertRaises(export.ExportError) as caught:export.run(args)
        self.assertEqual(caught.exception.code,'target.rejected')
        self.assertEqual(caught.exception.diagnostics[0]['file'],'linear_repetitions/parts/positive.inc.md')
        self.assertGreater(caught.exception.diagnostics[0]['line'],0)
        self.assertFalse(args.output.exists())
        fragment.write_bytes(original)
        outside=self.root/'outside.md';outside.write_bytes(original);fragment.unlink();fragment.symlink_to(outside)
        with self.assertRaises(export.ExportError) as caught:export.run(args)
        self.assertEqual(caught.exception.code,'path.inventory')
        self.assertFalse(args.output.exists())
        self.assertEqual(outside.read_bytes(),original)

    def test_draft_destination_guards_and_snapshot_capture(self):
        args=self.draft_args();source=export.capture(args.source)
        for output,code in ((args.source/'draft','path.overlap'),(args.source.parent/'draft','draft.location'),
                            (ROOT/'build/question-batches/draft-guard','path.overlap'),
                            (self.root/'custom store/generations/draft','draft.location')):
            with self.subTest(output=output):
                if 'custom store' in str(output):
                    export.write_tree(self.root/'custom store',{'active.json':b'unchanged store sentinel'})
                args.output=output
                with self.assertRaises(export.ExportError) as caught:export.run(args)
                self.assertEqual(caught.exception.code,code)
                self.assertFalse(output.exists())
                self.assertEqual(export.capture(args.source),source)
        self.assertEqual((self.root/'custom store/active.json').read_bytes(),b'unchanged store sentinel')
        alias=self.root/'alias';alias.symlink_to(self.root,target_is_directory=True);args.output=alias/'new draft'
        with self.assertRaises(export.ExportError) as caught:export.run(args)
        self.assertEqual(caught.exception.code,'path.symlink')
        args.output=self.root/'captured draft';inspect=export.Target.inspect_bytes
        entry=args.source/'documents/positive_integer.paths.md';original=entry.read_bytes()
        def inspect_then_edit(target,documents):
            result=inspect(target,documents)
            entry.write_bytes(original.replace(b'@template linear.v1',b'@template linear.v99',1))
            return result
        with patch.object(export.Target,'inspect_bytes',inspect_then_edit):report=export.run(args)
        self.assertEqual((Path(report['documents'])/entry.name).read_bytes(),original)
        self.assertNotEqual(entry.read_bytes(),original)

    def test_arithmetic_diversity_and_stable_extension(self):
        questions = batch.generate(12)
        self.assertEqual(batch.generate(24)[:12], questions)
        self.assertEqual(len({q['id'] for q in questions}), 12)
        self.assertEqual({g: sum(q['group'] == g for q in questions) for g in batch.GROUPS}, dict.fromkeys(batch.GROUPS, 4))
        positions = set()
        for q in batch.generate(36):
            self.assertTrue(batch.verify(q)['accepted'])
            positions.update(s['choices'].index(s['answer']) for s in q['steps'])
        self.assertEqual(positions, {0, 1, 2})
        self.assertEqual({name:export.sha(data) for name,data in batch.documents(questions).items()}, {
            'fractions.paths.md':'4cc101d3fdb97dd1d224dee1c8063fcbdd063048903dcf930a373e6f6cca2db7',
            'integers.paths.md':'639c5d5928d19ebefcec98cc5ba56c8e6c7a0247dffd5369210138ff35f76480',
            'negative.paths.md':'530f40f82ddceceb57bc44169ab6361d13e42dec0c1e29dfefba879e5b6042f4'})
        for count in (0, -3, 4, 39, True):
            with self.assertRaises(export.ExportError):
                batch.generate(count)
        for defect in range(4):
            q = copy.deepcopy(questions[0])
            if defect == 0:
                q['answer'][0] = '999'
            elif defect == 1:
                q['states'][1][1][2] = '999'
            elif defect == 2:
                q['steps'][0]['choices'][1] = q['steps'][0]['choices'][0]
            else:
                q['steps'][1]['answer'] = '0'
            with self.assertRaises(export.ExportError):
                batch.verify(q)

    def test_export_publish_repeat_and_append(self):
        first = batch.run(self.args())
        self.assertFalse(first['publication']['published'])
        self.assertFalse((self.store / 'active.json').exists())
        self.assertEqual(first['route_checks']['routes'], 60)
        self.assertEqual(first['route_checks']['wrong_choices'], 216)
        author = Path(first['generated_authoring'])
        generated = export.capture(author)
        published = batch.run(self.args(publish=True))
        self.assertTrue(published['publication']['published'])
        active = (self.store / 'active.json').read_bytes()
        stamp = (self.store / 'active.json').stat().st_mtime_ns
        before = export.Target(OPTIONS.target).inspect(store=self.store)['catalogue']['questions']
        again = batch.run(self.args(publish=True))
        self.assertTrue(again['publication']['unchanged'])
        self.assertEqual((self.store / 'active.json').read_bytes(), active)
        self.assertEqual((self.store / 'active.json').stat().st_mtime_ns, stamp)
        self.assertEqual(export.capture(author), generated)
        more = batch.run(self.args(count=24, version=2, publish=True))
        after = export.Target(OPTIONS.target).inspect(store=self.store)['catalogue']['questions']
        stamps = {q['id']: q['stamp_sha256'] for q in after}
        self.assertTrue(all(stamps[q['id']] == q['stamp_sha256'] for q in before))
        self.assertEqual(len(after), len(before) + 12)
        active = (self.store / 'active.json').read_bytes()
        with self.assertRaisesRegex(export.ExportError, 'Existing question must remain unchanged'):
            batch.run(self.args(count=6, version=3, publish=True))
        self.assertEqual((self.store / 'active.json').read_bytes(), active)
        self.assertEqual(more['route_checks']['routes'], 120)

    def test_bad_fresh_batch_cannot_publish_a_previous_good_export(self):
        result = batch.run(self.args(count=3, publish=True))
        active = (self.store / 'active.json').read_bytes()
        bad = batch.generate(3);bad[0]['answer'][0] = '999'
        with patch.object(batch, 'generate', return_value=bad), self.assertRaises(export.ExportError):
            batch.run(self.args(count=3, publish=True))
        self.assertEqual((self.store / 'active.json').read_bytes(), active)
        source = Path(result['generated_authoring']) / 'documents/integers.paths.md'
        source.write_text(source.read_text() + '\nChanged after verification.\n')
        with self.assertRaises(export.ExportError):
            batch.run(self.args(count=3, publish=True))
        self.assertEqual((self.store / 'active.json').read_bytes(), active)

    def test_compiler_failure_has_source_and_no_publication(self):
        original = batch.documents
        def damaged(questions):
            docs = original(questions)
            q = next(q for q in questions if q['group'] == 'integers')
            old = ('@after ' + batch.matrix(q['states'][1])).encode()
            docs['integers.paths.md'] = docs['integers.paths.md'].replace(old, b'@after [1, 1 | 1] [0, 1 | 999]', 1)
            return docs
        with patch.object(batch, 'documents', side_effect=damaged), self.assertRaises(export.ExportError) as caught:
            batch.run(self.args(count=3, publish=True))
        d = caught.exception.diagnostics[0]
        self.assertEqual(d['field'], 'after')
        self.assertEqual(d['file'], 'integers.paths.md')
        self.assertGreater(d['line'], 0)
        self.assertIn('accepted choice', d['message'])
        self.assertFalse((self.store / 'active.json').exists())

    def test_command_reports_rejection_as_json(self):
        result = subprocess.run([sys.executable, '-B', str(ROOT / 'tools/build_question_batch.py'), '--count', '4'],
                                text=True, capture_output=True, timeout=30)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(json.loads(result.stderr)['code'], 'batch.count')
        self.assertEqual(result.stdout, '')

    def test_worked_upgrade_preserves_published_cards_and_replays_saves(self):
        original=batch.run(self.args(publish=True))
        old_source=Path(original['generated_authoring'])/'documents'
        old_bytes=export.capture(old_source)
        before=export.Target(OPTIONS.target).inspect(store=self.store)['catalogue']
        upgraded=batch.run(self.args(version=2,format_version=2,publish=True))
        source=Path(upgraded['generated_authoring'])/'documents'
        after=export.Target(OPTIONS.target).inspect(store=self.store)['catalogue']
        self.assertEqual(upgraded['worked_questions'],12)
        self.assertEqual(upgraded['retained_questions'],12)
        self.assertEqual(upgraded['route_checks']['routes'],120)
        self.assertEqual(upgraded['route_checks']['wrong_choices'],432)
        self.assertEqual(upgraded['route_checks']['disclosure_checks'],528)
        self.assertTrue(all(export.read_bytes(source/name)==data for name,data in old_bytes.items()))
        for kind in ('questions','readings','chapters','subjects'):
            indexed={row['id']:row for row in after[kind]}
            self.assertTrue(all(indexed[row['id']]==row for row in before[kind]),kind)
        self.assertEqual(len(after['questions']),len(before['questions'])+12)
        migration=subprocess.run([str(OPTIONS.model),'--question-batch-upgrade',str(old_source),str(source)],
                                 text=True,capture_output=True,timeout=30)
        self.assertEqual(migration.returncode,0,migration.stderr)
        self.assertTrue(json.loads(migration.stdout)['save_replay'])
        active=(self.store/'active.json').read_bytes()
        repeat=batch.run(self.args(version=2,format_version=2,publish=True))
        self.assertTrue(repeat['publication']['unchanged'])
        self.assertEqual((self.store/'active.json').read_bytes(),active)
        more=batch.run(self.args(count=24,version=3,format_version=2,publish=True))
        expanded=export.capture(Path(more['generated_authoring'])/'documents')
        for name,data in export.capture(source).items():
            # Reading links grow; existing question text and IDs stay byte-stable.
            for card in data.decode().split('@question ')[1:]:
                self.assertIn('@question '+card.strip(),expanded[name].decode())
        with patch.object(batch,'REFERENCE_TEMPLATE',self.root/'invalid-template.md'):
            batch.REFERENCE_TEMPLATE.write_text('@question {{missing_field}}\n')
            with self.assertRaisesRegex(export.ExportError, 'Unknown field missing_field'):
                batch.run(self.args(version=4,format_version=2,publish=True))
        self.assertFalse((self.root/'batches/4').exists())

    def test_worked_template_number_signs_and_structure(self):
        template=batch.REFERENCE_TEMPLATE.read_text()
        self.assertEqual([batch.tex(v) for v in ('0','-3/2','2/3','-4','4')],
                         ['0',r'-\frac{3}{2}',r'\frac{2}{3}','-4','4'])
        for n,q in enumerate(batch.generate(36),1):
            fields=batch.worked_fields(q,n)
            text=batch.fill_template(template,fields)
            self.assertEqual(text.count('@hint\n'),3)
            self.assertNotIn('{{',text)
            self.assertNotIn('+-',text)
            for i in range(1,4):
                self.assertIn('$$'+fields[f'calculation_{i}']+'$$',text)
                self.assertEqual(fields[f'calculation_{i}'].count('&='),3)
                self.assertIn(batch.matrix_tex(q['states'][i]),text)
                hint=text.split('@hint\n')[i].split('@wrong')[0]
                self.assertNotIn('$',hint)
                self.assertNotIn(batch.matrix(q['states'][i]),hint)
            # Original equations are actually substituted, not just the final identity.
            for i,row in enumerate(q['states'][0],1):
                a,b,c=map(Fraction,row);x,y=map(Fraction,q['answer'])
                self.assertEqual(a*x+b*y,c)
                self.assertTrue(fields[f'check_{i}'].endswith('='+batch.tex(c)+'.'))
            self.assertEqual(text.count('$$')%2,0)
            self.assertEqual(text.count('{'),text.count('}'))

    def test_linear_recipe_template_and_all_six_cases(self):
        spec=json.loads(batch.linear.SPEC.read_text())
        small=batch.linear_questions(12,spec);large=batch.linear_questions(36,spec)
        self.assertEqual(large[:12],small)
        self.assertEqual({g:sum(q['stratum']==g for q in small) for g in batch.linear.STRATA},
                         dict.fromkeys(batch.linear.STRATA,2))
        template=batch.LINEAR_TEMPLATE.read_text()
        positions=set()
        for q in large:
            batch.linear.verify_question(q)
            fields=batch.linear_fields(q,1);text=batch.fill_template(template,fields,batch.LINEAR_TEMPLATE)
            a,b,c=(Fraction(q['parameters'][k]) for k in ('a','b','c'));answer=Fraction(q['answer']['value'])
            self.assertEqual((c-b)/a,answer)
            self.assertEqual(a*answer+b,c)
            for state in q['states']:
                aa,bb,cc=(Fraction(state[k]) for k in ('a','b','c'))
                self.assertEqual(aa*answer+bb,cc)
                self.assertIn(batch.linear.equation(state),text)
                self.assertNotIn('\\',batch.linear.equation(state,typeset=False))
            for step in q['steps']:
                positions.add(next(i for i,o in enumerate(step['choices']) if o['id']=='correct'))
            self.assertIn('$$'+fields['check']+'.$$',text)
            self.assertEqual(text.count('@hint\n'),2)
            for hint in text.split('@hint\n')[1:]:
                self.assertNotIn('$',hint.split('@wrong')[0])
            self.assertNotIn('{{',text)
            self.assertNotIn('+-',text)
            self.assertIn(r'\frac',text)
            self.assertEqual(text.count('{'),text.count('}'))
            delimiters=re.findall(r'(?<!\\)\$\$|(?<!\\)\$',text)
            self.assertTrue(all(delimiters[i]==delimiters[i+1] for i in range(0,len(delimiters),2)))
            self.assertGreater(text.count('$$'),20)
        self.assertEqual(positions,{0,1,2})
        for count in (0,3,7,42,True):
            with self.assertRaises(export.ExportError):batch.linear_questions(count,spec)
        original=batch.linear.runtime_bank(spec)
        self.assertEqual(json.dumps(original,ensure_ascii=False,indent=2)+'\n',
                         (ROOT/'content/corpus/linear_support.json').read_text())

    def test_linear_publish_extend_save_and_failure(self):
        before=export.Target(OPTIONS.target).inspect(documents=ROOT/'content/write')['catalogue']
        first=batch.run(self.args(count=6,family='linear',publish=True))
        old_source=Path(first['generated_authoring'])/'documents'
        more=batch.run(self.args(count=12,version=2,family='linear',publish=True))
        source=Path(more['generated_authoring'])/'documents'
        self.assertEqual(more['route_checks']['routes'],60)
        self.assertEqual(more['route_checks']['wrong_choices'],144)
        self.assertEqual(more['route_checks']['disclosure_checks'],384)
        self.assertEqual(more['worked_questions'],12)
        after=export.Target(OPTIONS.target).inspect(store=self.store)['catalogue']
        for kind in ('questions','readings','chapters','subjects'):
            indexed={row['id']:row for row in after[kind]}
            self.assertTrue(all(indexed[row['id']]==row for row in before[kind]),kind)
        self.assertEqual(len(after['questions']),len(before['questions'])+12)
        migration=subprocess.run([str(OPTIONS.model),'--question-batch-upgrade',str(old_source),str(source)],
                                 capture_output=True,text=True,timeout=30)
        self.assertEqual(migration.returncode,0,migration.stderr)
        self.assertTrue(json.loads(migration.stdout)['save_replay'])
        active=(self.store/'active.json').read_bytes()
        self.assertTrue(batch.run(self.args(version=2,family='linear',publish=True))['publication']['unchanged'])
        self.assertEqual((self.store/'active.json').read_bytes(),active)
        with self.assertRaisesRegex(export.ExportError,'Existing question must remain unchanged'):
            batch.run(self.args(count=6,version=3,family='linear',publish=True))
        self.assertEqual((self.store/'active.json').read_bytes(),active)
        original=batch.linear_fields
        def wrong(q,n):
            fields=original(q,n);fields['after_1']='3x=999';return fields
        with patch.object(batch,'linear_fields',side_effect=wrong),self.assertRaises(export.ExportError) as caught:
            batch.run(self.args(version=4,family='linear',publish=True))
        self.assertTrue(caught.exception.diagnostics)
        self.assertEqual((self.store/'active.json').read_bytes(),active)
        self.assertFalse((self.root/'batches/4').exists())
        damaged=self.root/'broken-linear.md.in';damaged.write_text('@question {{typo}}\n')
        with patch.object(batch,'LINEAR_TEMPLATE',damaged),self.assertRaisesRegex(export.ExportError,'broken-linear.md.in:1: Unknown field typo'):
            batch.run(self.args(version=4,family='linear',publish=True))
        # The original matrix format remains the default CLI family.
        result=subprocess.run([sys.executable,'-B',str(ROOT/'tools/build_question_batch.py'),'--family','linear','--count','3'],
                              capture_output=True,text=True,timeout=30)
        self.assertNotEqual(result.returncode,0)
        self.assertEqual(json.loads(result.stderr)['code'],'batch.count')

    def test_reference_card_arithmetic_and_authored_math(self):
        source = ROOT / 'content/authoring/learning/matrix_reference/documents'
        result = subprocess.run([str(OPTIONS.model), '--reference-card', str(source)],
                                text=True, capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report['disclosure_checks'], 48)
        self.assertEqual(report['hint_format_rejections'], 8)
        content = report['question']
        def rows(text):
            match = re.fullmatch(r'\[([^]]+)\]\s+\[([^]]+)\]', text)
            self.assertIsNotNone(match)
            values = [[str(Fraction(v.strip())) for v in re.split(r'[,|]', row)] for row in match.groups()]
            self.assertTrue(all(len(row) == 3 for row in values))
            return values
        supplied = content['support']
        steps = []
        for step, details in zip(content['steps'], supplied['steps']):
            accepted = [i for i, option in enumerate(step['options']) if option['id'] in step['accepted_option_ids']]
            self.assertEqual(len(accepted), 1)
            steps.append(dict(operation=details['operation'], choices=details['responses'], answer=details['responses'][accepted[0]]))
            self.assertTrue(step['hint'])
            self.assertNotIn('$$', step['hint'])
            self.assertIn('$$', details['teaching'])
            self.assertIn(r'\frac', details['teaching'])
            self.assertIsNone(re.search(r'\[-?\d+\s*,', details['teaching']))
            for prose in (details['definitions'], details['teaching']):
                delimiter = None
                for token in re.findall(r'(?<!\\)\$\$|(?<!\\)\$', prose):
                    if delimiter is None:
                        delimiter = token
                    else:
                        self.assertEqual(token, delimiter)
                        delimiter = None
                self.assertIsNone(delimiter, 'Math delimiters must be paired')
                depth = 0
                for c in prose:
                    depth += (c == '{') - (c == '}')
                    self.assertGreaterEqual(depth, 0)
                self.assertEqual(depth, 0, 'TeX grouping must be balanced')
        checked = batch.verify(dict(id=content['id'], answer=['-1/3', '-3/2'],
            states=[rows(supplied['equation']), *[rows(step['equation']) for step in supplied['steps']]], steps=steps))
        self.assertEqual(checked['determinant'], '-2')
        self.assertEqual(checked['wrong_choices_checked'], 6)

    def test_teaching_sequence_reasoning_arithmetic_and_replay(self):
        source=ROOT/'content/authoring/learning/linear_teaching_sequence/documents'
        result=subprocess.run([str(OPTIONS.model),'--teaching-sequence',str(source)],capture_output=True,text=True,timeout=45)
        self.assertEqual(result.returncode,0,result.stderr)
        report=json.loads(result.stdout);self.assertEqual(report['routes'],16)
        self.assertEqual(report['wrong_choices'],44)
        self.assertTrue(report['save_replay'] and report['matrix_feedback'] and report['live_feedback_edit'])
        cards=report['questions'];self.assertEqual(len(cards),8)
        self.assertEqual([q['title'][:2] for q in cards],[f'{i:02}' for i in range(1,9)])
        def number(label):
            value=label.removeprefix('x=')
            value=re.sub(r'\\frac\{(-?\d+)\}\{(\d+)\}',r'\1/\2',value)
            return Fraction(value)
        def correct_by_value(step,expected):
            independently_correct=[o['id'] for o in step['options'] if number(o['label'])==expected]
            self.assertEqual(len(independently_correct),1)
            self.assertEqual(step['accepted_option_ids'],independently_correct)
        def equation(text):
            # Bounded test oracle for the actual authored ax+b=c displays in this chapter.
            match=re.fullmatch(r'(-?\d*)x([+-]\d+(?:/\d+)?)?=(-?\d+(?:/\d+)?)',text)
            self.assertIsNotNone(match,text)
            a,b,c=match.groups()
            return Fraction('-1' if a=='-' else a or '1'),Fraction(b or '0'),Fraction(c)
        # Independent exact arithmetic from the original givens, not the authored answer IDs.
        cases=[(9,9,36),(5,4,29),(9,9,36),(4,-7,13),(6,12,30),(7,-5,16),(-3,6,15),(4,3,5)]
        for index,(card,(a,b,c)) in enumerate(zip(cards,cases)):
            answer=Fraction(c-b,a);self.assertEqual(a*answer+b,c)
            q=card['question'];steps=q['steps']
            self.assertEqual(equation(q['equation']),(a,b,c))
            for state in q['working_states']:
                if 'x' in state['display']:
                    aa,bb,cc=equation(state['display']);self.assertNotEqual(aa,0)
                    self.assertEqual((cc-bb)/aa,answer,'Actual displayed working preserves the unique solution')
                else:
                    self.assertEqual(state['display'],f'{a}({answer}){b:+}={c}')
                    self.assertEqual(a*answer+b,c,'Actual final substitution matches both original sides')
            for step in steps:
                wrong=[o for o in step['options'] if o['id'] not in step['accepted_option_ids']]
                self.assertEqual(len(wrong),2)
                self.assertEqual(len({o['wrong_feedback'] for o in wrong}),2)
                self.assertTrue(all('wrong_feedback' not in o for o in step['options'] if o['id'] in step['accepted_option_ids']))
            if index<2:
                correct_by_value(steps[0],c-b);correct_by_value(steps[1],answer)
                for step in q['support']['steps']:
                    self.assertLessEqual(len(step['definitions'].split()),65)
                    self.assertLessEqual(len(step['teaching'].split()),90)
            elif index in (3,4):correct_by_value(steps[1],answer)
            elif index==5:
                equations=[equation(o['label']) for o in steps[0]['options']]
                valid=[o['id'] for o,(aa,bb,cc) in zip(steps[0]['options'],equations) if aa*answer+bb==cc]
                self.assertEqual(steps[0]['accepted_option_ids'],valid);correct_by_value(steps[1],answer)
            elif index>=6:
                correct_by_value(steps[0],answer)
                if index==7:correct_by_value(steps[1],a*answer+b)
        # These three keys express the stated conceptual task. Other equivalent methods are not called invalid.
        for index,label in ((2,r'\text{Both sides change equally}'),(3,r'\text{Add 7 to both sides}'),(4,r'\text{The added constant}')):
            step=cards[index]['question']['steps'][0]
            self.assertEqual([o['label'] for o in step['options'] if o['id'] in step['accepted_option_ids']],[label])
        self.assertEqual(Fraction(12,6),2) # the omitted division in the repair card
        self.assertEqual(-7+7,0) # direct cancellation requested by the method card
        self.assertIn('is valid',cards[3]['question']['steps'][0]['options'][1]['wrong_feedback'])

    def test_feedback_directives_reject_at_source_and_old_stamps_stay_stable(self):
        source=ROOT/'content/authoring/learning/linear_teaching_sequence/documents/chapter.paths.md'
        text=source.read_text();line=next(row for row in text.splitlines() if row.startswith('@feedback 11 |'))
        mutations=[(line,'@feedback 999 | Unknown choice.'),(line,'@feedback 11 |   '),
                   (line,line+'\n'+line),(line,'@feedback 13 | A correct choice cannot be wrong.'),
                   (line,'@feedback 11 | '+('x'*2001)),
                   ('@template lesson.v2','@template lesson.v2\n@feedback 11 | Wrong scope.')]
        for included in (False,True):
            for old,new in mutations:
                with self.subTest(included=included,new=new[:60]):
                    path=self.root/'bad-documents';path.mkdir(exist_ok=True)
                    file=path/('part.inc.md' if included else 'chapter.paths.md')
                    file.write_text(text.replace(old,new,1))
                    if included:(path/'chapter.paths.md').write_text('@include part.inc.md\n')
                    with self.assertRaises(export.ExportError) as caught:export.Target(OPTIONS.target).inspect(documents=path)
                    d=caught.exception.diagnostics[0]
                    self.assertEqual(d['file'],file.name);self.assertEqual(d['field'],'feedback')
                    self.assertGreater(d['line'],0)
        # Omitting the optional directive must preserve the exact pre-feature question JSON.
        reference=ROOT/'content/authoring/learning/matrix_reference/documents'
        result=subprocess.run([str(OPTIONS.model),'--reference-card',str(reference)],capture_output=True,text=True,timeout=30)
        self.assertEqual(result.returncode,0,result.stderr)
        q=json.loads(result.stdout)['question']
        self.assertTrue(all('wrong_feedback' not in o for s in q['steps'] for o in s['options']))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--target', type=Path, required=True)
    parser.add_argument('--model', type=Path, required=True)
    OPTIONS, remaining = parser.parse_known_args()
    OPTIONS.target = OPTIONS.target.resolve();OPTIONS.model = OPTIONS.model.resolve()
    unittest.main(argv=[sys.argv[0], *remaining])
