"""Extract every numeric dimension token from a Viollet-le-Duc article.

WHY THIS EXISTS. Cycle 36 measured the previous ad-hoc extractor at 33% RECALL. Viollet writes
numbers in THREE forms and it matched one. Every Viollet-derived row in the manifests before
cycle 37 was harvested with that blind spot.

REGRESSION SUITE, kept because two numeral bugs shipped before anyone noticed (cycle 46 added it):
    un..seize, dix-sept/huit/neuf, vingt-deux, trente-cinq, quarante-deux, soixante-dix,
    soixante-quinze, quatre-vingt, quatre-vingt-dix, cent quinze, cent cinquante  -> 35 cases.
    Run it after ANY change to _word(). The failure mode is not a crash; it is a plausible number.

    F1   "1 m ,60"      -> 1.60 m    the form everyone notices
    F2   "0,70 c."      -> 0.70 m    "c." abbreviates centimetres OF A METRE. NOT 0.70 cm.
    F3   "deux mètres", "un mètre dix-huit centimètres", "cinq pieds"  -> spelled out

TRAPS, all of them found the hard way:
  * F1 needs \\s* BEFORE the comma. HTML stripping leaves a space, so "\\d+\\s*m[,.]\\d+" matches
    NOTHING - 9 hits become 0 on Meurtrière.
  * F2's "c." and the spelled-out word "centimètres" DO NOT MEAN THE SAME THING. In one article,
    "0,30 c." is 0.30 m while "trois centimètres" is 0.03 m. Handle them separately.
  * SCALE STATEMENTS LOOK LIKE DIMENSIONS. "à l'échelle de 0 m ,002 pour mètre" and "1 centimètre
    pour 15 mètres" are drawing scales, not measurements; the extractor cannot tell them apart, so
    a consumer must filter on the phrases "à l'échelle de" and "pour mètre".
  * Old units: 1 pied = 0.3248 m, 1 pouce = pied/12, 1 toise = 6 pieds = 1.949 m, 1 ligne = pouce/12.
"""
import re

# THE "PIED" IS NOT ONE UNIT. Viollet quotes sources using different feet and says so in passing:
#   pied de roi   0.3248 m   the French default, and what "pas = cinq pieds de roy" is built on
#   pied romain   0.2979 m   "(Scala parle ici de pieds romains 0,297896.)" - Architecture militaire
# Converting a Roman-foot passage with the pied de roi is an 8.4% error, and nothing in the
# surrounding text flags it except that one parenthesis. When a passage quotes a foreign author,
# check for a stated foot before converting. (Cycle 37.)
PIED, PIED_ROMAIN = 0.3248, 0.297896
POUCE, TOISE = PIED/12, 6*PIED

F1 = re.compile(r"(\d+)\s*m\s*[,.]\s*(\d+)")
F2 = re.compile(r"(\d+)\s*,\s*(\d+)\s*c\.")
NUM = {"un":1,"une":1,"deux":2,"trois":3,"quatre":4,"cinq":5,"six":6,"sept":7,"huit":8,"neuf":9,
       "dix":10,"onze":11,"douze":12,"treize":13,"quatorze":14,"quinze":15,"seize":16,
       "vingt":20,"trente":30,"quarante":40,"cinquante":50,"soixante":60,"cent":100}
_W = "|".join(sorted(NUM, key=len, reverse=True))
UNITS = {"mètre":1.0,"mètres":1.0,"metre":1.0,"metres":1.0,
         "centimètre":0.01,"centimètres":0.01,"centimetre":0.01,"centimetres":0.01,
         "pied":PIED,"pieds":PIED,"pouce":POUCE,"pouces":POUCE,"toise":TOISE,"toises":TOISE}
_U = "|".join(sorted(UNITS, key=len, reverse=True))
# UP TO THREE number-words. Two was not enough: "quatre vingt seize toises" (96 toises = 187 m)
# matched only "vingt seize" = 36, giving 70.16 m - a 62% error, and again a believable one.
# This is the THIRD numeral bug in this module and the third that returned a plausible number
# rather than failing. (Cycle 47.)
F3 = re.compile(rf"\b((?:{_W})(?:[- ](?:{_W})){{0,2}})\s+({_U})\b", re.I)
F4 = re.compile(rf"(\d+(?:[.,]\d+)?)\s*({_U})\b", re.I)   # "46 mètres", "5 pieds"

def _word(w):
    """French compound numerals. THREE FORMS, and the naive "first token wins" got two wrong:

        dix-sept / dix-huit / dix-neuf   ADDITIVE TEENS -> 17/18/19, not 10.
                 "dix-huit pieds" was returning 3.25 m instead of 5.85 m - a 44% error that
                 looks entirely plausible in a table of wall heights. (Cycle 45.)
        quatre-vingt(s)                  VIGESIMAL -> 80, not 4. quatre-vingt-dix -> 90.
        vingt-deux, cent quinze          additive on a round base -> 22, 115. These already worked.
    """
    parts = [p for p in re.split(r"[- ]", w.lower()) if p]
    vals = [NUM[p] for p in parts if p in NUM]
    if not vals:
        return None
    # quatre-vingt(-dix) : 4 x 20 [+ 10]
    if len(vals) >= 2 and vals[0] == 4 and vals[1] == 20:
        return 80 + (vals[2] if len(vals) > 2 else 0)
    # dix-sept / dix-huit / dix-neuf
    if len(vals) == 2 and vals[0] == 10 and vals[1] in (7, 8, 9):
        return 10 + vals[1]
    # soixante-dix (70), soixante-quinze (75)
    if len(vals) == 2 and vals[0] == 60 and vals[1] in (10, 11, 12, 13, 14, 15, 16):
        return 60 + vals[1]
    if len(vals) == 2 and vals[0] in (20, 30, 40, 50, 60, 100):
        return vals[0] + vals[1]
    return vals[0]

def find(text):
    """-> [{'metres':float,'raw':str,'form':str,'pos':int}] sorted by position, de-overlapped."""
    t = re.sub(r"\s+", " ", text)
    out = []
    for m in F1.finditer(t):
        out.append({"metres": int(m.group(1)) + int(m.group(2))/(10**len(m.group(2))),
                    "raw": m.group(0), "form": "F1", "pos": m.start(), "end": m.end()})
    for m in F2.finditer(t):
        out.append({"metres": int(m.group(1)) + int(m.group(2))/(10**len(m.group(2))),
                    "raw": m.group(0), "form": "F2 (c. = metres)", "pos": m.start(), "end": m.end()})
    for m in F3.finditer(t):
        n = _word(m.group(1))
        if n is None: continue
        out.append({"metres": n*UNITS[m.group(2).lower()], "raw": m.group(0),
                    "form": "F3 (spelled)", "pos": m.start(), "end": m.end()})
    for m in F4.finditer(t):
        out.append({"metres": float(m.group(1).replace(",", "."))*UNITS[m.group(2).lower()],
                    "raw": m.group(0), "form": "F4 (digit+word)", "pos": m.start(), "end": m.end()})
    out.sort(key=lambda d: (d["pos"], -(d["end"]-d["pos"])))
    keep, last = [], -1
    for d in out:
        if d["pos"] >= last: keep.append(d); last = d["end"]
    return keep

def context(text, hit, before=170, after=110):
    t = re.sub(r"\s+", " ", text)
    return t[max(0, hit["pos"]-before): hit["end"]+after].strip()
