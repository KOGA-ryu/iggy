"""Exact subject-math checks of compiler output; never generates learner Markdown."""
import argparse
import ast
import copy
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

SOURCE=Path(__file__).resolve().parent
ROOT=SOURCE.parents[4]
EVIDENCE=ROOT/'build/production/wave02/calculus'
VARIABLES='xuvUV'
ZERO=(0,)*5

def require(ok,message):
    if not ok: raise ValueError(message)

class Poly:
    """Exact sparse coefficients; five independent symbols only for rule checks."""
    def __init__(self,value=0):
        data=value if isinstance(value,dict) else {ZERO:Fraction(value)}
        self.c={k:Fraction(v) for k,v in data.items() if v}
    def __add__(self,other):
        other=other if isinstance(other,Poly) else Poly(other)
        d=dict(self.c)
        for k,v in other.c.items(): d[k]=d.get(k,0)+v
        return Poly(d)
    __radd__=__add__
    def __neg__(self): return Poly({k:-v for k,v in self.c.items()})
    def __sub__(self,other): return self+-other if isinstance(other,Poly) else self+(-Poly(other))
    def __rsub__(self,other): return Poly(other)+(-self)
    def __mul__(self,other):
        other=other if isinstance(other,Poly) else Poly(other)
        d={}
        for a,v in self.c.items():
            for b,w in other.c.items():
                k=tuple(x+y for x,y in zip(a,b));d[k]=d.get(k,0)+v*w
        return Poly(d)
    __rmul__=__mul__
    def scalar(self):
        require(all(k==ZERO for k in self.c),'Expected a number, not a function')
        return self.c.get(ZERO,Fraction(0))
    def __pow__(self,n):
        n=n.scalar() if isinstance(n,Poly) else Fraction(n)
        require(n.denominator==1 and 0<=n<=8,'Unsupported polynomial exponent')
        out=Poly(1)
        for _ in range(int(n)): out=out*self
        return out
    def __eq__(self,other): return isinstance(other,Poly) and self.c==other.c
    def derivative(self):
        return Poly({(k[0]-1,*k[1:]):k[0]*v for k,v in self.c.items() if k[0]})
    def at(self,x0):
        require(all(not any(k[1:]) for k in self.c),'Expected a polynomial in x only')
        return sum((v*Fraction(x0)**k[0] for k,v in self.c.items()),Fraction(0))
    def coefficients(self):
        require(all(not any(k[1:]) for k in self.c),'Not univariate')
        degree=max((k[0] for k in self.c),default=0)
        return [str(self.c.get((i,0,0,0,0),0)) for i in range(degree+1)]

def variable(name):
    k=[0]*5;k[VARIABLES.index(name)]=1;return Poly({tuple(k):1})

X=variable('x')

def polynomial(text,n=2):
    """Math-only normalization of our bounded TeX polynomial expressions.

    Input is a compiler-provided expression, never a Markdown document. Parse
    only exact integer arithmetic, known variables, +, -, products and powers.
    No floats, function calls, attribute access or sample-point comparison.
    """
    s=text.replace('\\left','').replace('\\right','').replace('\\cdot','*')
    s=s.replace("u'",'U').replace("v'",'V').replace('{','(').replace('}',')')
    s=re.sub(r'\s+','',s)
    tokens=re.findall(r'\d+|[xuvUVn()+*^\-]',s)
    require(''.join(tokens)==s,'Unsupported mathematical syntax: '+text)
    out=[]
    for i,t in enumerate(tokens):
        if i and (tokens[i-1].isdigit() or tokens[i-1] in VARIABLES+'n)') and (t.isdigit() or t in VARIABLES+'n('): out.append('*')
        out.append('**' if t=='^' else t)
    tree=ast.parse(''.join(out),mode='eval')
    env={s:variable(s) for s in VARIABLES};env['n']=Poly(n)
    def visit(node):
        if isinstance(node,ast.Constant) and type(node.value) is int: return Poly(node.value)
        if isinstance(node,ast.Name) and node.id in env: return env[node.id]
        if isinstance(node,ast.UnaryOp) and isinstance(node.op,(ast.UAdd,ast.USub)):
            p=visit(node.operand);return -p if isinstance(node.op,ast.USub) else p
        if isinstance(node,ast.BinOp):
            a,b=visit(node.left),visit(node.right)
            if isinstance(node.op,ast.Add): return a+b
            if isinstance(node.op,ast.Sub): return a-b
            if isinstance(node.op,ast.Mult): return a*b
            if isinstance(node.op,ast.Pow): return a**b
        raise ValueError('Unsupported math operation')
    return visit(tree.body)

def from_coefficients(values):
    return Poly({(i,0,0,0,0):v for i,v in enumerate(values)})

def original(case):
    require(case['kind'] in ('chain','product','mixed'),'Unknown case kind')
    require(type(case['x0']) is int and case['x0'] in (-2,-1,0,1,2),'Invalid input domain')
    for name in ('u','v'):
        if name in case:
            a=case[name]
            require(isinstance(a,list) and 2<=len(a)<=3 and a[-1]!=0 and all(type(v) is int and abs(v)<=5 for v in a),'Invalid coefficient domain')
    u=from_coefficients(case['u']);v=from_coefficients(case.get('v',[1]))
    if case['kind']=='product': require(len(case['u'])<=3 and len(case['v'])==2,'Invalid product degree')
    else: require(len(case['u'])==2 and type(case['n']) is int and case['n'] in (2,3,4),'Invalid affine-power input')
    if case['kind']=='mixed': require(len(case['v'])==2,'Invalid second factor')
    # Expand the ORIGINAL coefficients first; differentiate the resulting
    # coefficients, never the chosen derivative or displayed product rule.
    f=u if case['kind']=='product' else u**case['n']
    if case['kind']!='chain': f=f*v
    require(len(f.coefficients())<=6,'Expanded degree exceeds five')
    d=f.derivative();x0=case['x0'];y0=f.at(x0);m=d.at(x0)
    line=Poly(y0-m*x0)+m*X
    require(line.at(x0)==f.at(x0) and line.derivative()==Poly(d.at(x0)),'Independent tangent check failed')
    return u,v,f,d,y0,m,line

def field(text,kind,case):
    require('=' in text,'Missing equality')
    left,*right=text.split('=');n=case.get('n',2)
    if kind in ('factors','values'):
        expected="(u',v')" if kind=='factors' else f"(f({case['x0']}),f'({case['x0']}))"
        require(left==expected and len(right)==1,'Wrong evaluated objects or factor labels')
        rhs=right[0];require(rhs.startswith('(') and rhs.endswith(')'),'Expected ordered pair')
        pieces=rhs[1:-1].split(',');require(len(pieces)==2,'Expected two entries')
        return tuple(polynomial(p,n) for p in pieces)
    require(left=={'method':"f'",'derivative':"f'(x)",'tangent':'y'}[kind],'Wrong mathematical object')
    values=[polynomial(r,n) for r in right]
    require(all(v==values[0] for v in values),'False intermediate equality')
    return values[0]

def check(routes,packet):
    require(routes.get('accepted') is True,'Native routes not accepted')
    require(packet['local_goals']=={'chain':['method','derivative','values','tangent'],'product':['method','factors','derivative','values','tangent'],'mixed':['method','factors','derivative','values','tangent']},'Incomplete local-goal contract')
    cases=packet['cases'];require(len(cases)==12,'Need twelve original cases')
    require([c['group'] for c in cases]==['introductory']*4+['practice']*4+['mixed']*4,'Invalid coverage groups')
    require(sum(c['kind'] in ('chain','mixed') for c in cases)>=4 and sum(c['kind'] in ('product','mixed') for c in cases)>=4 and sum(c['kind']=='mixed' for c in cases)>=2,'Missing required mathematical coverage')
    expected_ids=[f'prod02_calc_derivative_tangent_q{i:02}' for i in range(1,13)]
    require([c['id'] for c in cases]==expected_ids,'Incorrect case identities/order')
    actual={w['id']:w['question'] for w in routes['questions']}
    require(set(actual)==set(expected_ids),'Unexpected compiled question identities')
    proof=[];positions=[];step_count=0
    for case in cases:
        u,v,f,d,y0,m,line=original(case);q=actual[case['id']]
        match=re.fullmatch(r'f\(x\)=(.+),\\quad x_0=(-?\d+)',q['equation'])
        require(match is not None and int(match[2])==case['x0'] and polynomial(match[1])==f,case['id']+': original given differs')
        require(q['description'].endswith(packet['domain']),case['id']+': domain differs')
        kinds=packet['local_goals'][case['kind']]
        require(len(q['steps'])==len(kinds) and len(q['working_states'])==len(kinds)+1,'Incomplete route')
        require(q['working_states'][0]['display']==q['equation'],'Initial working differs from original')
        U,V=variable('U'),variable('V');a,b=variable('u'),variable('v');n=case.get('n',2)
        methods={'chain':n*a**(n-1)*U,'product':U*b+a*V,'mixed':n*a**(n-1)*U*b+a**n*V}
        expected={'method':methods[case['kind']],'factors':(u.derivative(),v.derivative()),'derivative':d,'values':(Poly(y0),Poly(m)),'tangent':line}
        for i,(step,kind) in enumerate(zip(q['steps'],kinds)):
            opts=step['options'];require(len(opts)==3,'Exactly three choices required')
            values=[field(o['label'],kind,case) for o in opts]
            require(all(values[j]!=values[k] for j in range(3) for k in range(j)),'Equivalent option duplicate: '+case['id'])
            good=[o['id'] for o,val in zip(opts,values) if val==expected[kind]]
            require(len(good)==1 and good==step['accepted_option_ids'],case['id']+': false key at '+kind)
            require(field(q['working_states'][i+1]['display'],kind,case)==expected[kind],case['id']+': false reached work at '+kind)
            wrong=[o.get('wrong_feedback','') for o in opts if o['id'] not in good]
            require(all(wrong) and len(set(wrong))==2,'Missing or generic duplicate feedback')
            require(step['prompt'] and step['explanation'] and step['wrong_hint'],'Missing teaching field')
            if i==0: positions.append(next(j+1 for j,o in enumerate(opts) if o['id']==good[0]))
            step_count+=1
        proof.append({'id':case['id'],'expanded_original_coefficients':f.coefficients(),'derivative_coefficients':d.coefficients(),'x0':case['x0'],'f_x0':str(y0),'derivative_x0':str(m),'tangent_coefficients':line.coefficients(),'incidence_and_slope':True,'decisions':len(kinds)})
    require(sorted(positions)==[1]*4+[2]*4+[3]*4,'First-position imbalance')
    return {'accepted':True,'questions':12,'steps':step_count,'wrong_choices':2*step_count,'first_positions':positions,'cases':proof}

def rejection_tests(routes,packet):
    rejected=0
    def reject(data,p=packet):
        nonlocal rejected
        try: check(data,p)
        except (ValueError,SyntaxError): rejected+=1
        else: raise AssertionError('Invalid mutation accepted')
    def transform(label,duplicate=False):
        lhs,rhs=label.split('=',1)
        if lhs.startswith('('):
            a,b=rhs[1:-1].split(',')
            return lhs+'=('+(f'1*({a})' if duplicate else f'({a})+1')+','+b+')'
        rhs=rhs.split('=')[-1]
        return lhs+'='+(f'1*({rhs})' if duplicate else f'({rhs})+1')
    for qi,w in enumerate(routes['questions']):
        for si,s in enumerate(w['question']['steps']):
            bad=copy.deepcopy(routes);bs=bad['questions'][qi]['question']['steps'][si]
            bs['accepted_option_ids']=[next(o['id'] for o in bs['options'] if o['id'] not in s['accepted_option_ids'])];reject(bad)
            bad=copy.deepcopy(routes);state=bad['questions'][qi]['question']['working_states'][si+1]
            state['display']=transform(state['display']);reject(bad)
            bad=copy.deepcopy(routes);bs=bad['questions'][qi]['question']['steps'][si]
            good=next(o['label'] for o in bs['options'] if o['id'] in bs['accepted_option_ids'])
            next(o for o in bs['options'] if o['id'] not in bs['accepted_option_ids'])['label']=transform(good,True);reject(bad)
    bad=copy.deepcopy(routes);bad['questions'][0]['question']['description']='Complex inputs only.';reject(bad)
    p=copy.deepcopy(packet);p['cases'][0]['x0']=3;reject(routes,p)
    p=copy.deepcopy(packet);p['cases'][0]['u']=[1,0];reject(routes,p)
    bad=copy.deepcopy(routes);bad['questions'][0]['question']['equation']=r'f(x)=(2x+1)^2+(x-1)^2,\quad x_0=1';reject(bad)
    bad=copy.deepcopy(routes);bad['questions'][0]['question']['working_states'][2]['display']="f'(x)=8x+4+(x-1)";reject(bad)
    return {'rejected':rejected,'false_keys':56,'false_intermediates':56,'equivalent_options':56,'invalid_domain_or_case':3,'same_point_but_different_polynomial':2}

def source_hashes():
    names=['DESIGN.md','cases.json','certificate_tests.py','authoring/authoring.json','authoring/documents/chapter.paths.md']
    return {n:hashlib.sha256((SOURCE/n).read_bytes()).hexdigest() for n in names}

def actual_edit_tests(routes,packet,model):
    document=(SOURCE/'authoring/documents/chapter.paths.md').read_text()
    q=routes['questions'][0]['question'];step=q['steps'][0]
    wrong=next(o for o in step['options'] if 'wrong_feedback' in o)
    fixtures=[('prompt','@step 10 | '+step['prompt'],step['prompt']+' Recheck the original structure.'),
              ('feedback','@feedback '+str(wrong['id'])+' | '+wrong['wrong_feedback'],'Review the inner expression. '+wrong['wrong_feedback'])]
    results=[]
    start=document.index('@question '+q['id']+' | ')
    stop=document.find('\n@question ',start+1)
    stop=len(document) if stop<0 else stop
    block=document[start:stop]
    for kind,needle,new_text in fixtures:
        replacement=needle.split(' | ',1)[0]+' | '+new_text
        require(block.count(needle)==1,'Scratch edit target ambiguous within question')
        with tempfile.TemporaryDirectory(prefix='markdown-'+kind+'-',dir=EVIDENCE) as temporary:
            docs=Path(temporary)/'documents';docs.mkdir();(docs/'chapter.paths.md').write_text(document[:start]+block.replace(needle,replacement,1)+document[stop:])
            proc=subprocess.run([str(model),'--question-batch',str(docs)],capture_output=True,text=True,timeout=60)
            require(proc.returncode==0,proc.stderr or proc.stdout);edited=json.loads(proc.stdout);check(edited,packet)
            target=edited['questions'][0]['question'];expected=copy.deepcopy(q)
            if kind=='prompt': expected['steps'][0]['prompt']=new_text
            else: next(o for o in expected['steps'][0]['options'] if o['id']==wrong['id'])['wrong_feedback']=new_text
            require(target==expected,'Real edit changed unexpected question content')
            require(edited['questions'][1:]==routes['questions'][1:],'Real edit changed another question')
            results.append({'kind':kind,'compiled_corresponding_field_changed':True,'mathematics_and_other_fields_unchanged':True,'exit_code':proc.returncode})
    return results

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--routes',type=Path,required=True);args=parser.parse_args()
    EVIDENCE.mkdir(parents=True,exist_ok=True);before=source_hashes()
    packet=json.loads((SOURCE/'cases.json').read_text());routes=json.loads(args.routes.read_text())
    # Small exact-kernel identities independent of the twelve authored answers.
    require((X+1)**2==X**2+2*X+1 and ((X+1)**3).derivative()==3*X**2+6*X+3,'Polynomial kernel regression')
    report=check(routes,packet);report['mutations']=rejection_tests(routes,packet)
    model=ROOT/'b/paths_learning_document_tests';target=ROOT/'b/sorter'
    ready=json.loads((ROOT/'build/production/wave01/build-ready.json').read_text())
    for path in (model,target): require(hashlib.sha256(path.read_bytes()).hexdigest()==ready['sha256'][path.name],'Changed executable')
    report['actual_markdown_edits']=actual_edit_tests(routes,packet,model)
    require(before==source_hashes(),'Authoring source changed during tests')
    report.update(source_sha256=before,model_sha256=ready['sha256'][model.name],target_sha256=ready['sha256'][target.name],routes_sha256=hashlib.sha256(args.routes.read_bytes()).hexdigest())
    (EVIDENCE/'mathematical.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('cases','source_sha256')},indent=2))

if __name__=='__main__': main()
