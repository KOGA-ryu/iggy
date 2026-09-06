"""Extract the window/door figure inventory from Ellis 1902 + Mitchell 1898 OCR.

Writes section_inventory_raw.tsv, which census_v1.py ingests, validates and emits as
section_inventory_v1.csv. Split from the census generator ON PURPOSE: this script needs the
cached book OCR (set IABOOK_CACHE; it will hit the network only for uncached items), while
census_v1.py must stay deterministic over committed repo files. Re-run this only to refresh
the raw extraction; the committed TSV is the census input.

METHOD.
  Ellis prints real captions ("405. Method of Fixing Pulley Stile in Sill.") - extract
  number+title pairs. Mitchell prints bare labels ("Fig. 411.") - enumerate numbers only;
  subject comes from the leaf's topic tags. Topic tags are keyword scores over the leaf
  text. The worklist keeps leaves tagged with a window/door topic; everything else is
  counted in the summary so the scope cut is visible, not silent.

STATUS DISCIPLINE. Everything here is MAPPED (text-derived). VERIFIED is reserved for
figures on leaves actually inspected at full resolution - census cycle 1 inspected
mitchell1898 n206 and ellis1902 n141; those are marked by census_v1.py at ingest.
Caption slices straddle leaf boundaries (the iabook lead-in quirk), so a figure printed on
leaf N may be captioned in slice N+1 - the leaf column is the SLICE index and Phase 2 must
confirm the figure's true leaf when it fetches the image.

'dimensioned' is UNKNOWN throughout: all three books carry their dimensions on the figures,
and the OCR destroys fractions, so only full-resolution inspection can answer it.
"""
import os, re, sys

MANIF = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                     "blender", "reference_manifests")
sys.path.insert(0, MANIF)
import iabook  # noqa: E402

TOPICS = [
 ("sash_window", r"sash|pulley stile|meeting rail|sash frame|hung sash|balanc"),
 ("casement", r"casement|french window"),
 ("skylight", r"skylight|lantern light|roof light"),
 # AUDIT FIX (seed-42 spot check): bare "glass" fired on tools pages ("glass tube",
 # "glass-paper") - S012 false positive. Bare glass dropped; glazing also removed from
 # the WINDOW_DOOR qualifier below (it now tags but cannot qualify a leaf alone).
 ("glazing", r"glazing|glazed|putty"),
 ("shutter", r"shutter|boxing"),
 ("door", r"\bdoors?\b|ledged|framed and braced|panell?ed door|door frame|jamb lining"),
 ("frame_general", r"\bframes?\b|linings"),
 ("moulding", r"moulding|mould\b|bolection|ovolo|lamb'?s tongue"),
 ("stair", r"stair|handrail|baluster|newel|wreath"),
 ("roof", r"\broof\b|rafter|truss"),
]
WINDOW_DOOR = {"sash_window", "casement", "skylight", "shutter", "door"}

CAP = re.compile(r"(?<![\w.])(\d{1,4})\.\s{1,3}([A-Z][A-Za-z'’,()\- ]{5,80}?)\.(?!\d)")
PROSE_STARTS = ("The", "A ", "An ", "In ", "It ", "On ", "If ", "For ", "This", "These",
                "They", "When", "Where", "Enlarged margin")
# Mitchell cites BOTH ways: "Fig. 411." labels under figures AND "figure 386" prose
# references to figures on nearby pages. A worklist wants both; dedupe per leaf.
# AUDIT FIX: negative lookahead kills OCR-split numbers - "figure 1 16" was yielding
# fig 1 on a masonry page (S329 false positive).
FIGLABEL = re.compile(r"[Ff]ig(?:ure)?\.?\s+(\d{1,3})\b(?!\s*\d)")
SECTION_KEY = re.compile(r"section|detail|joint|elevation|plan\b|method|sketch of|"
                         r"vertical|horizontal", re.I)


def topics_for(text):
    """-> (tags, window_door_hits). AUDIT FIX: a leaf qualifies for the worklist only if
    its window/door keywords hit at least TWICE in total - S329's masonry page qualified
    on a single stray 'door'."""
    t, hits = [], 0
    for name, pat in TOPICS:
        n = len(re.findall(pat, text, re.I))
        if n:
            t.append(name)
            if name in WINDOW_DOOR:
                hits += n
    return t, hits


def main():
    rows = []
    skipped = {"non_window_door_leaves": 0, "figures_on_them": 0}

    # ---- Ellis: captions
    e = iabook.Book("india.history.resource.100246", "ellis1902")
    for n, raw in enumerate(e.load_text()):
        tt = re.sub(r"\s+", " ", raw)
        caps = [(int(m.group(1)), m.group(2).strip()) for m in CAP.finditer(tt)]
        caps = [(f, c) for f, c in caps
                if 1 <= f <= 1100 and not c.startswith(PROSE_STARTS)]
        if not caps:
            continue
        tags, wd_hits = topics_for(tt)
        if not (set(tags) & WINDOW_DOOR) or wd_hits < 2:
            skipped["non_window_door_leaves"] += 1
            skipped["figures_on_them"] += len(caps)
            continue
        for f, c in caps:
            rows.append(["ellis1902", n, "read off page (offset drifts)", f, c,
                         ";".join(tags), "yes" if SECTION_KEY.search(c) else "no"])

    # ---- Mitchell 1898: bare figure labels in the window/door clusters
    m = iabook.Book("buildingconstruc00mitc", "mitchell1898")
    for n, raw in enumerate(m.load_text()):
        tt = re.sub(r"\s+", " ", raw)
        tags, wd_hits = topics_for(tt)
        if not (set(tags) & WINDOW_DOOR) or wd_hits < 2:
            continue
        figs = sorted(set(int(x) for x in FIGLABEL.findall(tt) if 1 <= int(x) <= 600))
        if not figs:
            continue
        for f in figs:
            rows.append(["mitchell1898", n, str(n - 13), f, "",
                         ";".join(tags), "unknown"])

    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           "section_inventory_raw.tsv"), "w") as fh:
        fh.write("source_id\tleaf\tprinted_page\tfigure_no\tcaption\ttopic_tags\t"
                 "section_keyword\n")
        for r in rows:
            fh.write("\t".join(str(x) for x in r) + "\n")
    print("rows:", len(rows), " skipped:", skipped)
    from collections import Counter
    print("by source:", dict(Counter(r[0] for r in rows)))


if __name__ == "__main__":
    main()
