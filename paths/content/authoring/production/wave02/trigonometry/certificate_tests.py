"""Exact subject certificates for twelve complete routes; consumes compiler JSON."""
import argparse
import copy
from fractions import Fraction as F
import importlib.util
import json
import math
from pathlib import Path
import subprocess
import tempfile

import build_question_batch as batch
import export_learning as export

ROOT=batch.ROOT
SOURCE=ROOT/'content/authoring/production/wave02/trigonometry'
OUT=ROOT/'build/production/wave02/trigonometry'
DOCUMENTS=SOURCE/'authoring/documents'
HELPER=ROOT/'content/authoring/production/wave01/trigonometry/generate.py'
spec=importlib.util.spec_from_file_location('frozen_trig_math',HELPER)
w1=importlib.util.module_from_spec(spec); spec.loader.exec_module(w1)
PID='prod02_trig_full_equations'
READING=PID+'_r'


def need(ok,message):
    export.require(ok,'depth.trigonometry',message)


def read(path):
    return export.decoded(export.read_bytes(path))


def source_hashes():
    names=['DESIGN.md','cases.json','certificate_tests.py','authoring/authoring.json','authoring/documents/chapter.paths.md']
    return {n:export.sha(export.read_bytes(SOURCE/n)) for n in names}


def pi(v): return w1.pi(F(v))
def tex(v): return batch.tex(F(v))
def angles(v): return w1.angle_set(v)


def coordinate(fn,t):
    return w1.coordinate({'sin':'sine_turn','cos':'cosine_turn'}[fn],t)


def periodic(var,values,period=F(2)):
    if not values: return var+r'\in\varnothing'
    suffix=pi(period).replace(r'\pi',r'n\pi')
    return r',\quad\text{or}\quad '.join(var+'='+pi(v)+'+'+suffix for v in values)+r',\quad n\in\mathbb{Z}'


def plain(v): return w1.angle_plain(v)


def original(q):
    c=q['original']; need(set(c)=={'function','a','b','c','k','phi_pi'},'Unexpected original input fields')
    fn=c['function']; a,b,rhs,k=[c[n] for n in ('a','b','c','k')]
    need(fn in ('sin','cos') and all(type(v) is int for v in (a,b,rhs,k)), 'Original coefficients must be integers')
    need(0<abs(a)<=3 and abs(b)<=4 and abs(rhs)<=4 and k in (1,2), 'Original outside declared coefficient bounds')
    need(type(c['phi_pi']) is str and c['phi_pi'] in ('0','1/2','-1/2'),'Unsupported radian phase')
    p=F(c['phi_pi']); h=F(rhs-b,a)
    need(h in (0,F(1,2),F(-1,2),1,-1) or abs(h)>1,'Unsupported normalized coordinate')
    d=q['domain']
    expected={'kind':'general','units':'radians','integer_parameter':'n'} if d.get('kind')=='general' else {
        'kind':'interval','units':'radians','lower_pi':'0','upper_pi':'2','lower_included':True,'upper_included':False}
    need(d==expected and (d['kind']=='general' or (d['lower_included'] is True and d['upper_included'] is False)),
         'Only exact radian interval or integer-parameter general domain is supported')
    general=d['kind']=='general'
    stages=['method','isolate','range','branches','general_theta','verify'] if general else (
        ['isolate','range','empty'] if abs(h)>1 else
        ['isolate','range','branches','filter','verify'] if k==1 and p==0 else
        ['isolate','range','branches','bounds','filter','theta','verify'])
    need(q['local_goals']==stages,'Local goals must carry the whole original problem through')
    return fn,a,b,rhs,k,p,h,general,stages


def mathematics(q):
    fn,a,b,c,k,p,h,general,stages=original(q)
    # Exact finite value table via the existing independent pi/6 rotation helper.
    # This is not the authored branch list or a stored expected answer.
    lattice={F(n,6):coordinate(fn,F(n,6)) for n in range(12)}
    branches=[t for t,value in lattice.items() if value==(h,0)]
    count=0 if abs(h)>1 else 1 if abs(h)==1 else 2
    need(len(branches)==count, 'Circle intersection count and exact values disagree')
    lo,hi=p,2*k+p
    integers={b:list(range(math.ceil((lo-b)/2),math.ceil((hi-b)/2))) for b in branches}
    args=sorted(b+2*n for b in branches for n in integers[b])
    theta=[(u-p)/k for u in args]
    # A separate direct theta lattice enumeration checks transformed bounds.
    direct=[F(n,6*k) for n in range(12*k) if coordinate(fn,k*F(n,6*k)+p)==(h,0)]
    need(theta==direct and len(theta)==k*count,'Argument filtering disagrees with original theta-domain enumeration')
    need(len(theta)==len(set(theta)) and all(0<=t<2 for t in theta),'Endpoint or duplicate theta failure')
    need(all(a*coordinate(fn,k*t+p)[0]+b==c and coordinate(fn,k*t+p)[1]==0 for t in theta),'Original substitution failed')
    bases=[(b-p)/k for b in branches]; period=F(2,k)
    need(len({v%period for v in bases})==len(bases),'General branches overlap modulo their period')
    need(all(k*v+p==b for v,b in zip(bases,branches)) and k*period==2,'General inverse/period proof failed')
    return dict(branches=branches,count=count,lower=lo,upper=hi,argument_integers=integers,
                arguments=args,theta=theta,bases=bases,period=period,
                proof='The fixed-coordinate circle equation proves N intersections; positive k maps the domain bijectively to k turns. General k*(base+n*T)+phi=branch+2n*pi holds for every integer n.')


def expected(q):
    fn,a,b,c,k,p,h,general,stages=original(q); m=mathematics(q)
    arg=(r'\theta' if k==1 else r'2\theta')+('' if not p else ('+' if p>0 else '')+pi(p))
    eq=str(a)+'\\'+fn+r'\left('+arg+r'\right)'+('' if not b else ('+' if b>0 else '')+str(b))+'='+str(c)
    given=eq+(r',\quad\theta\in\mathbb{R}' if general else r',\quad0\le\theta<2\pi')
    ap=('theta' if k==1 else '2theta')+('' if not p else ('+' if p>0 else '')+plain(p))
    goal='Find every real solution with an integer parameter and justify the complete result.' if general else 'Find every solution in the stated interval and check the original equation and completeness.'
    domain=(f'Theta is real and measured in radians. Find all real solutions, with n ranging over all integers. Write u={ap} for the argument. Alpha in a method denotes any known solution argument, not necessarily acute. N counts matching points in one argument turn; M counts branches in one theta period.' if general else
            f'Theta is real and measured in radians. Find every solution in [0,2pi). Write u={ap} for the argument. N counts matching points in one argument turn; M counts solutions in the requested theta interval.')
    branch,N,lo,hi,args,theta,bases,T=[m[n] for n in ('branches','count','lower','upper','arguments','theta','bases','period')]
    outputs=[]
    for stage in stages:
        if stage=='method':
            sine=r'u=\alpha+2n\pi\ \text{or}\ u=\pi-\alpha+2n\pi'
            cosine=r'u=\alpha+2n\pi\ \text{or}\ u=-\alpha+2n\pi'
            correct=sine if fn=='sin' else cosine
            choices=[correct,cosine if fn=='sin' else sine,correct.replace(r'2n\pi',r'n\pi')]
            # At both actual nonzero half-coordinate branches the other
            # reflection and odd half-turn fail; only correct reflection holds.
            need(all(coordinate(fn,(1-t if fn=='sin' else -t))==(h,0) for t in branch),'Reflection proof failed')
            need(all(coordinate(fn,(-t if fn=='sin' else 1-t))!= (h,0) and coordinate(fn,t+1)!=(h,0) for t in branch),'Method distractor accidentally valid')
        elif stage=='isolate':
            values=(h,F(1 if h>0 else -1),F(0)) if abs(h)>1 else (h,h+F(1,2),h-F(1,2))
            choices=['\\'+fn+' u='+tex(v) for v in values]
            need([a*v+b==c for v in values]==[True,False,False],'Isolation choice equivalence')
        elif stage=='range':
            if abs(h)>1:
                choices=[tex(h)+(r'>1' if h>1 else r'<-1')+r',\quad N=0',
                         tex(h)+r'\in[-1,1],\quad N=2',tex(h)+r'\in[-1,1],\quad N=1']
            else:
                choices=[tex(h)+r'\in[-1,1],\quad N='+str(N),
                         tex(h)+r'\notin[-1,1],\quad N=0',
                         tex(h)+r'\in[-1,1],\quad N='+str(2 if N==1 else 1)]
        elif stage=='branches':
            choices=[periodic('u',branch),periodic('u',branch[:-1]),periodic('u',[v+F(1,6) for v in branch])]
            need(len(branch[:-1])<N and coordinate(fn,branch[0]+F(1,6))!=(h,0),'Branch distractor validity')
        elif stage=='bounds':
            choices=[pi(l)+r'\le u<'+pi(u) for l,u in ((lo,hi),(lo+F(1,2),hi),(lo,hi+1))]
        elif stage=='filter':
            v=r'\theta' if k==1 and p==0 else 'u'
            choices=[v+r'\in'+angles(vs) for vs in (args,args[1:],args+[hi])]
            need(args[1:]!=args and not lo<=hi<hi,'Filter distractor/domain logic')
        elif stage=='theta':
            choices=[r'\theta\in'+angles(vs) for vs in (theta,args,[t+F(1,12) for t in theta])]
            need(theta!=args and all(k*t+p==u for t,u in zip(theta,args)),'Inverse argument decision')
        elif stage=='general_theta':
            choices=[periodic(r'\theta',bases,T),periodic(r'\theta',[v/k for v in branch],T),periodic(r'\theta',bases,F(2))]
            need(p!=0 and k==2 and T==1,'General distractors require nonzero phase and frequency two')
            need(any(coordinate(fn,v+p)!=(h,0) for v in branch),'Omitted phase accidentally preserves all branches')
            # A correct branch advanced by T cannot be in either incorrectly
            # 2pi-spaced branch, so the wrong period loses real solutions.
            need(any(all((v+T-w)%2!=0 for w in bases) for v in bases),'Wrong-period distractor is complete')
        elif stage=='verify':
            count=N if general else len(theta)
            subst=f'({a})({tex(h)})+({b})={c}'
            choices=[subst+r',\quad M='+str(count),f'({a})({tex(h)})+({b})={c+1}'+r',\quad M='+str(count),
                     subst+r',\quad M='+str(count+1)]
        else:
            need(abs(h)>1,'Empty conclusion needs a range contradiction')
            choices=[r'S=\varnothing',r'S=\mathbb{R}',r'S=\left\{0\right\}']
        need(len(set(choices))==3,'Equivalent/repeated authored goal choices')
        after=('u='+arg+r',\quad'+choices[0]) if stage=='isolate' else choices[0]
        outputs.append(dict(stage=stage,choices=choices,after=after))
    return given,goal+' '+domain,outputs,m


def verify(cases,routes):
    need(routes['accepted'] is True and routes['routes']==12 and routes['save_replay'] is True,'Replay must cover twelve complete routes and saves')
    compiled=routes['questions']; need([q['id'] for q in compiled]==[q['id'] for q in cases],'Question identities/order differ')
    evidence=[]; positions=[]
    for i,(q,row) in enumerate(zip(cases,compiled)):
        given,description,steps,maths=expected(q); actual=row['question']
        need(actual['id']==q['id'] and actual['content_version']==1 and 'support' not in actual,'Unexpected identity/support schema')
        need(actual['equation']==given and actual['description']==description,q['id']+': compiled original/domain mismatch')
        need([v['display'] for v in actual['working_states']]==[given]+[s['after'] for s in steps],q['id']+': false intermediate working')
        need(len(actual['steps'])==len(steps),'Changed local route length')
        for j,(step,wanted) in enumerate(zip(actual['steps'],steps),1):
            by_id={o['id']:o for o in step['options']}; ids=[j*10+n for n in (1,2,3)]
            need(len(step['options'])==len(by_id)==3 and set(by_id)==set(ids),'Unexpected option identities')
            need([by_id[n]['label'] for n in ids]==wanted['choices'],q['id']+': false/equivalent option')
            need(step['accepted_option_ids']==[ids[0]],q['id']+': false accepted key')
            need(all(by_id[n].get('wrong_feedback','').strip() for n in ids[1:]) and not by_id[ids[0]].get('wrong_feedback'),'Missing/misassigned individual correction')
            need(bool(step['prompt'].strip()) and bool(step['explanation'].strip()),'Missing prompt or reached explanation')
            if j==1:
                position=next(i for i,o in enumerate(step['options']) if o['id']==ids[0])
                need(position==q['first_position'],'First position changed')
                positions.append(position)
        evidence.append(dict(id=q['id'],steps=len(steps),wrong_choices=2*len(steps),
                             normalized_value=str(F(q['original']['c']-q['original']['b'],q['original']['a'])),
                             argument_branches=[str(v) for v in maths['branches']],
                             transformed_bounds=[str(maths['lower']),str(maths['upper'])],
                             theta_interval=[str(v) for v in maths['theta']],
                             general_bases=[str(v) for v in maths['bases']],period=str(maths['period']),
                             proof=maths['proof']))
    need(sorted(positions)==[0]*4+[1]*4+[2]*4,'Unbalanced first positions')
    need(sum(r['steps'] for r in evidence)==66 and routes['wrong_choices']==132,'Whole-route counts differ')
    return evidence


def reject(fn):
    try: fn()
    except (export.ExportError,ValueError,KeyError,TypeError): return True
    raise AssertionError('Deliberate mutation was accepted')


def command(binary,mode,folder):
    r=subprocess.run([str(binary),mode,str(folder)],capture_output=True,text=True,timeout=60)
    need(r.returncode==0,r.stderr or r.stdout)
    return export.decoded(r.stdout)


def mutations(cases,routes,model):
    results={'false_keys':0,'false_working':0,'equivalent_duplicates':0,'invalid_cases':0}
    for i,row in enumerate(routes['questions']):
        for j in range(len(row['question']['steps'])):
            bad=copy.deepcopy(routes)
            bad['questions'][i]['question']['steps'][j]['accepted_option_ids']=[10*(j+1)+2]
            reject(lambda:verify(cases,bad));results['false_keys']+=1
            bad=copy.deepcopy(routes);bad['questions'][i]['question']['working_states'][j+1]['display']=r'\theta=99\pi'
            reject(lambda:verify(cases,bad));results['false_working']+=1
        bad=copy.deepcopy(routes);step=bad['questions'][i]['question']['steps'][0]
        good=next(o for o in step['options'] if o['id']==11)
        wrong=next(o for o in step['options'] if o['id']==12)
        wrong['label']=good['label']+r'\,' # same mathematical value with extra spacing
        reject(lambda:verify(cases,bad));results['equivalent_duplicates']+=1
        for field,value in [('a',0),('a',True),('k',3),('phi_pi','1/3')]:
            bad=copy.deepcopy(cases);bad[i]['original'][field]=value
            reject(lambda:verify(bad,routes));results['invalid_cases']+=1
        bad=copy.deepcopy(cases);bad[i]['domain']['units']='degrees'
        reject(lambda:verify(bad,routes));results['invalid_cases']+=1
        bad=copy.deepcopy(cases);bad[i]['original'].update(a=3,b=0,c=1)
        reject(lambda:verify(bad,routes));results['invalid_cases']+=1
        if cases[i]['domain']['kind']=='interval':
            for field,value in [('lower_included',False),('upper_included',True),('upper_pi','3')]:
                bad=copy.deepcopy(cases);bad[i]['domain'][field]=value
                reject(lambda:verify(bad,routes));results['invalid_cases']+=1
        else:
            bad=copy.deepcopy(cases);bad[i]['domain']['integer_parameter']='positive n'
            reject(lambda:verify(bad,routes));results['invalid_cases']+=1
    raw=export.read_bytes(DOCUMENTS/'chapter.paths.md').decode()
    first=routes['questions'][0]['question']['steps'][0]
    changes=[('prompt','@step 10 | '+first['prompt'],'@step 10 | Recheck the coordinate: '+first['prompt']),
             ('wrong_feedback','@feedback 12 | '+next(o for o in first['options'] if o['id']==12)['wrong_feedback'],
              '@feedback 12 | Recheck this specific choice. '+next(o for o in first['options'] if o['id']==12)['wrong_feedback'])]
    edits=[]
    for field,old,new in changes:
        need(old in raw,'Real Markdown edit target missing')
        with tempfile.TemporaryDirectory(prefix='trig-depth-edit-',dir=OUT) as temp:
            path=Path(temp).resolve()
            export.write_tree(path,{'chapter.paths.md':raw.replace(old,new,1).encode()})
            changed=command(model,'--question-batch',path)
            verify(cases,changed)
            expected_payload=copy.deepcopy(routes)
            expected_step=expected_payload['questions'][0]['question']['steps'][0]
            if field=='prompt': expected_step['prompt']='Recheck the coordinate: '+first['prompt']
            else: next(o for o in expected_step['options'] if o['id']==12)['wrong_feedback']=new.split(' | ',1)[1]
            need(changed['questions']==expected_payload['questions'],'Markdown edit did not change only its intended compiled field')
            edits.append(dict(field=field,source_before=export.sha(raw.encode()),
                              source_after=export.sha(export.read_bytes(path/'chapter.paths.md')),compiled_change=True,math_unchanged=True))
    results['real_markdown_edits']=edits
    return results


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--routes',type=Path,required=True);args=parser.parse_args()
    before=source_hashes(); helper_hash=export.sha(export.read_bytes(HELPER))
    ready=read(ROOT/'build/production/wave01/build-ready.json'); target_path=ROOT/'b/sorter';model=ROOT/'b/paths_learning_document_tests'
    need(ready['status']=='ready','Shared build not ready')
    target_hash=export.sha(export.read_bytes(target_path,64*1024*1024));model_hash=export.sha(export.read_bytes(model,64*1024*1024))
    need(target_hash==ready['sha256']['sorter'] and model_hash==ready['sha256']['paths_learning_document_tests'],'Release executable mismatch')
    manifest=read(SOURCE/'cases.json');cases=manifest['cases']
    need(manifest['format']=='paths_trigonometry_depth_cases' and manifest['version']==1,'Unsupported finite case manifest')
    need([q['id'] for q in cases]==[PID+f'_q{i:02d}' for i in range(1,13)],'Reserved identities differ')
    need([q['group'] for q in cases]==['introductory']*4+['practice']*4+['mixed']*4,'Coverage group order differs')
    need(sum(q['domain']['kind']=='general' for q in cases)==2 and sum(q['original']['function']=='sin' for q in cases)==6,'Subject coverage differs')
    routes=read(args.routes)
    fresh=command(model,'--question-batch',DOCUMENTS)
    need(routes==fresh,'Routes file is stale relative to actual Markdown input')
    maths=verify(cases,routes);negative=mutations(cases,routes,model)
    inspection=export.Target(target_path).inspect(documents=DOCUMENTS)
    export.provenance(read(SOURCE/'authoring/authoring.json'),inspection['entities'])
    need(inspection==read(OUT/'inspection.json'),'Inspection file stale')
    lessons=command(model,'--family-lessons',DOCUMENTS)
    need(lessons==read(OUT/'lessons.json') and lessons['questions']==12 and lessons['readings']==1,'Canonical lesson gate differs')
    need({e['id'] for e in inspection['entities'] if e['kind']=='lesson'}=={READING},'Unexpected reading identity')
    need(all(e.get('links')==[READING] for e in inspection['entities'] if e['kind']=='question'),'Reading links differ')
    need(before==source_hashes() and helper_hash==export.sha(export.read_bytes(HELPER)),'Source changed during evidence')
    need(target_hash==export.sha(export.read_bytes(target_path,64*1024*1024)) and model_hash==export.sha(export.read_bytes(model,64*1024*1024)),'Executables changed')
    evidence=dict(accepted=True,cases=maths,negative_tests=negative,source_sha256_before=before,source_sha256_after=source_hashes(),
                  reused_helper=dict(path=str(HELPER),sha256=helper_hash),original_givens_and_domain=True,
                  every_choice_key_and_working=True,original_substitution_and_completeness=True)
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'mathematics.json').write_bytes(export.encoded(evidence))
    production=dict(format='paths_depth_subject',format_version=1,wave='wave02',subject='trigonometry',
                    stage='ready_for_coordinator_review',published=False,source=str(SOURCE/'authoring'),
                    package_id=PID,reading_id=READING,question_ids=[q['id'] for q in cases],questions=12,steps=66,wrong_choices=132,
                    source_sha256=before,target_sha256=target_hash,model_sha256=model_hash,
                    inspection=str(OUT/'inspection.json'),routes=str(args.routes.resolve()),lessons=str(OUT/'lessons.json'),
                    mathematical_evidence=str(OUT/'mathematics.json'),
                    evidence_sha256={n:export.sha(export.read_bytes(OUT/n)) for n in ('inspection.json','routes.json','lessons.json','mathematics.json')},
                    remaining_concerns=['Coordinator mathematical/prose and aggregate/save review pending.','No native visual or learner-outcome observations.'])
    (OUT/'production.json').write_bytes(export.encoded(production))
    print(json.dumps(dict(stage=production['stage'],questions=12,steps=66,wrong_choices=132,
                         negative_tests=negative,production=str(OUT/'production.json')),indent=2))


if __name__=='__main__':
    main()
