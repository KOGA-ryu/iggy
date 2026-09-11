#!/usr/bin/env python3
"""Build Wave 01 exact one-turn trigonometry candidates through existing owners."""
import argparse
from fractions import Fraction
import json
from pathlib import Path
import subprocess
import sys
import tempfile

import build_question_batch as batch
import check_authoring_pilot as pilot
import export_learning as export

ROOT = batch.ROOT
SOURCE = ROOT / 'content/authoring/production/wave01/trigonometry'
FAMILIES = ('sine_turn', 'cosine_turn')
SETS = ('teaching', 'practice', 'fresh_check')
ROLES = tuple(batch.EXERCISE_ROLES)
OUT = ROOT / 'build/production/wave01/trigonometry'


def recipe():
    data = export.decoded(export.read_bytes(SOURCE/'recipe.json'))
    require(data['version'] == 2 and data['sets'] == list(SETS), 'Invalid recipe version/sets')
    require(data['domain'] == dict(units='radians', lower='0', upper='2', include_lower=True, include_upper=False), 'Only radian [0,2pi) is supported')
    require([r['role'] for r in data['roles']] == list(ROLES), 'Invalid role metadata')
    return data


def source_hashes():
    names = ['recipe.json','DESIGN.md','generate.py','certificate_tests.py']
    names += [f'families/{f}/{n}.md.in' for f in FAMILIES for n in ('lesson','questions.paths')]
    return {n:export.sha(export.read_bytes(SOURCE/n)) for n in names}


def require(ok, message):
    export.require(ok, 'production.trigonometry', message)


def tex(value):
    return batch.tex(Fraction(value))


def pi(value):
    value = Fraction(value); sign = '-' if value < 0 else ''; value = abs(value)
    if value == 0: return '0'
    if value == 1: return sign + r'\pi'
    if value.denominator == 1: return sign + str(value.numerator) + r'\pi'
    if value.numerator == 1: return sign + rf'\frac{{\pi}}{{{value.denominator}}}'
    return sign + rf'\frac{{{value.numerator}\pi}}{{{value.denominator}}}'


def angle_set(values):
    return r'\left\{'+','.join(pi(v) for v in values)+r'\right\}' if values else r'\varnothing'


BRANCHES = {
    'sine_turn': {Fraction(1,2):(Fraction(1,6),Fraction(5,6)), Fraction(-1,2):(Fraction(7,6),Fraction(11,6)),
                  Fraction(0):(Fraction(0),Fraction(1)), Fraction(1):(Fraction(1,2),), Fraction(-1):(Fraction(3,2),)},
    'cosine_turn': {Fraction(1,2):(Fraction(1,3),Fraction(5,3)), Fraction(-1,2):(Fraction(2,3),Fraction(4,3)),
                    Fraction(0):(Fraction(1,2),Fraction(3,2)), Fraction(1):(Fraction(0),), Fraction(-1):(Fraction(1),)},
}
FUN = {'sine_turn': (r'\sin', 'sine', 'vertical coordinate', r'\pi-\alpha'),
       'cosine_turn': (r'\cos', 'cosine', 'horizontal coordinate', r'2\pi-\alpha')}


def render_equation(symbol, a, b, c):
    lead = ('' if a == 1 else '-' if a == -1 else str(a)) + symbol + r'\theta'
    offset = '' if b == 0 else ('+' if b > 0 else '') + str(b)
    return lead + offset + '=' + str(c)


def original(question):
    case = batch.case_fields(question, ('family','a','b','c','alpha_pi'))
    family, a, b, c = case['family'], case['a'], case['b'], case['c']
    require(family in FAMILIES and all(type(v) is int for v in (a,b,c)) and 0 < abs(a) <= 2 and abs(b) <= 2 and abs(c) <= 4,
            f"{question['id']}: invalid original equation")
    level = Fraction(c-b, a)
    require(level in BRANCHES[family], f"{question['id']}: unsupported exact level")
    require(type(case['alpha_pi']) is str and len(case['alpha_pi'])<=20, 'Known angle must be exact rational text')
    alpha = Fraction(case['alpha_pi'])
    branches = BRANCHES[family][level]
    require(alpha in branches, f"{question['id']}: alpha must be a genuine one-turn branch")
    return family, a, b, c, level, alpha, branches


def coordinate(family, value):
    """Independent exact rotations in Q(sqrt(3)); never invert BRANCHES."""
    n = Fraction(value) * 6
    require(n.denominator == 1, 'Only exact sixth-pi lattice angles are certified')
    def add(u,v): return (u[0]+v[0],u[1]+v[1])
    def neg(u): return (-u[0],-u[1])
    def mul(u,v): return (u[0]*v[0]+3*u[1]*v[1],u[0]*v[1]+u[1]*v[0])
    x,y=(Fraction(1),Fraction(0)),(Fraction(0),Fraction(0))
    co,si=(Fraction(0),Fraction(1,2)),(Fraction(1,2),Fraction(0))
    for _ in range(int(n)%12):
        x,y=add(mul(x,co),neg(mul(y,si))),add(mul(x,si),mul(y,co))
    return y if family=='sine_turn' else x


def membership(family,value):
    rational,radical=coordinate(family,value)
    require(radical == 0, 'Candidate coordinate is irrational, not a supported exact level')
    return rational


def verify_solutions(family,level,answers):
    require(level in (0,Fraction(1,2),Fraction(-1,2),1,-1), 'Unsupported level')
    require(len(answers)==len(set(answers)) and len(answers)==len({v%2 for v in answers}),
            'Duplicate or coterminal answer')
    require(all(0 <= v < 2 for v in answers), 'Answer violates half-open radian domain')
    require(all(coordinate(family,v)==(level,0) for v in answers), 'Angle fails exact coordinate substitution')
    # x²+y²=1 with one coordinate fixed leaves t²=1-level².
    # Two distinct real roots for |level|<1, one root at each extreme.
    require(len(answers)==(1 if abs(level)==1 else 2), 'Missing or additional unit-circle intersection')


def solutions(family, level):
    branches=BRANCHES[family][level]
    require(len(branches)==len(set(branches)), 'Duplicate branch seed')
    # Every canonical branch is in [0,2); only k=0 survives. Check adjacent
    # turns explicitly; monotonic shifts rule out all larger |k|.
    require(all(0 <= v < 2 for v in branches), 'Noncanonical branch seed')
    answers=sorted(v+2*k for v in branches for k in (-1,0,1) if 0 <= v+2*k < 2)
    verify_solutions(family,level,answers)
    return answers


def facts(family, level, answers):
    verify_solutions(family,level,answers)
    return {'level':str(level),
            'interval_solutions_theta_over_pi':[str(v) for v in answers],
            'exact_coordinate_checks':[[str(t) for t in coordinate(family,v)] for v in answers],
            'intersection_count':1 if abs(level)==1 else 2,
            'completeness':'Fixing a coordinate in x^2+y^2=1 leaves t^2=1-level^2: two points for |level|<1, one for |level|=1. Exact rotation verifies membership; [0,2pi) is bijective with directions.'}


def context(q):
    family,a,b,c,level,alpha,branches = original(q); symbol,name,coordinate,reflection = FUN[family]
    given = render_equation(symbol,a,b,c)
    answers = solutions(family,level); return family,a,b,c,level,alpha,branches,symbol,name,coordinate,reflection,given,answers


def read_notation(q):
    family,a,b,c,level,alpha,branches,symbol,name,coordinate,reflection,given,answers = context(q)
    answer = rf'{symbol}\theta={tex(level)}'
    choices = [answer, rf'{symbol}\theta={tex(-level)}', rf'{symbol}\theta={tex(level + Fraction(1,2))}']
    require(len(set(choices)) == 3, 'Notation choices must be distinct')
    return given,[answer],[(choices,answer)],dict(original_equation=given, isolated_level=str(level), coordinate=name, **facts(family,level,answers))


def worked_check(q):
    family,a,b,c,level,alpha,branches,symbol,name,coordinate,reflection,given,answers = context(q)
    require(len(answers) == 2, 'Worked check needs two distinct one-turn points')
    other = next(v for v in answers if v != alpha); choices=[pi(alpha),pi(other),pi((alpha+Fraction(1,2))%2)]
    require(len(set(choices)) == 3, 'Worked choices must be distinct')
    shown = given + rf',\quad\theta={pi(alpha)}'
    return shown,[rf'\theta\in{angle_set(answers)}'],[(choices,pi(other))],dict(known_angle=str(alpha), other_angle=str(other), **facts(family,level,answers))


def choose_next_step(q):
    family,a,b,c,level,alpha,branches,symbol,name,coordinate,reflection,given,answers = context(q)
    answer=rf'{symbol}\theta={tex(level)}'; valid_misses=render_equation(symbol,a,0,c-b); one_sided=rf'{symbol}\theta={tex(level + Fraction(1,2))}'
    choices=[answer,valid_misses,one_sided]
    require(len(set(choices)) == 3, 'Algebra choices must be distinct')
    return given,[answer],[(choices,answer)],dict(algebra=dict(a=a,b=b,c=c,derived_level=str(level)), valid_but_misses_goal=valid_misses, **facts(family,level,answers))


def explain_step(q):
    family,a,b,c,level,alpha,branches,symbol,name,coordinate,reflection,given,answers = context(q)
    reflected = (1-alpha) % 2 if family == 'sine_turn' else (2-alpha) % 2
    wrong1=(alpha+1)%2; wrong2=(2-alpha)%2 if family == 'sine_turn' else (1-alpha)%2
    require(membership(family,reflected)==level and membership(family,wrong1)==-level and membership(family,wrong2)==-level,
            'Reflection contrast must distinguish coordinate signs')
    choices=[r'\alpha+\pi', r'2\pi-\alpha' if family=='sine_turn' else r'\pi-\alpha', reflection]
    return (rf'\alpha={pi(alpha)},\quad{symbol}\alpha={tex(level)}',
            [rf'{symbol}({reflection})={symbol}(\alpha)={tex(level)}'], [(choices,reflection)],
            dict(reflected=str(reflected), wrong_coordinate_values=[str(membership(family,wrong1)),str(membership(family,wrong2))], reason=f'{name.capitalize()} is the {coordinate}; the stated reflection preserves that coordinate.', **facts(family,level,answers)))


def repair_error(q):
    family,a,b,c,level,alpha,branches,symbol,name,coordinate,reflection,given,answers = context(q)
    wrong = sorted(set(answers+[Fraction(2)])); require(Fraction(2) not in answers, 'Upper endpoint must be excluded')
    shown = rf'\begin{{gathered}}{given},\quad0\le\theta<2\pi\\\text{{Deliberately incorrect attempt:}}\\L_1:\ {symbol}\theta={tex(level)}\\L_2:\ \theta\in{angle_set(wrong)}\\L_3:\ \text{{all listed angles solve the interval problem}}\end{{gathered}}'
    omitted = answers[:-1]
    options=[angle_set(omitted),angle_set(wrong),angle_set(answers)]
    require(len(set(options))==3, 'Repair options must be distinct')
    return shown,[r'L_2',rf'\theta\in{angle_set(answers)}'],[(['L_2','L_1','L_3'],'L_2'),(options,angle_set(answers))],dict(first_error='L_2', excluded_endpoint='2', **facts(family,level,answers))


def independent(q):
    family,a,b,c,level,alpha,branches,symbol,name,coordinate,reflection,given,answers = context(q)
    correct=angle_set(answers); contrast = -level if level != 0 else Fraction(1,2)
    wrong_level=angle_set(solutions(family,contrast))
    missing=angle_set(answers[:-1])
    choices=[wrong_level,correct,missing]; require(len(set(choices))==3, 'Fresh choices must be distinct')
    residuals=[str(Fraction(a)*membership(family,x)+b-c) for x in answers]
    require(residuals == ['0']*len(answers), 'Every independent angle must check original equation')
    return given,[rf'\theta\in{correct}'],[(choices,correct)],dict(original_residuals=residuals, fresh_no_hint=True, **facts(family,level,answers))


CHECKERS={'read_notation':read_notation,'worked_check':worked_check,'choose_next_step':choose_next_step,'explain_step':explain_step,'repair_error':repair_error,'independent':independent}


def sequence(family, set_name):
    data=[]; r=recipe(); seed=r['families'][family][set_name]
    require(len(seed['levels'])==len(seed['offsets'])==len(seed['coefficient_signs'])==6, 'Recipe needs six cases')
    for index,(meta,raw) in enumerate(zip(r['roles'],seed['levels'])):
        role=meta['role']; level=Fraction(raw)
        require(level in BRANCHES[family], 'Recipe level outside finite contract')
        sign=seed['coefficient_signs'][index]; b=seed['offsets'][index]
        require(type(sign) is int and sign in (-1,1) and type(b) is int, 'Invalid recipe coefficients')
        a=(2 if level.denominator==2 else 1)*sign; c=a*level+b
        require(c.denominator==1, 'Nonintegral original constant')
        branch_index=seed['known_branch_indices'][index]
        require(type(branch_index) is int and 0 <= branch_index < len(solutions(family,level)), 'Invalid known branch index')
        case={'family':family,'a':a,'b':b,'c':int(c),'alpha_pi':str(solutions(family,level)[branch_index])}
        package=f'prod01_trigonometry_{family}_{set_name}'
        data.append({'id':package+'_'+role[:3]+'_'+export.sha(export.encoded(case))[:8],
                     **meta,'title':meta['title']+' · '+set_name.replace('_',' '),'case':case})
    result={'format':'paths_exercise_roles','format_version':1,'questions':data}
    batch.validate_role_sequence(result,SOURCE/'recipe.json')
    return result


def angle_plain(v):
    v=Fraction(v)
    if v==0: return '0'
    numerator=('-' if v<0 else '')+(str(abs(v.numerator)) if abs(v.numerator)!=1 else '')+'pi'
    return numerator if v.denominator==1 else numerator+'/'+str(v.denominator)


def set_plain(values):
    return '{'+', '.join(angle_plain(v) for v in values)+'}'


def coordinate_plain(family,v):
    r,s=coordinate(family,v)
    return str(r) if not s else f'({s})sqrt(3)'


def presentation(family,set_name):
    """Numeric/symbolic fields only; every teaching sentence lives in Markdown."""
    values={'subject':'trigonometry','subject_title':'Trigonometry','chapter':'topic_0032',
            'chapter_title':'Unit Circle Framework','reading_id':f'prod01_trigonometry_{family}_r',
            'reading_title':FUN[family][1].capitalize()+' equations in one turn'}
    positions=recipe()['families'][family][set_name]['first_positions']
    require(sorted(positions)==[0,0,1,1,2,2], 'Unbalanced first decision positions')
    for index,q in enumerate(sequence(family,set_name)['questions']):
        role=q['role']; f,a,b,c,level,alpha,branches,symbol,name,coord,reflection,given,answers=context(q)
        checked=batch.reasoning_certificate(q,CHECKERS)['expected']
        fields=dict(id=q['id'],title=q['title'],objective=q['objective'],given=checked['given'],
                    a=str(a),b=str(b),c=str(c),rhs=str(c-b),level=str(level),opposite=str(-level),
                    shifted=str(level+Fraction(1,2)),answers=set_plain(answers),count=str(len(answers)),
                    known=angle_plain(alpha),other=angle_plain(next((v for v in answers if v!=alpha),alpha)),omitted=angle_plain(answers[-1]),
                    wrong_angle=angle_plain((alpha+Fraction(1,2))%2),
                    wrong_coordinate=coordinate_plain(f,(alpha+Fraction(1,2))%2),
                    reflected=angle_plain(((1 if f=='sine_turn' else 2)-alpha)%2),
                    reflected_raw=angle_plain((1 if f=='sine_turn' else 2)-alpha),
                    reflection_shift=angle_plain((((1 if f=='sine_turn' else 2)-alpha)%2)-((1 if f=='sine_turn' else 2)-alpha)),
                    halfturn=angle_plain((alpha+1)%2),
                    other_reflection=angle_plain(((2 if f=='sine_turn' else 1)-alpha)%2),
                    contrast=str(-level if level else Fraction(1,2)),
                    contrast_lhs=str(a*(-level if level else Fraction(1,2))+b),
                    wrong_sign_lhs=str(a*(-level)+b),
                    shifted_lhs=str(a*(level+Fraction(1,2))+b),
                    endpoint_lhs=str(a*membership(f,Fraction(2))+b))
        for number,(after,step) in enumerate(zip(checked['after'],checked['steps']),1):
            options=[(10*number+i+1,label) for i,label in enumerate(step['choices'])]
            correct=next(o for o in options if o[1]==step['answer'])
            wrong=[o for o in options if o!=correct]
            wrong.sort(key=lambda o:export.sha(export.encoded([q['id'],number,o[0]])))
            wrong.insert(positions[index] if number==1 else (index+SETS.index(set_name))%3,correct)
            fields['choices_'+str(number)]='\n'.join(f'@choice {i} | {v}' for i,v in wrong)+f'\n@answer {correct[0]}'
            fields['after_'+str(number)]=after
        values.update({role+'_'+k:v for k,v in fields.items()})
    return values


def lesson(family,set_name=None,reading=None):
    return export.read_bytes(SOURCE/'families'/family/'lesson.md.in').decode()


def question_text(family,set_name,template=None):
    path=SOURCE/'families'/family/'questions.paths.md.in'
    return batch.fill_template(template if template is not None else export.read_bytes(path).decode(),
                               presentation(family,set_name),path)


def build_one(family,set_name,target,model):
    pilot.packet(); seq=sequence(family,set_name); package=f'prod01_trigonometry_{family}_{set_name}'
    reading=f'prod01_trigonometry_{family}_r'
    values=presentation(family,set_name)
    author={'format':'paths_learning_authoring','format_version':1,'package_id':package,'package_version':1,'sources':[{'id':package+'_source','kind':'generated','title':'Original exact '+FUN[family][1]+' equation set','uri':'paths:original/'+package+'/v1','revision':'1','attribution':'Original Paths prose, exact cases, distractors, feedback, and certificates. Definitions/conditions checked against OpenStax Precalculus 2e sections 5.1, 5.2, and 7.5 on 2026-09-10.','reuse':'Original authoring; no external exercise wording copied.','content_ids':[reading]+[q['id'] for q in seq['questions']]}]}
    inputs={name:export.read_bytes(SOURCE/name) for name in source_hashes()}; digest=export.sha(export.encoded({'family':family,'set':set_name,'inputs':{k:export.sha(v) for k,v in inputs.items()},'sequence':seq}))
    staged=ROOT/'build/production/wave01/trigonometry'/family/set_name/digest/'source'
    export.immutable_directory(staged,{'sequence.json':export.encoded(seq),'lesson.md.in':lesson(family,set_name,reading).encode(),'questions.paths.md.in':inputs[f'families/{family}/questions.paths.md.in'],'certificates.py':inputs['generate.py'],'authoring.json':export.encoded(author)})
    assignment={'subject':'trigonometry','folder':str(staged.relative_to(ROOT)),'package_id':package,'values':values}
    result=pilot.check_assignment(assignment,target,model)
    require({name:export.sha(data) for name,data in inputs.items()}=={name:export.sha(export.read_bytes(SOURCE/name)) for name in inputs},'Production source changed during verification')
    candidate = Path(result['authoring']).parent
    exact_check = candidate/'checks'/export.sha(export.encoded(result))/'verification.json'
    require(exact_check.is_file(), 'The exact shared check receipt is missing')
    return {'family':family,'set':set_name,'authoring':result['authoring'],'verification':str(exact_check),'result':result}


def build_all(target,model):
    before=source_hashes()
    ready=export.decoded(export.read_bytes(ROOT/'build/production/wave01/build-ready.json'))
    require(ready['status']=='ready', 'Shared build is not ready')
    require(all(export.sha(export.read_bytes(p,64*1024*1024))==ready['sha256'][p.name] for p in (target,model)), 'Shared executable hash mismatch')
    rows=[build_one(family,set_name,target,model) for family in FAMILIES for set_name in SETS]
    families=[build_family(family,target,model,rows) for family in FAMILIES]
    require(before==source_hashes(),'Source changed across complete build')
    receipt={'source_sha256':before,'format':'paths_production_subject','format_version':1,'contract_revision':2,'wave':'wave01','subject':'trigonometry','accepted':True,'published':False,'candidates':[{k:v for k,v in row.items() if k in ('family','set','authoring','verification')} for row in rows],'families':families}
    out=ROOT/'build/production/wave01/trigonometry/production.json'; out.parent.mkdir(parents=True,exist_ok=True); snapshot=OUT/'receipts'/export.sha(export.encoded(receipt))/'production.json'; export.immutable_directory(snapshot.parent,{'production.json':export.encoded(receipt)}); out.write_bytes(export.encoded(dict(receipt,immutable_receipt=str(snapshot)))); return dict(receipt,immutable_receipt=str(snapshot))


def build_family(family,target_path,model_path,staging_rows):
    """Canonical family package: one reading and the 18 questions from three sets."""
    package=f'prod01_trigonometry_{family}'; reading=package+'_r'
    before=source_hashes()
    questions=[q for set_name in SETS for q in sequence(family,set_name)['questions']]
    certificates=[batch.reasoning_certificate(q,CHECKERS) for q in questions]
    questions_text='\n\n'.join(question_text(family,s) for s in SETS)
    values=dict(subject='trigonometry',subject_title='Trigonometry',chapter='topic_0032',chapter_title='Unit Circle Framework',reading_id=reading,reading_title=FUN[family][1].capitalize()+' equations in one turn',lesson_blocks=lesson(family,'teaching',reading),practice_links=''.join('@practice '+q['id']+'\n' for q in questions),questions=questions_text)
    documents={'chapter.paths.md':batch.chapter_text(values).encode()}
    author={'format':'paths_learning_authoring','format_version':1,'package_id':package,'package_version':1,'sources':[{'id':package+'_source','kind':'generated','title':'Original exact '+FUN[family][1]+' equation family','uri':'paths:original/'+package+'/v1','revision':'1','attribution':'Original Paths reading, exercises, feedback, and Fraction certificates; conventions checked against OpenStax Precalculus 2e sections 5.1, 5.2, and 7.5 on 2026-09-10.','reuse':'Original material; no external exercise wording copied.','content_ids':[reading]+[q['id'] for q in questions]}]}
    source_inputs=[SOURCE/name for name in before]
    hashes=before
    target=export.Target(target_path); target_hash=target.fingerprint; model_hash=export.sha(export.read_bytes(model_path,64*1024*1024)); inspected=target.inspect_bytes(documents); export.provenance(author,inspected['entities'])
    with tempfile.TemporaryDirectory(prefix='paths-prod-trig-', dir='/private/tmp') as temporary:
        staged=Path(temporary); export.write_tree(staged,documents)
        replay=subprocess.run([str(model_path),'--question-batch',str(staged)],capture_output=True,text=True,timeout=60)
        require(replay.returncode==0,replay.stderr or replay.stdout); routes=export.decoded(replay.stdout)
        require(routes['accepted'] is True and routes['routes']==18 and routes['wrong_choices']==42
                and routes['save_replay'] is True,'Canonical family must replay 18 routes and 42 wrong choices/save replay')
        batch.verify_role_content(questions,routes['questions'],certificates)
        require(sum(len(q['question']['steps']) for q in routes['questions'])==21, 'Family must contain 21 decisions')
        positions={}
        for index,set_name in enumerate(SETS):
            subset=routes['questions'][index*6:index*6+6]
            stage=next(row for row in staging_rows if row['family']==family and row['set']==set_name)
            require(subset==stage['result']['route_checks']['questions'], 'Consolidation changed compiled question semantics')
            positions[set_name]=[next(i for i,o in enumerate(q['question']['steps'][0]['options'])
                                      if o['id'] in q['question']['steps'][0]['accepted_option_ids']) for q in subset]
            require(positions[set_name]==recipe()['families'][family][set_name]['first_positions']
                    and sorted(positions[set_name])==[0,0,1,1,2,2], 'Compiled first-position balance changed')
        disclosure=subprocess.run([str(model_path),'--family-lessons',str(staged)],capture_output=True,text=True,timeout=60)
        require(disclosure.returncode==0,disclosure.stderr or disclosure.stdout)
    require(hashes=={str(path.relative_to(SOURCE)):export.sha(export.read_bytes(path)) for path in source_inputs},'Family source changed during verification')
    require(target_hash==export.sha(export.read_bytes(target_path,64*1024*1024)) and model_hash==export.sha(export.read_bytes(model_path,64*1024*1024)),'Checked executable changed during verification')
    payload={'authoring.json':export.encoded(author),**{'documents/'+name:data for name,data in documents.items()}}
    digest=export.sha(export.encoded({name:export.sha(data) for name,data in payload.items()}))
    root=ROOT/'build/production/wave01/trigonometry/families'/family/digest
    export.immutable_directory(root/'authoring',payload)
    report={'accepted':True,'family':family,'package_id':package,'questions':18,'readings':1,'decisions':21,
            'wrong_choices':42,'first_positions':positions,'staging_compiled_questions_unchanged':True,
            'authoring':str(root/'authoring'),'route_checks':routes,'family_lesson_check':export.decoded(disclosure.stdout),
            'mathematical_checks':certificates,'source_sha256':hashes,
            'source_sha256_before':hashes,'source_sha256_after':source_hashes(),'target_sha256':target_hash,
            'model_sha256':model_hash,'publication':'not_performed'}
    encoded=export.encoded(report); check=root/'checks'/export.sha(encoded)/'verification.json'; export.immutable_directory(check.parent,{'verification.json':encoded})
    return {'family':family,'authoring':str(root/'authoring'),'verification':str(check)}


def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--target',type=Path,default=ROOT/'b/sorter'); parser.add_argument('--model',type=Path,default=ROOT/'b/paths_learning_document_tests'); args=parser.parse_args()
    try: print(json.dumps(build_all(args.target.resolve(),args.model.resolve()),indent=2)); return 0
    except (export.ExportError,ValueError,OSError,KeyError,TypeError) as error: print(json.dumps({'accepted':False,'code':getattr(error,'code','production.invalid'),'message':str(error)},indent=2)); return 1
if __name__=='__main__': sys.exit(main())
