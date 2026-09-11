#!/usr/bin/env python3
"""Independent original-polynomial checks and real compiled-content regressions."""
import copy
from fractions import Fraction
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import generate as g
import build_question_batch as batch
import export_learning as export


def shifted_horner(c,a):
    """Repeated polynomial multiplication by (a+h); no binomial or derivative helper."""
    result=[0]
    for coefficient in c:
        multiplied=[0]*(len(result)+1)
        for i,value in enumerate(result):
            multiplied[i]+=a*value
            multiplied[i+1]+=value
        multiplied[0]+=coefficient
        result=multiplied
    return result[:len(c)]


def evaluate(c,x):
    total=Fraction(0)
    for coefficient in c: total=total*x+coefficient
    return total


class CalculusProduction(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.recipe=json.loads((g.ROOT/'recipe.json').read_text()); cls.questions=[]; cls.documents={}
        for family in g.FAMILIES:
            allq=[]; blocks=[]
            for selected in g.SETS:
                qs=g.sequence(cls.recipe,family,selected)['questions']; allq.extend(qs)
                blocks.extend(g.question_text(q,pos) for q,pos in zip(qs,g.POSITIONS[family,selected]))
            cls.questions.extend(allq)
            cls.documents[family+'.paths.md']=batch.chapter_text(g.values(family,allq,'\n\n'.join(blocks))).encode()
        cls.certificates=[batch.reasoning_certificate(q,g.CHECKERS) for q in cls.questions]
        cls.compiled=cls.compile(cls.documents)['questions']

    @staticmethod
    def compile(documents):
        with tempfile.TemporaryDirectory(prefix='paths-calculus-test-',dir='/private/tmp') as directory:
            root=Path(directory); export.write_tree(root,documents)
            result=subprocess.run([str(batch.ROOT/'b/paths_learning_document_tests'),'--question-batch',str(root)],capture_output=True,text=True,timeout=60)
            if result.returncode: raise AssertionError(result.stderr or result.stdout)
            return json.loads(result.stdout)

    def test_original_polynomial_identities_and_options(self):
        for q,cert in zip(self.questions,self.certificates):
            with self.subTest(question=q['id']):
                c,a=q['case']['coefficients'],q['case']['at']; e=shifted_horner(c,a)
                degree=len(c)-1
                # Direct closed forms are separate from both production loops.
                slope=2*c[0]*a+c[1] if degree==2 else 3*c[0]*a*a+2*c[1]*a+c[2]
                self.assertEqual(e,cert['evidence']['facts']['shifted_coefficients'])
                self.assertEqual(e[1],slope)
                for h in map(Fraction,(-2,-1,1,2,'1/2')):
                    quotient=(evaluate(c,a+h)-evaluate(c,a))/h
                    self.assertEqual(quotient,sum(v*h**i for i,v in enumerate(e[1:])))
                hp=g.polynomial(list(reversed(e[1:])),'h')
                if q['role']=='explain_step':
                    self.assertEqual(cert['expected']['after'],[rf'\frac{{f({a}+h)-f({a})}}{{h}}={hp},\quad h\ne0'])
                if q['role']=='independent':
                    self.assertIn(rf'\quad a={a}',cert['expected']['given'])
                if q['role'] in ('worked_check','repair_error','independent'):
                    step=cert['expected']['steps'][-1]
                    self.assertEqual(int(step['answer']),slope)
                    self.assertEqual(sum(int(v)==slope for v in step['choices']),1)
        self.assertEqual(len(self.questions),36)
        batch.verify_role_content(self.questions,self.compiled,self.certificates)

    def test_rejects_changed_key_working_and_missing_point(self):
        index=next(i for i,q in enumerate(self.questions) if q['role']=='independent')
        for defect in ('key','after','point'):
            broken=copy.deepcopy(self.compiled); actual=broken[index]['question']
            if defect=='key':
                step=actual['steps'][0]; step['accepted_option_ids']=[next(o['id'] for o in step['options'] if o['id'] not in step['accepted_option_ids'])]
            elif defect=='after': actual['working_states'][1]['display']="f'(99)=999"
            else: actual['equation']=actual['equation'].split(',\\quad')[0]
            with self.assertRaises(export.ExportError):
                batch.verify_role_content(self.questions,broken,self.certificates)
        index=next(i for i,q in enumerate(self.questions) if q['role']=='explain_step')
        broken=copy.deepcopy(self.compiled); q=self.questions[index]; t=g.context(q)
        broken[index]['question']['working_states'][1]['display']=rf"{t['quotient']}={t['d']},\quad h\ne0"
        with self.assertRaises(export.ExportError):
            batch.verify_role_content(self.questions,broken,self.certificates)
        # The higher-order term matters even when the derivative is correct.
        q=self.questions[index]; changed=g.expansion(q['case']['coefficients'],q['case']['at']); changed[2]+=1
        with patch.object(g,'expansion',return_value=changed):
            wrong=batch.reasoning_certificate(q,g.CHECKERS)
        self.assertNotEqual(wrong['evidence']['facts']['shifted_coefficients'],shifted_horner(q['case']['coefficients'],q['case']['at']))

    def test_real_markdown_edit_reaches_compiled_prompt(self):
        for family in g.FAMILIES:
            q=g.sequence(self.recipe,family,'teaching')['questions'][0]
            path=g.ROOT/'families'/family/'questions.paths.md.in'; template=path.read_text()
            original='Which limit or value defines the derivative at a={{a}}?'
            changed=template.replace(original,'Select the derivative definition at the fixed input a={{a}}.')
            self.assertNotEqual(template,changed)
            text=g.question_text(q,1,template=changed)
            doc=batch.chapter_text(g.values(family,[q],text)).encode()
            compiled=self.compile({'edit.paths.md':doc})['questions'][0]['question']
            self.assertIn('Select the derivative definition',compiled['steps'][0]['prompt'])

    def test_repair_disclosure_balancing_and_degenerate_cases(self):
        for family in g.FAMILIES:
            for selected in g.SETS:
                self.assertEqual(sorted(g.POSITIONS[family,selected]),[1,1,2,2,3,3])
                for q,pos in zip(g.sequence(self.recipe,family,selected)['questions'],g.POSITIONS[family,selected]):
                    if q['role']!='repair_error': continue
                    cert=batch.reasoning_certificate(q,g.CHECKERS)
                    self.assertEqual(cert['expected']['steps'][0]['answer'],'L2')
                    first=g.question_text(q,pos).split('@step 20')[0]
                    self.assertNotIn('coefficient differentiation',first)
                    self.assertIn('Subtracting f(',first)
                    self.assertNotIn("@why The corrected difference",first)
        for badcase in ({'family':'difference_quotient','coefficients':[0,0,3],'at':0},
                        {'family':'polynomial_rules','coefficients':[1,2],'at':0},
                        {'family':'difference_quotient','coefficients':[1,0,0],'at':4}):
            q=copy.deepcopy(self.questions[0]); q['case']=badcase
            with self.assertRaises(export.ExportError): batch.reasoning_certificate(q,g.CHECKERS)
        cert=copy.deepcopy(self.certificates[0]); cert['expected']['steps'][0]['choices'][1]=cert['expected']['steps'][0]['answer']
        broken=copy.deepcopy(self.certificates); broken[0]=cert
        with self.assertRaises(export.ExportError): batch.verify_role_content(self.questions,self.compiled,broken)


if __name__=='__main__': unittest.main(verbosity=2)
