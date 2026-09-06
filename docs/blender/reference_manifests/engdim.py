"""Extract English feet-and-inches dimensions from OCR'd Victorian architecture books.

The English sources state hall sizes as TRIPLES - length by width by height - which is what makes
them uniquely useful (Viollet almost never gives all three). The forms in the wild:

    "100 ft. by 27 ft."            "64 feet long by 38 feet 9 inches wide"
    "43 ft. 6 in. by 28 ft."       "26 ft. high"      "walls 3 ft. 6 in. thick"
    "12 ft. 6 in."                 "18 in."           "a foot and a half"

OCR TRAPS on these scans: "ft" appears as "ft.", "feet", "fl.", "f't"; inches as "in.", "ins.",
"inches"; and the multiplication word is "by" or "x" or the OCR's mangled "hy".
"""
import re

FT, IN = 0.3048, 0.0254
_FT = r"(?:ft\.?|feet|foot|fl\.)"
_IN = r"(?:in\.?|ins\.?|inches|inch)"
FRAC = {"½":0.5,"¼":0.25,"¾":0.75}

def _num(s):
    s=s.strip()
    for g,v in FRAC.items():
        if s.endswith(g): return float(s[:-1] or 0)+v
    m=re.match(r"^(\d+)\s+(\d+)/(\d+)$", s)
    if m: return int(m.group(1))+int(m.group(2))/int(m.group(3))
    m=re.match(r"^(\d+)/(\d+)$", s)
    if m: return int(m.group(1))/int(m.group(2))
    try: return float(s.replace(",", "."))
    except ValueError: return None

DIM = re.compile(rf"(\d+(?:\s*\d/\d|[½¼¾])?)\s*{_FT}\s*(?:(\d+(?:\s*\d/\d|[½¼¾])?)\s*{_IN})?", re.I)

def dims(text):
    """-> [(metres, raw, start, end)] for every feet[-inches] expression."""
    out=[]
    for m in DIM.finditer(text):
        f=_num(m.group(1))
        if f is None: continue
        i=_num(m.group(2)) if m.group(2) else 0.0
        out.append((f*FT + (i or 0)*IN, m.group(0), m.start(), m.end()))
    return out

def triples(text, gap=34):
    """Dimension expressions joined by 'by'/'x' - a hall's length x width [x height]."""
    d=dims(text); out=[]; i=0
    while i < len(d):
        run=[d[i]]
        while i+1 < len(d):
            between=text[d[i][3]:d[i+1][2]]
            if len(between) <= gap and re.search(r"\b(?:by|hy|x|and)\b|×", between, re.I):
                run.append(d[i+1]); i+=1
            else: break
        if len(run) >= 2: out.append(run)
        i+=1
    return out

def density(pages):
    """-> [(leaf, n_dims, n_triples)] sorted by triples then dims."""
    rows=[]
    for n,t in enumerate(pages):
        if len(t) < 200: continue
        tt=re.sub(r"\s+"," ",t)
        rows.append((n, len(dims(tt)), len(triples(tt))))
    rows.sort(key=lambda r: (-r[2], -r[1]))
    return rows
