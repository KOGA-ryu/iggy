"""Consolidate every mined dimension manifest into ONE building-dimensions table.

The manifests were mined in different cycles with different schemas and different units (English
feet-and-inches from Dollman and the Tudor volumes; metres from Viollet, written "2m,30" = 2.30 m).
A modeller needs one table, one unit. This normalises to METRES and keeps the original string
alongside, so nothing is lost to a parsing bug.

PARSING HISTORY, because every one of these returned a PLAUSIBLE number rather than failing, which
is why none of them announced itself:

  cycle 36  "19 8" with unit "ft in" (a bare feet-inches pair) was 471 of 493 parse failures.
  cycle 40  "21^ ft. (21 1/2 ft.)" - an OCR-mangled fraction - fell through to the prose fallback
            and returned 0.61 m, exactly 2 ft. Found by a 21.49:1 room aspect ratio.
  cycle 40  "47 feet 3 inches" silently dropped the inches: the pattern only accepted "ft"/"in".
  cycle 41  The prose fallback tried patterns IN SEQUENCE, so on "43 ft x 21 ft 6 in" the
            ft-and-inches pattern won and returned 21 ft 6 in instead of the leading 43 ft. Now
            every candidate is collected WITH its position; LEFTMOST wins, tie-broken by LONGEST.
  cycle 41  "100 x 27 ft" means 100 ft by 27 ft, but only "27 ft" carried the unit, so the row
            recorded its width as its length. The unit is now distributed leftwards over "N x M ft".
  cycle 41  The "= X" rule fired on a UNIT DEFINITION: "6 to 7 pieds = 1.95 to 2.28 (at Viollet's
            pied = 0.325)" returned 0.325. It now requires nothing numeric before the "=".
"""
import csv
import os
import re

FT = 0.3048
IN = 0.0254

NON_LENGTH = ("count", "rule", "ratio", "factor", "deg", "degrees", "fraction", "m2",
              "square metres", "openings", "rafters per bay")


def to_m(val, unit):
    """-> (metres, how) or (None, reason). `how` records which rule fired, for auditing.

    A mined `value` is often PROSE, not a number - agents returned things like "2 ft 6 in (0.76 m)
    maximum for a solid pillar; above that a thin wall round the newel". Most such strings carry
    their own metric conversion in parentheses, so look for that first: it is the author's or the
    miner's own reduction and is more trustworthy than re-parsing the imperial part. Units that are
    not lengths at all (counts, rules, angles, ratios) are reported as such rather than as parse
    failures - they are legitimate data, just not distances.
    """
    u0 = (unit or "").strip().lower()
    if u0 in NON_LENGTH:
        return None, f"non-length ({u0})"
    if val:
        pm = re.search(r"\(\s*(?:approx\.?|about|~|=)?\s*([\d]+[.,]?[\d]*)\s*m\s*\)", str(val))
        if pm:
            return float(pm.group(1).replace(",", ".")), "metric conversion in source string"
    m, how = _to_m_core(val, unit)
    if m is not None:
        return m, how
    return _first_measure(val, unit)


def _first_measure(val, unit):
    """LAST RESORT: pull the element's OWN dimension out of surrounding prose."""
    if not val:
        return None, "empty"
    s = str(val)
    u = (unit or "").strip().lower()

    # Distribute a trailing unit leftwards over "N x M ft" -> "N ft x M ft".
    s = re.sub(
        r"(?<![\d.])(\d+(?:\.\d+)?)(\s*)[x×](\s*)(\d+(?:\.\d+)?\s*)(ft\.?|feet|in\.?|inches|m)\b",
        lambda mm: f"{mm.group(1)} {mm.group(5)}{mm.group(2)}x{mm.group(3)}{mm.group(4)}{mm.group(5)}",
        s, count=1, flags=re.I)

    # Every candidate with its position; LEFTMOST wins, tie-broken by LONGEST match. The tie-break
    # matters: "40 ft 3 in" and "40 ft" both start at 0, and sorting on value silently preferred
    # the shorter one, dropping the inches (12.192 instead of 12.268).
    cands = []
    for rx, conv, tag in (
        (r"(\d+)\s*(?:ft\.?|feet|foot)\s*(\d+(?:\.\d+)?)\s*(?:in\.?|ins\.?|inches)",
         lambda mm: int(mm.group(1)) * FT + float(mm.group(2)) * IN, "ft in"),
        (r"(\d+(?:\.\d+)?)\s*(?:ft\.?|feet|foot)\b",
         lambda mm: float(mm.group(1)) * FT, "ft"),
        (r"(\d+(?:\.\d+)?)\s*(?:in\.?|ins\.?|inches)\b",
         lambda mm: float(mm.group(1)) * IN, "in"),
        (r"(\d+)\s*m\s*[,.]\s*(\d+)",
         lambda mm: int(mm.group(1)) + int(mm.group(2)) / (10 ** len(mm.group(2))), "viollet m"),
        (r"(\d+(?:\.\d+)?)\s*m\b", lambda mm: float(mm.group(1)), "m"),
    ):
        mm = re.search(rx, s, re.I)
        if mm:
            cands.append((mm.start(), -(mm.end() - mm.start()), conv(mm), tag))
    if cands:
        cands.sort()
        return cands[0][2], f"first measure in prose ({cands[0][3]})"

    # A "leading bare number + unit column" rule was tried in cycle 41 and REVERTED: the greedy
    # number kept backtracking into a partial one ("43 ft" -> "4") and it cost more rows than it
    # fixed. The single case it was meant to solve - "20 (approx.), taken 5 m above ground" with
    # unit "m", where the leading 20 is the diameter and the 5 m is only where it was measured -
    # is left as a KNOWN one-row defect rather than risk the twelve the leftmost rule gets right.
    mm = re.search(r"(\d+(?:[.,]\d+)?)", s)
    if mm and u.startswith("m"):
        return float(mm.group(1).replace(",", ".")), "first number, unit column (m)"
    if mm and u.startswith("ft"):
        return float(mm.group(1).replace(",", ".")) * FT, "first number, unit column (ft)"
    if mm and u.startswith("in"):
        return float(mm.group(1).replace(",", ".")) * IN, "first number, unit column (in)"
    return None, "unparsed"


def _to_m_core(val, unit):
    """Structured forms, tried before the prose fallback."""
    if val is None:
        return None, "no value"
    s = str(val).strip()
    if not s:
        return None, "empty"
    u = (unit or "").strip().lower()
    note = ""

    # OCR-mangled fraction with a parenthesised expansion: "21^ ft. (21 1/2 ft.)".
    par = re.search(r"\(([^)]*\d[^)]*)\)\s*$", s)
    if par and re.search(r"\d\s*\d/\d|\d\s*(?:ft|in|feet|inches)", par.group(1)):
        s = par.group(1).strip()
        note = " [from bracketed expansion of a mangled fraction]"

    # "= X" conversion, but ONLY when nothing numeric precedes the "=" (else it fires on a unit
    # definition and returns the size of the foot instead of the dimension).
    eq = re.search(r"=\s*([\d.,]+(?:\s*[x×]\s*[\d.,]+)?)\s*$", s)
    if eq and not re.search(r"\d", s[:eq.start()]):
        s = eq.group(1)
        note = " [from stated conversion]"

    s = re.sub(r"^(?:about|approx\.?|circa|c\.|~|up to|under|over|at least|no more than)\s*", "",
               s, flags=re.I)
    s = re.sub(r"\s*\((?:minimum|maximum|min|max|approx\.?|average)\)\s*$", "", s, flags=re.I)

    rng = re.match(r"^(\d+(?:[.,]\d+)?)\s*(?:to|-|–)\s*(\d+(?:[.,]\d+)?)(.*)$", s)
    if rng:
        lo = float(rng.group(1).replace(",", "."))
        hi = float(rng.group(2).replace(",", "."))
        s = f"{(lo + hi) / 2:g}{rng.group(3)}"
        note += " [range midpoint]"

    s = re.sub(r"(\d+)\s+(\d+)/(\d+)",
               lambda mm: f"{float(mm.group(1)) + float(mm.group(2)) / float(mm.group(3)):g}", s)
    s = re.sub(r"(?<![\d.])(\d+)/(\d+)",
               lambda mm: f"{float(mm.group(1)) / float(mm.group(2)):g}", s)

    tail = re.match(r"^(.*?)\s*(ft|feet|in|inch|inches|m|cm|mm)\.?$", s, re.I)
    if tail and re.match(r"^[\d.\s]+$", tail.group(1)):
        if not u or u in ("—", "-", ""):
            u = tail.group(2).lower()
        if " " not in tail.group(1).strip():
            s = tail.group(1).strip()

    m = re.match(r"^(\d+)\s*m\s*[,.]\s*(\d+)$", s)
    if m:
        return int(m.group(1)) + int(m.group(2)) / (10 ** len(m.group(2))), "viollet m,cm" + note
    m = re.match(r"^(\d+)\s*m$", s)
    if m and u.startswith("m"):
        return float(m.group(1)), "metres" + note

    # Bare feet-inches pair: value "19 8" with unit "ft in" means 19 ft 8 in.
    m = re.match(r"^(\d+)\s+(\d+(?:\.\d+)?)$", s)
    if m and ("ft" in u and "in" in u):
        return int(m.group(1)) * FT + float(m.group(2)) * IN, "ft in pair" + note

    m = re.match(r"^(\d+)\s*ft\s*(\d+(?:\.\d+)?)?\s*in\s*[x×]\s*(\d+(?:\.\d+)?)\s*in$", s, re.I)
    if m:
        a = int(m.group(1)) * FT + (float(m.group(2)) if m.group(2) else 0) * IN
        return max(a, float(m.group(3)) * IN), "scantling max face" + note

    m = re.match(r"^([\d.]+)\s*[x×]\s*([\d.]+)$", s)
    if m and u.startswith("m"):
        return max(float(m.group(1)), float(m.group(2))), "pair max (m)" + note

    m = re.match(r"^(\d+(?:\.\d+)?)\s*in\s*[x×]\s*(\d+(?:\.\d+)?)\s*in$", s, re.I)
    if m:
        return max(float(m.group(1)), float(m.group(2))) * IN, "scantling max face (in)" + note

    # Spelled-out units, not just abbreviations.
    m = re.match(
        r"^(\d+)\s*(?:ft\.?|feet|foot|')\s*(\d+(?:\.\d+)?)?\s*(?:in\.?|ins\.?|inches|inch|\")?$", s)
    if m:
        f = int(m.group(1))
        i = float(m.group(2)) if m.group(2) else 0.0
        return f * FT + i * IN, "ft-in" + note

    m = re.match(r"^(\d+(?:[.,]\d+)?)$", s)
    if m:
        v = float(m.group(1).replace(",", "."))
        if u in ("ft", "feet", "foot"):
            return v * FT, "ft" + note
        if u in ("in", "inch", "inches"):
            return v * IN, "in" + note
        if u in ("m", "metre", "metres", "meter", "meters"):
            return v, "m" + note
        if u in ("cm",):
            return v / 100.0, "cm" + note
        if u in ("mm",):
            return v / 1000.0, "mm" + note
        if u in ("deg", "degrees"):
            return None, "angle"
        if u.startswith("m ") or u.startswith("m("):
            return v, "m (qualified)" + note
        return None, f"bare number, unit '{u}'"
    return None, "unparsed"


def rows():
    """Read the five per-source manifests into one shape."""
    out = []

    def read(fn):
        if not os.path.exists(fn):
            return []
        with open(fn) as fh:
            return list(csv.DictReader(fh, delimiter="\t"))

    def add(**kw):
        out.append(kw)

    for r in read("domestic_dimensions_v1.tsv"):
        add(family="hall_manor", building=r.get("building", ""), place=r.get("place", ""),
            element=r.get("element", ""), value=r.get("value", ""), unit=r.get("unit", ""),
            source=r.get("source", ""), confidence=r.get("confidence", ""), quote="")
    for r in read("fortification_dimensions_v1.tsv"):
        add(family="fortification", building=r.get("subject", ""), place="",
            element=r.get("element", ""), value=r.get("value", ""), unit=r.get("unit", ""),
            source=r.get("source_article", ""), confidence=r.get("evidence_class", ""),
            quote=r.get("french_quote", ""))
    for r in read("viollet_dimensions_v1.tsv"):
        add(family="viollet_general", building=r.get("subject", ""), place="",
            element=r.get("element", ""), value=r.get("value", ""), unit=r.get("unit", ""),
            source=r.get("source_article", ""), confidence="", quote=r.get("french_quote", ""))
    for r in read("buttress_proportions_v1.tsv"):
        add(family="buttress", building="", place="", element=r.get("parameter", ""),
            value=r.get("value", ""), unit=r.get("unit", ""), source=r.get("source", ""),
            confidence=r.get("confidence", ""), quote=r.get("note", ""))
    for r in read("timber_roof_scantlings_v2.tsv"):
        for col, el in (("span_ft_in", "roof span"), ("truss_spacing_ft_in", "truss spacing")):
            if r.get(col):
                add(family="timber_roof", building=r.get("church", ""), place=r.get("county", ""),
                    element=el, value=r[col], unit="ft in", source="Brandon 1849",
                    confidence=r.get("status", ""), quote=r.get("note", ""))
        if r.get("member") and r.get("dim_a"):
            add(family="timber_roof", building=r.get("church", ""), place=r.get("county", ""),
                element=f"{r['member']} scantling",
                value=f"{r.get('dim_a', '')} x {r.get('dim_b', '')}", unit="in",
                source="Brandon 1849", confidence=r.get("status", ""), quote=r.get("note", ""))
    return out


if __name__ == "__main__":
    rs = rows()
    ok = 0
    with open("building_dimensions_v1.tsv", "w", newline="") as fh:
        w = csv.writer(fh, delimiter="\t")
        w.writerow(["family", "building", "place", "element", "value_original", "unit_original",
                    "value_m", "parse", "source", "confidence", "quote"])
        for r in rs:
            m, how = to_m(r["value"], r["unit"])
            if m is not None:
                ok += 1
            w.writerow([r["family"], r["building"], r["place"], r["element"], r["value"],
                        r["unit"], f"{m:.3f}" if m is not None else "", how,
                        r["source"], r["confidence"], (r["quote"] or "")[:400]])
    print(f"building_dimensions_v1.tsv: {len(rs)} rows, {ok} converted to metres "
          f"({ok / len(rs) * 100:.0f}%)")
