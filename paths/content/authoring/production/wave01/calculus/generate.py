#!/usr/bin/env python3
"""One original-input certificate and Markdown generation route for both families."""
import argparse
import json
from math import comb
from pathlib import Path
import subprocess
import sys
import build_question_batch as batch
import check_authoring_pilot as pilot
import export_learning as export

ROOT = batch.ROOT / 'content/authoring/production/wave01/calculus'
OUT = batch.ROOT / 'build/production/wave01/calculus'
SETS = ('teaching', 'practice', 'fresh_check')
ROLES = tuple(batch.EXERCISE_ROLES)
FAMILIES = ('difference_quotient', 'polynomial_rules')
ROLE_CODE = dict(zip(ROLES, ('rn', 'wc', 'cn', 'es', 're', 'in')))
POSITIONS = {('difference_quotient','teaching'):(1,2,3,1,2,3), ('difference_quotient','practice'):(2,3,1,2,3,1), ('difference_quotient','fresh_check'):(3,1,2,3,1,2), ('polynomial_rules','teaching'):(2,1,3,2,1,3), ('polynomial_rules','practice'):(3,2,1,3,2,1), ('polynomial_rules','fresh_check'):(1,3,2,1,3,2)}


def require(ok, message):
    export.require(ok, 'calculus.production', message)


def polynomial(coefficients, variable='x'):
    """Descending coefficients; parentheses keep negative coefficients unambiguous."""
    degree = len(coefficients)-1
    return '+'.join(f'({v})' + ('' if degree-i == 0 else variable if degree-i == 1 else f'{variable}^{degree-i}')
                    for i,v in enumerate(coefficients) if v) or '0'


def value(c,a):
    return sum(v*a**(len(c)-1-i) for i,v in enumerate(c))


def derivative(c,a):
    return sum(v*(len(c)-1-i)*a**(len(c)-2-i) for i,v in enumerate(c[:-1]))


def expansion(c,a):
    n = len(c)-1
    return [sum(v*comb(n-i,k)*a**(n-i-k) for i,v in enumerate(c) if k<=n-i) for k in range(n+1)]


def sum_terms(c,a, derivative_terms=False):
    terms=[]
    for i,v in enumerate(c):
        power=len(c)-1-i
        if not v or (derivative_terms and power==0): continue
        factor=f'({v})' + (f'({power})' if derivative_terms else '')
        exponent=power-int(derivative_terms)
        terms.append(factor + ('' if exponent==0 else f'({a})' if exponent==1 else f'({a})^{exponent}'))
    return '+'.join(terms) or '0'


def context(q):
    p=batch.case_fields(q, ('family','coefficients','at')); c,a=p['coefficients'],p['at']
    require(p['family'] in FAMILIES and type(c) is list and type(a) is int, 'invalid case types')
    require(len(c)==(3 if p['family']=='difference_quotient' else 4), 'wrong polynomial degree bound')
    require(all(type(v) is int and -9<=v<=9 for v in c) and -3<=a<=3 and any(c[:-1]), 'nonconstant bounded polynomial required')
    e=expansion(c,a); d=derivative(c,a)
    require(e[0]==value(c,a) and e[1]==d, 'expansion and differentiation disagree')
    hp=polynomial(list(reversed(e[1:])), 'h')
    numerator=polynomial(list(reversed([0,*e[1:]])), 'h')
    quotient=rf'\frac{{f({a}+h)-f({a})}}{{h}}'
    return dict(c=c,a=a,e=e,d=d,f=polynomial(c),given=rf'f(x)={polynomial(c)},\quad a={a}',
                expanded=polynomial(list(reversed(e)), 'h'),numerator=numerator,hp=hp,quotient=quotient,
                definition=rf'\lim_{{h\to0}}{quotient}',factored=rf'\frac{{h\left({hp}\right)}}{{h}}',
                derivative_expression=sum_terms(c,a,True), value_expression=sum_terms(c,a),function_value=value(c,a))


def numeric_errors(t):
    """Choose distinct, named computations; no arbitrary numeric filler."""
    c,a,d=t['c'],t['a'],t['d']
    candidates=[('the function value',t['value_expression'],t['function_value']),
                ('the derivative with the constant incorrectly retained',f"{t['derivative_expression']}+({c[-1]})",d+c[-1]),
                ('the negative of the derivative',rf"-\left({t['derivative_expression']}\right)",-d)]
    selected=[]; seen={d}
    for name,expression,v in candidates:
        if v not in seen:
            selected.append(dict(name=name,expression=expression,value=v)); seen.add(v)
        if len(selected)==2: return selected
    require(False, 'case needs two distinct misconception calculations')


def certificate(q):
    t=context(q); role=q['role']; a,d=t['a'],t['d']; given=t['given']
    slope=rf"f'({a})={d}"; facts={'coefficients':t['c'],'at':a,'shifted_coefficients':t['e'],'quotient_coefficients':t['e'][1:],'derivative':d,'function_value':t['function_value']}
    if role=='read_notation':
        answer=t['definition']; choices=[answer,rf'\lim_{{h\to0}}\frac{{h}}{{f({a}+h)-f({a})}}',rf'f({a})']; after=[rf"f'({a})={answer}"]
    elif role in ('worked_check','independent'):
        if role=='worked_check': given+=rf",\qquad f'({a})={t['derivative_expression']}"
        errors=numeric_errors(t); answer=str(d); choices=[answer,*[str(e['value']) for e in errors]]; after=[slope]
    elif role=='choose_next_step':
        given+=rf",\qquad f({a}+h)-f({a})={t['numerator']}"
        answer=t['factored']; choices=[answer,t['hp'],rf"\frac{{{t['hp']}}}{{h}}"]
        after=[rf"{t['quotient']}={answer}={t['hp']},\quad h\ne0"]
    elif role=='explain_step':
        given+=rf",\qquad {t['quotient']}={t['hp']}"
        answer=r'h\ne0'; choices=[answer,r'h=0',r'\text{all real }h']
        after=[rf"{t['quotient']}={t['hp']},\quad h\ne0"]
    elif role=='repair_error':
        wrong=t['e'].copy(); wrong[0]=0; wrong[1]=d-1
        bad_numerator=polynomial(list(reversed(wrong)),'h')
        given=rf"\begin{{gathered}}{given}\\\begin{{aligned}}L_1 &: f({a}+h)={t['expanded']}\\L_2 &: f({a}+h)-f({a})={bad_numerator}\\L_3 &: f'({a})={d-1}\end{{aligned}}\end{{gathered}}"
        errors=numeric_errors(t); facts['first_error']='L2'
        return given,[r'L_2',slope],[(['L2','L1','L3'],'L2'),([str(d),*[str(e['value']) for e in errors]],str(d))],facts
    else: require(False,'unknown role')
    return given,after,[(choices,answer)],facts


CHECKERS={role:certificate for role in ROLES}


def sequence(recipe,family,set_id):
    titles=('Read derivative notation','Complete a derivative calculation','Factor the increment','Explain cancellation','Repair a derivative calculation','Find a fresh derivative')
    objectives=('Identify the difference-quotient definition at the stated point.','Complete the stated derivative arithmetic.','Select the factored quotient before cancellation.','State when the displayed quotient identity is defined.','Locate the first error, then correct the derivative value.','Find the derivative at the given real input.')
    rows=recipe['cases'][family][set_id]; require(len(rows)==6,'six role cases required')
    questions=[]
    for role,(c,a),title,objective in zip(ROLES,rows,titles,objectives):
        case={'family':family,'coefficients':c,'at':a}; digest=export.sha(export.encoded(case))[:6]
        questions.append(dict(id=f'prod01_calculus_{family}_{set_id}_{ROLE_CODE[role]}_{digest}',role=role,title=title,objective=objective,
                              prerequisites='Real substitution, signed polynomial arithmetic and the linked derivative definition.',case=case))
    seq=dict(format='paths_exercise_roles',format_version=1,questions=questions)
    batch.validate_role_sequence(seq,ROOT/'recipe.json'); return seq


def question_text(q, position, template=None):
    t=context(q); cert=batch.reasoning_certificate(q,CHECKERS)['expected']; family=q['case']['family']; role=q['role']
    path=ROOT/'families'/family/'questions.paths.md.in'
    template=export.read_bytes(path).decode() if template is None else template
    marker=f'<!-- {role} -->'; require(template.count(marker)==1, f'{path}: expected one {role} block')
    body=template.split(marker,1)[1].split('<!--',1)[0].strip()
    fields={k:str(v) for k,v in t.items() if not isinstance(v,list)}
    fields.update(id=q['id'],title=q['title'],objective=q['objective'],given=cert['given'],reading_id=f'prod01_calculus_{family}_r')
    for index,step in enumerate(cert['steps'],1):
        labels=step['choices']; answer=step['answer']; pos=position-1 if index==1 else position%3
        ordered=labels[1:]; ordered.insert(pos,answer)
        fields[f'choices_{index}']=batch.choices_text(ordered,pos,index)
        fields[f'after_{index}']=cert['after'][index-1]
        for code,label in zip(('a','b'),labels[1:]): fields[f'wrong_{code}_{index}']=str(index*10+ordered.index(label)+1)
    if role in ('worked_check','independent','repair_error'):
        for code,error in zip(('a','b'),numeric_errors(t)):
            for key,v in error.items(): fields[f'error_{code}_{key}']=str(v)
    return batch.fill_template(body,fields,path)


def source_hashes():
    paths=[ROOT/name for name in ('generate.py','certificate_tests.py','recipe.json','DESIGN.md')]
    paths += [ROOT/'families'/f/name for f in FAMILIES for name in ('lesson.md.in','questions.paths.md.in')]
    return {str(p.relative_to(ROOT)):export.sha(export.read_bytes(p)) for p in paths}


def lesson(family):
    return export.read_bytes(ROOT/'families'/family/'lesson.md.in').decode()


def values(family, questions, text):
    return dict(subject='calculus',subject_title='Calculus',chapter='topic_0060',chapter_title='Differentiation',reading_id=f'prod01_calculus_{family}_r',
                reading_title={'difference_quotient':'Derivatives from nearby slopes','polynomial_rules':'Polynomial derivative rules'}[family],
                practice_links=''.join('@practice '+q['id']+'\n' for q in questions),lesson_blocks=lesson(family),questions=text)


def authoring(package,reading,questions):
    return dict(format='paths_learning_authoring',format_version=1,package_id=package,package_version=1,sources=[dict(id=package+'_original',kind='original',title='Original polynomial derivative exercises',uri='paths:original/'+package+'/v1',revision='1',attribution='Original Paths material; definitions and conditions checked against OpenStax Calculus Volume 1, sections 3.1 and 3.3.',reuse='Original prose and exercises; no external exercise text copied.',content_ids=[reading,*[q['id'] for q in questions]])])


def build_one(recipe,family,set_id,target,model):
    seq=sequence(recipe,family,set_id); qs=seq['questions']; package=f'prod01_calculus_{family}_{set_id}'
    text='\n\n'.join(question_text(q,pos) for q,pos in zip(qs,POSITIONS[family,set_id])); v=values(family,qs,text)
    data={'sequence.json':export.encoded(seq),'lesson.md.in':v['lesson_blocks'].encode(),'questions.paths.md.in':text.encode(),
          'certificates.py':export.read_bytes(ROOT/'generate.py'),'authoring.json':export.encoded(authoring(package,v['reading_id'],qs))}
    digest=export.sha(export.encoded({name:export.sha(b) for name,b in data.items()})); source=OUT/family/set_id/digest/'source'
    export.immutable_directory(source,data)
    result=pilot.check_assignment(dict(subject='calculus',folder=str(source.relative_to(batch.ROOT)),package_id=package,values={k:v[k] for k in ('subject','subject_title','chapter','chapter_title','reading_id','reading_title')}),target,model)
    receipt=Path(result['authoring']).parent/'checks'/export.sha(export.encoded(result))/'verification.json'
    require(receipt.is_file(),'exact shared receipt missing')
    return dict(family=family,set=set_id,authoring=result['authoring'],verification=str(receipt))


def build_family(recipe,family,target,model):
    before=source_hashes(); inspector=export.Target(target); target_hash=inspector.fingerprint; model_hash=export.sha(export.read_bytes(model,64*1024*1024))
    qs=[]; blocks=[]
    for selected in SETS:
        seq=sequence(recipe,family,selected); qs.extend(seq['questions'])
        blocks.extend(question_text(q,pos) for q,pos in zip(seq['questions'],POSITIONS[family,selected]))
    v=values(family,qs,'\n\n'.join(blocks)); package=f'prod01_calculus_{family}'; documents={'chapter.paths.md':batch.chapter_text(v).encode()}
    author=authoring(package,v['reading_id'],qs); inspection=inspector.inspect_bytes(documents); export.provenance(author,inspection['entities'])
    files={'authoring.json':export.encoded(author),'documents/chapter.paths.md':documents['chapter.paths.md']}
    digest=export.sha(export.encoded({name:export.sha(b) for name,b in files.items()})); root=OUT/'families'/family/digest; source=root/'authoring'
    export.immutable_directory(source,files)
    checks={}
    for key,flag in (('route_checks','--question-batch'),('family_lesson_check','--family-lessons')):
        run=subprocess.run([str(model),flag,str(source/'documents')],capture_output=True,text=True,timeout=60)
        require(run.returncode==0,run.stderr or run.stdout); checks[key]=export.decoded(run.stdout)
    routes=checks['route_checks']; require(routes['routes']==18 and routes['wrong_choices']==42,'all eighteen routes and wrong choices required')
    certificates=[batch.reasoning_certificate(q,CHECKERS) for q in qs]; batch.verify_role_content(qs,routes['questions'],certificates)
    require(before==source_hashes(),'source changed during family verification')
    require(target_hash==export.sha(export.read_bytes(target,64*1024*1024)) and model_hash==export.sha(export.read_bytes(model,64*1024*1024)),'executable changed during verification')
    report=dict(accepted=True,contract_revision=2,family=family,questions=18,readings=1,decisions=21,wrong_choices=42,reading=v['reading_id'],authoring=str(source),published=False,
                source_sha256=before,target_sha256=target_hash,model_sha256=model_hash,source_and_executables_stable=True,mathematical_checks=certificates,**checks)
    receipt=export.encoded(report); evidence=root/'checks'/export.sha(receipt); export.immutable_directory(evidence,{'verification.json':receipt})
    return dict(family=family,authoring=str(source),verification=str(evidence/'verification.json'))


def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--target',type=Path,default=batch.ROOT/'b/sorter'); parser.add_argument('--model',type=Path,default=batch.ROOT/'b/paths_learning_document_tests'); args=parser.parse_args()
    recipe=export.decoded(export.read_bytes(ROOT/'recipe.json')); require(recipe['format']=='paths_calculus_production_recipe' and recipe['format_version']==1,'recipe format')
    pilot.packet(); before=source_hashes()
    candidates=[build_one(recipe,f,s,args.target,args.model) for f in FAMILIES for s in SETS]
    families=[build_family(recipe,f,args.target,args.model) for f in FAMILIES]
    require(before==source_hashes(),'source changed during complete generation')
    out=dict(format='paths_production_subject',format_version=1,wave='wave01',contract_revision=2,subject='calculus',accepted=True,published=False,candidates=candidates,families=families)
    (OUT/'production.json').write_bytes(export.encoded(out)); print(json.dumps(out,indent=2))


if __name__=='__main__':
    try: main()
    except (export.ExportError,ValueError,OSError,KeyError) as error:
        print(json.dumps({'accepted':False,'message':str(error)})); sys.exit(1)
