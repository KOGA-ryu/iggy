"""Internet Archive book miner for the reference scout lane.

Recipe (differs from the Commons page-thumbnail recipe in cycle 11):
  IA exposes an OCR layer, so triage is TEXT-FIRST and images are fetched only
  for confirmed targets.

    1. /metadata/<id>            -> imagecount, dir, server, rights
    2. <id>_hocr_pageindex.json.gz -> [wordStart, wordEnd, byteStart, byteEnd] per page
    3. <id>_hocr_searchtext.txt.gz -> full OCR text, sliced by those byte ranges
    4. keyword search over per-page text -> candidate page indices (n-numbers)
    5. https://archive.org/download/<id>/page/n<N>_w<W>.jpg -> page image

Page numbering: n<N> in a DOWNLOAD URL is the scan-image index. It is NOT the book's printed
page number, and - the trap - it is NOT necessarily the same index the pageindex array uses.

TRAP - THE OCR LEAF INDEX AND THE IMAGE INDEX CAN DIVERGE, BY A DRIFTING AMOUNT. On
encyclopaediaofa00gwil the pageindex has 1478 entries against an imagecount of 1474, and the
byte slices straddle printed-page boundaries, so a running head turns up in the MIDDLE of a
slice rather than at its start. Measured on that item: the OCR slice containing "668 THEORY OF
ARCHITECTURE" is leaf 692, but printed page 668 is image n689 (offset 3), while the slice
containing "671" is leaf 696 against image n692 (offset 4). The offset is not constant, so it
cannot be computed from len(pageindex) - imagecount either.
    THE TESTABLE RULE (cycle 39, verified both ways):
      len(pageindex) == imagecount  ->  leaf N is image N. Trust it.
        Confirmed on gri_33125010766489: 364 == 364, and OCR leaf 125 holds the body of printed
        page 108, which is exactly what image n125 shows.
      len(pageindex) != imagecount  ->  the offset DRIFTS. Find the page by fetching two or three
        images around the leaf and reading the printed number off the page.
        Confirmed on encyclopaediaofa00gwil: 1478 vs 1474, offsets of 3 AND 4 in the same book.
    A separate, harmless quirk either way: a slice usually LEADS IN with the tail of the previous
    page, so t[:80] is often the end of page N-1. Do not conclude from a mid-word start that the
    index is misaligned - that was my first (wrong) reading in cycle 39. Search the whole slice.
(Found cycle 38 after cropping the wrong page twice; rule pinned down in cycle 39.)

TRAP - THE `w` IN THE DOWNLOAD URL IS A REQUEST, NOT A GUARANTEE. IA caps at the source
scan resolution and silently serves whatever it has under whatever name you asked for. On
manualofgothicmo00pale_0, `n88_w1000.jpg`, `n88_w1100.jpg` and `n88_w1400.jpg` are ALL
1930x2951 - three names, one image. So a larger `w` does not buy more detail, and the
requested width must never be recorded as if it were the plate's resolution. Verify with
`sips -g pixelWidth -g pixelHeight` before quoting a scan size anywhere. (Found cycle 23,
while trying to explain why a measurement would not close.)
    It cuts the other way too: on evolutionenglis00addygoog a request for w=1600 returns
    2759x4109, the full source scan. So `w` is neither a floor nor a ceiling. Measure it.

TRAP - SOME SCANS DESTROY EVERY VULGAR FRACTION, AND THE LOSS IS MOSTLY INVISIBLE. On
evolutionenglis00addygoog the OCR contains ZERO occurrences of the ½ ¼ ¾ glyphs across all
263 leaves, yet the book prints them freely - leaf n166 carries THREE in two lines. They
arrive as junk: `33^`, `2\\`, `5^`, `6i`, `9!`.
    Why it matters for engdim.py: `DIM` requires the number to sit immediately against its
    unit, so `33^ feet` matches NOTHING. The failure mode is SILENT OMISSION rather than
    silent corruption - safer, but it means the density scan under-counts and a hall printed
    as `20 ft by 15½ ft` returns one dimension instead of a pair, so `triples()` never fires.
    THE CHECK, one line, run it on every new book before trusting a density scan:
        whole.count('½') == 0 and the book is English  ->  assume total fraction loss.
    And the honest limit: a fraction lost with NO junk left behind cannot be detected from
    the text layer at all. There is no way to bound the loss from OCR alone - only the page
    image resolves it. So any fractional value must be read off the scan or shipped as
    `ocr-uncertain`. Never reconstruct the fraction from context. (Found cycle 48.)
    Confirmed universal on the joinery shelf (census cycle 1): mitchell1898, mitchell1894
    and ellis1902 ALL score zero fraction glyphs - and joinery stock dims are mostly
    fractional, so for section books the numbers must come from the PLATES, not the prose.

TRAP - THE DERIVATIVE PREFIX IS NOT ALWAYS THE IDENTIFIER. On india.history.resource.100246
the hOCR derivatives are named 100246_hocr_pageindex.json.gz etc. - the item's internal file
prefix, not the identifier. This module builds derivative URLs as {id}_hocr_*, which on such
items downloads an HTML error page and falls through to the djvu.xml fallback (which may
ALSO carry the other prefix, yielding zero pages with no error). FIX: when load_text returns
0 pages on an item that plainly has text, read meta['files'] and fetch the real names, or
pre-place the files in the cache dir as pageindex.json.gz / searchtext.txt.gz - Book checks
for existing cache files before constructing URLs. Page IMAGES are unaffected: the
/page/n{N} endpoint resolves normally. (Found census cycle 1, on Ellis.)

TRAP ADDENDUM to the offset rule: imagecount can be ABSENT from an item's metadata entirely
(india.history.resource.100246 again), which makes len(pageindex)==imagecount UNEVALUABLE -
and that item's offset does in fact DRIFT (printed = leaf-17 at n141, leaf-32 at n300;
uncounted plate leaves). Treat missing imagecount as the untrusted branch: read the printed
number off the page for every citation.
"""
import gzip, json, os, re, subprocess, time, urllib.parse

UA = "iggy3d-reference-scout/1.0 (contact: dethislikethewind@gmail.com)"
# CACHE defaults to this module's directory, which puts multi-megabyte scan downloads straight
# into the repo - Gwilt alone landed 5.8 MB in reference_manifests/. Set IABOOK_CACHE to a
# scratchpad path. (Found cycle 38, after having to move it back out.)
CACHE = os.environ.get("IABOOK_CACHE") or os.path.dirname(os.path.abspath(__file__))


def _curl(url, out=None, retries=3):
    for attempt in range(retries):
        cmd = ["curl", "-sL", "-A", UA, "--max-time", "120"]
        if out:
            cmd += ["-o", out, "-w", "%{http_code}"]
        cmd.append(url)
        r = subprocess.run(cmd, capture_output=True)
        if out:
            code = r.stdout.decode().strip()
            if code == "200" and os.path.getsize(out) > 0:
                return True
        else:
            if r.returncode == 0 and r.stdout:
                return r.stdout
        time.sleep(1.5 * (attempt + 1))
    return False if out else b""


class Book:
    def __init__(self, ident, slug=None):
        self.id = ident
        self.slug = slug or ident
        self.dir = os.path.join(CACHE, self.slug)
        os.makedirs(self.dir, exist_ok=True)
        self.meta = self._meta()
        self.pages = None

    def _meta(self):
        p = os.path.join(self.dir, "meta.json")
        if not os.path.exists(p):
            _curl(f"https://archive.org/metadata/{self.id}", p)
        return json.load(open(p))

    @property
    def imagecount(self):
        return int(self.meta.get("metadata", {}).get("imagecount", 0))

    def rights(self):
        md = self.meta.get("metadata", {})
        return {k: md.get(k) for k in
                ("possible-copyright-status", "licenseurl", "rights", "sponsor", "contributor")
                if md.get(k)}

    def load_text(self):
        """Per-page OCR text, indexed by scan leaf number."""
        if self.pages is not None:
            return self.pages
        idx_p = os.path.join(self.dir, "pageindex.json.gz")
        txt_p = os.path.join(self.dir, "searchtext.txt.gz")
        if not os.path.exists(idx_p):
            _curl(f"https://archive.org/download/{self.id}/{self.id}_hocr_pageindex.json.gz", idx_p)
        if not os.path.exists(txt_p):
            _curl(f"https://archive.org/download/{self.id}/{self.id}_hocr_searchtext.txt.gz", txt_p)
        try:
            idx = json.load(gzip.open(idx_p))
            raw = gzip.open(txt_p, "rb").read()
        except Exception:
            # FALLBACK: not every IA item has the hOCR derivatives (the download
            # returns an HTML error page, so gzip.open raises "Not a gzipped file").
            # _djvu.xml always carries one <OBJECT> per page -> a reliable splitter.
            # Do NOT fall back to _djvu.txt: it may have zero form feeds.
            self.pages = self._pages_from_djvu_xml()
            return self.pages
        # TRAP: pageindex entries are [searchtextStart, searchtextEnd, hocrStart, hocrEnd].
        # Elements 2/3 index the (much larger) hOCR HTML — using them silently yields
        # empty or garbage slices. Always slice searchtext with elements 0/1.
        out = []
        for entry in idx:
            b0, b1 = entry[0], entry[1]
            out.append(raw[b0:b1].decode("utf-8", "replace"))
        self.pages = out
        return out

    def _pages_from_djvu_xml(self):
        p = os.path.join(self.dir, "djvu.xml")
        if not os.path.exists(p):
            _curl(f"https://archive.org/download/{self.id}/{self.id}_djvu.xml", p)
        try:
            x = open(p, encoding="utf-8", errors="replace").read()
        except Exception as e:
            print(f"  ! no OCR layer at all for {self.id}: {e}")
            return []
        out = []
        for obj in re.split(r"<OBJECT\b", x)[1:]:
            words = re.findall(r"<WORD[^>]*>(.*?)</WORD>", obj, re.S)
            out.append(" ".join(w.strip() for w in words))
        return out

    def find(self, patterns, flags=re.I):
        """-> {leaf: [matched terms]} for any regex in `patterns`."""
        pages = self.load_text()
        hits = {}
        for n, txt in enumerate(pages):
            got = [p for p in patterns if re.search(p, txt, flags)]
            if got:
                hits[n] = got
        return hits

    def page_url(self, n, width=1200):
        return f"https://archive.org/download/{self.id}/page/n{n}_w{width}.jpg"

    def fetch_pages(self, leaves, width=1200, pace=0.7):
        """Download page images; returns {leaf: local_path}. Skips existing."""
        got = {}
        for n in leaves:
            p = os.path.join(self.dir, f"n{n}_w{width}.jpg")
            if os.path.exists(p) and os.path.getsize(p) > 5000:
                got[n] = p
                continue
            if _curl(self.page_url(n, width), p):
                got[n] = p
            else:
                print(f"  ! failed leaf {n}")
            time.sleep(pace)
        return got


def contact_sheet(paths_by_leaf, out_html, cols=5, title="contact sheet", thumb_px=340):
    """HTML grid for browser-pane triage (no PIL on this machine)."""
    cells = []
    for n, p in sorted(paths_by_leaf.items()):
        rel = os.path.basename(os.path.dirname(p)) + "/" + os.path.basename(p)
        cells.append(
            f'<figure><img src="{rel}" loading="lazy"><figcaption>n{n}</figcaption></figure>')
    html = f"""<meta charset=utf-8><title>{title}</title>
<style>
body{{background:#141414;color:#ddd;font:13px system-ui;margin:12px}}
h1{{font-size:15px;font-weight:600;margin:0 0 10px}}
.grid{{display:grid;grid-template-columns:repeat({cols},1fr);gap:8px}}
figure{{margin:0;background:#000;border:1px solid #333}}
img{{width:100%;height:{thumb_px}px;object-fit:contain;display:block}}
figcaption{{font:11px ui-monospace;padding:3px 5px;color:#8ab4f8}}
</style><h1>{title}</h1><div class=grid>{''.join(cells)}</div>"""
    open(out_html, "w").write(html)
    return out_html
