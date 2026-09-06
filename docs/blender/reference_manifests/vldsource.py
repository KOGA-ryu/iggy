"""fr.wikisource harvester for Viollet-le-Duc's Dictionnaire raisonné.

WHY THIS EXISTS: `iabook.py` mines Internet Archive scans (page images + OCR). The
Dictionnaire needs a different recipe entirely, because fr.wikisource carries the whole
10-volume work already TRANSCRIBED, with every woodcut extracted to Wikimedia Commons as
a separate PNG. So there are no page images to crop and no OCR to fight: the text is
clean, and the figures are individually addressable files.

RECIPE
  1. list=allpages&apprefix=<PREFIX>          -> the article list (533 articles)
  2. action=parse&page=<title>&prop=text      -> rendered HTML, then stripped to text
  3. prop=images                              -> per-article figure list
  4. commons prop=imageinfo&iiprop=url|size|extmetadata -> file URL, pixel size, licence

TRAPS
  - The article page's own WIKITEXT IS TINY (~250 bytes). The content is transcluded from
    the `Page:` namespace by ProofreadPage, so `prop=revisions` size and `prop=extracts`
    both report or return nothing. **Only action=parse expands the transclusion.**
  - The title prefix uses a TYPOGRAPHIC APOSTROPHE (U+2019) in "l’architecture", not "'".
    A straight quote silently matches nothing.
  - Every article's image list includes ProofreadPage status icons (100%.svg,
    "100 percent.svg", "100 percents.svg", Ambox, etc). Filter them or they pollute counts.
  - Both APIs are open to plain curl - no bot wall, unlike the Met - but Commons rate-limits
    bursts on upload.wikimedia.org, so pace DOWNLOADS (not metadata) at ~1s.
  - TWO PAGE LAYOUTS EXIST on fr.wikisource, and only one of them cites the original book:
      Type A -- opens with publisher metadata and a header "( vol. N , p. A - B )".
                e.g. Abaque, Escalier. This is the only place the original pagination appears.
      Type B -- opens with a bare navigation line ("Charnier < Index alphabetique - C > Chateau
                Index par tome") and NO volume or page at all. e.g. Charpente, Chateau, Voute.
    Both give the FULL transcluded text, so this only matters for citation. Measured on a run of
    251 articles, only ~6% were Type A -- so for most articles the printed page range must be
    recovered elsewhere (the "Index par tome" pages, or the Commons DjVu) if it is needed at all.
    Do not treat a missing header as a harvest failure.
"""
import html, json, os, re, subprocess, time, urllib.parse

UA = "iggy3d-reference-scout/1.0 (contact: dethislikethewind@gmail.com)"
WS = "https://fr.wikisource.org/w/api.php"
COMMONS = "https://commons.wikimedia.org/w/api.php"
PREFIX = "Dictionnaire raisonné de l’architecture française du XIe au XVIe siècle/"

JUNK = re.compile(r"(100\s*%|100[\s_]percents?|Ambox|Wikisource|Info[_ ]Simple|"
                  r"Symbol|Nuvola|Commons-logo|Edit-clear)", re.I)


def _api(api, params, retries=3):
    params = dict(params); params.setdefault("format", "json")
    url = api + "?" + urllib.parse.urlencode(params)
    for a in range(retries):
        r = subprocess.run(["curl", "-sL", "-A", UA, "--max-time", "90", url],
                           capture_output=True)
        try:
            return json.loads(r.stdout)
        except Exception:
            time.sleep(1.0 * (a + 1))
    return {}


def articles(include_indexes=False):
    """-> sorted list of article names (prefix stripped)."""
    out, cont = [], None
    while True:
        p = {"action": "query", "list": "allpages", "apprefix": PREFIX,
             "apnamespace": "0", "aplimit": "500"}
        if cont:
            p["apcontinue"] = cont
        d = _api(WS, p)
        out += [x["title"][len(PREFIX):] for x in d.get("query", {}).get("allpages", [])]
        cont = d.get("continue", {}).get("apcontinue")
        if not cont:
            break
        time.sleep(0.3)
    if not include_indexes:
        out = [a for a in out if not a.startswith("Index communes")]
    return sorted(out)


def image_counts(names, batch=40, pace=0.35):
    """Batched figure counts. -> {article: [File titles]} with junk filtered."""
    got = {}
    for i in range(0, len(names), batch):
        chunk = names[i:i + batch]
        d = _api(WS, {"action": "query", "prop": "images", "imlimit": "max",
                      "titles": "|".join(PREFIX + n for n in chunk),
                      "formatversion": "2"})
        for pg in d.get("query", {}).get("pages", []):
            art = pg.get("title", "")[len(PREFIX):]
            got[art] = [im["title"] for im in pg.get("images", [])
                        if not JUNK.search(im["title"])]
        time.sleep(pace)
    return got


def text(article, pace=0.0):
    """Full transcluded article text. -> (plaintext, header_line)."""
    d = _api(WS, {"action": "parse", "page": PREFIX + article,
                  "prop": "text", "formatversion": "2"})
    h = d.get("parse", {}).get("text", "")
    if not h:
        return "", ""
    t = re.sub(r"<style.*?</style>|<script.*?</script>", " ", h, flags=re.S)
    t = re.sub(r"<[^>]+>", " ", t)
    t = re.sub(r"\s+", " ", html.unescape(t)).strip()
    m = re.search(r"\(\s*vol\.\s*\d+\s*,\s*p\.\s*[\d\s\-–]+\)", t)
    if pace:
        time.sleep(pace)
    return t, (m.group(0) if m else "")


def commons_info(files, batch=40, pace=0.4):
    """-> {File title: {url,width,height,license}}"""
    out = {}
    files = list(files)
    for i in range(0, len(files), batch):
        chunk = files[i:i + batch]
        d = _api(COMMONS, {"action": "query", "prop": "imageinfo",
                           "iiprop": "url|size|extmetadata",
                           "titles": "|".join(chunk), "formatversion": "2"})
        for pg in d.get("query", {}).get("pages", []):
            ii = (pg.get("imageinfo") or [{}])[0]
            if not ii:
                continue
            ex = ii.get("extmetadata", {})
            out[pg["title"]] = {
                "url": ii.get("url"), "width": ii.get("width"),
                "height": ii.get("height"),
                "license": ex.get("LicenseShortName", {}).get("value", "?"),
            }
        time.sleep(pace)
    return out


def fetch_image(url, dest, pace=1.0):
    """Download one Commons file. Pace ~1s: bursts trip the 429 robot policy."""
    if os.path.exists(dest) and os.path.getsize(dest) > 4000:
        return True
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    r = subprocess.run(["curl", "-sL", "-A", UA, "--max-time", "120",
                        "-o", dest, "-w", "%{http_code}", url], capture_output=True)
    time.sleep(pace)
    return r.stdout.decode().strip() == "200" and os.path.getsize(dest) > 4000
