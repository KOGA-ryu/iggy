"""Harvest gate-relevant facts for furniture candidates from museum open-access APIs.

WHY AN API AND NOT AN AGENT READING PAGES. The admission gates are factual: does the record
publish overall measurements, does it offer three or more materially different views, is the
image rights status clear. An agent reading HTML can hallucinate an accession number or a
dimension; an API returns them or it does not. So FACTS COME FROM HERE and agents are only
ever asked for judgement (component breakdown, reuse potential, level-design use) on records
that have already passed a mechanical gate.

Anything the API does not supply is recorded as the literal string "NOT STATED" - never filled in.

TRAP - THE MET SEARCH DEPARTMENT PARAM IS `departmentIds`, PLURAL. The singular `departmentId`
is accepted and returns total=0 rather than an error, so a sweep looks exhaustive and finds
nothing. Compare: `?q=chest&departmentIds=1` -> 151 hits, `?q=chest&departmentId=1` -> 0.
Also note free-text `q` matches DESCRIPTIONS, so `q=footstool` alone is dominated by paintings
that depict one; always constrain by department and then re-check `objectName` on the record.
And `dateBegin`/`dateEnd` combined with `q` narrowed a 204-hit search to 8 paintings - avoid.

Sources:
  Met Museum   https://collectionapi.metmuseum.org/public/collection/v1/objects/<id>
               `additionalImages` is a real array, so the view count is countable.
               `isPublicDomain` makes the rights field factual.
  Cleveland    https://openaccess-api.clevelandart.org/api/artworks/<accession>
               `images` dict plus `share_license_status`.
"""
import json
import subprocess
import time

UA = "iggy3d-furniture-corpus/1.0 (contact: dethislikethewind@gmail.com)"
NS = "NOT STATED"


def _get(url, retries=3):
    for a in range(retries):
        r = subprocess.run(["curl", "-sL", "-A", UA, "--max-time", "60", url],
                           capture_output=True)
        try:
            return json.loads(r.stdout)
        except Exception:
            time.sleep(1.5 * (a + 1))
    return {}


def met(object_id):
    d = _get(f"https://collectionapi.metmuseum.org/public/collection/v1/objects/{object_id}")
    if not d or d.get("message"):
        return None
    add = d.get("additionalImages") or []
    return {
        "museum": "Metropolitan Museum of Art",
        "record_id": f"met/{object_id}",
        "url": d.get("objectURL") or NS,
        "accession": d.get("accessionNumber") or NS,
        "museum_title": d.get("title") or NS,
        "object_name": d.get("objectName") or NS,
        "maker": d.get("artistDisplayName") or d.get("culture") or NS,
        "maker_role": d.get("artistRole") or NS,
        "date": d.get("objectDate") or NS,
        "geography": " ".join(x for x in (d.get("city"), d.get("state"),
                                          d.get("country")) if x) or NS,
        "culture": d.get("culture") or NS,
        "medium": d.get("medium") or NS,
        "dimensions_published": d.get("dimensions") or NS,
        "credit_line": d.get("creditLine") or NS,
        "classification": d.get("classification") or NS,
        "is_public_domain": d.get("isPublicDomain"),
        "rights": ("Public Domain / CC0 (Met Open Access)" if d.get("isPublicDomain")
                   else "NOT open access - rights restricted"),
        "primary_image": d.get("primaryImage") or "",
        "additional_images": add,
        "view_count": (1 if d.get("primaryImage") else 0) + len(add),
        "department": d.get("department") or NS,
    }


def cleveland(accession):
    d = (_get(f"https://openaccess-api.clevelandart.org/api/artworks/{accession}")
         or {}).get("data") or {}
    if not d:
        return None
    imgs = d.get("images") or {}
    alt = d.get("alternate_images") or []
    return {
        "museum": "Cleveland Museum of Art",
        "record_id": f"cma/{accession}",
        "url": d.get("url") or NS,
        "accession": d.get("accession_number") or NS,
        "museum_title": d.get("title") or NS,
        "object_name": (d.get("type") or NS),
        "maker": "; ".join(c.get("description", "") for c in (d.get("creators") or [])) or NS,
        "maker_role": NS,
        "date": d.get("creation_date") or NS,
        "geography": d.get("culture", [NS])[0] if d.get("culture") else NS,
        "culture": "; ".join(d.get("culture") or []) or NS,
        "medium": d.get("technique") or d.get("medium") or NS,
        "dimensions_published": (d.get("measurements") or NS),
        "credit_line": d.get("creditline") or NS,
        "classification": d.get("type") or NS,
        "is_public_domain": (d.get("share_license_status") == "CC0"),
        "rights": d.get("share_license_status") or NS,
        "primary_image": (imgs.get("web") or {}).get("url", ""),
        "additional_images": [ (a.get("web") or {}).get("url","") for a in alt ],
        "view_count": (1 if imgs else 0) + len(alt),
        "department": d.get("department") or NS,
    }


# ---------------------------------------------------------------- admission gates
def gate(rec):
    """-> (passes: bool, failures: [str]). Mechanical only - no judgement."""
    f = []
    if not rec:
        return False, ["record not retrievable"]
    if rec["url"] == NS:
        f.append("no stable source URL")
    if rec["accession"] == NS:
        f.append("no accession number")
    if rec["dimensions_published"] == NS:
        f.append("no published overall measurements")
    if rec["view_count"] < 3:
        f.append(f"only {rec['view_count']} view(s); gate requires 3+")
    if rec["maker"] == NS and rec["culture"] == NS:
        f.append("no maker/culture/provenance information")
    if not rec["is_public_domain"]:
        f.append("image rights not open access (supplementary use only)")
    return (len(f) == 0), f
