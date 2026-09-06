"""Per-figure isolation on a decoded engraving. Pure Python."""
import png, sys
from collections import deque

def load(path, thr=150):
    W,H,g = png.read_gray(path)
    return W,H,g,thr

def downsample(W,H,g,thr,k):
    """Block-OR downsample: a cell is dark if ANY pixel in it is dark.
    This doubles as a dilation, which bridges the gaps in dashed lines and
    between adjacent hatch strokes so a figure comes out as ONE component."""
    w,h = (W+k-1)//k, (H+k-1)//k
    m = bytearray(w*h)
    for y in range(H):
        row=y*W; cy=(y//k)*w
        for x in range(W):
            if g[row+x]<thr: m[cy+x//k]=1
    return w,h,m

def components(w,h,m,minpix=40):
    """8-connected BFS. -> [(npix,x0,y0,x1,y1)] sorted by area desc."""
    seen=bytearray(w*h); out=[]
    for s in range(w*h):
        if m[s] and not seen[s]:
            q=deque([s]); seen[s]=1; n=0
            x0=x1=s%w; y0=y1=s//w
            while q:
                p=q.popleft(); n+=1
                px,py=p%w,p//w
                if px<x0:x0=px
                if px>x1:x1=px
                if py<y0:y0=py
                if py>y1:y1=py
                for dy in(-1,0,1):
                    for dx in(-1,0,1):
                        nx,ny=px+dx,py+dy
                        if 0<=nx<w and 0<=ny<h:
                            q2=ny*w+nx
                            if m[q2] and not seen[q2]: seen[q2]=1; q.append(q2)
            if n>=minpix: out.append((n,x0,y0,x1,y1))
    out.sort(reverse=True)
    return out

if __name__=="__main__":
    K=int(sys.argv[1]) if len(sys.argv)>1 else 4
    W,H,g,thr = load("plate1.png")
    w,h,m = downsample(W,H,g,thr,K)
    print(f"full {W}x{H} -> grid {w}x{h} (k={K}), dark cells {sum(m)}")
    cc = components(w,h,m)
    print(f"components >=40 cells: {len(cc)}")
    for i,(n,x0,y0,x1,y1) in enumerate(cc[:28]):
        print(f"{i:3d} n={n:6d}  full-res box x[{x0*K:5d},{(x1+1)*K:5d}] "
              f"y[{y0*K:5d},{(y1+1)*K:5d}]  {(x1-x0+1)*K}x{(y1-y0+1)*K}")
