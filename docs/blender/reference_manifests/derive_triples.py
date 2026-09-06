"""Derive length x width x height triples from the canonical dimension table.

STRICTLY internal_* ONLY. The generic `length` / `width` / `height` classes are fallbacks for rows
whose wording never said what they measured, and feeding them into a room triple produces nonsense:
in cycle 40 Westminster Hall came out 17.0 x 21.0 m because `length` had picked up "length of
arbalétriers and chevrons, tenons included" - a roof member, not the hall. A triple is only a triple
when all three rows say INTERNAL.
"""
import csv
from collections import defaultdict

def triples(path="building_dimensions_canonical_v1.tsv"):
    by = defaultdict(dict)
    for r in csv.DictReader(open(path), delimiter="\t"):
        if not r["value_m"] or not r["building"]:
            continue
        try:
            v = float(r["value_m"])
        except ValueError:
            continue
        k = r["canonical_element"]
        b = r["building"].strip()
        if   k == "internal_length": by[b].setdefault("L", v)
        elif k == "internal_width":  by[b].setdefault("W", v)
        elif k == "internal_height": by[b].setdefault("H", v)
    return {b: v for b, v in by.items() if len(v) == 3}
