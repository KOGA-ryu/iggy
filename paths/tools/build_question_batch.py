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
CHAPTER_TEMPLATE = ROOT / "content/authoring/learning/chapter.paths.md.in"
LINEAR_TITLES = ("Positive integers", "Negative coefficients", "Negative offsets",
                 "Negative solutions", "Zero as the solution", "Fractional solutions")
PROBABILITY_ROOT = ROOT / 'content/authoring/learning/probability_reference'
PROBABILITY_FAMILY = 'finite_probability_v1'
PROBABILITY_GROUPS = ('event', 'complement', 'boundary')
PROBABILITY_TITLES = ('Count an event', 'Count its complement', 'Impossible or certain')
REASONING_ROOT = ROOT / 'content/authoring/learning/probability_reasoning'
MATRIX_REASONING_ROOT = ROOT / 'content/authoring/learning/matrix_reasoning'
# Authoring roles describe the decision and required certificate, not a new runtime solver.
EXERCISE_ROLES = {
    'read_notation':'notation_interpretation', 'worked_check':'worked_example',
    'choose_next_step':'method_condition', 'explain_step':'justification',
    'repair_error':'first_error', 'independent':'fresh_context',
}


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
        before=states[i]; after=states[i+1]
        choices=choices_text(step['choices'],step['choices'].index(step['answer']),i+1)
        destination, source = (0,1) if i==2 else (1,0)
        if i==1:
            operation=rf"R_2\leftarrow {tex(Fraction(1,d))}R_2."
            calculations=[rf"\frac{{{tex(v)}}}{{{tex(d)}}}&={tex(w)}" for v,w in zip(before[1],after[1])]
        else:
            operand=step['answer']
            operation=add_tex(rf"R_{destination+1}\leftarrow R_{destination+1}",operand,rf"R_{source+1}")+"."
            calculations=[add_tex(tex(a),operand,rf"\left({tex(b)}\right)")+"&="+tex(c)
                          for a,b,c in zip(before[destination],before[source],after[destination])]
        fields.update({f'choices_{i+1}':choices,f'after_{i+1}':matrix(after),f'matrix_{i+1}':matrix_tex(after)+'.',
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


def choices_text(values, accepted_index, step, render=str):
    """One directive writer; the family checker establishes the accepted index."""
    require(type(accepted_index) is int and 0<=accepted_index<len(values),'Choice writer needs a checked accepted index')
    base=step*10
    return "\n".join([f"@choice {base+j+1} | {render(v)}" for j,v in enumerate(values)]
                     +[f"@answer {base+accepted_index+1}"])


def chapter_text(values):
    return fill_template(export.read_bytes(CHAPTER_TEMPLATE,128*1024).decode('utf-8'),values,CHAPTER_TEMPLATE)


def teaching_documents(questions, *, groups, titles, group_key, subject, chapter,
                       reading_prefix, question_template, lesson_template, fields, filename_prefix="", numbered_files=False):
    """Use the existing document grammar and textbook block types for every family."""
    lesson_path=lesson_template
    lesson=export.read_bytes(lesson_path,128*1024).decode('utf-8')
    question=export.read_bytes(question_template,128*1024).decode('utf-8')
    docs={};reading_ids=[]
    for section,(group,title) in enumerate(zip(groups,titles),1):
        selected=[q for q in questions if q[group_key]==group]
        reading_id=reading_prefix+'_'+group+'_reading';reading_ids.append(reading_id)
        values=dict(group=group,reading_id=reading_id,reading_title=title,
                    subject=subject[0],subject_title=subject[1],chapter=chapter[0],chapter_title=chapter[1])
        values['lesson_blocks']=fill_template(lesson,values,lesson_path)
        values['practice_links']=''.join(f"@practice {q['id']}\n" for q in selected)
        values['questions']='\n'.join(fill_template(question,dict(fields(q,n),reading_id=reading_id),question_template)
                                     for n,q in enumerate(selected,1))
        prefix=f'{section:02}_' if numbered_files else filename_prefix
        docs[prefix+group+'.paths.md']=chapter_text(values).encode()
    return docs,reading_ids


def worked_documents(questions):
    return teaching_documents(questions,groups=GROUPS,titles=TITLES,group_key='group',
        subject=('linear_algebra','Linear Algebra'),chapter=('worked_matrix_practice','Worked matrix practice'),
        reading_prefix=WORKED_FAMILY,question_template=REFERENCE_TEMPLATE,
        lesson_template=ROOT/'content/authoring/learning/matrix_reference/lesson.md.in',fields=worked_fields,filename_prefix='worked_')[0]


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
        values=[o['value'] for o in step['choices']]
        choices=choices_text(values,next(j for j,o in enumerate(step['choices']) if o['id']=='correct'),i)
        fields.update({f'choices_{i}':choices,f'after_{i}':linear.equation(q['states'][i],typeset=False),
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
    docs,reading_ids=teaching_documents(questions,groups=linear.STRATA,titles=LINEAR_TITLES,group_key='stratum',
        subject=('algebra','Algebra'),chapter=('worked_linear_practice','Worked linear practice'),
        reading_prefix='linear_worked_v1',question_template=LINEAR_TEMPLATE,
        lesson_template=ROOT/'content/authoring/learning/linear_reference/lesson.md.in',fields=linear_fields)
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
        docs.update(worked_documents(worked))
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


def probability_questions(count, recipe):
    export.require(type(count) is int and 3<=count<=36 and count%3==0,
                   'batch.count','Probability count must be a multiple of 3 from 3 through 36')
    require(set(recipe)=={'family','cases'} and recipe['family']==PROBABILITY_FAMILY,
            'Use the finite-probability version-1 recipe')
    require(len(recipe['cases'])==12 and len({tuple(c) for c in recipe['cases']})==12,
            'The fixed probability pool needs twelve distinct cases')
    groups=[]
    for group in PROBABILITY_GROUPS:
        pool=[]
        for i,(n,divisor) in enumerate(recipe['cases']):
            require(type(n) is int and type(divisor) is int and 4<=n<=15 and 2<=divisor<=n,
                    'Probability cases require integers 4 <= n <= 15 and 2 <= divisor <= n')
            complement=group=='complement' or (group=='boundary' and i%2==1)
            if group=='boundary':divisor=n+1
            params=dict(n=n,divisor=divisor,complement=complement)
            identity=PROBABILITY_FAMILY+'_'+export.sha(export.encoded(params))[:24]
            # Construction uses integer division. The oracle enumerates the original outcomes.
            k=n-n//divisor if complement else n//divisor
            counts=[k]+sorted((v for v in range(n+1) if v!=k),key=lambda v:(abs(v-k),v))[:2]
            steps=[]
            for step,values in enumerate((list(map(str,counts)),[str(Fraction(v,n)) for v in counts]),1):
                answer=values[0];values.sort(key=lambda v:export.sha(export.encoded([identity,step,v])))
                steps.append(dict(choices=values,answer=answer))
            pool.append(dict(id=identity,group=group,parameters=params,count=k,answer=str(Fraction(k,n)),steps=steps))
        groups.append(pool[:count//3])
    return [q for row in zip(*groups) for q in row]


def verify_probability(q):
    """An exact finite-measure oracle, independent of the integer-division constructor."""
    p=q['parameters'];n=p['n'];d=p['divisor'];complement=p['complement']
    require(type(n) is int and 4<=n<=15 and type(d) is int and 2<=d<=n+1
            and type(complement) is bool,'Invalid uniform finite sample space or event')
    outcomes=list(range(1,n+1))
    members=[v for v in outcomes if (v%d!=0 if complement else v%d==0)]
    probability=sum((Fraction(1,n) for _ in members),Fraction(0))
    require(type(q['count']) is int and q['count']==len(members) and Fraction(q['answer'])==probability,
            f"{q['id']}: declared answer disagrees with enumeration of the original outcomes")
    require(len(q['steps'])==2,'Finite probability has two checked steps')
    for i,(step,answer) in enumerate(zip(q['steps'],(Fraction(len(members)),probability))):
        values=list(map(Fraction,step['choices']))
        require(len(values)==3 and len(set(values))==3 and values.count(answer)==1
                and Fraction(step['answer'])==answer,f"{q['id']}: step {i+1} needs one correct and two distinct incorrect choices")
        require(all(0<=v<=n and v.denominator==1 for v in values) if i==0 else all(0<=v<=1 for v in values),
                f"{q['id']}: choices are outside the count/probability domain")
    return dict(id=q['id'],accepted=True,outcomes=outcomes,event=members,answer=str(probability),
                total_mass=str(sum((Fraction(1,n) for _ in outcomes),Fraction(0))),steps_checked=2,wrong_choices_checked=4)


def probability_fields(q, number):
    p=q['parameters'];n=p['n'];d=p['divisor'];k=q['count']
    relation='not divisible' if p['complement'] else 'divisible'
    symbol=r'\nmid' if p['complement'] else r'\mid'
    members=[v for v in range(1,n+1) if (v%d!=0 if p['complement'] else v%d==0)]
    event=r'\varnothing' if not members else r'\{'+','.join(map(str,members))+r'\}'
    fields=dict(id=q['id'],title=f"{PROBABILITY_TITLES[PROBABILITY_GROUPS.index(q['group'])]} · Exercise {number}",
                n=str(n),divisor=str(d),relation=relation,count=str(k),answer=tex(q['answer']),
                given=rf"\Omega=\{{1,2,\ldots,{n}\}},\quad E=\{{j\in\Omega:{d}{symbol} j\}}",
                after_1=rf"E={event},\quad |E|={k}",after_2=rf"P(E)=\frac{{{k}}}{{{n}}}={tex(q['answer'])}",
                count_reason=f"The event contains {k} of the {n} labels: "+(', '.join(map(str,members)) if members else 'none')+'.',
                complement_note=('Count the labels outside the multiples. Subtract the number of multiples from the total; the sample space still has the same size.'
                                 if p['complement'] else 'Count the multiples in the stated range, including the final label if it qualifies.'))
    for i,step in enumerate(q['steps'],1):
        values=step['choices'];correct=values.index(step['answer'])
        fields[f'choices_{i}']=choices_text(values,correct,i,tex)
        corrections=[]
        for j,v in enumerate(values):
            if j==correct:continue
            if i==1:
                direction='too many' if Fraction(v)>k else 'too few'
                reason=f"This counts {direction} labels. Test each label once against '{relation} by {d}'; zero is outside the sample space."
            else:
                implied=Fraction(v)*n
                reason=f"This fraction would assign the event {implied} of the {n} equally likely labels. Use the event count already established, and divide by the total."
            corrections.append(f'@feedback {i*10+j+1} | {reason}')
        fields[f'feedback_{i}']='\n'.join(corrections)
    return fields


def validate_role_sequence(sequence, source):
    """Shared authoring contract; family-specific mathematics supplies the certificates."""
    def check(ok,field,message):
        export.require(ok,'batch.role',f'{source}:{field}: {message}')
    check(type(sequence) is dict and set(sequence)=={'format','format_version','questions'},'/', 'Expected the exercise-role sequence fields')
    check(sequence['format']=='paths_exercise_roles' and type(sequence['format_version']) is int and sequence['format_version']==1,'/format','Unsupported exercise-role format')
    questions=sequence['questions']
    check(type(questions) is list and len(questions)==len(EXERCISE_ROLES),'/questions','Supply one question for each of the six exercise roles')
    for i,q in enumerate(questions):
        field=f'/questions/{i}'
        check(type(q) is dict and set(q)=={'id','role','title','objective','prerequisites','case'},field,'Each role needs id, role, title, objective, prerequisites and a family case')
        for name in ('id','role','title','objective','prerequisites'):
            check(type(q[name]) is str and 0<len(q[name].strip())<=1000,field+'/'+name,'Expected a nonempty bounded string')
        check(q['role'] in EXERCISE_ROLES,field+'/role','Unknown exercise role')
        check(type(q['case']) is dict and bool(q['case']),field+'/case','Supply original inputs for the family checker')
    check(len({q['id'] for q in questions})==len(questions),'/questions','Repeated question identity')
    check([q['role'] for q in questions]==list(EXERCISE_ROLES),'/questions','Use each role once in teaching order')
    return questions


def verify_role_content(questions, compiled, certificates):
    """A common certificate boundary for authored multiple-choice role sequences."""
    def check(ok,q,field,message):
        export.require(ok,'batch.role',f"{q['id']} ({q['role']}) {field}: {message}")
    by_id={q['id']:q['question'] for q in compiled};checks={q['id']:q for q in certificates}
    export.require(len(by_id)==len(compiled)==len(questions)==len(checks)==len(certificates)
                   and set(by_id)==set(checks)=={q['id'] for q in questions},
                   'batch.role','Role questions, compiled questions and certificates must have identical unique identities')
    for q in questions:
        actual=by_id[q['id']];certificate=checks[q['id']];expected=certificate['expected']
        check(certificate['accepted'] is True and certificate['role']==q['role']
              and certificate['evidence']['kind']==EXERCISE_ROLES[q['role']] and bool(certificate['evidence']['facts']),
              q,'certificate','Missing the mathematical evidence required by this role')
        check('support' not in actual and actual['description'].startswith(q['objective']+' '),q,'goal','Role objective or response mode differs from the checked sequence')
        check(actual['equation']==expected['given'] and [s['display'] for s in actual['working_states']]==[expected['given'],*expected['after']],
              q,'given/after','Compiled givens or working disagree with the independent certificate')
        check(len(actual['steps'])==len(expected['steps']),q,'steps','Unexpected step count')
        for i,(step,certified) in enumerate(zip(actual['steps'],expected['steps']),1):
            labels=[o['label'] for o in step['options']]
            accepted=[o['label'] for o in step['options'] if o['id'] in step['accepted_option_ids']]
            check(len(labels)==len(set(labels)) and sorted(labels)==sorted(certified['choices']) and accepted==[certified['answer']],
                  q,f'step {i}','Compiled choices or key disagree with the checked decision')
            check(all(o.get('wrong_feedback','').strip() for o in step['options'] if o['id'] not in step['accepted_option_ids']),
                  q,f'step {i}','Each distractor needs its own correction')
        if q['role']=='independent':
            check(len(actual['steps'])==1 and all(not s.get('hint') for s in actual['steps']),q,'guidance','The fresh problem must remain one uncued choice; linked reading is optional')


def case_fields(q, keys):
    case=q['case'];require(set(case)==set(keys),f"{q['id']}: unexpected original case fields")
    return case


def finite_set(values):
    return r'\{'+','.join(map(str,values))+r'\}' if values else r'\varnothing'


def uniform_case(q, keys=('n','event')):
    case=case_fields(q,keys);n=case['n']
    require(type(n) is int and 3<=n<=32,f"{q['id']}: expected 3 through 32 equally likely outcomes")
    outcomes=list(range(1,n+1))
    if 'divisor' in case:
        d=case['divisor'];require(type(d) is int and 2<=d<=n,'Invalid divisor')
        event=[v for v in outcomes if v%d==0]
    else:event=case['event']
    require(type(event) is list and all(type(v) is int and v in outcomes for v in event) and len(event)==len(set(event)),'Event must contain distinct permitted labels')
    return n,sorted(event),sum((Fraction(1,n) for _ in event),Fraction(0))


def notation_certificate(q):
    n,divisible,_=uniform_case(q,('n','divisor'));d=q['case']['divisor']
    event=[v for v in range(1,n+1) if v not in divisible]
    given=rf'\Omega=\{{1,2,\ldots,{n}\}},\quad E=\{{j\in\Omega:{d}\nmid j\}}'
    return given,[f'E={finite_set(event)}'],[(list(map(finite_set,(event,divisible,[0,*divisible[:-1]]))),finite_set(event))],{'event':event,'excluded':divisible}


def worked_certificate(q):
    n,event,probability=uniform_case(q);k=len(event)
    given=rf'\Omega=\{{1,2,\ldots,{n}\}},\quad E={finite_set(event)},\quad P(E)=\frac{{{k}}}{{\square}}'
    return given,[rf'P(E)=\frac{{{k}}}{{{n}}}={tex(probability)}'],[([str(n-k),str(n),str(k)],str(n))],{'event_count':k,'sample_count':n,'probability':str(probability)}


def method_certificate(q):
    case=case_fields(q,('weights','event'));event=case['event']
    require(type(case['weights']) is dict and list(case['weights'])==['a','b','c'] and event==['a','b'],'This bounded contrast uses outcomes a, b, c and event a or b')
    require(all(type(v) is str for v in case['weights'].values()),'Outcome weights must be exact fraction strings')
    weights={k:Fraction(v) for k,v in case['weights'].items()}
    require(all(0<w<1 for w in weights.values()) and sum(weights.values())==1 and len(set(weights.values()))>1,'Contrast needs positive unequal weights summing to one')
    probability=sum(weights[k] for k in event);counting=Fraction(len(event),len(weights))
    require(probability!=counting,'Avoid a counting shortcut that accidentally gives the same answer')
    given=rf'P(a)={tex(weights["a"])},\quad P(b)={tex(weights["b"])},\quad P(c)={tex(weights["c"])},\quad E=\{{a,b\}}'
    answer=tex(weights['a'])+'+'+tex(weights['b'])
    return given,[rf'P(E)={answer}={tex(probability)}'],[([tex(counting),tex(weights['c']),answer],answer)],{'weights':{k:str(v) for k,v in weights.items()},'weighted_sum':str(probability),'counting_shortcut':str(counting)}


def explanation_certificate(q):
    n,event,probability=uniform_case(q);outside=[v for v in range(1,n+1) if v not in event]
    require(bool(event) and bool(outside) and len(event)!=len(outside),'Complement contrast needs distinct nonempty event sizes')
    given=rf'\Omega=\{{1,2,\ldots,{n}\}},\quad E={finite_set(event)},\quad P(E^c)=1-P(E)'
    answer=r'E\cap E^c=\varnothing,\quad E\cup E^c=\Omega'
    return given,[rf'P(E^c)=1-{tex(probability)}={tex(1-probability)}'],[([r'|E|=|E^c|',answer,r'E^c\subseteq E'],answer)],{'event':event,'complement':outside,'intersection':[],'union':list(range(1,n+1))}


def repair_certificate(q):
    n,event,probability=uniform_case(q,('n','divisor'));d=q['case']['divisor'];wrong=[0,*event];k=len(wrong)
    given=(rf'\begin{{gathered}}\Omega=\{{1,2,\ldots,{n}\}},\quad E=\{{j\in\Omega:{d}\mid j\}}\\'
           rf'\begin{{aligned}}L_1 &: E={finite_set(wrong)}\\L_2 &: |E|={k}\\L_3 &: P(E)=\frac{{{k}}}{{{n}}}\end{{aligned}}\end{{gathered}}')
    after=[r'L_1:\quad 0\notin\Omega',rf'E={finite_set(event)},\quad P(E)=\frac{{{len(event)}}}{{{n}}}={tex(probability)}']
    return given,after,[(['L_2','L_1','L_3'],'L_1'),([tex(Fraction(k,n)),tex(Fraction(1,len(event)+1)),tex(probability)],tex(probability))],{'first_error':'L_1','excluded_label':0,'event':event,'probability':str(probability)}


def transfer_certificate(q):
    case=case_fields(q,('blue','red','green'));require(all(type(v) is int and 1<=v<=10 for v in case.values()),'Token counts must be positive integers at most ten')
    # Enumerate physical tokens: colours are events, not equally likely outcomes.
    tokens=[(colour,i) for colour,count in case.items() for i in range(1,count+1)];event=[t for t in tokens if t[0]!='red'];n=len(tokens)
    probability=sum((Fraction(1,n) for _ in event),Fraction(0))
    given=rf'B={case["blue"]},\quad R={case["red"]},\quad G={case["green"]}'
    return given,[rf'P(\mathrm{{not\ red}})=\frac{{{len(event)}}}{{{n}}}={tex(probability)}'],[([tex(Fraction(case['red'],n)),tex(probability),tex(Fraction(case['blue'],n))],tex(probability))],{'tokens':tokens,'event_tokens':event,'probability':str(probability)}


ROLE_CHECKERS={
    'read_notation':notation_certificate, 'worked_check':worked_certificate,
    'choose_next_step':method_certificate, 'explain_step':explanation_certificate,
    'repair_error':repair_certificate, 'independent':transfer_certificate,
}


def reasoning_certificate(q, checkers=ROLE_CHECKERS):
    try:
        given,after,steps,facts=checkers[q['role']](q)
    except export.ExportError as error:
        raise export.ExportError(error.code,f"{q['id']} ({q['role']}): {error}",error.diagnostics) from error
    require(len(after)==len(steps) and all(len(choices)==3 and len(set(choices))==3 and choices.count(answer)==1 for choices,answer in steps),f"{q['id']}: ambiguous reasoning choices")
    return dict(id=q['id'],role=q['role'],accepted=True,steps_checked=len(steps),wrong_choices_checked=2*len(steps),
        evidence=dict(kind=EXERCISE_ROLES[q['role']],facts=facts),
        expected=dict(given=given,after=after,steps=[dict(choices=choices,answer=answer) for choices,answer in steps]))


def reasoning_documents(root, checkers, values, filename):
    source=root/'sequence.json';raw=export.read_bytes(source,128*1024)
    questions=validate_role_sequence(export.decoded(raw),source);certificates=[reasoning_certificate(q,checkers) for q in questions]
    for q in questions:
        for key in ('id','title','objective'):values[q['role']+'_'+key]=q[key]
    values['practice_links']=''.join(f"@practice {q['id']}\n" for q in questions)
    for field,name in (('lesson_blocks','lesson.md.in'),('questions','questions.paths.md.in')):
        path=root/name;values[field]=fill_template(export.read_bytes(path,128*1024).decode('utf-8'),values,path)
    return questions,certificates,{filename:chapter_text(values).encode()},values['reading_id'],export.sha(raw)


def verify_probability_compiled(questions, compiled):
    """Check what the real compiler will grade, not just the producer's in-memory keys."""
    by_id={q['id']:q for q in compiled}
    require(len(by_id)==len(compiled) and set(by_id)=={q['id'] for q in questions},'Compiled probability identities differ')
    roles=[q for q in questions if 'role' in q]
    if roles:verify_role_content(roles,[by_id[q['id']] for q in roles],[reasoning_certificate(q) for q in roles])
    for q in (q for q in questions if 'role' not in q):
        oracle=verify_probability(q);actual=by_id[q['id']]['question'];params=q['parameters']
        n=len(oracle['outcomes']);k=len(oracle['event']);symbol=r'\nmid' if params['complement'] else r'\mid'
        given=rf"\Omega=\{{1,2,\ldots,{n}\}},\quad E=\{{j\in\Omega:{params['divisor']}{symbol} j\}}"
        members=r'\{'+','.join(map(str,oracle['event']))+r'\}' if k else r'\varnothing'
        # Do not call the rendering adapter here: a bad adapter must fail before export.
        states=[given,rf"E={members},\quad |E|={k}",rf"P(E)=\frac{{{k}}}{{{n}}}={tex(oracle['answer'])}"]
        require(actual['equation']==given and len(actual['steps'])==2
                and [s['display'] for s in actual['working_states']]==states,
                f"{q['id']}: compiled givens or working differ from the independently checked problem")
        for i,(step,answer) in enumerate(zip(actual['steps'],(str(len(oracle['event'])),oracle['answer']))):
            expected=q['steps'][i];accepted=step['accepted_option_ids']
            require([o['label'] for o in step['options']]==[tex(v) for v in expected['choices']]
                    and accepted==[10*(i+1)+expected['choices'].index(answer)+1],
                    f"{q['id']}: compiled step {i+1} answer key or choices disagree with the enumerated event")


def probability_batch(args):
    export.require(args.format_version in (1,2),'batch.format','Finite probability uses format version 1 or 2')
    export.require(args.format_version==1 or args.version>=2,'batch.version','Exercise roles require package version 2 or later')
    recipe_bytes=export.read_bytes(PROBABILITY_ROOT/'recipe.json',128*1024)
    questions=probability_questions(args.count,export.decoded(recipe_bytes))
    results=[verify_probability(q) for q in questions]
    docs,readings=teaching_documents(questions,groups=PROBABILITY_GROUPS,titles=PROBABILITY_TITLES,group_key='group',
        subject=('probability_statistics','Probability and Statistics'),
        chapter=('finite_probability_practice','Finite probability: count and compare'),reading_prefix=PROBABILITY_FAMILY,
        question_template=PROBABILITY_ROOT/'question.paths.md.in',lesson_template=PROBABILITY_ROOT/'lesson.md.in',fields=probability_fields,numbered_files=True)
    author=dict(format='paths_learning_authoring',format_version=1,package_id='finite_probability_practice',package_version=args.version,
        sources=[dict(id=PROBABILITY_FAMILY,kind='generated',title='Original uniform finite-probability practice',
            uri='paths:generated/'+PROBABILITY_FAMILY,revision='1',
            attribution='Original Paths questions and explanations. Counting and complement definitions checked against OpenStax Introductory Statistics 2e, section 3.1: https://openstax.org/books/introductory-statistics-2e/pages/3-1-terminology',
            reuse='Original examples; no external exercise text copied.',content_ids=[q['id'] for q in questions]+readings)])
    audit=dict(family=PROBABILITY_FAMILY,family_version=1,format_version=1,count=len(questions),questions=questions,
        mathematical_checks=results,recipe_sha256=export.sha(recipe_bytes),
        template_sha256={role:export.sha(export.read_bytes(p,128*1024)) for role,p in
                         (('chapter',CHAPTER_TEMPLATE),('question',PROBABILITY_ROOT/'question.paths.md.in'),('lesson',PROBABILITY_ROOT/'lesson.md.in'))},
        response_mode='choices.v1',written_checker=False,worked_questions=len(questions),retained_questions=0,
        teaching_review='Original finite counting lesson; user visual acceptance pending')
    if args.format_version==2:
        roles,certificates,role_docs,reading_id,manifest_hash=reasoning_documents(REASONING_ROOT,ROLE_CHECKERS,
            dict(subject='probability_statistics',subject_title='Probability and Statistics',
                 chapter='finite_probability_practice',chapter_title='Finite probability: count and compare',
                 reading_id='probability_reasoning_v1_reading',reading_title='Reason about probability'),'04_reasoning.paths.md')
        questions=questions+roles;results=results+certificates;docs.update(role_docs)
        author['sources'].append(dict(id='probability_reasoning_v1',kind='generated',title='Original probability reasoning sequence',
            uri='paths:generated/probability_reasoning_v1',revision='1',attribution=author['sources'][0]['attribution']
                +' Addition of disjoint outcome probabilities checked against section 3.3: https://openstax.org/books/introductory-statistics-2e/pages/3-3-two-basic-rules-of-probability',
            reuse='Original Paths questions, explanations and exact finite-outcome certificates.',content_ids=[q['id'] for q in roles]+[reading_id]))
        audit.update(format_version=2,count=len(questions),questions=questions,mathematical_checks=results,
                     retained_questions=args.count,worked_questions=len(roles),exercise_roles=list(EXERCISE_ROLES),role_manifest_sha256=manifest_hash,
                     role_template_sha256={name:export.sha(export.read_bytes(REASONING_ROOT/name,128*1024)) for name in ('lesson.md.in','questions.paths.md.in')})
    return questions,results,docs,author,audit


def row_equation(row):
    lhs=''
    for value,symbol in zip(row,('x','y')):
        if value:lhs+=('-' if value<0 else '+' if lhs else '')+('' if abs(value)==1 else tex(abs(value)))+symbol
    return (lhs or '0')+'='+tex(row[2])


def exact_rows(rows, count, identity):
    require(type(rows) is list and len(rows)==count and all(type(row) is list and len(row)==3
            and all(type(v) is int and abs(v)<=20 for v in row) for row in rows),
            f'{identity}: expected {count} augmented rows of three integers, magnitude at most 20')
    return [[Fraction(v) for v in row] for row in rows]


def system_solution(rows):
    (a,b,c),(d,e,f)=rows;det=a*e-b*d
    require(det!=0,'This bounded matrix sequence requires a unique solution')
    xy=[(c*e-b*f)/det,(a*f-c*d)/det]
    require(all(a*xy[0]+b*xy[1]==c for a,b,c in rows),'Original-row substitution failed')
    return xy


def matrix_case(q, operation=False):
    case=case_fields(q,('rows','multiplier') if operation else ('rows',))
    rows=exact_rows(case['rows'],2,q['id']);xy=system_solution(rows)
    if operation:require(type(case['multiplier']) is int and 0<abs(case['multiplier'])<=6,f"{q['id']}: expected a nonzero row-addition multiplier, magnitude at most 6")
    return rows,xy


def row_addition(rows, multiplier):
    after=operate(rows,'add_row_1_to_2',multiplier)
    require(system_solution(after)==system_solution(rows),'Row addition changed the original solution')
    require(operate(after,'add_row_1_to_2',-multiplier)==rows,'Inverse did not recover every original entry')
    return after


def row_operation(multiplier, destination=2, source=1):
    return add_tex(rf'R_{destination}\leftarrow R_{destination}',multiplier,rf'R_{source}')


def matrix_notation_certificate(q):
    row=exact_rows([case_fields(q,('row',))['row']],1,q['id'])[0];a,b,c=row
    answer=row_equation(row);choices=[answer,row_equation([a,-b,c]),row_equation([a,-c,b])]
    return matrix_tex([row]),[answer],[(choices,answer)],dict(coefficients=list(map(str,row[:2])),constant=str(c),equation=answer)


def matrix_worked_certificate(q):
    rows,xy=matrix_case(q,True);k=q['case']['multiplier'];after=row_addition(rows,k);a,b,c=after[1]
    require(a==0 and b==1,'Worked role supplies the two completed coefficients 0 and 1')
    operation=row_operation(k)
    missing=rf"R'_2=\left[\begin{{array}}{{cc|c}}{tex(a)}&{tex(b)}&\square\end{{array}}\right]"
    given=r'\begin{gathered}'+matrix_tex(rows)+r'\\'+operation+r'\\'+missing+r'\end{gathered}'
    values=[rows[1][2],c,rows[1][2]-k*rows[0][2]]
    return given,[matrix_tex(after)],[(list(map(tex,values)),tex(c))],dict(new_constant=str(c),new_row=list(map(str,after[1])),original_solution=list(map(str,xy)))


def matrix_method_certificate(q):
    rows,xy=matrix_case(q);require(rows[0][0]!=0 and rows[1][0]!=0,'Elimination needs a nonzero source and target coefficient')
    k=-rows[1][0]/rows[0][0];after=row_addition(rows,k)
    candidates=[(-k,2,1),(k,1,2),(k,2,1)]
    changed=[operate(rows,'add_row_1_to_2' if destination==2 else 'add_row_2_to_1',factor) for factor,destination,_ in candidates]
    matches=[i for i,result in enumerate(changed) if result[0]==rows[0] and result[1][0]==0]
    require(matches==[2],'Exactly one operation must cancel row 2 while retaining row 1')
    choices=[row_operation(*candidate) for candidate in candidates]
    return matrix_tex(rows),[matrix_tex(after)],[(choices,choices[2])],dict(multiplier=str(k),target_entries=[str(r[1][0]) for r in changed],original_solution=list(map(str,xy)))


def matrix_explanation_certificate(q):
    rows,xy=matrix_case(q,True);k=q['case']['multiplier'];after=row_addition(rows,k)
    repeated=operate(after,'add_row_1_to_2',k);erased=[after[0],[Fraction(0)]*3]
    recovered=operate(after,'add_row_1_to_2',-k)
    require(repeated!=rows and erased!=rows and recovered==rows,'Only the inverse may recover the original system')
    given=r'\begin{gathered}'+matrix_tex(rows)+r'\\'+row_operation(k)+r'\\'+matrix_tex(after)+r'\end{gathered}'
    choices=[row_operation(k),r'R_2\leftarrow 0R_2',row_operation(-k)]
    return given,[matrix_tex(rows)],[(choices,choices[2])],dict(inverse_multiplier=str(-k),recovered_rows=[[str(v) for v in row] for row in recovered],original_solution=list(map(str,xy)))


def matrix_repair_certificate(q):
    rows,xy=matrix_case(q,True);k=q['case']['multiplier'];after=row_addition(rows,k)
    require(after[1][:2]==[0,1] and k<0 and rows[0][2]!=0,'Repair case requires cancellation to [0, 1 | c] and a changed constant')
    a,b,c=rows[1];source=rows[0];m=-k;bad=[after[1][0],after[1][1],c]
    # L1 omits only the constant operation; L2 evaluates it and L3 reads that bad row.
    expressions=[rf'{tex(value)}-{tex(m)}({tex(other)})' for value,other in zip(rows[1],source)]
    given=(r'\begin{gathered}'+matrix_tex(rows)+r"\\\begin{aligned}L_1 &: R'_2=["+expressions[0]+r',\,'+expressions[1]+r'\mid'+tex(c)+r"]\\L_2 &: R'_2=[0,\,1\mid"+tex(c)+r']\\L_3 &: y='+tex(c)+r'\end{aligned}\end{gathered}')
    corrected=after[1][2];wrong_xy=[(source[2]-source[1]*c)/source[0],c]
    require(sum(value*component for value,component in zip(rows[1][:2],wrong_xy))!=c,'The incomplete operation must change the original solution')
    correction=rf'L_1:\quad {tex(c)}\ \mathrm{{must\ become}}\ {expressions[2]}'
    choices=list(map(tex,(c,c-source[2],corrected)))
    return given,[correction,matrix_tex(after)+rf',\quad y={tex(corrected)}'],[(['L_1','L_2','L_3'],'L_1'),(choices,tex(corrected))],dict(first_error='L_1',incorrect_row=list(map(str,bad)),corrected_row=list(map(str,after[1])),original_solution=list(map(str,xy)),incorrect_pair=list(map(str,wrong_xy)))


def matrix_transfer_certificate(q):
    rows,(x,y)=matrix_case(q);pairs=[(y,x),(x,y),(2*x,y)]
    residuals=[[a*u+b*v-c for a,b,c in rows] for u,v in pairs]
    require([i for i,r in enumerate(residuals) if r==[0,0]]==[1],'Only one ordered pair must satisfy both original equations')
    choices=[rf'(x,y)=\left({tex(u)},{tex(v)}\right)' for u,v in pairs]
    return matrix_tex(rows),[rf'x={tex(x)},\quad y={tex(y)}'],[(choices,choices[1])],dict(solution=list(map(str,(x,y))),choice_residuals=[[str(v) for v in r] for r in residuals])


MATRIX_ROLE_CHECKERS={
    'read_notation':matrix_notation_certificate,'worked_check':matrix_worked_certificate,
    'choose_next_step':matrix_method_certificate,'explain_step':matrix_explanation_certificate,
    'repair_error':matrix_repair_certificate,'independent':matrix_transfer_certificate,
}


def matrix_reasoning_batch(args):
    export.require(args.count==6 and type(args.count) is int,'batch.count','Matrix reasoning is one six-question sequence; --count must be 6')
    export.require(args.format_version==1,'batch.format','Matrix reasoning uses format version 1')
    questions,checks,docs,reading,manifest_hash=reasoning_documents(MATRIX_REASONING_ROOT,MATRIX_ROLE_CHECKERS,
        dict(subject='linear_algebra',subject_title='Linear Algebra',chapter='worked_matrix_practice',chapter_title='Worked matrix practice',
             reading_id='matrix_reasoning_v1_reading',reading_title='Reason about row operations'),'matrix_reasoning.paths.md')
    author=dict(format='paths_learning_authoring',format_version=1,package_id='matrix_reasoning_practice',package_version=args.version,
        sources=[dict(id='matrix_reasoning_v1',kind='generated',title='Original matrix row-operation reasoning sequence',
            uri='paths:generated/matrix_reasoning_v1',revision='1',
            attribution='Original Paths examples and explanations. Augmented matrices and row-operation conventions checked against OpenStax College Algebra 2e, section 7.6: https://openstax.org/books/college-algebra-2e/pages/7-6-solving-systems-with-gaussian-elimination',
            reuse='Original questions and exact rational certificates; no external exercise text copied.',content_ids=[q['id'] for q in questions]+[reading])])
    audit=dict(family='matrix_reasoning_v1',family_version=1,format_version=1,count=6,questions=questions,mathematical_checks=checks,
        response_mode='choices.v1',written_checker=False,worked_questions=6,retained_questions=0,exercise_roles=list(EXERCISE_ROLES),role_manifest_sha256=manifest_hash,
        template_sha256={name:export.sha(export.read_bytes(MATRIX_REASONING_ROOT/name,128*1024)) for name in ('lesson.md.in','questions.paths.md.in')},
        chapter_sha256=export.sha(export.read_bytes(CHAPTER_TEMPLATE,128*1024)),teaching_review='Original six-role matrix sequence; user visual and teaching acceptance pending')
    return questions,checks,docs,author,audit


def verify_matrix_reasoning_compiled(questions, compiled):
    verify_role_content(questions,compiled,[reasoning_certificate(q,MATRIX_ROLE_CHECKERS) for q in questions])


BUILDERS={'matrix':matrix_batch,'linear':linear_batch,'probability':probability_batch,'matrix-reasoning':matrix_reasoning_batch}
COMPILED_CHECKS={'probability':verify_probability_compiled,'matrix-reasoning':verify_matrix_reasoning_compiled}
ROUTES_PER_QUESTION={'matrix':5,'linear':5,'probability':1,'matrix-reasoning':1}


def authoring_inputs(family):
    paths={
        'matrix':(REFERENCE_TEMPLATE,ROOT/'content/authoring/learning/matrix_reference/lesson.md.in'),
        'linear':(LINEAR_TEMPLATE,ROOT/'content/authoring/learning/linear_reference/lesson.md.in',linear.SPEC,Path(linear.__file__)),
        'probability':tuple(PROBABILITY_ROOT/name for name in ('recipe.json','question.paths.md.in','lesson.md.in'))
                      +tuple(REASONING_ROOT/name for name in ('sequence.json','questions.paths.md.in','lesson.md.in')),
        'matrix-reasoning':tuple(MATRIX_REASONING_ROOT/name for name in ('sequence.json','questions.paths.md.in','lesson.md.in')),
    }[family]
    return {str(p.relative_to(ROOT) if p.is_relative_to(ROOT) else p):export.sha(export.read_bytes(p,128*1024))
            for p in (Path(__file__),CHAPTER_TEMPLATE,*paths)}


def run(args):
    inputs=authoring_inputs(args.family)
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
                       and routes['routes'] == len(questions) * ROUTES_PER_QUESTION[args.family], "batch.routes", "Model gate did not exercise every generated question")
        if args.family in COMPILED_CHECKS:COMPILED_CHECKS[args.family](questions,routes['questions'])
    export.require(export.sha(export.read_bytes(model, 64 * 1024 * 1024)) == model_hash,
                   "batch.model_changed", "Model executable changed during verification; rerun the batch")
    export.require(authoring_inputs(args.family)==inputs,'batch.source_changed',
                   'Authoring input changed during verification; rerun the batch before publishing')
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
                authoring_inputs_sha256=inputs,publication=published, visual_acceptance="pending_user")


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--family', choices=tuple(BUILDERS), default='matrix')
    cli.add_argument('--count', type=int, help='matrix/probability: 3..36 by 3 (default 12); linear: 6..36 by 6 (default 12); matrix-reasoning: exactly 6')
    cli.add_argument('--version', type=int, help='immutable package version; increase when extending a batch')
    cli.add_argument('--format-version', type=int, choices=(1,2), help='matrix/probability default to 2, linear/matrix-reasoning to 1')
    cli.add_argument('--target', type=Path, default=ROOT / 'b/sorter')
    cli.add_argument('--model', type=Path, default=ROOT / 'b/paths_learning_document_tests')
    cli.add_argument('--output', type=Path)
    cli.add_argument('--store', type=Path)
    cli.add_argument('--base-documents', type=Path)
    cli.add_argument('--publish', action='store_true', help='activate through the existing publisher; otherwise export only')
    try:
        args = cli.parse_args()
        default={"matrix":2,"linear":1,"probability":2,"matrix-reasoning":1}[args.family]
        if args.count is None:args.count={'matrix':12,'linear':12,'probability':12,'matrix-reasoning':6}[args.family]
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
