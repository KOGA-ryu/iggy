"""Minimal greyscale PNG writer + crop/magnify. Pure Python (zlib only).
Exists so plate coordinates are OURS, not sips's ambiguous --cropOffset order."""
import zlib, struct

def write_gray(path, W, H, g):
    raw = b''.join(b'\x00' + bytes(g[y*W:(y+1)*W]) for y in range(H))
    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t+d) & 0xffffffff)
    open(path, 'wb').write(
        b'\x89PNG\r\n\x1a\n'
        + chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 0, 0, 0, 0))
        + chunk(b'IDAT', zlib.compress(raw, 9))
        + chunk(b'IEND', b''))

def crop(W, H, g, x0, y0, x1, y1):
    w, h = x1-x0, y1-y0
    out = bytearray(w*h)
    for y in range(h):
        out[y*w:(y+1)*w] = g[(y0+y)*W + x0 : (y0+y)*W + x1]
    return w, h, out

def magnify(W, H, g, k):
    w, h = W*k, H*k
    out = bytearray(w*h)
    for y in range(h):
        src = (y//k)*W
        row = y*w
        for x in range(w):
            out[row+x] = g[src + x//k]
    return w, h, out

def grid(W, H, g, step=50, x0=0, y0=0, val=110):
    """Burn a coordinate grid in, so a measurement read by eye lands on real plate pixels."""
    out = bytearray(g)
    for y in range(H):
        for x in range(W):
            if (x0+x) % step == 0 or (y0+y) % step == 0:
                if out[y*W+x] > val: out[y*W+x] = val
    return out
