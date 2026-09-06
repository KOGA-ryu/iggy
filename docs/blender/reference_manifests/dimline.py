"""Find DIMENSION LINES on an engraved plate.

Not by scanning whole rows - that finds shared baselines and the plate border, which run the
full sheet (cycle 31). A dimension line is LOCAL: a cluster of short dashes at one y, a few
hundred px long, with an arrowhead at each end and clean paper above and below THAT SEGMENT.

The "paper 9 px above and below" test alone is defeated by diagonal hatching, which leaves gaps
that read as paper (cycle 31). So the spike in dark fraction is measured over the SEGMENT's own
x-range and compared against the same x-range on neighbouring rows.
"""

def dash_runs(g, W, y, xa, xb, thr=150, lo=3, hi=16):
    runs=[]; s=None
    for x in range(xa,xb):
        d=g[y*W+x]<thr
        if d and s is None: s=x
        elif not d and s is not None: runs.append((s,x-1)); s=None
    if s is not None: runs.append((s,xb-1))
    return [r for r in runs if lo<=r[1]-r[0]+1<=hi]

def clusters(runs, maxgap=26, minn=5):
    """Group dashes into segments; a dimension line's dashes are evenly and closely spaced."""
    out=[]; cur=[runs[0]] if runs else []
    for r in runs[1:]:
        if r[0]-cur[-1][1] <= maxgap: cur.append(r)
        else:
            if len(cur)>=minn: out.append(cur)
            cur=[r]
    if len(cur)>=minn: out.append(cur)
    return out

def frac(g,W,y,xa,xb,thr=150):
    return sum(1 for x in range(xa,xb) if g[y*W+x]<thr)/max(1,xb-xa)

def merge_label_gap(cls, maxgap=70):
    """A dimension line is BROKEN BY ITS OWN LABEL - Paley sets '11 in' inside the line, leaving
    a gap far wider than the dash spacing. Merge clusters separated by up to `maxgap`."""
    if not cls: return cls
    out=[cls[0]]
    for c in cls[1:]:
        if c[0][0]-out[-1][-1][1] <= maxgap: out[-1]=out[-1]+c
        else: out.append(c)
    return out

def band_mean(g,W,H,y,x0,x1,d0,d1):
    """Mean grey over a TALL band above/below the segment. A one-row offset can land between
    diagonal hatch strokes and read as paper (cycle 31); a 15-row mean cannot. This is what
    separates a dimension line - which lies IN PAPER - from a row inside a hatched region."""
    def side(sgn):
        s=n=0
        for dy in range(d0,d1):
            yy=y+sgn*dy
            if not (0<=yy<H): continue
            for x in range(x0,x1): s+=g[yy*W+x]; n+=1
        return s/max(1,n)
    # A DIMENSION LINE SITS DIRECTLY UNDER THE FIGURE IT MEASURES, so it has paper on ONE side
    # only. Requiring both (the obvious first guess) rejects fig 6's own line. A row inside a
    # hatched region has neither side clean, which is what we are actually screening out.
    return max(side(+1), side(-1))

def vthick(g,W,x,y,thr=150,r=12):
    if g[y*W+x]>=thr: return 0
    u=d=0
    while u<r and g[(y-u-1)*W+x]<thr: u+=1
    while d<r and g[(y+d+1)*W+x]<thr: d+=1
    return u+d+1

def arrowheads(g,W,y,x0,x1,near=16):
    """THE discriminator. Every surroundings-based test failed: a dimension line on a packed
    plate has figures above AND below it, so no local paper test can isolate one (cycles 31-32).
    What it always has is an ARROWHEAD at each end - a solid wedge several times thicker than
    the ~3 px dashes. Require a thick cluster within `near` px of BOTH ends."""
    prof=[(x,vthick(g,W,x,y)) for x in range(x0,x1+1)]
    ts=sorted(t for _,t in prof if t>0)
    if len(ts)<12: return False
    med=ts[len(ts)//2]; need=max(5,2*med)
    thick=[x for x,t in prof if t>=need]
    if not thick: return False
    return any(x-x0<=near for x in thick) and any(x1-x<=near for x in thick)

def find(g, W, H, xa, xb, ya, yb, minspan=90, maxspan=700, spike=0.12, paper=172):
    hits=[]
    for y in range(ya,yb):
        for cl in merge_label_gap(clusters(dash_runs(g,W,y,xa,xb))):
            x0,x1 = cl[0][0], cl[-1][1]
            span = x1-x0
            if not (minspan<=span<=maxspan): continue
            here = frac(g,W,y,x0,x1)
            nb = [frac(g,W,y+d,x0,x1) for d in (-14,-11,-8,8,11,14) if 0<=y+d<H]
            base = sum(nb)/len(nb)
            if here-base<spike: continue
            if arrowheads(g,W,y,x0,x1):
                hits.append({"y":y,"x0":x0,"x1":x1,"span":span,"n":len(cl),
                             "spike":round(here-base,3),"frac":round(here,3)})
    return hits

def dedupe(hits, dy=8, dx=60):
    """Collapse the same line detected on adjacent rows."""
    hits=sorted(hits, key=lambda h:-h["spike"]); keep=[]
    for h in hits:
        if not any(abs(h["y"]-k["y"])<=dy and abs(h["x0"]-k["x0"])<=dx for k in keep): keep.append(h)
    return sorted(keep, key=lambda h:(h["y"],h["x0"]))


# ---------------------------------------------------------------- arrowhead-first
# The clustering-then-validate design above fails where two dimension lines ABUT: figs 5 and 6
# share an arrowhead at x~1005, so the label-gap merge runs fig 5's dashes into fig 6's and the
# resulting segment starts 62 px before fig 6's own arrowhead. Arrowheads are the natural
# terminators, so find them FIRST and take consecutive pairs as candidate lines.

def thick_clusters(g,W,y,xa,xb,thr=150):
    prof=[(x,vthick(g,W,x,y,thr)) for x in range(xa,xb)]
    ts=sorted(t for _,t in prof if t>0)
    if len(ts)<12: return [],0
    med=ts[len(ts)//2]; need=max(5,2*med)
    cl=[]; cur=[]
    for x,t in prof:
        if t>=need: cur.append(x)
        else:
            if len(cur)>=3: cl.append((cur[0]+cur[-1])//2)
            cur=[]
    if len(cur)>=3: cl.append((cur[0]+cur[-1])//2)
    return cl, med

def find2(g,W,H,xa,xb,ya,yb,minspan=90,maxspan=700,spike=0.10,mindash=5):
    hits=[]
    for y in range(ya,yb):
        heads,med = thick_clusters(g,W,y,xa,xb)
        if len(heads)<2: continue
        for a,b in zip(heads,heads[1:]):
            span=b-a
            if not (minspan<=span<=maxspan): continue
            d=dash_runs(g,W,y,a,b)
            if len(d)<mindash: continue
            here=frac(g,W,y,a,b)
            nb=[frac(g,W,y+dy,a,b) for dy in (-14,-11,-8,8,11,14) if 0<=y+dy<H]
            base=sum(nb)/len(nb)
            if here-base>=spike:
                hits.append({"y":y,"x0":a,"x1":b,"span":span,"n":len(d),
                             "spike":round(here-base,3)})
    return hits


def chain(hits, maxspan=700):
    """A dimension line's LABEL is also thick, so it registers as a false arrowhead and splits
    the line in two ('11 in' splits fig 6 at x~1108). Merge segments that share an endpoint on
    the same row: the true line runs between the OUTERMOST arrowheads of the chain."""
    out=[]
    for y in sorted({h["y"] for h in hits}):
        row=sorted([h for h in hits if h["y"]==y], key=lambda h:h["x0"])
        cur=None
        for h in row:
            # CAP AT TWO SEGMENTS. A real dimension line has at most ONE thick cluster strictly
            # between its arrowheads - its label. Three or more chained segments means several
            # figures' features happened to align on this row, which produced 480-700 px spans
            # in the cycle-32 census.
            if (cur and cur.get("parts",1)<2 and abs(h["x0"]-cur["x1"])<=12
                    and h["x1"]-cur["x0"]<=maxspan):
                cur={"y":y,"x0":cur["x0"],"x1":h["x1"],"span":h["x1"]-cur["x0"],
                     "n":cur["n"]+h["n"],"spike":max(cur["spike"],h["spike"]),
                     "parts":cur.get("parts",1)+1}
            else:
                if cur: out.append(cur)
                cur=dict(h)
        if cur: out.append(cur)
    return out


def gap_cv(g,W,y,x0,x1):
    """Dash-spacing regularity, with the SINGLE LARGEST gap dropped - that gap is the label,
    which Paley sets inside the line, and leaving it in inflates the CV of a perfectly regular
    line (fig 6: 0.85 with it, 0.43 without). A ruled dashed line is regular; a row that merely
    happens to cross several figures' features is not."""
    import statistics as st
    d=dash_runs(g,W,y,x0,x1+1)
    if len(d)<5: return None
    gaps=sorted(d[i+1][0]-d[i][1] for i in range(len(d)-1))[:-1]
    if len(gaps)<4: return None
    return st.pstdev(gaps)/max(1,st.mean(gaps))

def census(g,W,H,xa,xb,ya,yb,maxcv=0.6):
    out=[]
    for h in dedupe(chain(find2(g,W,H,xa,xb,ya,yb))):
        c=gap_cv(g,W,h["y"],h["x0"],h["x1"])
        if c is not None and c<=maxcv:
            h["cv"]=round(c,2); out.append(h)
    return sorted(out,key=lambda k:k["y"])
