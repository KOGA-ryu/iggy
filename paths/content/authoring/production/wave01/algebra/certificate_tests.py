"""Independent finite-pool checks for the Wave 01 algebra providers."""
import copy
from fractions import Fraction
import json
import sys
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[5]
sys.path.insert(0,str(ROOT/"tools"))
import build_question_batch as batch
import export_learning as export
import generate

class ProductionAlgebraCertificates(unittest.TestCase):
    def compiled_route(self, question, certificate, template=None):
        family=question["case"]["family"]; old=generate.READING_ID; generate.READING_ID="algebra_test_r"
        try:
            text=generate.question_text(question,certificate,0,template=template)
            values=dict(subject="algebra",subject_title="Algebra",chapter="worked_linear_practice",chapter_title="Worked linear practice",
                reading_id="algebra_test_r",reading_title="Test reading",lesson_blocks=generate.lesson_blocks(family,"test","test"),
                practice_links=f"@practice {question['id']}\n",questions=text)
            document=batch.chapter_text(values).encode()
        finally: generate.READING_ID=old
        with tempfile.TemporaryDirectory(prefix="paths-algebra-test-",dir="/private/tmp") as directory:
            root=Path(directory); export.write_tree(root,{"test.paths.md":document})
            result=subprocess.run([str(ROOT/"b/paths_learning_document_tests"),"--question-batch",str(root)],capture_output=True,text=True,timeout=60)
            if result.returncode: raise AssertionError(result.stderr or result.stdout)
            return json.loads(result.stdout)["questions"]
    def test_all_36_cases_have_unique_exact_choices_and_84_distractors(self):
        checks=[]
        for family in generate.POOL:
            for set_name in generate.SETS:
                package=f"prod01_algebra_{family}_{set_name}"; sequence=generate.sequence(family,set_name,package)
                provider=generate.load_provider(Path("."))
                current=[batch.reasoning_certificate(q,provider) for q in sequence["questions"]]
                checks.extend(current)
                self.assertEqual(sum(x["steps_checked"] for x in current),7)
                self.assertEqual(list(generate.POSITION[(family,set_name)]).count(0),2)
                self.assertEqual(list(generate.POSITION[(family,set_name)]).count(1),2)
                self.assertEqual(list(generate.POSITION[(family,set_name)]).count(2),2)
        self.assertEqual(len(set(generate.POSITION.values())),6)
        self.assertEqual(len(checks),36); self.assertEqual(sum(c["wrong_choices_checked"] for c in checks),84)
    def test_fractional_seeds_are_preserved_without_rounding(self):
        provider=generate.load_provider(Path("."))
        for family,sets in generate.POOL.items():
            for selected,seeds in sets.items():
                questions=generate.sequence(family,selected,"seed")['questions']
                for q,seed in zip(questions,seeds):
                    with self.subTest(family=family,set=selected,role=q['role']):
                        certificate=batch.reasoning_certificate(q,provider)
                        self.assertEqual(Fraction(certificate['evidence']['facts']['solution']),Fraction(seed[2]))
        for family,seed in (("signed_balance",(5,4,Fraction(3,2),0)),("distributive_linear",(-5,-2,Fraction(3,2),-1))):
            with self.assertRaisesRegex(export.ExportError,'rounding is not allowed'):
                generate.make_case(family,'independent',seed)
        q=generate.sequence('signed_balance','teaching','guard')['questions'][3]
        q['case']['b']=1
        with self.assertRaisesRegex(export.ExportError,'constant already removed'):
            batch.reasoning_certificate(q,provider)
    def test_degenerate_changed_and_equivalent_cases_reject(self):
        provider=generate.load_provider(Path(".")); q=generate.sequence("signed_balance","teaching","x")["questions"][0]
        bad=copy.deepcopy(q); bad["case"]["a"]=0
        with self.assertRaises(export.ExportError): batch.reasoning_certificate(bad,provider)
        bad=copy.deepcopy(q); bad["case"]["family"]="unknown"
        with self.assertRaises(export.ExportError): batch.reasoning_certificate(bad,provider)
        q=generate.sequence("distributive_linear","fresh_check","x")["questions"][-1]
        given,after,steps,facts=provider["independent"](q)
        with self.assertRaises(export.ExportError): batch.reasoning_certificate(q,{"independent":lambda _: (given,after,[([steps[0][0][0]]*3,steps[0][1])],facts)})
    def test_original_equations_and_false_distribution_are_independent(self):
        provider=generate.load_provider(Path("."))
        for family in generate.POOL:
            for set_name in generate.SETS:
                for question in generate.sequence(family,set_name,"x")["questions"]:
                    case=question["case"]; a,b=case["a"],case["b"]
                    if family=="signed_balance":
                        x=Fraction(case["c"]-b,a); self.assertEqual(a*x+b,case["c"])
                    else:
                        x=Fraction(case["e"]-case["d"],a)-b; self.assertEqual(a*(x+b)+case["d"],case["e"])
                distributive=generate.sequence("distributive_linear",set_name,"x")["questions"][2]
                given,after,steps,_=provider["choose_next_step"](distributive)
                a,b,d,e=(distributive["case"][key] for key in ("a","b","d","e"))
                missing=batch.tex(Fraction(b+d)); wrong_sign=batch.tex(Fraction(-a*b+d))
                self.assertNotEqual(steps[0][0][1],after[0]); self.assertNotEqual(steps[0][0][2],after[0])
                self.assertIn(missing,steps[0][0][1]); self.assertIn(wrong_sign,steps[0][0][2])
    def test_editable_templates_supply_every_role_prompt(self):
        for family in generate.POOL:
            template=(generate.SOURCE/"families"/family/"questions.paths.md.in").read_text()
            sequence=generate.sequence(family,"teaching","template")
            provider=generate.load_provider(Path("."))
            for position,question in enumerate(sequence["questions"]):
                certificate=batch.reasoning_certificate(question,provider)
                rendered=generate.question_text(question,certificate,position%3)
                self.assertIn(f"<!-- {question['role']} -->",template)
                self.assertIn("@step",rendered); self.assertIn("@why",rendered); self.assertNotIn("{{",rendered)
                if question['role']=='read_notation':
                    domain=next(line for line in rendered.splitlines() if line.startswith('@domain '))
                    self.assertNotIn(f"a={question['case']['a']}",domain)
                    self.assertNotIn(f"b={question['case']['b']}",domain)
            question=sequence["questions"][0]; certificate=batch.reasoning_certificate(question,provider)
            changed=template.replace("Which ordered pair names", "Select the ordered pair that names",1)
            self.assertNotEqual(template,changed)
            compiled=self.compiled_route(question,certificate,changed)[0]["question"]
            self.assertIn("Select the ordered pair that names",compiled["steps"][0]["prompt"])
    def test_false_key_and_false_reached_work_are_detectable(self):
        provider=generate.load_provider(Path(".")); q=generate.sequence("signed_balance","practice","x")["questions"][2]
        certificate=batch.reasoning_certificate(q,provider); route=self.compiled_route(q,certificate)
        batch.verify_role_content([q],route,[certificate])
        false_key=copy.deepcopy(route); false_key[0]["question"]["steps"][0]["accepted_option_ids"]=[12]
        with self.assertRaises(export.ExportError): batch.verify_role_content([q],false_key,[certificate])
        false_after=copy.deepcopy(route); false_after[0]["question"]["working_states"][1]["display"]="0=0"
        with self.assertRaises(export.ExportError): batch.verify_role_content([q],false_after,[certificate])
if __name__=="__main__": unittest.main(verbosity=2)
