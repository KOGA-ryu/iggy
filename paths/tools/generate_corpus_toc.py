#!/usr/bin/env python3
"""Publish the pinned Markdown corpus as a reading catalogue, never as exercises."""
import argparse
from datetime import date
from collections import Counter
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SNAPSHOT = ROOT / "content/source_snapshots/math_terms_v1"
MAPPING = ROOT / "content/authoring/math_corpus_map.json"
OUTPUT = ROOT / "content/corpus/toc.json"
REVIEWS = ROOT / "content/authoring/matrix_corpus_review.json"
NOTATION = ROOT / "content/references/matrix_notation.json"
SUBJECTS = ["algebra", "trigonometry", "calculus", "linear_algebra", "discrete_math", "probability_statistics"]


def extract(snapshot=SNAPSHOT):
    subjects, entries, sources = [], [], []
    for subject in SUBJECTS:
        path = snapshot / "sections" / (subject + ".md")
        raw = path.read_bytes()
        lines = raw.decode("utf-8").splitlines()
        subjects.append({"id": subject, "title": lines[0].removeprefix("# ")})
        sources.append({"path": "sections/" + path.name, "sha256": hashlib.sha256(raw).hexdigest()})
        headings = [(i, len(m[1]), m[2]) for i, line in enumerate(lines)
                    if (m := re.fullmatch(r"(#{1,4}) (.+)", line))]
        stack, occurrences = {}, Counter()
        for h, (start, level, title) in enumerate(headings):
            stack = {k: v for k, v in stack.items() if k < level}
            stack[level] = title
            stop = headings[h + 1][0] if h + 1 < len(headings) else len(lines)
            body = "\n".join(lines[start + 1:stop]).strip()
            term = level == 4 and title.startswith("Term: ")
            if not term and not (level == 3 and body):
                continue
            major = stack.get(2, "")
            if term:
                kind, label = "term", title[6:]
                topic = stack.get(3, major) if major.startswith("Round ") else major
            else:
                kind = ("theorem" if title.startswith("Theorem (") else
                        "proof" if "Proof" in major else "example")
                label = re.sub(r"^(Theorem|Example) \((.*)\)$", r"\2", title)
                topic = {"theorem": "Theorems", "proof": "Proof sketches", "example": "Worked examples"}[kind]
            if not body or not topic:
                raise ValueError(f"Missing body/topic: {path}:{start + 1}")
            heading_path = list(stack.values())
            locator = json.dumps([subject, heading_path], ensure_ascii=False)
            occurrences[locator] += 1
            entries.append({"subject": subject, "title": label, "kind": kind, "body": body,
                            "topic_title": topic, "heading_path": heading_path,
                            "occurrence": occurrences[locator], "source": "sections/" + path.name,
                            "first_line": start + 1, "last_line": stop})
    return subjects, entries, sources


def locator(entry):
    return json.dumps([entry["source"], entry["heading_path"], entry["occurrence"]], ensure_ascii=False)


def initial_map(subjects, entries, sources):
    topics, records, topic_ids = [], [], {}
    for entry in entries:
        key = (entry["subject"], entry["topic_title"])
        if key not in topic_ids:
            topic_ids[key] = f"topic_{len(topics) + 1:04d}"
            topics.append({"id": topic_ids[key], "subject": key[0], "title": key[1]})
        records.append({"id": f"corpus_{len(records) + 1:05d}", "topic": topic_ids[key],
                        **{k: entry[k] for k in ("source", "heading_path", "occurrence")}})
    return {"schema_version": 1, "subjects": subjects, "topics": topics, "sources": sources, "entries": records}


def publish(mapping, entries, sources):
    if mapping["schema_version"] != 1 or mapping["sources"] != sources:
        raise ValueError("Snapshot changed: reconcile source hashes and stable IDs in the authoring map first")
    found = {locator(e): e for e in entries}
    if len(found) != len(entries):
        raise ValueError("Ambiguous source occurrence")
    records, used_ids = [], set()
    for record in mapping["entries"]:
        if record["id"] in used_ids:
            raise ValueError("Duplicate persistent entry ID")
        used_ids.add(record["id"])
        entry = found.pop(locator(record))
        records.append({"id": record["id"], "topic": record["topic"],
                        **{k: entry[k] for k in ("title", "kind", "body", "source", "first_line", "last_line")}})
    if found:
        raise ValueError("Unmapped source entries: assign permanent IDs before publishing")
    return {"schema_version": 1, "review_status": "unreviewed", "subjects": mapping["subjects"], "topics": mapping["topics"], "entries": records}


def encoded(value):
    return (json.dumps(value, ensure_ascii=False, indent=2) + "\n").encode("utf-8")


def source_digest(entry):
    source = {key: entry[key] for key in ("id", "title", "kind", "body", "source")}
    return hashlib.sha256(json.dumps(source, ensure_ascii=False, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def apply_reviews(catalogue, document, base):
    """Publish separately reviewed adaptations while retaining all original notes."""
    if document["schema_version"] != 1:
        raise ValueError("Unknown review schema")
    sources = {s["id"]: s for s in document["sources"]}
    if len(sources) != len(document["sources"]):
        raise ValueError("Duplicate review source")
    for s in sources.values():
        if not s["url"].startswith("https://") or not s["title"] or not s["locator"]:
            raise ValueError("Review requires an HTTPS source, title and locator")
    entries = {e["id"]: e for e in catalogue["entries"]}
    lessons = {lesson["id"]: lesson for lesson in base["lessons"]}
    selected = [lessons[key] for key in document["base_lesson_ids"]]
    used = {token["term_id"] for lesson in selected for token in lesson["tokens"]}
    terms = [term for term in base["terms"] if term["id"] in used]
    seen = set()
    for note in document["reviews"]:
        key = note["entry_id"]
        if key in seen or key not in entries or source_digest(entries[key]) != note["source_sha256"]:
            raise ValueError("Duplicate, missing or changed source entry requires a fresh review")
        seen.add(key)
        if type(note["content_version"]) is not int or not 0 < note["content_version"] <= 2**32-1:
            raise ValueError("Positive review version required")
        date.fromisoformat(note["reviewed_on"])
        if not note["source_ids"] or len(set(note["source_ids"])) != len(note["source_ids"]):
            raise ValueError("Review requires distinct supporting sources")
        for field, limit in (("meaning", 160), ("definition", 480), ("example", 240), ("conditions", 800), ("change", 800)):
            text = note[field]
            if not isinstance(text, str) or not text.strip() or len(text.encode()) > limit or any(ord(c)<32 and c!='\n' for c in text):
                raise ValueError("Invalid reviewed text: " + field)
        entry = entries[key]
        entry["review"] = {field: note[field] for field in ("content_version", "reviewed_on", "meaning", "definition", "example", "conditions", "change")}
        entry["review"].update(source_title=entry["title"], source_body=entry["body"],
                                sources=[{k: sources[s][k] for k in ("title", "url", "locator")} for s in note["source_ids"]])
        terms.append({"id": key, "title": entry["title"], **{k: note[k] for k in ("content_version", "meaning", "definition", "example")}})
        lesson = {"content_version": note["content_version"], **note["lesson"]}
        if not lesson["tokens"] or any(t["term_id"] != key for t in lesson["tokens"]):
            raise ValueError("Reviewed lesson must bind to its reviewed definition")
        selected.append(lesson)
    if len({l["id"] for l in selected}) != len(selected) or len({t["id"] for t in terms}) != len(terms):
        raise ValueError("Duplicate notation identity")
    links = document.get("reading_links", {})
    if not isinstance(links, dict):
        raise ValueError("Reading links require an entry-to-targets map")
    for key, targets in links.items():
        if (key not in seen or not isinstance(targets, list) or not 1 <= len(targets) <= 4
                or any(not isinstance(t, str) or t not in seen or t == key for t in targets)
                or len(set(targets)) != len(targets)):
            raise ValueError("Reading links require distinct, other reviewed entries")
        entries[key]["related"] = targets
    return {"schema_version": 1, "terms": terms, "lessons": selected}


def require_versions(previous, current):
    old = {record["id"]: record for record in previous}
    for record in current:
        before = old.get(record["id"])
        if before and record != before and record["content_version"] <= before["content_version"]:
            raise ValueError("Changed teaching content requires an increased content_version: " + record["id"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--init-map", action="store_true", help="Assign IDs once; refuses to replace an existing map")
    args = parser.parse_args()
    subjects, entries, sources = extract()
    if args.init_map:
        if args.check or MAPPING.exists():
            raise ValueError("Initial mapping already exists or --check was supplied")
        MAPPING.write_bytes(encoded(initial_map(subjects, entries, sources)))
    mapping = json.loads(MAPPING.read_text())
    catalogue = publish(mapping, entries, sources)
    notation = apply_reviews(catalogue, json.loads(REVIEWS.read_text()),
                             json.loads((ROOT / "content/references/math_notation.json").read_text()))
    if OUTPUT.exists():
        previous = [{"id": e["id"], **e["review"]} for e in json.loads(OUTPUT.read_text())["entries"] if "review" in e]
        require_versions(previous, [{"id": e["id"], **e["review"]} for e in catalogue["entries"] if "review" in e])
    if NOTATION.exists():
        previous = json.loads(NOTATION.read_text())
        for kind in ("terms", "lessons"):
            require_versions(previous[kind], notation[kind])
    outputs = {OUTPUT: encoded(catalogue), NOTATION: encoded(notation)}
    if args.check:
        if any(path.read_bytes() != data for path, data in outputs.items()):
            raise ValueError("Published table of contents or notation differs; regenerate it")
    else:
        for path, data in outputs.items():
            path.parent.mkdir(parents=True, exist_ok=True)
            temporary = path.with_suffix(".tmp")
            temporary.write_bytes(data)
            temporary.replace(path)
    print(f"Corpus TOC: {len(subjects)} subjects, {len(mapping['topics'])} topics, {len(entries)} entries")


if __name__ == "__main__":
    main()
