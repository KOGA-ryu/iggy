"""Exact original-given certificates for both Wave 01 algebra families."""
from fractions import Fraction
import build_question_batch as batch
import export_learning as export

def req(ok, msg): export.require(ok, "production.algebra.math", msg)
def tx(v): return batch.tex(Fraction(v))
def ad(left,v): return left if v==0 else left+("+" if v>0 else "-")+tx(abs(v))
def signed(a,b,c,rev=False):
    text=ad(tx(a)+"x",b)+"="+tx(c); return "=".join(reversed(text.split("="))) if rev else text
def bracket(a,b,d,e):
    inner="x"+("+" if b>=0 else "-")+tx(abs(b)); body=tx(a)+r"\left("+inner+r"\right)"
    return ad(body,d)+"="+tx(e) if d else body+"="+tx(e)
def expanded(a,b,d,e): return ad(tx(a)+"x",a*b+d)+"="+tx(e)
def original(q):
    p=q["case"]; family=p.get("family"); req(family in ("signed_balance","distributive_linear"),"unknown family")
    keys={"signed_balance":{"family","a","b","c","d","reverse"},"distributive_linear":{"family","a","b","d","e","reverse"}}[family]
    req(set(p)==keys and all(type(p[k]) is int for k in keys if k not in ("family","reverse")) and type(p["reverse"]) is bool,"malformed original case")
    a,b,d=p["a"],p["b"],p["d"]; req(a and abs(a)<=9 and abs(b)<=9 and abs(d)<=9,"nonzero bounded multiplier required")
    if family=="signed_balance":
        right=p["c"]; x=Fraction(right-b,a); given=signed(a,b,right,p["reverse"]); after=signed(a,0,right-b); req(a*x+b==right,"signed substitution failed")
    else:
        right=p["e"]; x=Fraction(right-d,a)-b; given=bracket(a,b,d,right); after=expanded(a,b,d,right); req(a*(x+b)+d==right,"bracket substitution failed")
    return family,a,b,d,right,x,given,after
def facts(f,a,b,d,r,x): return {"family":f,"solution":str(x),"residual":"0","uniqueness":"Subtract two alleged original solutions: a(x1-x2)=0; nonzero a gives x1=x2."}
def read_notation(q):
    f,a,b,d,r,x,g,after=original(q); ans=rf"(a,b)=({tx(a)},{tx(b)})"; return g,[ans], [([ans,rf"(a,b)=({tx(-a)},{tx(b)})",rf"(a,b)=({tx(b)},{tx(a)})"],ans)],dict(facts(f,a,b,d,r,x),notation=[a,b])
def worked_check(q):
    f,a,b,d,r,x,g,after=original(q); values=[r-b,r,r+b] if f=="signed_balance" else [a*b,b,-a*b]; labels=list(map(tx,values)); return g,[after],[(labels,labels[0])],dict(facts(f,a,b,d,r,x),worked_value=str(values[0]))
def choose_next_step(q):
    f,a,b,d,r,x,g,after=original(q)
    labels=[after,signed(a,2*b,r+b),signed(a,0,r)] if f=="signed_balance" else [after,ad(tx(a)+"x",b+d)+"="+tx(r),ad(tx(a)+"x",-a*b+d)+"="+tx(r)]
    return g,[after],[(labels,labels[0])],dict(facts(f,a,b,d,r,x),goal="complete required transformation")
def explain_step(q):
    f,a,b,d,r,x,g,after=original(q)
    if f=="signed_balance":
        req(b==0,"division-inverse question requires the constant already removed")
        base=signed(a,0,r); return base+rf"\quad\Longrightarrow\quad x={tx(x)}",[base],[( [rf"\times {tx(a)}\ \text{{on both sides}}",rf"\div {tx(a)}\ \text{{on both sides}}",r"\times0\ \text{on both sides}"],rf"\times {tx(a)}\ \text{{on both sides}}")],dict(facts(f,a,b,d,r,x),inverse="multiply by a")
    req(d!=0,"outside-constant inverse needs d nonzero"); base=bracket(a,b,d,r); labels=[rf"+{tx(d)}\ \text{{on both sides}}",rf"-{tx(d)}\ \text{{on both sides}}",r"\times0\ \text{on both sides}"]
    return expanded(a,b,0,r-d)+rf"\quad\Longrightarrow\quad {base}",[base],[(labels,labels[0])],dict(facts(f,a,b,d,r,x),inverse="add d")
def repair_error(q):
    f,a,b,d,r,x,g,after=original(q)
    if f=="signed_balance":
        wrong=r+b; given=rf"\begin{{gathered}}{g}\\\begin{{aligned}}L_1 &: {tx(a)}x={tx(r)}+({tx(b)})\\L_2 &: {tx(a)}x={tx(wrong)}\\L_3 &: x={tx(Fraction(wrong,a))}\end{{aligned}}\end{{gathered}}"; values=[r-b,r+b,r]
    else:
        given=rf"\begin{{gathered}}{g}\\\begin{{aligned}}L_1 &: {ad(tx(a)+'x',b)}={tx(r)}\\L_2 &: {tx(a)}x={tx(r-b)}\\L_3 &: x={tx(Fraction(r-b,a))}\end{{aligned}}\end{{gathered}}"; values=[a*b,b,-a*b]
    labels=list(map(tx,values)); req(len(set(labels))==3,"duplicate repair values")
    return given,[r"L_1",after],[([r"L_1",r"L_2",r"L_3"],r"L_1"),(labels,labels[0])],dict(facts(f,a,b,d,r,x),first_error="L_1")
def independent(q):
    f,a,b,d,r,x,g,after=original(q); values=[x,-x,Fraction(r-d,a) if f=="distributive_linear" else Fraction(r-b)]
    residuals=[a*(v+b)+d-r if f=="distributive_linear" else a*v+b-r for v in values]; req(residuals[0]==0 and residuals[1]!=0 and residuals[2]!=0,"ambiguous independent options")
    labels=["x="+tx(v) for v in values]; return g,[labels[0]],[(labels,labels[0])],dict(facts(f,a,b,d,r,x),residuals=list(map(str,residuals)))
CHECKERS={"read_notation":read_notation,"worked_check":worked_check,"choose_next_step":choose_next_step,"explain_step":explain_step,"repair_error":repair_error,"independent":independent}
