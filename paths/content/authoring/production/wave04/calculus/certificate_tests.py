"""Exact, bounded substitution certificates over native compiler output.

Not a generator, CAS or document parser. Reuse frozen rational-polynomial
arithmetic, then add only this family's elementary outer-function identities.
The original coefficient cases, not answer labels, determine the oracle.
"""
import argparse
import copy
from datetime import datetime, timezone
from functools import lru_cache
from fractions import Fraction as Q
import hashlib
import importlib.util
import json
from math import comb
from pathlib import Path
import re

import export_learning as export

SOURCE=Path(__file__).resolve().parent
ROOT=SOURCE.parents[4]
OUT=ROOT/'build/production/wave04/calculus'
DOCS=SOURCE/'authoring/documents'
HELPER=ROOT/'content/authoring/production/wave03/calculus/certificate_tests.py'
spec=importlib.util.spec_from_file_location('frozen_integral_arithmetic',HELPER)
frozen=importlib.util.module_from_spec(spec);spec.loader.exec_module(frozen)
Poly=frozen.Poly;X=frozen.X;ZERO=frozen.ZERO
need=frozen.need;sha=frozen.sha;read=frozen.read;poly=frozen.poly
coeff=frozen.kernel.from_coefficients
PID='prod04_calc_substitution';READING=PID+'_r'
PRE=ROOT/'build/production/wave04/coordinator-preflight/calculus/addb623bd47eaafb26128aa8b33e06d8a4d2677f403f90428c6e68dda8829f8a'
CHAPTER_HASH='addb623bd47eaafb26128aa8b33e06d8a4d2677f403f90428c6e68dda8829f8a'
GOALS={'family':'Find all antiderivatives of p in x and check the original derivative.',
 'definite':'Calculate I exactly by substitution and check the original derivative and endpoint order.'}
DOMAINS={'R':'Work on R. C is an arbitrary real constant; H is a primitive in u.',
 'positive':'Work on x>0, a connected interval. C is an arbitrary real constant; H is a primitive in u. Preserve this original domain.',
 'negative':'Work on x<0, a connected interval. C is an arbitrary real constant; H is a primitive in u. Preserve this original domain.',
 'closed':'p is continuous on the closed interval between the displayed limits. Keep their order. H is a primitive in u; P(x)=H(g(x)).'}

def ps(p):return tuple(sorted(p.c.items()))
def poly(text):
    # One explicit degree-six normalization extends the frozen adapter's
    # literal exponent cap without adding another arithmetic parser.
    text=re.sub(r'([xu])\^(?:\{6\}|6)(?![0-9])',lambda m:'('+m[1]+'^3*'+m[1]+'^3)',text)
    return frozen.poly(text)
def degree(p):return len(p.coefficients())-1
def constant(p):return degree(p)==0
def compose(p,g):
    return sum((g**i*Q(v) for i,v in enumerate(p.coefficients())),Poly())
def domain_sign(p,domain):
    """Sufficient exact sign proofs; unknown is never treated as safe."""
    if constant(p):return (p.scalar()>0)-(p.scalar()<0)
    name=domain[0]
    if name in ('positive','negative'):
        values=[Q(v)*(-1 if name=='negative' and i%2 else 1) for i,v in enumerate(p.coefficients())]
        if all(v>=0 for v in values) and any(values):return 1
        if all(v<=0 for v in values) and any(values):return -1
    if name=='R':
        values=[Q(v) for v in p.coefficients()]
        if values[0] and all(not v for i,v in enumerate(values) if i%2):
            if all(v>=0 for v in values):return 1
            if all(v<=0 for v in values):return -1
    if name=='closed':
        a,b=sorted(domain[1:]);n=degree(p)
        translated=compose(p,Poly(a)+(b-a)*X)
        power=[Q(v) for v in translated.coefficients()]+[Q(0)]*(n+1)
        bernstein=[sum((power[j]*Q(comb(k,j),comb(n,j)) for j in range(k+1)),Q(0)) for k in range(n+1)]
        if all(v>0 for v in bernstein):return 1
        if all(v<0 for v in bernstein):return -1
    return 0

class Expr:
    """Finite linear combination of elementary atoms with exact poly factors."""
    def __init__(self,terms=None):self.terms={k:v for k,v in (terms or {}).items() if v.c}
    @staticmethod
    def polynomial(p):return Expr({None:p if isinstance(p,Poly) else Poly(p)})
    def __add__(self,other):
        other=other if isinstance(other,Expr) else Expr.polynomial(other)
        out=dict(self.terms)
        for k,v in other.terms.items():out[k]=out.get(k,Poly())+v
        return Expr(out)
    __radd__=__add__
    def __mul__(self,p):return Expr({k:v*p for k,v in self.terms.items()})
    __rmul__=__mul__
    def __neg__(self):return self*-1
    def __sub__(self,other):return self+-other
    def signature(self):return tuple(sorted(((repr(k),ps(v)) for k,v in self.terms.items())))
    def __eq__(self,other):return isinstance(other,Expr) and self.signature()==other.signature()
    def nonconstant_signature(self):
        out={}
        for k,p in self.terms.items():
            if k is None or constant(Poly(dict(k[1]))):p=Poly({a:v for a,v in p.c.items() if a!=ZERO})
            if p.c:out[k]=p
        return Expr(out).signature()
    def substitute(self,g):
        out=Expr()
        for k,p in self.terms.items():
            factor=compose(p,g)
            if k is None:out+=Expr.polynomial(factor)
            elif k[0]=='logprime':out+=Expr({k:factor})
            else:out+=atom(k[0],compose(Poly(dict(k[1])),g))*factor
        return out
    def derivative(self):
        out=Expr()
        for k,p in self.terms.items():
            if k is None:out+=Expr.polynomial(p.derivative());continue
            kind,arg=k[0],Poly(dict(k[1]))
            if kind=='logprime':out+=Expr({k:p.derivative()});continue
            if constant(arg):out+=atom(kind,arg)*p.derivative();continue
            out+=atom(kind,arg)*p.derivative()
            derivative_outer={'exp':('exp',1),'sin':('cos',1),'cos':('sin',-1),'logabs':('reciprocal',1)}
            need(kind in derivative_outer,'derivative outside primitive scope')
            outer,sign=derivative_outer[kind]
            out+=atom(outer,arg)*(sign*p*arg.derivative())
        return out

def atom(kind,arg):
    need(kind in ('exp','sin','cos','logabs','reciprocal'),'unsupported outer atom')
    if constant(arg):
        v=arg.scalar()
        if kind=='reciprocal':need(v!=0,'zero reciprocal');return Expr.polynomial(1/v)
        if kind=='logabs':
            need(v!=0,'log zero');v=abs(v)
            # Prime-factor log identities make ln(a/b) and ln(a)-ln(b)
            # identical here. Integers are bounded; no numeric log sampling.
            out=Expr()
            for integer,sign in ((v.numerator,1),(v.denominator,-1)):
                need(integer<=100000,'log rational outside finite scope')
                divisor=2
                while integer>1:
                    count=0
                    while integer%divisor==0:integer//=divisor;count+=1
                    if count:out+=Expr({('logprime',ps(Poly(divisor))):Poly(sign*count)})
                    divisor+=1
            return out
        if v==0:return Expr.polynomial(0 if kind=='sin' else 1)
        if kind in ('sin','cos') and v<0:return atom(kind,-arg)*(-1 if kind=='sin' else 1)
    lead=Q(arg.coefficients()[-1])
    if kind in ('sin','cos') and lead<0:return atom(kind,-arg)*(-1 if kind=='sin' else 1)
    if kind=='reciprocal':return Expr({(kind,ps(arg*(1/lead))):Poly(1/lead)})
    if kind=='logabs':return Expr({(kind,ps(arg*(1/lead))):Poly(1)})+atom('logabs',Poly(abs(lead)))
    return Expr({(kind,ps(arg)):Poly(1)})

def eval_at(expr,x):
    out=Expr()
    for k,p in expr.terms.items():
        factor=p.at(x)
        if k is None:out+=Expr.polynomial(factor)
        elif k[0]=='logprime':out+=Expr({k:Poly(factor)})
        else:out+=atom(k[0],Poly(Poly(dict(k[1])).at(x)))*factor
    return out

@lru_cache(maxsize=2048)
def mathfield(text,domain):
    """Read only bounded compiler math fields, using the frozen polynomial AST.

    Return expression, arbitrary-C coefficient, used coordinate set, validity.
    No authored key/expected label is an input to this function.
    """
    need(isinstance(text,str) and len(text)<512,'finite field size')
    s=text.replace(r'\left','').replace(r'\right','').replace(r'\,','').replace(r'\cdot','*')
    coords={v for v in 'xu' if v in s};s=s.replace('u','x')
    safe=True;atoms=[]
    def replace(kind,argtext):
        nonlocal safe
        arg=poly(argtext);need(all(not any(k[1:]) for k in arg.c),'atom argument not univariate')
        if kind in ('log','logabs','reciprocal'):
            sign=domain_sign(arg,domain)
            safe=safe and (sign>0 if kind=='log' else sign!=0)
        value=atom('logabs' if kind=='log' else kind,arg)
        need(len(atoms)<3,'too many elementary atoms')
        name='uUV'[len(atoms)];atoms.append(value);return name
    s=re.sub(r'e\^\{([^{}]+)\}|e\^([x0-9]+)',lambda m:replace('exp',m[1] or m[2]),s)
    s=re.sub(r'\\(sin|cos)\(([^()]*)\)',lambda m:replace(m[1],m[2]),s)
    s=re.sub(r'\\ln\|([^|]+)\|',lambda m:replace('logabs',m[1]),s)
    s=re.sub(r'\\ln\(([^()]*)\)',lambda m:replace('log',m[1]),s)
    def fraction(m):
        numerator,denominator=m[1],m[2]
        if 'x' not in denominator:return m[0]
        need(not any(v in numerator+denominator for v in 'uUV'),'nested function reciprocal')
        return '('+numerator+')*'+replace('reciprocal',denominator)
    s=re.sub(r'\\frac\{([^{}]+)\}\{([^{}]+)\}',fraction,s)
    parsed=poly(s);out=Expr();c=Q(0)
    for key,v in parsed.c.items():
        x,aa,cc,bb,dd=key;exponents=(aa,bb,dd)
        need(cc in (0,1) and (not cc or not(x or sum(exponents))),'nonlinear arbitrary constant')
        if cc:c+=v;continue
        need(sum(exponents)<=1,'products/powers of elementary atoms outside finite scope')
        factor=X**x*v
        if not sum(exponents):out+=Expr.polynomial(factor)
        else:
            index=exponents.index(1);need(index<len(atoms),'unknown placeholder')
            out+=atoms[index]*factor
    return out,c,coords,safe

def outer(c,arg):
    return Expr.polynomial(arg**c['power']) if c['outer']=='power' else atom(c['outer'],arg)
def primitive(c,arg,scale):
    if c['outer']=='power':return Expr.polynomial(arg**(c['power']+1)*(scale/Q(c['power']+1)))
    kind,sign={'exp':('exp',1),'cos':('sin',1),'sin':('cos',-1),'reciprocal':('logabs',1)}[c['outer']]
    return atom(kind,arg)*(scale*sign)

def original(c):
    need(c['kind'] in GOALS and c['domain'] in DOMAINS,'case kind/domain')
    for field,n,bound in (('inner',4,11),('multiplier',3,2)):
        a=c[field];need(isinstance(a,list) and 1<=len(a)<=n and a[-1]!=0 and all(type(v)is int and abs(v)<=bound for v in a),'finite original coefficients')
    g,m=coeff(c['inner']),coeff(c['multiplier'])
    need(degree(g)>0 and len(m.c)==1,'inner must vary and multiplier must be monomial')
    need(c['outer'] in ('exp','sin','cos','reciprocal','power'),'outer scope')
    if c['outer']=='power':need(type(c['power'])is int and c['power'] in (2,3),'power scope')
    gp=g.derivative();key=next(iter(gp.c));scale=m.c.get(key,Q(0))/gp.c[key]
    need(scale!=0 and scale.denominator<=15 and gp*scale==m,'exact function-derivative pair')
    domain=(c['domain'],)
    if c['kind']=='definite':
        need(c['domain']=='closed' and all(type(c[n])is int and c[n] in (0,1,2) for n in ('a','b')),'finite endpoint domain')
        domain=('closed',Q(c['a']),Q(c['b']))
    else:need(c['domain']!='closed','missing family interval')
    sign=domain_sign(g,domain)
    if c['outer']=='reciprocal':need(sign!=0,'unproved nonzero domain / pole crossing')
    p=outer(c,g)*m;H=primitive(c,X,scale);P=primitive(c,g,scale);converted=outer(c,X)*scale
    need(H.derivative()==converted and P.derivative()==p,'exact original derivative identity')
    d=dict(g=g,gp=gp,m=m,scale=scale,p=p,H=H,P=P,converted=converted,domain=domain,sign=sign)
    if c['kind']=='definite':
        a,b=Q(c['a']),Q(c['b']);ga,gb=g.at(a),g.at(b)
        value=eval_at(H,gb)-eval_at(H,ga)
        need(value==eval_at(P,b)-eval_at(P,a),'independent original endpoint evaluation')
        d.update(a=a,b=b,ga=ga,gb=gb,value=value,udomain=('closed',ga,gb))
    else:d['udomain']=('positive' if sign>0 else 'negative' if sign<0 else 'R',)
    d['goals']=['inner','du','convert','primitive','family','derivative'] if c['kind']=='family' else ['inner','du','bounds','convert','primitive','value','verify']
    src=c['source'];need(src['source_id']=='active_substitution' and all(src[n] for n in ('locator','original_givens','adaptation_kind','changes')),'missing source mapping')
    return d

def field(label,goal,d):
    """Semantic signature, mathematical truth, requested-form truth."""
    if goal=='verify':
        parts=label.split(r',\quad ');need(len(parts)==2,'verification pair')
        first=field(parts[0],'derivative',d);last=field(parts[1],'value',d)
        return ('verify',first[0],last[0]),first[1] and last[1],first[2] and last[2]
    if goal=='bounds':
        match=re.fullmatch(r'\(g\(a\),g\(b\)\)=\(([^,]+),([^,]+)\)',label)
        need(match is not None,'ordered bound syntax');pair=tuple(frozen.number(match[i]) for i in (1,2))
        return ('bounds',pair),pair==(d['ga'],d['gb']),True
    if goal=='convert':
        lhs,rhs=label.split('=',1);definite='a' in d
        need(lhs==('I' if definite else r'\int p(x)\,dx'),'integral object')
        pattern=r'(.*?)\\int_\{([^{}]+)\}\^\{([^{}]+)\}(.+)\\,d([ux])' if definite else r'(.*?)\\int(.+)\\,d([ux])'
        match=re.fullmatch(pattern,rhs);need(match is not None,'converted integral syntax')
        scale=frozen.number(match[1] or '1');coord=match[5] if definite else match[3]
        expr,c,coords,safe=mathfield(match[4] if definite else match[2],d['udomain'])
        value=expr*scale;pair=tuple(frozen.number(match[i]) for i in (2,3)) if definite else ()
        equivalent=value==d['converted'];rightbounds=not definite or pair==(d['ga'],d['gb'])
        # Reversing bounds together with the scale is mathematically equivalent.
        if definite and pair==(d['gb'],d['ga']) and value==-d['converted']:equivalent=True;rightbounds=True
        signature=('integral',value.signature(),pair,coord)
        if definite and pair[0]>pair[1]:signature=('integral',(-value).signature(),pair[::-1],coord)
        form=coord=='u' and coords<={'u'} and (not definite or pair==(d['ga'],d['gb']))
        return signature,safe and c==0 and equivalent and rightbounds,form
    lhs,rhs=label.split('=',1)
    if goal=='du':
        need(lhs=='du' and rhs.endswith(r'\,dx'),'differential object')
        rhs=rhs[:-4];value=poly(rhs)
        return ('du',ps(value)),value==d['gp'],True
    if goal=='inner':
        need(lhs=='u','inner object');value=poly(rhs)
        # Every offered nonconstant polynomial is a possible definition, but
        # the local operation specifically requests the full original inner.
        return ('inner',ps(value)),degree(value)>0,value==d['g']
    if goal=='primitive':need(lhs=='H(u)','primitive object');domain=d['udomain']
    elif goal=='family':need(lhs in ('F(x)','H(u)'),'family object');domain=d['udomain'] if lhs=='H(u)' else d['domain']
    elif goal=='derivative':need(lhs in ("F'(x)","P'(x)"),'derivative object');domain=d['domain']
    elif goal=='value':need(lhs=='I','definite value object');domain=('R',)
    else:raise ValueError('unknown local goal')
    value,c,coords,safe=mathfield(rhs,domain)
    if goal=='primitive':
        return ('primitive' if safe else 'invalid_real_domain',value.signature(),str(c)),safe and c==0 and value.derivative()==d['converted'],coords<={'u'}
    if goal=='family':
        in_u=lhs=='H(u)';target=d['H'] if in_u else d['P']
        truth=safe and value.derivative()==target.derivative()
        canonical=value.substitute(d['g']) if in_u else value
        sig=('family',canonical.nonconstant_signature()) if c else ('single',canonical.signature())
        if not safe:sig=('invalid_real_domain',sig)
        return sig,truth and c!=0,not in_u and coords<={'x'}
    if goal=='derivative':return ('function',value.signature()),safe and c==0 and value==d['p'],coords<={'x'}
    return ('value',value.signature()),safe and c==0 and value==d['value'],not coords

def given(question,c,d):
    expression=question['equation'];parts=expression.split(r',\quad ')
    need(len(parts)==(2 if c['kind']=='definite' else 1) and parts[0].startswith('p(x)='),'original given structure')
    e,k,coords,safe=mathfield(parts[0][5:],d['domain'])
    need(safe and k==0 and coords<={'x'} and e==d['p'],'original compiled integrand/domain mismatch')
    if c['kind']=='definite':
        m=re.fullmatch(r'I=\\int_\{([^{}]+)\}\^\{([^{}]+)\}p\(x\)\\,dx',parts[1])
        need(m and tuple(frozen.number(m[i]) for i in (1,2))==(d['a'],d['b']),'original endpoint/order mismatch')
    need(question['description']==GOALS[c['kind']]+' '+DOMAINS[c['domain']],'compiled goal/original domain changed')

def check(routes,packet):
    need(packet['format']=='paths_substitution_cases' and packet['version']==1 and packet['coefficient_order']=='ascending','case contract')
    ids=[PID+f'_q{i:02}' for i in range(1,13)]
    need(routes['accepted'] is True and routes['save_replay'] is True and routes['routes']==12 and routes['windows']==0,'native route receipt')
    need(routes['question_ids']==ids and [c['id'] for c in packet['cases']]==ids,'case/route coverage')
    total=0;choices=0;first=[0,0,0];rows=[]
    for qi,(row,c) in enumerate(zip(routes['questions'],packet['cases'])):
        q=row['question'];d=original(c);given(q,c,d)
        need(row['id']==q['id']==c['id'] and q['skill']=='document_choices','question identity')
        need(c['group']==('introductory' if qi<4 else 'practice' if qi<8 else 'mixed'),'coverage groups')
        need(c['kind']==('family' if qi<8 else 'definite'),'request groups')
        steps=q['steps'];states=q['working_states'];need(len(steps)==len(d['goals']) and len(states)==len(steps)+1,'complete route/state count')
        need(states[0]['display']==q['equation'],'original working state')
        step_results=[]
        for si,(step,goal) in enumerate(zip(steps,d['goals'])):
            options=step['options'];key=step['accepted_option_ids']
            need(len(options)==3 and len(key)==1 and len({o['id'] for o in options})==3,'one-of-three option structure')
            need(step['prompt'] and step['explanation'] and step['wrong_hint'],'missing teaching field')
            results=[field(o['label'],goal,d) for o in options]
            # A correct u-family is mathematically the same family as its
            # x-version but is distinguished by the explicit coordinate goal.
            signatures=[(r[0],r[2]) for r in results]
            need(len(set(signatures))==3,f'semantic duplicate: {c["id"]}/{step["id"]}')
            correct=[o['id'] for o,r in zip(options,results) if r[1] and r[2]]
            need(correct==key,f'false key or nonunique goal-correct answer: {c["id"]}/{step["id"]}')
            sem=step['semantics'];need(sem['before']==states[si]['id'] and sem['after']==states[si+1]['id'],'state route linkage')
            after=field(states[si+1]['display'],goal,d)
            need(after[1] and after[2],f'false reached state: {c["id"]}/{step["id"]}')
            wrong=[o for o in options if o['id'] not in key]
            need(len({o.get('wrong_feedback','') for o in wrong})==2 and all(o.get('wrong_feedback') for o in wrong),'individual misconception feedback')
            if si==0:first[next(i for i,o in enumerate(options) if o['id'] in key)]+=1
            step_results.append({'step_id':step['id'],'goal':goal,'options':[{'id':o['id'],'mathematically_true':r[1],'requested_form':r[2]} for o,r in zip(options,results)],'reached_state_verified':True})
            total+=1;choices+=3
        record={'id':c['id'],'inner':d['g'].coefficients(),'inner_derivative':d['gp'].coefficients(),'multiplier':d['m'].coefficients(),'lambda':str(d['scale']),
            'domain':c['domain'],'nonzero_argument_sign':d['sign'],'primitive_derivative_equals_original':True,'steps':step_results}
        if c['kind']=='definite':record.update(transformed_bounds=[str(d['ga']),str(d['gb'])],exact_value=repr(d['value'].signature()),original_endpoint_check=True)
        else:record['completeness']='Derivative identity on the stated connected interval; mean value theorem implies every primitive differs by one arbitrary real constant.'
        rows.append(record)
    need(total==76 and choices==228 and routes['wrong_choices']==152 and first==[4,4,4],'coverage/position totals')
    need(len({c['source']['locator'] for c in packet['cases']})==10,'ten distinct source seed prompts')
    return dict(accepted=True,questions=12,decisions=total,options=choices,wrong_choices=152,first_key_positions=first,cases=rows)

def changed(routes,qi,si,text):
    out=copy.deepcopy(routes);q=out['questions'][qi]['question'];s=q['steps'][si]
    next(o for o in s['options'] if o['id'] in s['accepted_option_ids'])['label']=text
    q['working_states'][si+1]['display']=text
    return out

def controls(routes,packet):
    counts={'false_keys':0,'false_states':0,'semantic_duplicates':0,'invalid_domains_inputs':0,'true_wrong_form':0,'valid_alternatives':0}
    def reject(r,p=packet,reason=None):
        try:check(r,p)
        except ValueError as e:
            if reason:need(reason in str(e),f'wrong rejection reason: {e}')
            return
        raise ValueError('falsification control accepted')
    for qi,row in enumerate(routes['questions']):
        for si,s in enumerate(row['question']['steps']):
            wrong=next(o for o in s['options'] if o['id'] not in s['accepted_option_ids'])
            bad=copy.deepcopy(routes);bad['questions'][qi]['question']['steps'][si]['accepted_option_ids']=[wrong['id']]
            reject(bad);counts['false_keys']+=1
            bad=copy.deepcopy(routes);bad['questions'][qi]['question']['working_states'][si+1]['display']=wrong['label']
            reject(bad);counts['false_states']+=1
            label=next(o['label'] for o in s['options'] if o['id'] in s['accepted_option_ids'])
            # Nonliteral but semantically identical rational factors/C shifts.
            if '+C' in label:duplicate=label.replace('+C','+5-2C')
            elif '=u' in label:duplicate=label.replace('=u','=1u')
            else:duplicate=label.replace('=', '= ',1)
            bad=copy.deepcopy(routes);bs=bad['questions'][qi]['question']['steps'][si]
            next(o for o in bs['options'] if o['id'] not in bs['accepted_option_ids'])['label']=duplicate
            # Most whitespace variants are parsed semantically; bound syntax
            # is deliberately exact and gets an exact-label duplicate instead.
            if si==2 and qi>=8:next(o for o in bs['options'] if o['id'] not in bs['accepted_option_ids'])['label']=label
            reject(bad,reason='semantic duplicate');counts['semantic_duplicates']+=1
    for qi in range(12):
        for field_name,value in [('domain','complex'),('inner',[0,1,0,0,1]),('multiplier',[1,1]),('inner',[True,3])]:
            bad=copy.deepcopy(packet);bad['cases'][qi][field_name]=value;reject(routes,bad);counts['invalid_domains_inputs']+=1
        bad=copy.deepcopy(routes);bad['questions'][qi]['question']['description']+=' Extended domain.'
        reject(bad);counts['invalid_domains_inputs']+=1
    for qi,domain in [(6,'R'),(7,'R')]:
        bad=copy.deepcopy(packet);bad['cases'][qi]['domain']=domain
        reject(routes,bad,reason='pole crossing');counts['invalid_domains_inputs']+=1
    need(domain_sign(poly('x-1'),('closed',Q(0),Q(2)))==0,'pole-crossing control')
    counts['explicit_closed_pole_crossing']=1
    # A semantically valid substituted family is not a returned x-family.
    wrong_forms=[(0,0,'u=x'),(2,4,r'H(u)=-\frac{1}{28}u^4+C'),(4,4,r'H(u)=\frac{1}{2}e^u+C'),
                 (9,0,'u=2x-3'),(9,3,r'I=\frac{1}{2}\int_{1}^{3}u^2\,du')]
    for qi,si,label in wrong_forms:
        d=original(packet['cases'][qi]);truth=field(label,d['goals'][si],d)
        need(truth[1] and not truth[2],'control must be true but wrong requested form')
        reject(changed(routes,qi,si,label));counts['true_wrong_form']+=1
    negatives=[(0,4,r'F(x)=\frac{1}{3}e^{3x}'),(7,4,r'F(x)=\frac{1}{11}\ln(11x-9)+C'),
               (8,3,r'I=\frac{1}{8}\int_{1}^{2}\frac{1}{u}\,du'),(9,3,r'I=-\frac{1}{2}\int_{0}^{1}u^2\,du'),
               (5,4,r'F(x)=\frac{1}{24}(4x^2)^3+C')]
    for qi,si,label in negatives:reject(changed(routes,qi,si,label))
    counts['missing_C_invalid_log_mixed_bounds_false_backsub']=len(negatives)
    alternatives=[(0,4,r'F(x)=\frac{1}{3}e^{3x}+5-2C'),
        (1,4,r'F(x)=-\frac{1}{5}\sin(-5x-1)+C'),
        (2,4,r'F(x)=-\frac{1}{28}(7x-2)^4+C'),
        (5,4,r'F(x)=\frac{8}{3}x^6+14x^4+\frac{49}{2}x^2+C'),
        (6,4,r'F(x)=\frac{1}{15}\ln|x^3+\frac{1}{5}|+C'),
        (7,4,r'F(x)=\frac{1}{11}\ln(9-11x)+C'),
        (8,5,r'I=\frac{1}{8}\ln(17/5)'),
        (9,3,r'I=-\frac{2}{4}\int_{3}^{1}u^2\,du'),
        (9,5,r'I=\frac{26}{6}'),(10,5,r'I=\frac{1}{2}e^1-\frac{1}{2}e^0'),
        (11,5,r'I=\frac{1}{2}(\sin(1)-\sin(0))')]
    for qi,si,label in alternatives:check(changed(routes,qi,si,label),packet);counts['valid_alternatives']+=1
    # General C changes and alternate log syntax must also reject as competing
    # answers, not only accept when they replace the correct answer.
    for qi,si,label in alternatives[:8]:
        bad=copy.deepcopy(routes);step=bad['questions'][qi]['question']['steps'][si]
        next(o for o in step['options'] if o['id'] not in step['accepted_option_ids'])['label']=label
        reject(bad,reason='semantic duplicate');counts['semantic_duplicates']+=1
    return counts

def actual_edits(routes,packet):
    raw=(DOCS/'chapter.paths.md').read_text();s=routes['questions'][0]['question']['steps'][0]
    wrong=next(o for o in s['options'] if o.get('wrong_feedback'))
    probes=[('prompt','@step 10 | '+s['prompt'],'Identify the whole argument first. '+s['prompt']),
            ('wrong_feedback',f"@feedback {wrong['id']} | "+wrong['wrong_feedback'],'Check the requested coordinate. '+wrong['wrong_feedback'])]
    results=[]
    def retain(path,data):
        path.parent.mkdir(parents=True,exist_ok=True)
        if path.exists():need(path.read_bytes()==data,'retained edit artifact changed')
        else:path.write_bytes(data)
    for field_name,old,new in probes:
        need(old in raw,'missing real Markdown target')
        text=raw.replace(old,old.split(' | ',1)[0]+' | '+new,1)
        edit_hash=hashlib.sha256(text.encode()).hexdigest()
        path=OUT/'edit-probes'/field_name/edit_hash
        changed_source=path/'documents/chapter.paths.md'
        retain(changed_source,text.encode())
        actual=frozen.command(ROOT/'b/paths_learning_document_tests','--question-batch',changed_source.parent);check(actual,packet)
        expected=copy.deepcopy(routes);step=expected['questions'][0]['question']['steps'][0]
        json_path='/questions/0/question/steps/0/'
        if field_name=='prompt':
            step['prompt']=new;json_path+='prompt';old_value=s['prompt']
        else:
            option_index=next(i for i,o in enumerate(step['options']) if o['id']==wrong['id'])
            step['options'][option_index]['wrong_feedback']=new
            json_path+=f'options/{option_index}/wrong_feedback';old_value=wrong['wrong_feedback']
        need(actual==expected,'scratch edit changed unintended compiled field')
        replay=path/'routes.json';diff=path/'compiled-diff.json'
        retain(replay,export.encoded(actual))
        retain(diff,export.encoded({'format':'paths_one_field_edit_diff','field':field_name,'changes':[{'json_pointer':json_path,'before':old_value,'after':new}],
            'source_before_sha256':sha(DOCS/'chapter.paths.md'),'source_after_sha256':edit_hash,
            'baseline_routes_sha256':hashlib.sha256(export.encoded(routes)).hexdigest(),
            'edited_routes_sha256':sha(replay),'only_intended_compiled_field_changed':True,'mathematics_unchanged':True}))
        artifacts={str(p.relative_to(OUT)):sha(p) for p in (changed_source,replay,diff)}
        results.append(dict(field=field_name,source_before=sha(DOCS/'chapter.paths.md'),source_after=edit_hash,
            only_intended_compiled_field_changed=True,mathematics_unchanged=True,retained_artifacts=artifacts))
    return results

def source_hashes():
    return {str(p.relative_to(SOURCE)):sha(p) for p in sorted(SOURCE.rglob('*')) if p.is_file() and '__pycache__' not in p.parts}
def write(name,value):(OUT/name).write_bytes(export.encoded(value))

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--routes',type=Path,required=True);args=parser.parse_args()
    before=source_hashes();need(sha(DOCS/'chapter.paths.md')==CHAPTER_HASH,'reviewed chapter changed')
    ready=read(ROOT/'build/production/wave01/build-ready.json')
    for name in ('sorter','paths_learning_document_tests'):need(sha(ROOT/'b'/name)==ready['sha256'][name],'binary pin mismatch')
    dependencies={str(p):sha(p) for p in (HELPER,frozen.HELPER,ROOT/'tools/export_learning.py')}
    packet=read(SOURCE/'cases.json');routes=read(args.routes);report=check(routes,packet)
    report['controls']=controls(routes,packet)
    report['actual_markdown_edits']=actual_edits(routes,packet)
    # Fresh native parse binds the unchanged source to the reused preflight
    # routes; no build, publication or active store is involved.
    actual=frozen.command(ROOT/'b/paths_learning_document_tests','--question-batch',DOCS)
    need(routes==actual==read(PRE/'routes.json'),'stale native routes')
    inspection=export.Target(ROOT/'b/sorter').inspect(documents=DOCS)
    metadata=read(SOURCE/'authoring/authoring.json')
    export.provenance(metadata,inspection['entities'])
    need(inspection==read(PRE/'inspection.json'),'stale native inspection')
    lessons=read(PRE/'lessons.json')
    need(lessons['accepted'] and lessons['questions']==12 and lessons['readings']==1 and lessons['windows']==0 and lessons['independent_worked_disclosures']==3,'lesson preflight failed')
    need(frozen.command(ROOT/'b/paths_learning_document_tests','--family-lessons',DOCS)==lessons,'stale disclosure evidence')
    need(all(e['links']==[READING] for e in inspection['entities'] if e['kind']=='question'),'reading links')
    need([(e['id'],e['links']) for e in inspection['entities'] if e['kind']=='lesson']==[(READING,[c['id'] for c in packet['cases']])],'ordered lesson practice links')
    # Independently solve the distinct worked example, not a source answer key.
    wg=poly('x^2+2');wp=atom('sin',wg)*(2*X);wP=-atom('cos',wg)
    need(wP.derivative()==wp,'worked original derivative')
    wvalue=eval_at(wP,Q(1))-eval_at(wP,Q(0))
    need(wvalue==atom('cos',Poly(2))-atom('cos',Poly(3)),'worked endpoints')
    report['worked_example']={'inner':['2','0','1'],'inner_derivative':['0','2'],'lambda':'1','primitive':'-cos(x^2+2)+C','bounds':['2','3'],'integral':'cos(2)-cos(3)','original_identity_and_endpoints':True}
    registry=ROOT/'content/authoring/production/wave04/SOURCES.json';entry=next(s for s in read(registry)['sources'] if s['id']=='active_substitution')
    snapshots={str(Path(entry[n])):sha(Path(entry[n])) for n in ('local_snapshot','local_text')}
    need(snapshots[entry['local_snapshot']]==entry['download']['sha256'] and snapshots[entry['local_text']]==entry['download']['text_sha256'],'pinned source bytes changed')
    credit=metadata['sources'][0]
    need(metadata['package_id']==PID and credit['kind']=='adapted' and credit['uri']==entry['source_url'],'adaptation provenance identity')
    need(credit['content_ids']==[READING]+[c['id'] for c in packet['cases']] and entry['license_url'] in credit['attribution'],'license/content coverage')
    manual=ROOT/'build/production/wave04/manual-review-progress.json'
    source_review=ROOT/'build/production/wave04/source-review.json'
    reviewed=next(v for v in read(manual)['subjects'] if v['subject']=='calculus')
    seeds_review=next(v for v in read(source_review)['subjects'] if v['subject']=='calculus')
    need(reviewed['chapter_sha256']==CHAPTER_HASH and seeds_review['cases_sha256']==sha(SOURCE/'cases.json'),'preliminary review hash mismatch')
    reused_review={str(p):sha(p) for p in (manual,source_review,ROOT/'build/production/wave04/independent-facts.json',PRE/'preflight.json',PRE/'inspection.json',PRE/'routes.json',PRE/'lessons.json')}
    need(before==source_hashes() and all(sha(Path(p))==h for p,h in dependencies.items()),'source/dependency changed during checks')
    for name in ('sorter','paths_learning_document_tests'):need(sha(ROOT/'b'/name)==ready['sha256'][name],'binary changed during checks')
    report.update(source_sha256_before=before,source_sha256_after=source_hashes(),reused_helpers=dependencies,source_snapshots=snapshots,
        reused_coordinator_evidence=reused_review,provenance_verified=True,ordered_practice_and_reading_links=True,
        source_registry_sha256=sha(registry),requested_forms_separate_from_truth=True,original_givens_and_domains=True,every_option_and_reached_state=True,
        limitations=['Finite elementary grammar and sufficient exact domain proofs; not a general CAS, all trigonometric identities, or exhaustive coefficient-family theorem.',
          'Polynomial and reciprocal identities are exact; constant logs use prime factorization. Unhandled expressions fail closed.',
          'Presence/edit propagation of teaching is automated; natural-language correctness relies on writer and hash-matched coordinator review.',
          'No visual, window, clipboard or learner-outcome observations.'])
    write('inspection.json',inspection);write('routes.json',routes);write('lessons.json',lessons);write('mathematics.json',report)
    phases=read(OUT/'phases.json')
    phases['checks_done_utc']=datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M:%S UTC')
    phases['checks_done_basis']='Observed checker completion timestamp after native, mathematics, mutation, provenance and hash-stability gates.'
    write('phases.json',phases)
    need((SOURCE/'review.md').is_file(),'missing final review')
    receipt=dict(format='paths_depth_subject',format_version=1,wave='wave04',subject='calculus',stage='ready_for_coordinator_review',published=False,
        source=str(SOURCE/'authoring'),package_id=PID,package_version=1,reading_id=READING,question_ids=[c['id'] for c in packet['cases']],
        questions=12,readings=1,steps=76,wrong_choices=152,source_questions=12,distinct_seed_prompts=10,
        source_registry_sha256=sha(registry),content_license=entry['license'],content_license_url=entry['license_url'],
        source_sha256=source_hashes(),evidence_sha256={str(p.relative_to(OUT)):sha(p) for p in sorted(OUT.rglob('*')) if p.is_file() and p.name!='production.json'},
        target_sha256=ready['sha256']['sorter'],model_sha256=ready['sha256']['paths_learning_document_tests'],
        inspection=str(OUT/'inspection.json'),routes=str(OUT/'routes.json'),lessons=str(OUT/'lessons.json'),mathematical_evidence=str(OUT/'mathematics.json'),
        reused_helpers=dependencies,source_snapshots=snapshots,reused_coordinator_evidence=reused_review,phase_measurements=phases,
        remaining_concerns=['Final coordinator capture and certificate review, aggregate import and preserved progress gates remain before publication.',
          'Finite expression/domain certificates and image-free native gates do not establish arbitrary-expression equivalence, visual appearance, or learner outcomes.',
          'No token or weekly-usage saving was measured; the observed wall-clock span includes a recorded usage-limit interruption.'])
    write('production.json',receipt)
    print(json.dumps({'stage':receipt['stage'],'questions':12,'decisions':76,'options':228,'controls':report['controls'],
        'production':str(OUT/'production.json'),'production_sha256':sha(OUT/'production.json')},indent=2))

if __name__=='__main__':main()
