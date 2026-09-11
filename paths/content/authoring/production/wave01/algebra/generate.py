#!/usr/bin/env python3
"""Build Wave 01 algebra through the existing compiler/model candidate gate.

The recipe constructs givens from exact seed values, but the staged
certificate provider derives every result from those givens with Fraction.
"""
import argparse
from fractions import Fraction
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess

import build_question_batch as batch
import check_authoring_pilot as pilot
import export_learning as export

ROOT = batch.ROOT
SOURCE = ROOT / "content/authoring/production/wave01/algebra"
OUTPUT = ROOT / "build/production/wave01/algebra"
SETS = ("teaching", "practice", "fresh_check")
ROLES = tuple(batch.EXERCISE_ROLES)
POSITION = {
    ("signed_balance", "teaching"): (0, 1, 2, 0, 1, 2),
    ("signed_balance", "practice"): (1, 2, 0, 1, 2, 0),
    ("signed_balance", "fresh_check"): (2, 0, 1, 2, 0, 1),
    ("distributive_linear", "teaching"): (0, 2, 1, 0, 2, 1),
    ("distributive_linear", "practice"): (1, 0, 2, 1, 0, 2),
    ("distributive_linear", "fresh_check"): (2, 1, 0, 2, 1, 0),
}
TITLES = {
    "read_notation": "01 Read the structure",
    "worked_check": "02 Complete a calculation",
    "choose_next_step": "03 Choose a goal-directed move",
    "explain_step": "04 Explain an inverse",
    "repair_error": "05 Repair the first error",
    "independent": "06 Solve a fresh equation",
}
OBJECTIVES = {
    "read_notation": "Identify the signed parts of the original equation.",
    "worked_check": "Complete one exact arithmetic result in a balanced calculation.",
    "choose_next_step": "Select the operation that preserves equality and meets the stated goal.",
    "explain_step": "Choose the inverse operation that recovers a preceding equation.",
    "repair_error": "Locate the first invalid line and correct its missing arithmetic.",
    "independent": "Select the exact value that satisfies the original equality.",
}

RECIPE=json.loads((SOURCE/"recipe.json").read_text())
export.require(RECIPE["format"]=="paths_wave01_algebra_pool" and RECIPE["version"]==1 and RECIPE["families"]==["signed_balance","distributive_linear"] and RECIPE["sets"]==list(SETS),"production.algebra","recipe family/set contract changed")
POOL={family:{set_name:[(row[0],row[1],Fraction(row[2]),row[3]) for row in rows]
              for set_name,rows in sets.items()} for family,sets in RECIPE["pool"].items()}
export.require(set(POOL)==set(RECIPE["families"]) and all(set(POOL[family])==set(SETS) and all(len(row)==4 for row in rows) for family,sets in POOL.items() for rows in sets.values()),"production.algebra","recipe pool is malformed")


def require(ok, message):
    export.require(ok, "production.algebra", message)


def make_case(family, role, seed):
    a, b, solution, d = seed
    if family == "signed_balance":
        c = Fraction(a) * Fraction(solution) + b
        require(c.denominator==1,"seed must produce an integral right side; rounding is not allowed")
        return dict(family=family, a=a, b=b, c=int(c), d=0, reverse=(role == "independent"))
    e = Fraction(a) * (Fraction(solution) + b) + d
    require(e.denominator==1,"seed must produce an integral right side; rounding is not allowed")
    return dict(family=family, a=a, b=b, d=d, e=int(e), reverse=False)


def sequence(family, set_name, package):
    questions=[]
    for role, seed in zip(ROLES, POOL[family][set_name]):
        questions.append(dict(id=f"{package}_{role}", role=role, title=TITLES[role], objective=OBJECTIVES[role],
                              prerequisites="Signed arithmetic, equality, and exact fractions as real numbers.",
                              case=make_case(family, role, seed)))
    return dict(format="paths_exercise_roles", format_version=1, questions=questions)


def load_provider(path):
    provider=SOURCE/"families/provider.py"
    spec=importlib.util.spec_from_file_location("wave01_algebra_provider",provider)
    module=importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module.CHECKERS


def reordered(labels, correct, position):
    rest=[label for label in labels if label != correct]
    return rest[:position] + [correct] + rest[position:]


def question_text(question, certificate, position, template=None):
    role=question["role"]; expected=certificate["expected"]; case=question["case"]; family=case["family"]
    a,b,d=case["a"],case["b"],case["d"]; right=case["c"] if family=="signed_balance" else case["e"]
    solution=Fraction(certificate["evidence"]["facts"]["solution"])
    substitution=(rf"{a}\left({batch.tex(solution)}\right)+({b})={right}" if family=="signed_balance" else
                  rf"{a}\left({batch.tex(solution)}+({b})\right)+({d})={right}")
    fields={"id":question["id"],"title":question["title"],"objective":question["objective"],"given":expected["given"],
            "reading_id":READING_ID,"a":str(a),"b":str(b),"d":str(d),"right":str(right),"product":str(a*b),
            "balance":str(right-b),"added":str(right+b),"substitution":substitution}
    for number,step in enumerate(expected["steps"],1):
        labels,answer=step["choices"],step["answer"]; index=position if number==1 else (position+1)%3
        ordered=reordered(labels,answer,index)
        fields[f"choices_{number}"]=batch.choices_text(ordered,index,number)
        fields[f"after_{number}"]=expected["after"][number-1]
        for code,label in zip(("a","b"),labels[1:]): fields[f"wrong_{code}_{number}"]=str(number*10+ordered.index(label)+1)
    if role=="independent":
        residuals=certificate["evidence"]["facts"]["residuals"]
        fields.update(wrong_a_residual=residuals[1],wrong_b_residual=residuals[2])
    path=SOURCE/"families"/family/"questions.paths.md.in"
    template=path.read_text() if template is None else template
    marker=f"<!-- {role} -->"; require(template.count(marker)==1,f"expected one editable {role} block in {path}")
    body=template.split(marker,1)[1].split("<!--",1)[0].strip()
    return batch.fill_template(body,fields,path)


def lesson_blocks(family, set_name, package):
    template=SOURCE/"families"/family/"lesson.md.in"
    require(template.is_file(),f"missing editable lesson template for {family}")
    return template.read_text()


READING_ID=""
def build_one(family, set_name, target, model):
    global READING_ID
    package=f"prod01_algebra_{family}_{set_name}"
    READING_ID=f"prod01_algebra_{family}_r"
    stage=OUTPUT/"sources"/package
    stage.mkdir(parents=True, exist_ok=True)
    seq=sequence(family,set_name,package)
    checkers=load_provider(stage)
    questions=batch.validate_role_sequence(seq, stage/"sequence.json")
    certificates=[batch.reasoning_certificate(q,checkers) for q in questions]
    positions=POSITION[(family,set_name)]
    text="\n".join(question_text(q,c,positions[i]) for i,(q,c) in enumerate(zip(questions,certificates)))
    values=dict(subject="algebra",subject_title="Algebra",chapter=("worked_linear_practice" if family=="signed_balance" else "topic_0003"),
        chapter_title=("Worked linear practice" if family=="signed_balance" else "Equations and Relations"),reading_id=READING_ID,
        reading_title=("Balance signed linear equations" if family=="signed_balance" else "Distribute before isolating x"),
        lesson_blocks=lesson_blocks(family,set_name,package), practice_links="".join(f"@practice {q['id']}\n" for q in questions),questions=text)
    document=batch.chapter_text(values).encode()
    author=dict(format="paths_learning_authoring",format_version=1,package_id=package,package_version=1,sources=[dict(
        id=package+"_original",kind="original",title="Original Wave 01 algebra choice sequence",uri="paths:original/"+package+"/v1",revision="1",
        attribution="Original Paths prose, examples, questions and certificates. Definitions and equality conditions checked against OpenStax Elementary Algebra 2e sections 2.1 and 2.2, accessed 2026-09-10.",
        reuse="Original project material; no external exercise wording copied.",content_ids=[READING_ID]+[q["id"] for q in questions])])
    (stage/"sequence.json").write_bytes(export.encoded(seq)); (stage/"lesson.md.in").write_text(lesson_blocks(family,set_name,package)); (stage/"questions.paths.md.in").write_text(text)
    shutil.copyfile(SOURCE/"families/provider.py",stage/"certificates.py"); (stage/"authoring.json").write_bytes(export.encoded(author))
    assignment=dict(subject="algebra",folder=str(stage.relative_to(ROOT)),package_id=package,values={k:values[k] for k in ("subject","subject_title","chapter","chapter_title","reading_id","reading_title")})
    report=pilot.check_assignment(assignment,target,model)
    receipt=Path(report["authoring"]).parent/"checks"/export.sha(export.encoded(report))/"verification.json"
    require(receipt.is_file(),"shared staging receipt is missing at the result-addressed path")
    return dict(family=family,set=set_name,authoring=report["authoring"],verification=str(receipt),questions=6,reading=READING_ID,
                first_decision_positions=list(positions),source_sha256=report["source_sha256"],route_checks=report["route_checks"])


def build_family(family, target, model):
    """Compile/replay one canonical reading with all 18 family questions."""
    global READING_ID
    source_files=(SOURCE/"DESIGN.md",SOURCE/"recipe.json",SOURCE/"generate.py",SOURCE/"certificate_tests.py",SOURCE/"families"/"provider.py",
                  SOURCE/"families"/"signed_balance"/"lesson.md.in",SOURCE/"families"/"signed_balance"/"questions.paths.md.in",
                  SOURCE/"families"/"distributive_linear"/"lesson.md.in",SOURCE/"families"/"distributive_linear"/"questions.paths.md.in")
    source_hashes={str(path.relative_to(SOURCE)):export.sha(export.read_bytes(path)) for path in source_files}
    package=f"prod01_algebra_{family}"
    READING_ID=package+"_r"
    checkers=load_provider(SOURCE)
    all_questions=[]; all_certificates=[]; question_blocks=[]
    for set_name in SETS:
        selected=sequence(family,set_name,f"prod01_algebra_{family}_{set_name}")
        current=batch.validate_role_sequence(selected, SOURCE/"recipe.json")
        certificates=[batch.reasoning_certificate(q,checkers) for q in current]
        all_questions.extend(current); all_certificates.extend(certificates)
        positions=POSITION[(family,set_name)]
        question_blocks.extend(question_text(q,c,positions[i]) for i,(q,c) in enumerate(zip(current,certificates)))
    values=dict(subject="algebra",subject_title="Algebra",chapter=("worked_linear_practice" if family=="signed_balance" else "topic_0003"),
        chapter_title=("Worked linear practice" if family=="signed_balance" else "Equations and Relations"),reading_id=READING_ID,
        reading_title=("Balance signed linear equations" if family=="signed_balance" else "Distribute before isolating x"),
        lesson_blocks=lesson_blocks(family,"teaching, practice, and fresh check",package),
        practice_links="".join(f"@practice {q['id']}\n" for q in all_questions),questions="\n".join(question_blocks))
    documents={"chapter.paths.md":batch.chapter_text(values).encode()}
    author=dict(format="paths_learning_authoring",format_version=1,package_id=package,package_version=1,sources=[dict(
        id=package+"_original",kind="original",title="Original Wave 01 algebra family",uri="paths:original/"+package+"/v1",revision="1",
        attribution="Original Paths prose, examples, questions and exact certificates. Conditions checked against OpenStax Elementary Algebra 2e sections 2.1 and 2.2, accessed 2026-09-10.",
        reuse="Original project material; no external exercise wording copied.",content_ids=[READING_ID]+[q["id"] for q in all_questions])])
    target_view=export.Target(target); target_sha=target_view.fingerprint
    model_sha=export.sha(export.read_bytes(model,64 * 1024 * 1024))
    target_view.inspect_bytes(documents)
    staged=OUTPUT/"family_replay"/package/export.sha(documents["chapter.paths.md"])
    export.immutable_directory(staged,documents)
    played=subprocess.run([str(model),"--question-batch",str(staged)],capture_output=True,text=True,timeout=60)
    require(played.returncode==0,played.stderr or played.stdout)
    routes=export.decoded(played.stdout)
    require(routes["accepted"] is True and routes["routes"]==18 and len(routes["question_ids"])==18,"family model did not replay all 18 routes")
    batch.verify_role_content(all_questions,routes["questions"],all_certificates)
    require(target_sha == export.sha(export.read_bytes(target,64 * 1024 * 1024)),"target executable changed during family replay")
    require(model_sha == export.sha(export.read_bytes(model,64 * 1024 * 1024)),"model executable changed during family replay")
    files={"authoring.json":export.encoded(author),**{"documents/"+name:data for name,data in documents.items()}}
    family_fingerprint=export.sha(export.encoded({"files":{name:export.sha(data) for name,data in files.items()}}))
    family_root=OUTPUT/"families"/package/family_fingerprint
    export.immutable_directory(family_root/"authoring",files)
    lesson_gate=subprocess.run([str(model),"--family-lessons",str(family_root/"authoring"/"documents")],capture_output=True,text=True,timeout=60)
    require(lesson_gate.returncode==0,lesson_gate.stderr or lesson_gate.stdout)
    lesson_checks=export.decoded(lesson_gate.stdout)
    require(lesson_checks["accepted"] is True and lesson_checks["readings"]==1 and lesson_checks["questions"]==18,"family lesson disclosure gate rejected final authoring")
    require(source_hashes=={str(path.relative_to(SOURCE)):export.sha(export.read_bytes(path)) for path in source_files},"source changed during final verification")
    require(target_sha==export.sha(export.read_bytes(target,64*1024*1024)) and model_sha==export.sha(export.read_bytes(model,64*1024*1024)),"binary changed during final disclosure verification")
    report=dict(source_and_executables_stable=True,accepted=True,stage="family_content_checked",family=family,package_id=package,questions=18,readings=1,
        route_checks=routes,lesson_disclosure_checks=lesson_checks,mathematical_checks=all_certificates,source_sha256=source_hashes,publication="not_performed",visual_acceptance="pending")
    report["executables_sha256"]={"target":target_sha,"model":model_sha}
    receipt=export.encoded(report); check=family_root/"checks"/export.sha(receipt)/"verification.json"
    export.immutable_directory(check.parent,{"verification.json":receipt})
    return dict(family=family,authoring=str(family_root/"authoring"),verification=str(check),questions=18,reading=READING_ID)


def main():
    parser=argparse.ArgumentParser(); parser.add_argument("--target",type=Path,required=True); parser.add_argument("--model",type=Path,required=True); args=parser.parse_args()
    require(args.target.is_file() and args.model.is_file(),"required read-only executables are missing")
    candidates=[build_one(family,set_name,args.target,args.model) for family in ("signed_balance","distributive_linear") for set_name in SETS]
    try:
        families=[build_family(family,args.target,args.model) for family in ("signed_balance","distributive_linear")]
        accepted=True; blocker=None
    except export.ExportError as error:
        families=[]; accepted=False
        blocker=dict(code=error.code,message=str(error),required_contract="One lesson.v2 reading must link all 18 questions in its family.",
                     observed_contract="The final compiler, model replay, and family-lesson disclosure gate must all accept the canonical package.",
                     completed_unaffected_work="All six set-level candidates passed compiler/model replay; no package was published.")
    receipt=dict(format="paths_production_subject",format_version=1,wave="wave01",contract_revision=2,subject="algebra",accepted=accepted,published=False,candidates=[{k:r[k] for k in ("family","set","authoring","verification")} for r in candidates],
                 families=families,coverage=dict(families=2,sets=6,questions=36,readings=2,roles=list(ROLES),wrong_choices=84),blocker=blocker)
    OUTPUT.mkdir(parents=True,exist_ok=True); (OUTPUT/"production.json").write_bytes(export.encoded(receipt)); print(json.dumps(receipt,indent=2))
if __name__=="__main__": main()
