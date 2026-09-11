"""Finite source-assisted equation certificates; reuse frozen exact math, never keys."""
import argparse
import copy
from datetime import datetime, timezone
from fractions import Fraction as F
import importlib.util
import json
from pathlib import Path
import subprocess

import export_learning as export
import build_question_batch as batch

SOURCE = Path(__file__).resolve().parent
ROOT = SOURCE.parents[4]
OUT = ROOT/'build/production/wave04/trigonometry'
AUTHOR = SOURCE/'authoring'
PACKAGE = 'prod04_trig_identity_equations'
MODEL = ROOT/'b/paths_learning_document_tests'
TARGET = ROOT/'b/sorter'
W3 = ROOT/'content/authoring/production/wave03/trigonometry/certificate_tests.py'
W1 = ROOT/'content/authoring/production/wave01/trigonometry/generate.py'
REGISTRY = SOURCE.parent/'SOURCES.json'


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec); spec.loader.exec_module(result)
    return result


ident = module('frozen_identity_math', W3)
circle = module('frozen_circle_math', W1)
need = ident.need
I = r'0\le\theta<2\pi'
DOMAIN = dict(units='radians', lower='0', upper='2', include_lower=True, include_upper=False)


def validate(x, depth=0, nodes=None):
    nodes = [0] if nodes is None else nodes; nodes[0] += 1
    need(depth <= 10 and nodes[0] <= 96, 'Finite AST depth/node bound')
    if type(x) is int:
        need(abs(x) <= 12, 'Coefficient outside [-12,12]'); return
    if isinstance(x, str):
        need(x in ('s','c','sin','cos','tan','cot','sec','csc'), 'Unsupported argument/function'); return
    need(isinstance(x,list) and len(x)==3 and x[0] in ('add','sub','mul','div','pow'), 'Invalid finite expression')
    validate(x[1], depth+1, nodes)
    if x[0]=='pow': need(type(x[2]) is int and x[2]==2, 'Only squares supported')
    else: validate(x[2],depth+1,nodes)


def diff(eq): return ['sub', eq['lhs'], eq['rhs']]
def eqtex(eq): return ident.tex(eq['lhs'])+'='+ident.tex(eq['rhs'])
def domaintex(case): return 'D:'+ident.DTEX[case['domain']]+r',\quad '+I
def branchkey(items):
    need(all(v in ('s','c') for v,h in items), 'Only coordinate branches supported')
    values = [(v,F(h)) for v,h in items]
    need(len(values)==len(set(values)), 'Repeated coordinate branch')
    return frozenset(values)


def display(option):
    kind = option['kind']
    if kind=='strategy':
        action=option['action']
        if action=='factor_difference': return r'\text{factor }'+ident.tex(option['expression'])
        if action=='rewrite_reciprocal': return r'\sec\theta=\frac{1}{c}'
        if action=='divide_coordinate': return r'\text{divide by }'+option['coordinate']
        if action=='isolate_then_square': return eqtex(option['result'])
        raise ValueError('Unsupported finite strategy')
    if kind=='equation': return eqtex(option)
    if kind=='domain': return ident.DTEX[option['value']]
    if kind=='branches':
        return (r',\quad\text{or}\quad '.join(v+'='+batch.tex(F(h)) for v,h in option['items']) if option['items'] else r'\varnothing')
    if kind=='angles': return 'S='+circle.angle_set(list(map(F,option['values'])))
    if kind=='check': return 'N='+str(option['N'])+r',\quad R='+str(option['R'])
    raise ValueError('Unsupported finite option kind')


def linear(factor):
    need(ident.poly(factor), 'Factor is not polynomial')
    n,d,excluded = ident.value(factor)
    need(d==ident.ONE and not excluded and set(n) <= {(0,0),(1,0),(0,1)}, 'Factor is not affine')
    s,c = n.get((1,0),F(0)), n.get((0,1),F(0))
    need(bool(s) != bool(c), 'Factor must use exactly one coordinate with nonzero coefficient')
    return ('s',s,n.get((0,0),F(0))) if s else ('c',c,n.get((0,0),F(0)))


def only_variable(x, variable):
    return type(x) is int or x==variable or (isinstance(x,list) and only_variable(x[1],variable) and (x[0]=='pow' or only_variable(x[2],variable)))


def goal_form(local, option):
    if option['kind']!='equation': return True
    lhs,rhs = option['lhs'],option['rhs']; form=local['form']
    if form=='expanded': return rhs==0 and ident.expanded(lhs)
    if form=='factored':
        if rhs!=0 or not ident.isop(lhs,'mul'): return False
        try: linear(lhs[1]); linear(lhs[2]); return True
        except ValueError: return False
    if form=='reciprocal_rewrite': return ident.only_coordinates(lhs) and ident.only_coordinates(rhs) and (not ident.poly(lhs) or not ident.poly(rhs))
    if form=='identity_rewrite': return only_variable(lhs,local['variable']) and only_variable(rhs,local['variable'])
    raise ValueError('Unsupported equation goal '+form)


def pairadd(a,b): return a[0]+b[0],a[1]+b[1]
def pairmul(a,b): return a[0]*b[0]+3*a[1]*b[1],a[0]*b[1]+a[1]*b[0]
def pairdiv(a,b):
    den=b[0]*b[0]-3*b[1]*b[1]; need(den!=0,'Invalid reciprocal at proposed angle')
    return pairmul(a,(b[0]/den,-b[1]/den))


def at(x,angle):
    if type(x) is int: return F(x),F(0)
    if isinstance(x,str):
        s=circle.coordinate('sine_turn',angle); c=circle.coordinate('cosine_turn',angle); one=(F(1),F(0))
        if x in ('s','sin'): return s
        if x in ('c','cos'): return c
        return {'tan':lambda:pairdiv(s,c),'cot':lambda:pairdiv(c,s),'sec':lambda:pairdiv(one,c),'csc':lambda:pairdiv(one,s)}[x]()
    op,a,b=x; u=at(a,angle)
    if op=='pow': return pairmul(u,u)
    v=at(b,angle)
    if op=='add': return pairadd(u,v)
    if op=='sub': return pairadd(u,(-v[0],-v[1]))
    return pairmul(u,v) if op=='mul' else pairdiv(u,v)


def certificate(case):
    need(case['interval']==DOMAIN, 'Only exact radian [0,2pi) is supported')
    validate(case['lhs']); validate(case['rhs']); validate(case['multiplier'])
    original=diff(case); excluded=ident.value(original)[2]
    need(excluded==ident.DOMAINS[case['domain']], 'Wrong original reciprocal domain')
    n,d,exc=ident.value(case['multiplier'])
    need(not (ident.zeros(n)|exc)-excluded, 'Clearing multiplier may vanish on original domain')
    factors=case['factors']; need(len(factors)==2,'Two-factor witness required')
    for f in factors: validate(f)
    product=['mul',*factors]
    need(ident.equal(product,['mul',case['multiplier'],original]), 'Factor witness does not expand to original-derived polynomial')
    branches=branchkey([(v,str(-b/a)) for v,a,b in map(linear,factors)])
    allowed=frozenset((v,h) for v,h in branches if abs(h)<=1)
    representatives=[]; branch_evidence=[]
    for variable,h in sorted(allowed):
        need(h in (0,F(1,2),F(-1,2),1,-1),'Unsupported exact coordinate branch')
        family='sine_turn' if variable=='s' else 'cosine_turn'
        proposed=list(circle.BRANCHES[family][h])
        # A proposal becomes a proof only after exact membership AND intersection count.
        circle.verify_solutions(family,h,proposed)
        representatives.extend(proposed)
        branch_evidence.append({'coordinate':variable,'value':str(h),'representatives':[str(a) for a in proposed],
                                'complete_circle_intersections':1 if abs(h)==1 else 2})
    answers=sorted(a for a in set(representatives) if not (a*2).denominator==1 or int(a*2)%4 not in excluded)
    checks=[]
    for angle in answers:
        need(0<=angle<2,'Interval failure')
        left,right=at(case['lhs'],angle),at(case['rhs'],angle)
        need(left==right,'Exact original substitution failed')
        checks.append({'theta_over_pi':str(angle),'original_left':[str(x) for x in left],'original_right':[str(x) for x in right]})
    return {'branches':branches,'allowed':allowed,'answers':answers,'excluded':excluded,
            'branch_evidence':branch_evidence,'original_checks':checks}


def strategy_valid(case,option,cert):
    """Two bounded method-choice slots: prove reversibility or an exact counterexample."""
    action=option['action']; result=option['result']
    validate(result['lhs']); validate(result['rhs'])
    if action=='factor_difference':
        return (ident.equal(option['expression'],diff(case)) and
                ident.equal(diff(result),diff(case)) and goal_form({'form':'factored'},result))
    if action=='rewrite_reciprocal':
        return (ident.equal(diff(result),diff(case)) and goal_form({'form':'reciprocal_rewrite'},result)
                and ident.value(diff(result))[2]==cert['excluded'])
    theta=F(option['counterexample_theta_over_pi'])
    need(0<=theta<2,'Strategy witness outside interval')
    if action=='divide_coordinate':
        coordinate=option['coordinate']; need(coordinate in ('s','c'),'Unsupported divisor')
        need(ident.equal(['mul',coordinate,diff(result)],diff(case)),'Incorrect division consequence')
        need(theta in cert['answers'] and at(coordinate,theta)==(0,0),'Division witness must be a lost original zero-factor solution')
        return False
    if action=='isolate_then_square':
        before=option['before']
        need(ident.equal(diff(before),diff(case)),'Squaring setup changed original equation')
        need(ident.equal(result['lhs'],['pow',before['lhs'],2]) and ident.equal(result['rhs'],['pow',before['rhs'],2]),'Incorrect squared consequence')
        need(at(result['lhs'],theta)==at(result['rhs'],theta) and at(case['lhs'],theta)!=at(case['rhs'],theta),'Squaring witness must be an added non-solution')
        return False
    raise ValueError('Unsupported method certificate')


def truth(case,local,option,cert):
    kind=option['kind']
    if kind=='strategy': return strategy_valid(case,option,cert)
    if kind=='domain': return ident.DOMAINS[option['value']]==cert['excluded']
    if kind=='equation':
        validate(option['lhs']); validate(option['rhs'])
        return ident.equal(diff(option),['mul',local['scale'],diff(case)]) and not ident.value(diff(option))[2]-cert['excluded']
    if kind=='branches': return branchkey(option['items'])==cert['allowed' if local['form']=='allowed' else 'branches']
    if kind=='angles':
        angles=list(map(F,option['values']))
        return len(angles)==len(set(angles)) and all(0<=a<2 for a in angles) and set(angles)==set(cert['answers'])
    if kind=='check': return type(option['N']) is int and type(option['R']) is int and option['N']==len(cert['answers']) and option['R']==0
    raise ValueError('Unknown typed choice')


def equivalent(a,b):
    if a['kind']!=b['kind']: return False
    if a['kind']=='strategy': return equivalent(a['result'],b['result'])
    if a['kind']=='equation':
        n,d,_=ident.value(diff(a)); m,e,_=ident.value(diff(b))
        left,right=ident.mul(n,e),ident.mul(m,d)
        return (not left and not right) or (bool(left) and bool(right) and ident.signature(left)==ident.signature(right))
    if a['kind']=='branches': return branchkey(a['items'])==branchkey(b['items'])
    if a['kind']=='angles': return {F(v)%2 for v in a['values']}=={F(v)%2 for v in b['values']}
    return a==b


def option_registry(case):
    entries=[o for s in case['steps'] for o in s['candidates']]+case.get('controls',[])
    result={}
    for o in entries:
        key=ident.normal(display(o)); need(key not in result or result[key]==o,'Conflicting option display')
        result[key]=o
    return result


def reached(case,local,option,cert):
    body=eqtex(option['result']) if option['kind']=='strategy' else display(option)
    if local['form']=='domain': return domaintex(case)
    if local['form']=='check': body='S='+circle.angle_set(cert['answers'])+r',\quad '+body
    return body+r',\quad '+domaintex(case)


def audit(payload,cases):
    need(payload['accepted'] and payload['routes']==12 and payload['windows']==0 and payload['save_replay'],'Headless route gate failed')
    need([c['id'] for c in cases]==payload['question_ids']==[PACKAGE+f'_q{i:02}' for i in range(1,13)],'Wrong question order')
    need([c['group'] for c in cases]==['introductory']*4+['practice']*4+['mixed']*4,'Wrong group allocation')
    records=[]; positions=[]; originals=[]
    for case,row in zip(cases,payload['questions']):
        cert=certificate(case); q=row['question']; registry=option_registry(case)
        need(q['id']==case['id']==row['id'] and q['content_version']==1 and q['schema_version']==1,'Changed content identity')
        need(q['equation']==eqtex(case)+r',\quad '+I,'Changed original given/interval')
        need(q['description']==case['goal']+' '+case['domain_text'],'Changed original goal/domain')
        need(len(q['steps'])==len(case['steps']) and len(q['working_states'])==len(q['steps'])+1,'Wrong workflow length')
        states={s['id']:s['display'] for s in q['working_states']}
        need(q['working_states'][0]['display']==q['equation'],'Original work changed')
        for si,(step,local) in enumerate(zip(q['steps'],case['steps'])):
            need(step['id']==10*(si+1) and step['semantics']['before']==(q['working_states'][0]['id'] if si==0 else q['steps'][si-1]['semantics']['after']),'Broken semantic route')
            options=step['options']; need(len(options)==3 and sorted(o['id'] for o in options)==[step['id']+i for i in (1,2,3)],'Wrong option IDs')
            decoded=[]
            for o in options:
                key=ident.normal(o['label']); need(key in registry,'Unknown finite compiled mathematics')
                decoded.append(registry[key])
            need(all(o['kind']==local['candidates'][0]['kind'] for o in decoded),'Wrong decision type')
            for i in range(3):
                for j in range(i): need(not equivalent(decoded[i],decoded[j]),f"Semantic duplicate {case['id']} step {step['id']}")
            flags=[truth(case,local,o,cert) and goal_form(local,o) for o in decoded]
            need(sum(flags)==1,f"No unique mathematical/goal key {case['id']} step {step['id']}")
            correct=flags.index(True)
            need(step['accepted_option_ids']==[options[correct]['id']],'False accepted key')
            if si==0: positions.append(correct)
            valid=[ident.normal(reached(case,local,o,cert)) for o in registry.values() if o['kind']==decoded[0]['kind'] and truth(case,local,o,cert) and goal_form(local,o)]
            need(ident.normal(states[step['semantics']['after']]) in valid,'False reached state, lost domain or wrong goal')
            records.append({'question':case['id'],'step':step['id'],'form':local['form'],
                            'options':[{'id':o['id'],'truth':truth(case,local,d,cert),'goal':goal_form(local,d)} for o,d in zip(options,decoded)]})
        originals.append({'question':case['id'],'original_axis_exclusions':sorted(cert['excluded']),
                          'factor_identity_verified':True,'all_algebraic_branches':[[v,str(h)] for v,h in sorted(cert['branches'])],
                          'allowed_branches':[[v,str(h)] for v,h in sorted(cert['allowed'])],
                          'branch_completeness':cert['branch_evidence'],'original_checks':cert['original_checks'],
                          'complete_angles_theta_over_pi':[str(a) for a in cert['answers']]})
    need(sorted(positions)==[0]*4+[1]*4+[2]*4,'Unbalanced first positions')
    return records,originals


def reject(fn):
    try: fn()
    except (ValueError,KeyError,TypeError,ZeroDivisionError,export.ExportError) as e: return str(e)
    raise AssertionError('Corrupted input passed')


def command(args):
    p=subprocess.run(list(map(str,args)),capture_output=True,text=True,timeout=60)
    need(p.returncode==0,p.stderr or p.stdout); return json.loads(p.stdout)


def negative_positive(payload,cases):
    negatives=[]; positives=[]
    for qi,row in enumerate(payload['questions']):
        cert=certificate(cases[qi]); registry=option_registry(cases[qi])
        for si,step in enumerate(row['question']['steps']):
            bad=copy.deepcopy(payload); q=bad['questions'][qi]['question']; s=q['steps'][si]
            wrong=next(o for o in s['options'] if o['id'] not in s['accepted_option_ids'])
            s['accepted_option_ids']=[wrong['id']]
            negatives.append({'probe':'false_key','question':qi+1,'step':si+1,'rejected':reject(lambda:audit(bad,cases))})
            bad=copy.deepcopy(payload); q=bad['questions'][qi]['question']; local=cases[qi]['steps'][si]
            state=next(s for s in q['working_states'] if s['id']==step['semantics']['after'])
            obj=registry[ident.normal(wrong['label'])]
            state['display']=('D:'+display(obj)+r',\quad '+I if local['form']=='domain' else reached(cases[qi],local,obj,cert))
            negatives.append({'probe':'false_work','question':qi+1,'step':si+1,'rejected':reject(lambda:audit(bad,cases))})
    # Controls are registered mathematical objects, not unrecognized strings.
    for name,qi,si,obj in [
        ('missing_zero_factor_branch',0,2,{'kind':'branches','items':[['s','-1/2']]}),
        ('principal_only_angles',0,3,{'kind':'angles','values':['0','7/6']}),
        ('excluded_two_pi',0,3,{'kind':'angles','values':['0','1','7/6','11/6','2']}),
        ('lost_valid_zero',0,3,{'kind':'angles','values':['1','7/6','11/6']}),
        ('true_expanded_but_not_factored',0,1,{'kind':'equation','lhs':cases[0]['lhs'],'rhs':0})]:
        altered=copy.deepcopy(cases); altered[qi]['controls'].append(obj)
        bad=copy.deepcopy(payload); step=bad['questions'][qi]['question']['steps'][si]
        next(o for o in step['options'] if o['id'] in step['accepted_option_ids'])['label']=display(obj)
        negatives.append({'probe':name,'rejected':reject(lambda:audit(bad,altered))})
    for name,qi,si,obj in [
        ('reordered_factor_product',0,1,{'kind':'equation','lhs':['mul',*reversed(cases[0]['factors'])],'rhs':0}),
        ('reordered_complete_angles',0,3,{'kind':'angles','values':list(reversed([str(a) for a in certificate(cases[0])['answers']]))})]:
        altered=copy.deepcopy(cases); altered[qi]['controls'].append(obj)
        good=copy.deepcopy(payload); q=good['questions'][qi]['question']; step=q['steps'][si]
        next(o for o in step['options'] if o['id'] in step['accepted_option_ids'])['label']=display(obj)
        next(s for s in q['working_states'] if s['id']==step['semantics']['after'])['display']=reached(altered[qi],altered[qi]['steps'][si],obj,certificate(altered[qi]))
        audit(good,altered); positives.append(name)
        bad=copy.deepcopy(payload); step=bad['questions'][qi]['question']['steps'][si]
        next(o for o in step['options'] if o['id'] not in step['accepted_option_ids'])['label']=display(obj)
        negatives.append({'probe':'duplicate_'+name,'rejected':reject(lambda:audit(bad,altered))})
    for field,val in [('include_lower',False),('include_upper',True),('units','degrees'),('upper','3')]:
        altered=copy.deepcopy(cases); altered[0]['interval'][field]=val
        negatives.append({'probe':'invalid_interval_'+field,'rejected':reject(lambda:audit(payload,altered))})
    altered=copy.deepcopy(cases); altered[10]['domain']='R'
    negatives.append({'probe':'lost_original_cosine_exclusion','rejected':reject(lambda:audit(payload,altered))})
    for x in [True,13,['pow','s',3],['div',1,0]]:
        negatives.append({'probe':'invalid_input','rejected':reject(lambda:(validate(x),ident.value(x)))})
    negatives.append({'probe':'reciprocal_at_forbidden_angle','rejected':reject(lambda:at('sec',F(1,2)))})
    return negatives,positives


def markdown_edits(payload):
    original=export.read_bytes(AUTHOR/'documents/chapter.paths.md').decode(); result=[]
    for directive,field in [('@step 10 | ','prompt'),('@feedback ','wrong_feedback')]:
        start=original.index(directive,original.index('@question ')); end=original.index('\n',start)
        old=original[start:end]; extra=' Check the stated condition before proceeding.'
        edited=original[:end]+extra+original[end:]
        folder=OUT/'markdown-edits'/field/export.sha(edited.encode())
        export.immutable_directory(folder/'source',{'chapter.paths.md':edited.encode()})
        compiled=command([MODEL,'--question-batch',folder/'source']); expected=copy.deepcopy(payload)
        step=expected['questions'][0]['question']['steps'][0]
        if field=='prompt':
            previous=step[field]; step[field]+=extra
            field_path='/questions/0/question/steps/0/prompt'
        else:
            oid=int(old.split(' | ')[0].split()[1])
            index=next(i for i,o in enumerate(step['options']) if o['id']==oid)
            previous=step['options'][index][field]; step['options'][index][field]+=extra
            field_path=f'/questions/0/question/steps/0/options/{index}/wrong_feedback'
        need(compiled==expected and previous!=previous+extra,'Real Markdown edit changed unrelated fields')
        delta={'accepted':True,'changed_fields':[{'path':field_path,'before':previous,'after':previous+extra}],
               'all_other_compiled_fields_equal':True,'canonical_routes_sha256':export.sha(export.encoded(payload)),
               'original_markdown_sha256':export.sha(original.encode()),'edited_markdown_sha256':export.sha(edited.encode()),
               'command':[str(MODEL),'--question-batch',str(folder/'source')]}
        export.immutable_directory(folder/'checks',{'routes.json':export.encoded(compiled),'field-changes.json':export.encoded(delta)})
        files={str(p.relative_to(OUT)):export.sha(export.read_bytes(p)) for p in sorted(folder.rglob('*')) if p.is_file()}
        result.append({'field':field,'only_intended_compiled_field_changed':True,'retained_evidence_sha256':files})
    return result


def hashes(folder):
    return {str(p.relative_to(folder)):export.sha(export.read_bytes(p)) for p in sorted(folder.rglob('*')) if p.is_file() and p.name!='BRIEF.md' and '__pycache__' not in p.parts}


def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--routes',required=True); args=ap.parse_args()
    before=hashes(SOURCE); need(set(before)=={'DESIGN.md','cases.json','certificate_tests.py','review.md','authoring/authoring.json','authoring/documents/chapter.paths.md'},'Missing or extra maintained source')
    cases=json.loads(export.read_bytes(SOURCE/'cases.json'))['cases']
    ready=json.loads(export.read_bytes(ROOT/'build/production/wave01/build-ready.json'))
    binary={p.name:export.sha(export.read_bytes(p,64*1024*1024)) for p in (TARGET,MODEL)}
    need(ready['status']=='ready' and all(ready['sha256'][k]==v for k,v in binary.items()),'Wrong shared binary pins')
    source_registry=json.loads(export.read_bytes(REGISTRY)); seed=next(s for s in source_registry['sources'] if s['id']=='precalc_trig')
    snapshots={}
    for path,key in [(seed['local_snapshot'],'sha256'),(seed['local_text'],'text_sha256')]:
        actual=export.sha(export.read_bytes(Path(path),16*1024*1024)); need(actual==seed['download'][key],'Changed pinned primary source'); snapshots[path]=actual
    snapshot_root=Path(seed['local_text']).parent
    for row in seed['license_evidence']:
        for field,key in [('file','sha256'),('text_file','text_sha256')]:
            if field in row:
                path=snapshot_root/row[field]; actual=export.sha(export.read_bytes(path,16*1024*1024)); need(actual==row[key],'Changed license evidence'); snapshots[str(path)]=actual
    locators=set()
    for case in cases:
        src=case['source']; need(src['source_id']=='precalc_trig' and all(src[k] for k in ('locator','original_mathematical_givens','adaptation_kind','changes')),'Missing exact seed provenance')
        locators.add(src['locator'])
    need(len(locators)>=6,'Insufficient distinct approved seeds')
    path=Path(args.routes).resolve(); payload=json.loads(export.read_bytes(path)); need(payload==command([MODEL,'--question-batch',AUTHOR/'documents']),'Stale compiled routes')
    records,originals=audit(payload,cases); negatives,positives=negative_positive(payload,cases); edits=markdown_edits(payload)
    inspection=export.Target(TARGET).inspect(documents=AUTHOR/'documents'); need(inspection==json.loads(export.read_bytes(OUT/'inspection.json')),'Stale inspection')
    entities=inspection['entities']; content=[e for e in entities if e['kind'] in ('lesson','question')]
    need(len(content)==13 and all(e['parent']=='topic_0033' and e['subject']=='trigonometry' for e in content),'Wrong chapter binding')
    reading=next(e for e in content if e['kind']=='lesson'); need(reading['id']==PACKAGE+'_r' and reading['links']==[c['id'] for c in cases],'Wrong reading/practice order')
    need(all(e['links']==[PACKAGE+'_r'] for e in content if e['kind']=='question'),'Wrong question reading')
    author=json.loads(export.read_bytes(AUTHOR/'authoring.json')); need(author['package_id']==PACKAGE and author['package_version']==1,'Wrong metadata package')
    provenance=json.loads(export.provenance(author,entities)); need(all(s['kind']=='adapted' and seed['license_url'] in s['reuse'] for s in author['sources']),'Missing adapted license notice')
    lessons=command([MODEL,'--family-lessons',AUTHOR/'documents']); need(lessons==json.loads(export.read_bytes(OUT/'lessons.json')) and lessons['accepted'],'Stale/failed disclosure gate')
    need(hashes(SOURCE)==before,'Source changed during checks')
    need(all(export.sha(export.read_bytes(p,64*1024*1024))==binary[p.name] for p in (TARGET,MODEL)),'Binary changed during checks')
    dependencies={str(p.relative_to(ROOT)):export.sha(export.read_bytes(p)) for p in (W3,W1,ROOT/'tools/export_learning.py',ROOT/'tools/build_question_batch.py',ROOT/'tools/check_authoring_pilot.py')}
    strategies=[]
    for case in cases:
        for step in case['steps']:
            if step['form']!='strategy': continue
            for option in step['candidates']:
                row={'question':case['id'],'action':option['action'],'preserves_exact_original_solution_set':strategy_valid(case,option,certificate(case))}
                if 'counterexample_theta_over_pi' in option:
                    theta=F(option['counterexample_theta_over_pi']); consequence=option['result']
                    row.update(theta_over_pi=str(theta),original_left=[str(x) for x in at(case['lhs'],theta)],original_right=[str(x) for x in at(case['rhs'],theta)],consequence_left=[str(x) for x in at(consequence['lhs'],theta)],consequence_right=[str(x) for x in at(consequence['rhs'],theta)])
                    if option['action']=='divide_coordinate': row['divisor_value']=[str(x) for x in at(option['coordinate'],theta)]
                strategies.append(row)
    report={'accepted':True,'questions':12,'steps':len(records),'wrong_choices':2*len(records),'decisions':records,'original_certificates':originals,'strategy_certificates':strategies,'negative_probes':negatives,'positive_controls':positives,'real_markdown_edits':edits,'source_sha256':before,'provenance':provenance,'windows':0}
    (OUT/'mathematics.json').write_bytes(export.encoded(report))
    trial=json.loads(export.read_bytes(OUT/'trial.json'))
    trial['checks_done_utc']=datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M:%S UTC')
    (OUT/'trial.json').write_bytes(export.encoded(trial))
    evidence={name:export.sha(export.read_bytes(OUT/name)) for name in ('inspection.json','routes.json','lessons.json','mathematics.json','source-page8-raw.txt','trial.json')}
    for edit in edits: evidence.update(edit['retained_evidence_sha256'])
    receipt={'format':'paths_depth_subject','format_version':1,'wave':'wave04','subject':'trigonometry','stage':'ready_for_coordinator_review','published':False,'source':str(AUTHOR),'package_id':PACKAGE,'reading_id':PACKAGE+'_r','question_ids':[c['id'] for c in cases],'questions':12,'readings':1,'steps':len(records),'wrong_choices':2*len(records),'source_sha256':before,'evidence_sha256':evidence,'target_sha256':binary[TARGET.name],'model_sha256':binary[MODEL.name],'inspection':str(OUT/'inspection.json'),'routes':str(path),'lessons':str(OUT/'lessons.json'),'mathematical_evidence':str(OUT/'mathematics.json'),'source_questions':12,'distinct_seed_prompts':len(locators),'source_registry_sha256':export.sha(export.read_bytes(REGISTRY)),'content_license':'CC-BY-SA-4.0','source_snapshots_sha256':snapshots,'reused_helpers_sha256':dependencies,'trial_measurement':str(OUT/'trial.json'),'remaining_concerns':['Coordinator independent review/publication remain separate. Finite exact scope only; native appearance and learner outcomes unobserved.']}
    (OUT/'production.json').write_bytes(export.encoded(receipt))
    print(json.dumps({'accepted':True,'questions':12,'steps':len(records),'wrong_choices':2*len(records),'distinct_seed_prompts':len(locators),'negative_probes':len(negatives),'positive_controls':len(positives),'real_markdown_edits':len(edits),'production':str(OUT/'production.json')},indent=2))


if __name__=='__main__': main()
