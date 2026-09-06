"""Binarize by MEASURED local mean, label, and Moore-trace the outer boundary.

THRESHOLD IS PER-SCAN, NOT A CONSTANT. An 1845 plate scan has no white, and different
plates in the SAME book sit at different levels:
    Plate I  - clear paper median 180, hatched stone 154  -> threshold 167
    Plate II - clear paper median 167, hatched stone 146  -> threshold 156
Plate II's PAPER is darker than Plate I's threshold, so reusing 172 across the two made
the entire second sheet read as solid stone and every trace ran to the scan-window edge.
Measure both medians on each scan and ASSERT paper_median > threshold before trusting a
trace. (Cycle 26 recorded "measure the levels, then threshold"; cycle 29 reused the number
across scans anyway.)"""
import png
from collections import deque

def binarize(W,H,g,x0,y0,x1,y1,thr,R=5):
    w,h=x1-x0,y1-y0
    b=bytearray(w*h)
    for y in range(h):
        Y=y0+y
        for x in range(w):
            X=x0+x
            s=n=0
            for yy in range(max(0,Y-R),min(H,Y+R+1)):
                row=yy*W
                for xx in range(max(0,X-R),min(W,X+R+1)): s+=g[row+xx]; n+=1
            b[y*w+x]= 1 if s/n<thr else 0
    return w,h,b

def largest(w,h,b):
    seen=bytearray(w*h); best=(0,None)
    for s in range(w*h):
        if b[s] and not seen[s]:
            q=deque([s]); seen[s]=1; cells=[]
            while q:
                p=q.popleft(); cells.append(p)
                px,py=p%w,p//w
                for dx,dy in((1,0),(-1,0),(0,1),(0,-1)):
                    nx,ny=px+dx,py+dy
                    if 0<=nx<w and 0<=ny<h:
                        t=ny*w+nx
                        if b[t] and not seen[t]: seen[t]=1; q.append(t)
            if len(cells)>best[0]: best=(len(cells),cells)
    m=bytearray(w*h)
    for p in best[1]: m[p]=1
    return best[0], m

def trace(w,h,m):
    """Moore-neighbour boundary trace, clockwise, from the topmost-leftmost cell."""
    start=None
    for s in range(w*h):
        if m[s]: start=s; break
    if start is None: return []
    N=[(1,0),(1,1),(0,1),(-1,1),(-1,0),(-1,-1),(0,-1),(1,-1)]
    sx,sy=start%w,start//w
    out=[(sx,sy)]; cx,cy=sx,sy; d=0
    for _ in range(8*w*h):
        found=False
        for k in range(8):
            i=(d+6+k)%8
            nx,ny=cx+N[i][0],cy+N[i][1]
            if 0<=nx<w and 0<=ny<h and m[ny*w+nx]:
                cx,cy,d=nx,ny,i; out.append((cx,cy)); found=True; break
        if not found or (cx,cy)==(sx,sy) and len(out)>4: break
    return out
