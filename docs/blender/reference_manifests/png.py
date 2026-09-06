import zlib, struct
def read_gray(path):
    d=open(path,'rb').read(); assert d[:8]==b'\x89PNG\r\n\x1a\n'
    i=8; idat=b''; W=H=bd=ct=None
    while i<len(d):
        ln=struct.unpack('>I',d[i:i+4])[0]; typ=d[i+4:i+8]; body=d[i+8:i+8+ln]; i+=12+ln
        if typ==b'IHDR': W,H,bd,ct=struct.unpack('>IIBB',body[:10])
        elif typ==b'IDAT': idat+=body
        elif typ==b'IEND': break
    raw=zlib.decompress(idat)
    ch={0:1,2:3,3:1,4:2,6:4}[ct]; bpp=ch*(bd//8); stride=W*bpp
    out=bytearray(H*stride); prev=bytearray(stride); pos=0
    for y in range(H):
        f=raw[pos]; pos+=1; line=bytearray(raw[pos:pos+stride]); pos+=stride
        for x in range(stride):
            a=line[x-bpp] if x>=bpp else 0; b=prev[x]; c=prev[x-bpp] if x>=bpp else 0
            if f==1: line[x]=(line[x]+a)&255
            elif f==2: line[x]=(line[x]+b)&255
            elif f==3: line[x]=(line[x]+(a+b)//2)&255
            elif f==4:
                p=a+b-c; pa,pb,pc=abs(p-a),abs(p-b),abs(p-c)
                pr=a if (pa<=pb and pa<=pc) else (b if pb<=pc else c)
                line[x]=(line[x]+pr)&255
        out[y*stride:(y+1)*stride]=line; prev=line
    g=bytearray(W*H)
    for y in range(H):
        for x in range(W):
            o=y*stride+x*bpp
            g[y*W+x]= out[o] if ch==1 else (out[o]*30+out[o+1]*59+out[o+2]*11)//100
    return W,H,g
