#!/usr/bin/env python3
"""Generate, independently check, replay and export a supported question family."""
import argparse
from fractions import Fraction
import itertools
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

import export_learning as export
import question_workflow as linear

ROOT = Path(__file__).resolve().parents[1]
FAMILY = "matrix_reps_v1"
GROUPS = ("integers", "negative", "fractions")
TITLES = ("Integer solutions", "Negative solutions", "Fractional solutions")
CHAPTER = "matrix_repetitions"
REFERENCE_TEMPLATE = ROOT / "content/authoring/learning/matrix_reference/question.paths.md.in"
WORKED_FAMILY = "matrix_reps_v2"
LINEAR_TEMPLATE = ROOT / "content/authoring/learning/linear_reference/question.paths.md.in"
LINEAR_TITLES = ("Positive integers", "Negative coefficients", "Negative offsets",
                 "Negative solutions", "Zero as the solution", "Fractional solutions")


def require(ok, message):
    export.require(ok, "batch.math", message)


def matrix(rows):
    return " ".join(f"[{a}, {b} | {c}]" for a, b, c in rows)


def operate(rows, operation, operand):
    rows = [[Fraction(v) for v in row] for row in rows]
    k = Fraction(operand)
    if operation == "divide_row_2":
        require(k != 0, "A row divisor must be nonzero")
        rows[1] = [v / k for v in rows[1]]
    else:
        destination, source = {"add_row_1_to_2": (1, 0), "add_row_2_to_1": (0, 1)}[operation]
        rows[destination] = [v + k * w for v, w in zip(rows[destination], rows[source])]
    return rows


def make_question(p, k, d, x, y, group):
    c, b = x + p * y, k * x + (k * p + d) * y
    states = [[[1, p, c], [k, k * p + d, b]],
              [[1, p, c], [0, d, d * y]],
              [[1, p, c], [0, 1, y]], [[1, 0, x], [0, 1, y]]]
    states = [[[str(v) for v in row] for row in state] for state in states]
    identity = FAMILY + "_" + export.sha(export.encoded(states[0]))[:24]
    steps = []
    for i, (operation, answer) in enumerate((("add_row_1_to_2", -k), ("divide_row_2", d), ("add_row_2_to_1", -p))):
        choices = [str(answer), str(-answer), str(2 * answer)]
        choices.sort(key=lambda v: export.sha(export.encoded([identity, i, v])))
        steps.append(dict(operation=operation, choices=choices, answer=str(answer)))
    return dict(id=identity, group=group, parameters=dict(p=p, k=k, d=d),
                states=states, steps=steps, answer=[str(x), str(y)])


def generate(count):
    export.require(type(count) is int and 3 <= count <= 36 and count % 3 == 0,
                   "batch.count", "Count must be a multiple of 3 from 3 through 36")
    solutions = ([(Fraction(x), Fraction(y)) for x in range(1, 5) for y in range(1, 5)],
                 [(Fraction(x), Fraction(y)) for x in (-3, -2, -1, 1, 2) for y in (-3, -1, 1, 2) if min(x, y) < 0],
                 [(Fraction(x, 3), Fraction(y, 2)) for x in (-4, -2, -1, 1, 2, 4) for y in (-3, -1, 1, 3)])
    groups = []
    for group, pairs in zip(GROUPS, solutions):
        pool = list(itertools.product((-2, -1, 1, 2), (-3, -2, 2, 3), (-3, -2, 2, 3), pairs))
        pool.sort(key=lambda v: export.sha(export.encoded([FAMILY, group, *v[:3], *map(str, v[3])])))
        groups.append([make_question(p, k, d, *xy, group) for p, k, d, xy in pool[:count // 3]])
    # Interleave strata so increasing the count preserves the smaller batch prefix.
    return [q for row in zip(*groups) for q in row]


def verify(q):
    require(len(q['states']) == 4 and len(q['steps']) == 3, "Expected three row-operation steps")
    states = [[[Fraction(v) for v in row] for row in state] for state in q['states']]
    (a, b, c), (d, e, f) = states[0]
    determinant = a * e - b * d
    require(determinant != 0, "Given must have a unique solution")
    # Cramer's rule uses the original givens independently of the construction.
    x, y = (c * e - b * f) / determinant, (a * f - c * d) / determinant
    require([x, y] == list(map(Fraction, q['answer'])), "Final answer disagrees with original givens")
    for state in states:
        require(all(a * x + b * y == c for a, b, c in state), "Intermediate row changes the original solution")
    require(states[-1] == [[1, 0, x], [0, 1, y]], "Final coefficients must be identity")
    for i, step in enumerate(q['steps']):
        values = list(map(Fraction, step['choices']))
        require(len(values) == 3 and len(set(values)) == 3, "Choices must be distinct exact numbers")
        matching = [v for v in values if operate(states[i], step['operation'], v) == states[i + 1]]
        require(matching == [Fraction(step['answer'])], "Exactly the declared choice must produce the next matrix")
    return dict(id=q['id'], accepted=True, determinant=str(determinant), answer=list(map(str, (x, y))),
                steps_checked=3, wrong_choices_checked=6)


def question_text(q):
    p, k, d = (q['parameters'][key] for key in ('p', 'k', 'd'))
    states = q['states']; x, y = q['answer']; c = states[0][0][2]; b = states[0][1][2]
    prompts = ("Eliminate x from row 2. Choose the multiple of row 1 to add.",
               "Make the leading entry of row 2 equal to 1. Choose its divisor.",
               "Eliminate y from row 1. Choose the multiple of row 2 to add.")
    definitions = (
        "A coefficient multiplies an unknown. R1 and R2 name complete equations, including their constants. Adding a multiple of one row to another preserves their common solutions: subtracting that multiple reverses the change.",
        "A pivot is a leading nonzero entry used in elimination. Divide every entry of a row, including its constant, by the same nonzero number. Multiplying back reverses the operation; division by zero is undefined.",
        "The identity coefficient matrix has 1 on its diagonal and 0 elsewhere. Each row then isolates one unknown. A solution must satisfy both original equations; substitution checks this simultaneously.")
    teaching = (
        f"The first coefficient of row 2 is {k}; the first coefficient of row 1 is 1. Choose a multiplier m so {k} + m(1) = 0. Thus m = {-k}. Apply it to all three columns. The new row 2 is {matrix([states[1][1]])}: its constant is ({b}) + ({-k})({c}) = {states[1][1][2]}. Row 1 stays unchanged. Adding the opposite multiple would recover the original system.",
        f"The remaining pivot in row 2 is {d}. Divide the complete row by {d}: the pivot becomes 1 and the constant becomes ({states[1][1][2]})/({d}) = {y}. The new row is {matrix([states[2][1]])}, which states y = {y}. Keep fractions exact and retain the sign of the divisor. Row 1 stays unchanged.",
        f"The y coefficient in row 1 is {p}; row 2 has y coefficient 1. Add {-p} times row 2 to row 1. Its x coefficient remains 1, its y coefficient becomes 0, and its constant is ({c}) + ({-p})({y}) = {x}. Read x = {x} and y = {y}. Substitute both in each original row; checking only the final identity matrix would not verify the original problem.")
    reasons = (f"Adding {-k} times row 1 gives {matrix([states[1][1]])} in row 2; row 1 is unchanged.",
               f"Dividing row 2 by {d} gives y = {y}.",
               f"Subtracting {p} times row 2 isolates x = {x}. Together with y = {y}, these values satisfy both original equations.")
    wrong = (f"Set the new first coefficient {k} + m(1) to zero. Reversing the sign or doubling the cancelling multiple will not cancel it.",
             f"Choose a nonzero divisor that makes {d} divided by it equal 1. Apply that divisor to the constant as well.",
             f"Set the new y coefficient {p} + m(1) to zero while keeping the x coefficient unchanged.")
    lines = [f"@question {q['id']} | Matrix reps: {q['group']} {q['id'][-6:]}", "@template matrix.v1", "@version 1",
             "@goal Solve the two equations by row reduction.", f"@given {matrix(states[0])}",
             "@domain Real x and y; the column after the bar holds constants."]
    for i, step in enumerate(q['steps']):
        base = (i + 1) * 10
        lines += ["", f"@step {base} | {prompts[i]}", f"@operation {step['operation']}"]
        lines += [f"@choice {base+j+1} | {v}" for j, v in enumerate(step['choices'])]
        lines += [f"@answer {base+step['choices'].index(step['answer'])+1}", f"@after {matrix(states[i+1])}",
                  f"@wrong {wrong[i]}", f"@why {reasons[i]}", "@definitions", definitions[i], "@teaching", teaching[i]]
    return "\n".join(lines + ["@end", ""])


def documents(questions):
    result = {}
    for group, title in zip(GROUPS, TITLES):
        selected = [q for q in questions if q['group'] == group]
        text = f"@paths 1\n@subject linear_algebra | Linear Algebra\n@chapter {CHAPTER} | Matrix repetitions\n\n"
        text += f"@lesson {FAMILY}_{group}_reading | Matrix repetitions: {title}\n@template lesson.v2\n"
        text += (f"@block introduction | {FAMILY}_{group}_intro | - | One system, three operations\n@prose\n"
                 "Each question asks for the common solution of two equations. First cancel x in row 2, then divide its pivot to obtain 1, then cancel y in row 1. Learn explains each step; Practice offers the same symbolic choices with Help on demand. The given stays beside the working.\n@endblock\n"
                 f"@block definition | {FAMILY}_{group}_rows | - | Rows and solutions\n@prose\n"
                 "The first two entries of each augmented row multiply x and y. The entry after the bar is the constant. Apply each row operation to the constant too. Row addition is reversible, and row division requires a nonzero divisor. Finish by checking both original equations.\n@endblock\n")
        text += "".join(f"@practice {q['id']}\n" for q in selected) + "@end\n\n"
        text += "\n".join(question_text(q) for q in selected)
        result[group + ".paths.md"] = text.encode()
    return result


def tex(value):
    value = Fraction(value)
    magnitude = abs(value)
    return ("-" if value < 0 else "") + (str(magnitude.numerator) if magnitude.denominator == 1
        else rf"\frac{{{magnitude.numerator}}}{{{magnitude.denominator}}}")


def matrix_tex(rows):
    return r"\left[\begin{array}{cc|c}" + r"\\".join("&".join(map(tex, row)) for row in rows) + r"\end{array}\right]"


def add_tex(first, multiplier, second):
    return first + ("-" if Fraction(multiplier) < 0 else "+") + tex(abs(Fraction(multiplier))) + second


def worked_fields(q, number):
    states = q['states']; p, k, d = (q['parameters'][key] for key in ('p', 'k', 'd'))
    fields = dict(id=q['id'], title=f"{TITLES[GROUPS.index(q['group'])]} · Exercise {number}",
                  given=matrix(states[0]), p=tex(p), k=tex(k), d=tex(d), minus_p=tex(-p), minus_k=tex(-k),
                  x=tex(q['answer'][0]), y=tex(q['answer'][1]))
    for i, step in enumerate(q['steps']):
        base=(i+1)*10; before=states[i]; after=states[i+1]
        choices=[f"@choice {base+j+1} | {v}" for j,v in enumerate(step['choices'])]
        choices.append(f"@answer {base+step['choices'].index(step['answer'])+1}")
        destination, source = (0,1) if i==2 else (1,0)
        if i==1:
            operation=rf"R_2\leftarrow {tex(Fraction(1,d))}R_2."
            calculations=[rf"\frac{{{tex(v)}}}{{{tex(d)}}}&={tex(w)}" for v,w in zip(before[1],after[1])]
        else:
            operand=step['answer']
            operation=add_tex(rf"R_{destination+1}\leftarrow R_{destination+1}",operand,rf"R_{source+1}")+"."
            calculations=[add_tex(tex(a),operand,rf"\left({tex(b)}\right)")+"&="+tex(c)
                          for a,b,c in zip(before[destination],before[source],after[destination])]
        fields.update({f'choices_{i+1}':"\n".join(choices),f'after_{i+1}':matrix(after),f'matrix_{i+1}':matrix_tex(after)+'.',
                       f'operation_{i+1}':operation,f'calculation_{i+1}':r"\begin{aligned}"+r"\\".join(calculations)+r".\end{aligned}"})
    fields['matrix_3']=matrix_tex(states[3])
    for i,(a,b,c) in enumerate(states[0],1):
        first=tex(a)+rf"\left({fields['x']}\right)"
        fields[f'check_{i}']=add_tex(first,b,rf"\left({fields['y']}\right)")+"="+tex(c)+"."
    return fields


def fill_template(template, fields, source=None):
    source=source or REFERENCE_TEMPLATE
    def substitute(match):
        name=match[1]
        export.require(name in fields, 'batch.template',
                       f"{source}:{template.count(chr(10),0,match.start())+1}: Unknown field {name}")
        return fields[name]
    filled=re.sub(r"\{\{([^{}\n]*)\}\}", substitute, template)
    export.require('{{' not in filled, 'batch.template', f"{source}: Unclosed or malformed template field")
    return filled


def worked_documents(questions, template):
    result={}
    for group,title in zip(GROUPS,TITLES):
        selected=[q for q in questions if q['group']==group]
        text=("@paths 1\n@subject linear_algebra | Linear Algebra\n@chapter worked_matrix_practice | Worked matrix practice\n\n"
              f"@lesson {WORKED_FAMILY}_{group}_reading | {title}\n@template lesson.v2\n"
              f"@block introduction | {group}_start | 1 | Solve two equations together\n"
              "@prose Find the pair of values that satisfies both equations. Keep every fraction exact. These exercises revisit the earlier matrix repetitions with expanded explanations.\n@endblock\n"
              f"@block definition | {group}_rows | 1.1 | Read an augmented matrix\n"
              "@prose The first two columns contain the coefficients of the unknowns. The column after the bar contains the constants. A row operation acts on every entry, including its constant.\n@display\n"
              r"\left[\begin{array}{cc|c}a&b&c\end{array}\right]\quad\longleftrightarrow\quad ax+by=c"+"\n@endblock\n"
              f"@block introduction | {group}_help | 1.2 | Choose your support\n"
              "@prose Learn opens definitions and the worked step. Practice keeps the symbolic choices. Terms defines notation; Hint gives a direction; Next line reveals one reached matrix; Solution reveals the complete route. Solve and Write accept your own working. Completion stays until Next.\n@endblock\n")
        text+=''.join(f"@practice {q['id']}\n" for q in selected)+"@end\n\n"
        text+='\n'.join(fill_template(template,worked_fields(q,n)) for n,q in enumerate(selected,1))
        result['worked_'+group+'.paths.md']=text.encode()
    return result


def linear_questions(count, spec):
    export.require(type(count) is int and 6<=count<=36 and count%6==0, 'batch.count',
                   'Linear count must be a multiple of 6 from 6 through 36')
    # Select from one fixed pool so extending another stratum cannot reshuffle it.
    pool=linear.generate(spec,per_stratum=6)
    groups=[[q for q in pool if q['stratum']==group][:count//6] for group in linear.STRATA]
    return [q for row in zip(*groups) for q in row]


def linear_fields(q, number):
    a,b,c=(linear.exact(q['parameters'][k]) for k in ('a','b','c'))
    answer=linear.exact(q['answer']['value']); rhs=c-b
    given=linear.equation(q['states'][0]);left=given.split('=')[0]
    offset=('Subtracting a negative number adds its positive opposite.' if b<0 else
            'Subtracting the added positive constant cancels it.')
    fields=dict(id=q['id'],title=f"{LINEAR_TITLES[linear.STRATA.index(q['stratum'])]} · Exercise {number}",
                given=linear.equation(q['states'][0],typeset=False),given_tex=given,
                a=tex(a),b=tex(b),c=tex(c),rhs=tex(rhs),answer=tex(answer),offset_note=offset,
                prompt_1=f'Subtract ({b}) on both sides. Complete {a}x = ?',
                remove_left=left+rf' -\left({tex(b)}\right)',remove_right=tex(c)+rf' -\left({tex(b)}\right)',
                check=tex(a)+rf'\left({tex(answer)}\right)'+('+' if b>0 else '-')+tex(abs(b))+'='+tex(c))
    for i,step in enumerate(q['steps'],1):
        base=i*10
        choices=[f"@choice {base+j+1} | {o['value']}" for j,o in enumerate(step['choices'])]
        choices += [f"@answer {base+j+1}" for j,o in enumerate(step['choices']) if o['id']=='correct']
        fields.update({f'choices_{i}':'\n'.join(choices),f'after_{i}':linear.equation(q['states'][i],typeset=False),
                       f'equation_{i}':linear.equation(q['states'][i])})
    return fields


def linear_batch(args):
    export.require(args.format_version==1,'batch.format','Linear practice currently uses format version 1')
    spec_bytes=export.read_bytes(linear.SPEC,128*1024);spec=export.decoded(spec_bytes)
    template=export.read_bytes(LINEAR_TEMPLATE,128*1024)
    questions=linear_questions(args.count,spec);results=[]
    for q in questions:
        linear.verify_question(q)
        q['source_id']=q['id'];q['id']='linear_worked_v1_'+linear.digest(q['parameters'])[:32]
        results.append(dict(id=q['id'],accepted=True,answer=q['answer']['value'],steps_checked=2,
                            wrong_choices_checked=4,original_substitution=q['verification']))
    docs={};reading_ids=[]
    for group,title in zip(linear.STRATA,LINEAR_TITLES):
        selected=[q for q in questions if q['stratum']==group];reading='linear_worked_v1_'+group+'_reading';reading_ids.append(reading)
        text=("@paths 1\n@subject algebra | Algebra\n@chapter worked_linear_practice | Worked linear practice\n\n"
              f"@lesson {reading} | {title}\n@template lesson.v2\n"
              f"@block introduction | {group}_start | 1 | One unknown, balanced operations\n"
              "@prose Find the real value that makes the original equation true. Apply the same operation to both sides and keep fractions exact.\n@endblock\n"
              f"@block definition | {group}_terms | 1.1 | Coefficient and constant\n"
              "@prose A coefficient multiplies the unknown. The added constant is a separate term. A nonzero coefficient gives a unique solution.\n@display\n"
              r"ax+b=c,\qquad a\ne0"+"\n@endblock\n"
              f"@block introduction | {group}_controls | 1.2 | Choose your support\n"
              "@prose Learn explains each balanced step. Practice keeps the symbolic choices and opens help on request. Terms defines notation; Hint gives a direction; Next line reveals one equation; Solution reveals the full route. Solve and Write accept your own working. Completion stays until Next.\n@endblock\n")
        text+=''.join(f"@practice {q['id']}\n" for q in selected)+'@end\n\n'
        text+='\n'.join(fill_template(template.decode('utf-8'),linear_fields(q,n),LINEAR_TEMPLATE) for n,q in enumerate(selected,1))
        docs[group+'.paths.md']=text.encode()
    author=dict(format='paths_learning_authoring',format_version=1,package_id='linear_repetitions',package_version=args.version,
                sources=[dict(id='linear_worked_v1',kind='generated',title='Original worked linear-equation practice',
                              uri='paths:generated/linear_worked_v1',revision='1',
                              attribution='Original Paths questions and teaching; reuses the checked linear_balance_ax_b recipe.',
                              reuse='Generated for this project. No external exercise text copied.',
                              content_ids=[q['id'] for q in questions]+reading_ids)])
    audit=dict(family='linear_worked_v1',family_version=1,format_version=1,count=len(questions),questions=questions,
               mathematical_checks=results,recipe_sha256=export.sha(spec_bytes),template_sha256=export.sha(template),
               retained_questions=0,worked_questions=len(questions),teaching_review='Shared reference format; linear variants await user visual check')
    return questions,results,docs,author,audit


def matrix_batch(args):
    questions = generate(args.count)
    export.require(args.format_version in (1,2), 'batch.format', 'Format version must be 1 or 2')
    export.require(args.format_version==1 or args.version>=2, 'batch.version', 'Reference format requires package version 2 or later')
    docs = documents(questions)
    if args.format_version==2:
        # Retain the published v1 questions verbatim; new teaching requires new IDs.
        worked=[dict(q,id=q['id'].replace(FAMILY,WORKED_FAMILY,1)) for q in questions]
        template=export.read_bytes(REFERENCE_TEMPLATE,128*1024)
        docs.update(worked_documents(worked,template.decode('utf-8')))
        questions+=worked
    results, failures = [], []
    for q in questions:
        try:
            results.append(verify(q))
        except (ValueError, KeyError, ZeroDivisionError) as error:
            failures.append(dict(id=q['id'], reason=str(error)))
    export.require(not failures, "batch.math", json.dumps(failures))
    ids = [q['id'] for q in questions] + [f"{FAMILY}_{g}_reading" for g in GROUPS]
    if args.format_version==2:ids += [f"{WORKED_FAMILY}_{g}_reading" for g in GROUPS]
    author = dict(format="paths_learning_authoring", format_version=1, package_id="matrix_repetitions",
                  package_version=args.version, sources=[dict(id=FAMILY, kind="generated", title="Original two-equation repetition family",
                  uri="paths:generated/matrix_reps_v1", revision="1", attribution="Original Paths questions and teaching; standard reversible row operations.",
                  reuse="Generated for this project. No external exercise text copied.", content_ids=ids)])
    audit = dict(family=FAMILY, family_version=1, count=len(questions), questions=questions, mathematical_checks=results,
                 teaching_review="Original bounded template; user visual acceptance pending")
    if args.format_version==2:
        author['sources'][0].update(revision='2',title='Original two-equation repetitions and worked reference format')
        audit.update(format_version=2,template_sha256=export.sha(template),retained_questions=args.count,
                     worked_questions=args.count,teaching_review='Reference format accepted by user; generated variants await visual check')
    return questions, results, docs, author, audit


BUILDERS={'matrix':matrix_batch,'linear':linear_batch}


def run(args):
    questions, results, docs, author, audit = BUILDERS[args.family](args)
    export.require(len({q['id'] for q in questions}) == len(questions), "batch.duplicate", "Duplicate question identity")
    target = export.Target(args.target)
    model = export.real_path(args.model)
    model_hash = export.sha(export.read_bytes(model, 64 * 1024 * 1024))
    target.inspect_bytes(docs)
    with tempfile.TemporaryDirectory(prefix="paths-batch-check-") as temporary:
        root = Path(temporary).resolve();export.write_tree(root, docs)
        played = subprocess.run([str(model), "--question-batch", str(root)], capture_output=True, text=True, timeout=60)
        export.require(played.returncode == 0, "batch.routes", played.stderr or played.stdout)
        routes = export.decoded(played.stdout)
        export.require(routes['accepted'] is True and set(routes['question_ids']) == {q['id'] for q in questions}
                       and routes['routes'] == len(questions) * 5, "batch.routes", "Model gate did not exercise every generated question")
    export.require(export.sha(export.read_bytes(model, 64 * 1024 * 1024)) == model_hash,
                   "batch.model_changed", "Model executable changed during verification; rerun the batch")
    files = {"authoring.json": export.encoded(author), "audit.json": export.encoded(audit),
             **{"documents/" + name: data for name, data in docs.items()}}
    output = export.real_path(args.output or ROOT / "build/question-batches" / author["package_id"] / str(args.version))
    store = export.real_path(args.store or target.path.parent / "learning-store")
    baseline = export.real_path(args.base_documents or target.path.parent / "content/write")
    export.disjoint(output, [store, baseline, target.path, model])
    with export.locked(output.parent / (".batch-" + output.name + ".lock")):
        authoring = output / "authoring"
        export.immutable_directory(authoring, files)
        published = export.run(argparse.Namespace(command="publish" if args.publish else "export", source=authoring,
            target=target.path, library=None, store=store, base_documents=baseline, output=output / "export"))
    return dict(accepted=True, family=audit["family"], format_version=args.format_version, count=len(questions),
                retained_questions=audit.get("retained_questions",0),
                worked_questions=audit.get("worked_questions",0), generated_authoring=str(authoring),
                mathematical_checks=results, route_checks=routes, model_sha256=model_hash,
                publication=published, visual_acceptance="pending_user")


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--family', choices=tuple(BUILDERS), default='matrix')
    cli.add_argument('--count', type=int, default=12, help='3..36 in multiples of 3 for matrix, 6..36 in multiples of 6 for linear')
    cli.add_argument('--version', type=int, help='immutable package version; increase when extending a batch')
    cli.add_argument('--format-version', type=int, choices=(1,2), help='matrix defaults to 2, linear to 1')
    cli.add_argument('--target', type=Path, default=ROOT / 'b/sorter')
    cli.add_argument('--model', type=Path, default=ROOT / 'b/paths_learning_document_tests')
    cli.add_argument('--output', type=Path)
    cli.add_argument('--store', type=Path)
    cli.add_argument('--base-documents', type=Path)
    cli.add_argument('--publish', action='store_true', help='activate through the existing publisher; otherwise export only')
    try:
        args = cli.parse_args()
        default={"matrix":2,"linear":1}[args.family]
        if args.version is None:args.version=default
        if args.format_version is None:args.format_version=default
        export.require(0 < args.version <= 2**32-1, 'batch.version', 'Version must be a positive 32-bit integer')
        print(json.dumps(run(args), indent=2))
        return 0
    except (export.ExportError, ValueError, OSError, KeyError, TypeError, subprocess.TimeoutExpired) as error:
        print(json.dumps(dict(accepted=False, code=getattr(error, 'code', 'batch.failed'), message=str(error),
                              diagnostics=getattr(error, 'diagnostics', [])), indent=2), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
