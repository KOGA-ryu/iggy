"""Finite integration certificates for actual compiler JSON, never a generator.

Reuse the frozen Wave 02 Poly kernel. The bounded math-field AST adapter below
adds exact scalar division and C-family semantics for these twelve cases only.
It is not a document parser, general CAS or runtime answer owner.
"""
import argparse
import ast
import copy
from fractions import Fraction as Q
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import tempfile

import export_learning as export

SOURCE=Path(__file__).resolve().parent
ROOT=SOURCE.parents[4]
OUT=ROOT/'build/production/wave03/calculus'
DOCS=SOURCE/'authoring/documents'
HELPER=ROOT/'content/authoring/production/wave02/calculus/certificate_tests.py'
spec=importlib.util.spec_from_file_location('frozen_calc_poly',HELPER)
kernel=importlib.util.module_from_spec(spec); spec.loader.exec_module(kernel)
Poly=kernel.Poly; X=kernel.X; K=kernel.variable('v'); ZERO=kernel.ZERO
KKEY=(0,0,1,0,0)
PID='prod03_calc_integration'; READING=PID+'_r'
DOMAINS={
 'family':'All identities hold for real x on R. P denotes a primitive with P(0)=0; C is an arbitrary real constant.',
 'initial':'All identities hold for real x on R. P denotes a primitive with P(0)=0; write F=P+C and determine the real constant C from the given condition.',
 'definite':'The polynomial is defined on R and continuous on the closed interval between the given limits. a is the lower displayed limit and b the upper displayed limit; retain their order. P is a primitive with P(0)=0.'}
GOALS={
 'family':'Find all antiderivatives of p and prove the family complete.',
 'initial':'Find the unique function F satisfying both the derivative and the initial condition, and check both.',
 'definite':'Calculate the signed definite integral from its original polynomial and ordered limits, and check the result.'}

def need(ok,reason):
    if not ok: raise ValueError(reason)
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def read(path): return json.loads(path.read_text())
def write(name,value): (OUT/name).write_bytes(export.encoded(value))
def signature(p): return tuple(sorted(p.c.items()))

def tree(text):
    """Only bounded exact polynomial expressions from compiled math fields."""
    need(isinstance(text,str) and len(text)<512,'math field exceeds finite bound')
    s=text.replace(r'\left','').replace(r'\right','').replace(r'\cdot','*').replace(r'\,','')
    for _ in range(8):
        new=re.sub(r'\\frac\{([^{}]+)\}\{([^{}]+)\}',r'((\1)/(\2))',s)
        if new==s: break
        s=new
    s=s.replace('{','(').replace('}',')').replace('^','**')
    s=re.sub(r'\s+','',s)
    need(re.fullmatch(r'[0-9xCuvUV()+*/\-]+',s) is not None,'unsupported math field')
    s=re.sub(r'(?<=[0-9xCuvUV)])(?=[xCuvUV(])','*',s)
    node=ast.parse(s,mode='eval').body
    need(len(list(ast.walk(node)))<=128,'math tree exceeds finite bound')
    return node

def calculate(node):
    if isinstance(node,ast.Constant):
        need(type(node.value) is int and abs(node.value)<=100000,'exact bounded integer required')
        return Poly(node.value)
    if isinstance(node,ast.Name):
        env={'x':X,'C':K,**{v:kernel.variable(v) for v in 'uvUV'}}
        need(node.id in env,'unknown math symbol'); return env[node.id]
    if isinstance(node,ast.UnaryOp) and isinstance(node.op,(ast.UAdd,ast.USub)):
        p=calculate(node.operand); return -p if isinstance(node.op,ast.USub) else p
    need(isinstance(node,ast.BinOp),'unsupported operation')
    a,b=calculate(node.left),calculate(node.right)
    if isinstance(node.op,ast.Add): result=a+b
    elif isinstance(node.op,ast.Sub): result=a-b
    elif isinstance(node.op,ast.Mult): result=a*b
    elif isinstance(node.op,ast.Div):
        denominator=b.scalar();need(denominator!=0,'zero divisor');result=a*Q(1,denominator)
    elif isinstance(node.op,ast.Pow):
        n=b.scalar();need(n.denominator==1 and 0<=n<=5,'power outside finite integration scope');result=a**n
    else: raise ValueError('unsupported arithmetic operation')
    need(len(result.c)<=64 and all(sum(k)<=6 for k in result.c),'polynomial exceeds finite scope')
    return result
def poly(text): return calculate(tree(text))
def number(text): return poly(text).scalar()

def atom(node):
    if isinstance(node,ast.Constant) and type(node.value) is int: return Q(node.value)
    if isinstance(node,ast.UnaryOp) and isinstance(node.op,(ast.UAdd,ast.USub)):
        v=atom(node.operand);return None if v is None else (-v if isinstance(node.op,ast.USub) else v)
    if isinstance(node,ast.BinOp) and isinstance(node.op,ast.Div):
        a,b=atom(node.left),atom(node.right)
        if a is not None and b not in (None,0): return a/b
    return None
def monomial(node):
    a=atom(node)
    if a is not None: return 0,a
    if isinstance(node,ast.Name) and node.id=='x': return 1,Q(1)
    if isinstance(node,ast.UnaryOp) and isinstance(node.op,(ast.UAdd,ast.USub)):
        m=monomial(node.operand)
        return None if m is None else (m[0],-m[1] if isinstance(node.op,ast.USub) else m[1])
    if isinstance(node,ast.BinOp):
        if isinstance(node.op,ast.Pow) and isinstance(node.left,ast.Name) and node.left.id=='x':
            n=atom(node.right)
            if n is not None and n.denominator==1 and 1<=n<=5:return int(n),Q(1)
        if isinstance(node.op,ast.Mult):
            for scalar,term in ((node.left,node.right),(node.right,node.left)):
                a,m=atom(scalar),monomial(term)
                if a is not None and m is not None:return m[0],a*m[1]
        if isinstance(node.op,ast.Div):
            m,a=monomial(node.left),atom(node.right)
            if m is not None and a not in (None,0):return m[0],m[1]/a
    return None
def summands(node,sign=1):
    if isinstance(node,ast.BinOp) and isinstance(node.op,(ast.Add,ast.Sub)):
        return summands(node.left,sign)+summands(node.right,-sign if isinstance(node.op,ast.Sub) else sign)
    return [(sign,node)]
def collected(node):
    terms=[monomial(n) for _,n in summands(node)]
    return (all(m is not None and m[1]!=0 for m in terms) and
            len({m[0] for m in terms})==len(terms))

def family(p):
    need(all(not(k[1] or k[3] or k[4]) and (k[2]==0 or k==KKEY) for k in p.c),'unsupported constant-family expression')
    if p.c.get(KKEY,0):
        # Any nonzero scalar times arbitrary real C still parametrizes every
        # constant. Ignore its scaling and any fixed constant shift.
        return ('family',signature(Poly({k:v for k,v in p.c.items() if k!=KKEY and k!=ZERO})))
    return ('single',signature(p))

def original(c):
    kind=c['kind'];need(kind in GOALS and c['domain']=='R','invalid real domain/kind')
    vals=c['p'];need(isinstance(vals,list) and 1<=len(vals)<=5,'invalid polynomial degree')
    def bounded(text,bound,denominators):
        need(type(text) is str and re.fullmatch(r'-?\d+(?:/[1-9]\d*)?',text),'exact rational input string required')
        v=Q(text);need(abs(v)<=bound and v.denominator in denominators,'rational input out of bounds');return v
    coefficients=[bounded(v,6,(1,2,3,4,6)) for v in vals]
    need(coefficients[-1]!=0,'zero/trailing coefficient excluded')
    p=kernel.from_coefficients(coefficients)
    P=kernel.from_coefficients([0]+[a/Q(j+1) for j,a in enumerate(coefficients)])
    need(P.derivative()==p and P.at(0)==0,'coefficient inverse derivative failed')
    data={'p':p,'P':P,'coefficients':coefficients,'family':family(P+K)}
    if kind=='initial':
        x0=bounded(c['x0'],2,(1,2));y0=bounded(c['y0'],4,(1,2));v=P.at(x0);constant=y0-v;F=P+constant
        need(F.derivative()==p and F.at(x0)==y0,'original IVP verification')
        data.update(x0=x0,y0=y0,primitive_value=v,constant=constant,F=F)
    if kind=='definite':
        a,b=[bounded(c[n],2,(1,2)) for n in ('a','b')];pa,pb=P.at(a),P.at(b)
        contributions=[coef*(b**(j+1)-a**(j+1))/Q(j+1) for j,coef in enumerate(coefficients) if coef]
        integral=sum(contributions,Q(0));need(integral==pb-pa,'independent original coefficient integral')
        data.update(a=a,b=b,pa=pa,pb=pb,integral=integral,contributions=contributions)
    goals=(['primitive','derivative','family','complete'] if kind=='family' else
           ['primitive','condition','constant','particular','verify'] if kind=='initial' else
           ['primitive','endpoints','value'] if data['a']==data['b'] else
           ['method','primitive','endpoints','value','independent'])
    need(c['local_goals']==goals,'incomplete/changed route goals')
    return data

def field(label,goal,d):
    """Return semantic signature, mathematical truth, requested-goal/form truth."""
    if goal=='method':
        expression,condition=label.split(r',\quad ')
        need(condition=="P'=p" and expression.startswith('I='),'method conditions/objects')
        rhs=expression[2:]
        for old,new in [('P(b)','u'),('P(a)','v'),('p(b)','U'),('p(a)','V')]:rhs=rhs.replace(old,new)
        value=poly(rhs);expected=kernel.variable('u')-kernel.variable('v')
        return ('method',signature(value)),value==expected,True
    if goal=='complete':
        first,second=label.split(r',\quad ')
        need(first.startswith("(F-P)'=") and second.startswith('F-P='),'completeness objects')
        v=number(first.split('=',1)[1]);f=family(poly(second.split('=',1)[1]))
        return ('difference',v,f),v==0 and f==family(K),True
    if goal in ('endpoints','verify'):
        lhs,rhs=label.split('=',1)
        if goal=='endpoints':need(lhs=='(P(b),P(a))','endpoint pair order/names')
        else:
            match=re.fullmatch(r"\(F'\(x\),F\((.+)\)\)",lhs)
            need(match is not None and number(match[1])==d['x0'],'original IVP input in check')
        need(rhs.startswith('(') and rhs.endswith(')'),'pair delimiters')
        parts=rhs[1:-1].split(',');need(len(parts)==2,'pair arity')
        values=[poly(p) for p in parts]
        expected=[Poly(d['pb']),Poly(d['pa'])] if goal=='endpoints' else [d['p'],Poly(d['y0'])]
        return ('pair',*(signature(v) for v in values)),values==expected,True
    if goal=='condition':
        left,right=label.split('=');a,b=poly(left),poly(right);diff=a-b
        need(all(k in (KKEY,ZERO) for k in diff.c),'condition must be affine in C only')
        coefficient=diff.c.get(KKEY,0);need(coefficient!=0,'condition must determine one C')
        answer=-diff.c.get(ZERO,0)/coefficient
        form=(a==K+d['primitive_value'] and b==Poly(d['y0']))
        return ('constant_equation',answer),answer==d['constant'],form
    left,*parts=label.split('=');need(parts,'missing equality')
    expected_left={'primitive':'P(x)','derivative':"P'(x)",'family':'F(x)',
                   'constant':'C','particular':'F(x)','value':'I','independent':'I'}[goal]
    need(left==expected_left,'wrong mathematical object for '+goal)
    trees=[tree(p) for p in parts];values=[calculate(t) for t in trees]
    need(all(v==values[0] for v in values),'false earlier equality in '+goal)
    value=values[0]
    if goal=='family':
        sig=family(value);return sig,sig==d['family'],True
    if goal in ('primitive','derivative','particular'):
        expected=d[{'primitive':'P','derivative':'p','particular':'F'}[goal]]
        form=all(collected(t) for t in trees) if goal in ('primitive','particular') else True
        return ('polynomial',signature(value)),value==expected,form
    result=value.scalar()
    if goal=='constant':return ('number',result),result==d['constant'],True
    if goal=='value':
        t=trees[0]
        form=(len(trees)>=2 and isinstance(t,ast.BinOp) and isinstance(t.op,ast.Sub) and
              calculate(t.left)==Poly(d['pb']) and calculate(t.right)==Poly(d['pa']))
    else:
        terms=[sign*calculate(n).scalar() for sign,n in summands(trees[0])]
        form=len(trees)>=2 and terms==d['contributions']
    return ('number',result),result==d['integral'],form

def given(q,c,d):
    expression=q['equation']
    if c['kind']=='family':
        need(expression.startswith('p(x)=') and poly(expression[5:])==d['p'],'original integrand mismatch')
    elif c['kind']=='initial':
        match=re.fullmatch(r"F'\(x\)=(.+),\\quad F\((.+)\)=(.+)",expression)
        need(match is not None and poly(match[1])==d['p'] and number(match[2])==d['x0'] and number(match[3])==d['y0'],'original IVP mismatch')
    else:
        match=re.fullmatch(r'p\(x\)=(.+),\\quad I=\\int_\{([^{}]+)\}\^\{([^{}]+)\}p\(x\)\\,dx',expression)
        need(match is not None and poly(match[1])==d['p'] and Q(match[2])==d['a'] and Q(match[3])==d['b'],'original integral/order mismatch')
    need(q['description']==GOALS[c['kind']]+' '+DOMAINS[c['kind']],'compiled domain/goal mismatch')

def check(routes,packet):
    need(packet['format']=='paths_polynomial_integration_cases' and packet['version']==1 and packet['coefficient_order']=='ascending','case manifest contract')
    need(routes['accepted'] is True and routes['routes']==12 and routes['save_replay'] is True,'native route/replay receipt')
    cases=packet['cases'];ids=[PID+f'_q{i:02}' for i in range(1,13)]
    need([c['id'] for c in cases]==ids and [r['id'] for r in routes['questions']]==ids,'reserved identities/order')
    need([c['group'] for c in cases]==['introductory']*4+['practice']*4+['mixed']*4,'coverage groups')
    need([c['kind'] for c in cases]==['family']*4+['initial']*4+['definite']*4,'assigned problem kinds')
    proof=[];positions=[];steps=0
    for c,row in zip(cases,routes['questions']):
        d=original(c);q=row['question'];given(q,c,d)
        need(q['id']==c['id'] and q['content_version']==1 and 'support' not in q,'identity/support contract')
        need(len(q['steps'])==len(c['local_goals']) and len(q['working_states'])==len(q['steps'])+1,'route completeness')
        need(q['working_states'][0]['display']==q['equation'],'original working differs')
        for i,(step,goal) in enumerate(zip(q['steps'],c['local_goals'])):
            need(len(step['options'])==3 and len({o['id'] for o in step['options']})==3,'three distinct option identities')
            results=[field(o['label'],goal,d) for o in step['options']]
            need(len({r[0] for r in results})==3,c['id']+': semantic duplicate at '+goal)
            good=[o['id'] for o,r in zip(step['options'],results) if r[1] and r[2]]
            need(len(good)==1 and good==step['accepted_option_ids'],c['id']+': false key or requested form at '+goal)
            reached=field(q['working_states'][i+1]['display'],goal,d)
            need(reached[1] and reached[2],c['id']+': false reached truth/form at '+goal)
            need(step['semantics']['before']==i+1 and step['semantics']['after']==i+2,'state routing')
            wrong=[o.get('wrong_feedback','').strip() for o in step['options'] if o['id'] not in good]
            need(len(set(wrong))==2 and all(wrong),'individual feedback missing/duplicated')
            need(step['prompt'].strip() and step['explanation'].strip() and step['wrong_hint'].strip(),'missing teaching field')
            if i==0:
                pos=next(j for j,o in enumerate(step['options']) if o['id']==good[0]);positions.append(pos)
                need(pos==c['first_position'],'first position differs from case')
            steps+=1
        proof.append(dict(id=c['id'],kind=c['kind'],original_coefficients=d['p'].coefficients(),
            zero_constant_primitive=d['P'].coefficients(),derivative_identity=True,
            initial={k:str(d[k]) for k in ('x0','y0','primitive_value','constant')} if c['kind']=='initial' else None,
            particular=d['F'].coefficients() if c['kind']=='initial' else None,
            definite={k:str(d[k]) for k in ('a','b','pa','pb','integral')} if c['kind']=='definite' else None,
            original_coefficient_contributions=list(map(str,d['contributions'])) if c['kind']=='definite' else None,
            complete_constant_family=c['kind']=='family',decisions=len(q['steps'])))
    need(sorted(positions)==[0]*4+[1]*4+[2]*4,'first-position balance')
    need(steps==54 and routes['wrong_choices']==108,'declared full-route counts')
    return dict(accepted=True,questions=12,readings=1,steps=steps,options=3*steps,wrong_choices=2*steps,first_positions=positions,cases=proof)

def controls(routes,packet):
    counts={'false_keys':0,'false_working':0,'semantic_duplicates':0,'invalid_inputs_domains':0,
            'missing_C':0,'shifted_family_duplicate':0,'wrong_limit_order':0,'true_wrong_form':0,'valid_alternatives':0}
    def reject(data,p=packet,reason=None):
        try:check(data,p)
        except (ValueError,SyntaxError,KeyError) as error:
            if reason:need(reason in str(error),'control failed for unintended reason: '+str(error))
        else:raise AssertionError('invalid control accepted')
    def change(qi,si,label):
        bad=copy.deepcopy(routes);q=bad['questions'][qi]['question'];s=q['steps'][si]
        next(o for o in s['options'] if o['id'] in s['accepted_option_ids'])['label']=label
        q['working_states'][si+1]['display']=label
        return bad
    for qi,row in enumerate(routes['questions']):
        c=packet['cases'][qi]
        for si,s in enumerate(row['question']['steps']):
            wrong=next(o for o in s['options'] if o['id'] not in s['accepted_option_ids'])
            bad=copy.deepcopy(routes);bad['questions'][qi]['question']['steps'][si]['accepted_option_ids']=[wrong['id']]
            reject(bad);counts['false_keys']+=1
            bad=copy.deepcopy(routes);bad['questions'][qi]['question']['working_states'][si+1]['display']=wrong['label']
            reject(bad);counts['false_working']+=1
            label=next(o['label'] for o in s['options'] if o['id'] in s['accepted_option_ids'])
            # Thin spaces are mathematically ignored by the bounded reader;
            # rejection is required specifically at semantic distinctness.
            duplicate=label.replace('=',r'=\,',1) if c['local_goals'][si] not in ('method','complete','condition','endpoints','verify') else label
            if c['local_goals'][si]=='method':duplicate=label.replace('P(b)-P(a)','-P(a)+P(b)')
            elif c['local_goals'][si]=='complete':duplicate=label.replace('F-P=C','F-P=C+5')
            elif c['local_goals'][si]=='condition':
                left,right=label.split('=');duplicate=left+'+1=('+right+')+1'
            elif c['local_goals'][si] in ('endpoints','verify'):duplicate=label.replace(')=(',')=( ',1)
            bad=copy.deepcopy(routes);bs=bad['questions'][qi]['question']['steps'][si]
            next(o for o in bs['options'] if o['id'] not in bs['accepted_option_ids'])['label']=duplicate
            reject(bad,reason='semantic duplicate');counts['semantic_duplicates']+=1
    # Full family equivalence, not literal C suffixes.
    q=routes['questions'][0]['question'];label=next(o['label'] for o in q['steps'][2]['options'] if o['id']==31)
    reject(change(0,2,label.replace('+C','')));counts['missing_C']+=1
    bad=copy.deepcopy(routes);bs=bad['questions'][0]['question']['steps'][2]
    next(o for o in bs['options'] if o['id']==33)['label']=label.replace('+C','+5+C')
    reject(bad,reason='semantic duplicate');counts['shifted_family_duplicate']+=1
    reject(change(8,3,'I=-3-6=-9'));counts['wrong_limit_order']+=1
    for qi,si,text in [(0,0,'P(x)=x(2x^2-2x+3)'),(4,3,'F(x)=x(2x-3)+3'),
                       (4,1,'C=3'),(8,3,'I=9=9'),(8,4,'I=9=9')]:
        d=original(packet['cases'][qi]);goal=packet['cases'][qi]['local_goals'][si]
        result=field(text,goal,d);need(result[1] and not result[2],'goal-form control must be mathematically true')
        reject(change(qi,si,text));counts['true_wrong_form']+=1
    alternatives=[(0,0,'P(x)=3x-2x^2+2x^3'),(0,0,r'P(x)=\frac{4}{2}x^3-\frac{4}{2}x^2+3x'),
                  (0,2,label.replace('+C','+5+C')),(0,2,label.replace('+C','-2C')),
                  (4,3,'F(x)=3-3x+2x^2'),(8,0,"I=-P(a)+P(b),\\quad P'=p"),
                  (8,3,r'I=\frac{12}{2}-(-\frac{6}{2})=\frac{18}{2}'),(8,4,'I=(3-3)+9=9')]
    for qi,si,text in alternatives:check(change(qi,si,text),packet);counts['valid_alternatives']+=1
    for qi in range(12):
        for name,value in [('domain','C'),('p',['1']*6),('p',['1/7','1']),('p',[True,'1']),('p',['1','0'])]:
            bad=copy.deepcopy(packet);bad['cases'][qi][name]=value;reject(routes,bad);counts['invalid_inputs_domains']+=1
    for qi,name,value in [(4,'x0','3'),(4,'y0','5'),(8,'a','-3'),(8,'b','1/3')]:
        bad=copy.deepcopy(packet);bad['cases'][qi][name]=value;reject(routes,bad);counts['invalid_inputs_domains']+=1
    return counts

def command(binary,mode,folder):
    run=subprocess.run([str(binary),mode,str(folder)],capture_output=True,text=True,timeout=60)
    need(run.returncode==0,run.stderr or run.stdout);return json.loads(run.stdout)
def source_hashes():
    return {n:sha(SOURCE/n) for n in ('DESIGN.md','cases.json','certificate_tests.py','review.md',
                                      'authoring/authoring.json','authoring/documents/chapter.paths.md')}
def actual_edits(routes,packet,model):
    raw=(DOCS/'chapter.paths.md').read_text();s=routes['questions'][0]['question']['steps'][0]
    wrong=next(o for o in s['options'] if o.get('wrong_feedback'))
    probes=[('prompt','@step 10 | '+s['prompt'],'Recheck each coefficient. '+s['prompt']),
            ('wrong_feedback',f"@feedback {wrong['id']} | "+wrong['wrong_feedback'],'Check the derivative first. '+wrong['wrong_feedback'])]
    results=[]
    for field,old,new in probes:
        need(old in raw,'missing actual Markdown edit target')
        changed=raw.replace(old,old.split(' | ',1)[0]+' | '+new,1)
        with tempfile.TemporaryDirectory(prefix='markdown-edit-',dir=OUT) as directory:
            path=Path(directory);export.write_tree(path,{'chapter.paths.md':changed.encode()})
            actual=command(model,'--question-batch',path);check(actual,packet)
            expected=copy.deepcopy(routes);target=expected['questions'][0]['question']['steps'][0]
            if field=='prompt':target['prompt']=new
            else:next(o for o in target['options'] if o['id']==wrong['id'])['wrong_feedback']=new
            need(actual==expected,'actual Markdown edit changed unexpected compiled fields')
            results.append(dict(field=field,source_before=hashlib.sha256(raw.encode()).hexdigest(),
                source_after=hashlib.sha256(changed.encode()).hexdigest(),compiled_change=True,math_and_other_fields_unchanged=True))
    return results

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--routes',type=Path,required=True);args=parser.parse_args()
    OUT.mkdir(parents=True,exist_ok=True);before=source_hashes();helper_hash=sha(HELPER)
    ready=read(ROOT/'build/production/wave01/build-ready.json');need(ready['status']=='ready','Release not ready')
    target=ROOT/'b/sorter';model=ROOT/'b/paths_learning_document_tests'
    for path in (target,model):need(sha(path)==ready['sha256'][path.name],'Release executable mismatch')
    packet=read(SOURCE/'cases.json');routes=read(args.routes)
    need(routes==command(model,'--question-batch',DOCS),'stale compiled routes')
    # Exact kernel and parser controls include a non-sampled identity.
    need(poly(r'\frac{1}{2}x^4-\frac{3}{4}x^2+2x').derivative()==poly('2x^3-(3/2)x+2'),'rational derivative regression')
    need(poly('x(2x^2-2x+3)')==poly('2x^3-2x^2+3x'),'factored identity control')
    report=check(routes,packet);report['controls']=controls(routes,packet)
    report['actual_markdown_edits']=actual_edits(routes,packet,model)
    inspection=export.Target(target).inspect(documents=DOCS)
    export.provenance(read(SOURCE/'authoring/authoring.json'),inspection['entities'])
    need(inspection==read(OUT/'inspection.json'),'stale inspection')
    lessons=command(model,'--family-lessons',DOCS)
    need(lessons==read(OUT/'lessons.json') and lessons['accepted'] and lessons['questions']==12 and lessons['readings']==1,'stale/failed lesson gate')
    need({e['id'] for e in inspection['entities'] if e['kind']=='lesson'}=={READING},'reading identity')
    need(all(e['links']==[READING] for e in inspection['entities'] if e['kind']=='question'),'reading references')
    # Independent worked-example coefficient check, separate from authored keys.
    wp=kernel.from_coefficients([-2,0,3]);wP=kernel.from_coefficients([0,-2,0,1]);wF=wP+3
    need(wF.derivative()==wp and wF.at(-1)==4 and wP.at(1)-wP.at(-1)==-2,'worked arithmetic')
    report['worked_example']={'primitive':['0','-2','0','1'],'constant':'3','F_minus1':'4','integral_minus1_to1':'-2'}
    need(before==source_hashes() and helper_hash==sha(HELPER),'source/helper changed during checks')
    for path in (target,model):need(sha(path)==ready['sha256'][path.name],'binary changed during checks')
    report.update(source_sha256_before=before,source_sha256_after=source_hashes(),
        reused_helpers={str(HELPER):helper_hash},original_givens_and_domains=True,
        every_choice_and_reached_state=True,requested_forms_separate_from_truth=True,
        limitations=['Finite twelve-case mathematics and bounded expression forms, not all coefficient combinations or a universal CAS.',
                    'Automated prose checks establish presence/edit propagation, not natural-language truth; writer read all prose.',
                    'No native visual, clipboard, window or learner-outcome observations.'])
    write('mathematics.json',report)
    receipt=dict(format='paths_depth_subject',format_version=1,wave='wave03',subject='calculus',
        stage='ready_for_coordinator_review',published=False,source=str(SOURCE/'authoring'),
        package_id=PID,reading_id=READING,question_ids=[c['id'] for c in packet['cases']],
        questions=12,readings=1,steps=54,wrong_choices=108,source_sha256=before,
        evidence_sha256={n:sha(OUT/n) for n in ('inspection.json','routes.json','lessons.json','mathematics.json')},
        target_sha256=ready['sha256']['sorter'],model_sha256=ready['sha256']['paths_learning_document_tests'],
        inspection=str(OUT/'inspection.json'),routes=str(OUT/'routes.json'),lessons=str(OUT/'lessons.json'),
        mathematical_evidence=str(OUT/'mathematics.json'),reused_helpers={str(HELPER):helper_hash},
        remaining_concerns=['Coordinator independent mathematical/prose and aggregate/save review pending.',
                            'Native appearance and learner outcomes unobserved; prepared choices are not four-level written support.'])
    write('production.json',receipt)
    print(json.dumps({'stage':receipt['stage'],'questions':12,'steps':54,'options':162,'wrong_choices':108,
                      'controls':report['controls'],'production':str(OUT/'production.json')},indent=2))

if __name__=='__main__':main()
