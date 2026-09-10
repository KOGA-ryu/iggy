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


def teaching_documents(questions, *, groups, titles, group_key, subject, chapter,
                       reading_prefix, question_template, lesson_template, fields, filename_prefix="", numbered_files=False):
    """Use the existing document grammar and textbook block types for every family."""
    wrapper=export.read_bytes(CHAPTER_TEMPLATE,128*1024).decode('utf-8')
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
        docs[prefix+group+'.paths.md']=fill_template(wrapper,values,CHAPTER_TEMPLATE).encode()
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


def verify_probability_compiled(questions, compiled):
    """Check what the real compiler will grade, not just the producer's in-memory keys."""
    by_id={q['id']:q for q in compiled}
    require(len(by_id)==len(compiled) and set(by_id)=={q['id'] for q in questions},'Compiled probability identities differ')
    for q in questions:
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
    export.require(args.format_version==1,'batch.format','Finite probability currently uses format version 1')
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
    return questions,results,docs,author,audit


BUILDERS={'matrix':matrix_batch,'linear':linear_batch,'probability':probability_batch}
COMPILED_CHECKS={'probability':verify_probability_compiled}
ROUTES_PER_QUESTION={'matrix':5,'linear':5,'probability':1}


def authoring_inputs(family):
    paths={
        'matrix':(REFERENCE_TEMPLATE,ROOT/'content/authoring/learning/matrix_reference/lesson.md.in'),
        'linear':(LINEAR_TEMPLATE,ROOT/'content/authoring/learning/linear_reference/lesson.md.in',linear.SPEC,Path(linear.__file__)),
        'probability':tuple(PROBABILITY_ROOT/name for name in ('recipe.json','question.paths.md.in','lesson.md.in')),
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
    cli.add_argument('--count', type=int, default=12, help='3..36 in multiples of 3 for matrix/probability, 6..36 in multiples of 6 for linear')
    cli.add_argument('--version', type=int, help='immutable package version; increase when extending a batch')
    cli.add_argument('--format-version', type=int, choices=(1,2), help='matrix defaults to 2, linear/probability to 1')
    cli.add_argument('--target', type=Path, default=ROOT / 'b/sorter')
    cli.add_argument('--model', type=Path, default=ROOT / 'b/paths_learning_document_tests')
    cli.add_argument('--output', type=Path)
    cli.add_argument('--store', type=Path)
    cli.add_argument('--base-documents', type=Path)
    cli.add_argument('--publish', action='store_true', help='activate through the existing publisher; otherwise export only')
    try:
        args = cli.parse_args()
        default={"matrix":2,"linear":1,"probability":1}[args.family]
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
