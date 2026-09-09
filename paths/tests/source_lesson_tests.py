#!/usr/bin/env python3
"""Source-card preparation and publication integration; no native host or fonts."""
from __future__ import annotations

import argparse
from fractions import Fraction as F
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "tools"))
from export_learning import ExportError, capture, encoded, sha
from prepare_source_lesson import prepare


def check(value, message):
    if not value:
        raise AssertionError(message)


def run(*args):
    result = subprocess.run([str(a) for a in args], capture_output=True, text=True, timeout=90)
    check(result.returncode == 0, result.stdout + result.stderr)
    return result.stdout


def exact_math(text):
    section = text.split("@question source_002_reduction |", 1)[1]
    def matrix(raw):
        return [[F(n.strip()) for n in row.replace("|", ",").split(",")]
                for row in re.findall(r"\[([^]]+)\]", raw)]
    current = matrix(re.search(r"^@given (.+)$", section, re.M)[1])
    check(current == [[1, -1, 1], [1, 1, 2]], "Reduced givens retain the original two equations")
    for step in section.split("@step ")[1:]:
        op = re.search(r"^@operation (.+)$", step, re.M)[1]
        answer = re.search(r"^@answer (\d+)$", step, re.M)[1]
        expected = matrix(re.search(r"^@after (.+)$", step, re.M)[1])
        matches = []
        for key, raw in re.findall(r"^@choice (\d+) \| (.+)$", step, re.M):
            k = F(raw); candidate = [row[:] for row in current]
            if op == "add_row_1_to_2":
                candidate[1] = [b + k*a for a, b in zip(*current)]
            elif op == "divide_row_2":
                candidate[1] = [b/k for b in current[1]]
            elif op == "add_row_2_to_1":
                candidate[0] = [a + k*b for a, b in zip(*current)]
            else:
                raise AssertionError("Unexpected operation in this reviewed example")
            if candidate == expected:
                matches.append(key)
        check(matches == [answer], "Independent fractions agree with every answer and reject every distractor")
        current = expected
    a, b, c = current[0][2], current[1][2], F(0)
    check((a, b, c) == (F(3, 2), F(1, 2), 0), "Independently recovered coefficients")
    for x, y in ((-1, 1), (0, 0), (1, 2)):
        check(a*x*x+b*x+c == y, "Original observation is satisfied exactly")
    # Direct determinant expansion, independent of the app's matrix owner.
    m = [[1, -1, 1], [0, 0, 1], [1, 1, 1]]
    determinant = sum(m[0][i] * (m[1][(i+1)%3]*m[2][(i+2)%3]-m[1][(i+2)%3]*m[2][(i+1)%3]) for i in range(3))
    check(determinant == -2, "The stated uniqueness claim has the stated nonzero determinant")


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--parser-root", type=Path, required=True)
    cli.add_argument("--target", type=Path, default=ROOT/"b/sorter")
    cli.add_argument("--model", type=Path, default=ROOT/"b/paths_learning_document_tests")
    args = cli.parse_args()
    reviewed = ROOT/"content/authoring/learning/claude_002"
    with tempfile.TemporaryDirectory(prefix="paths-source-lesson-") as temporary:
        temp = Path(temporary).resolve(); mapping = temp/"review"; mapping.mkdir(); source = temp/"source"; source.mkdir()
        review = json.loads((reviewed/"review.json").read_bytes())
        # Frozen source snapshot avoids any dependency on a learner's Attempt file.
        (source/"002_quadratic_through_three_points.md").write_bytes((ROOT/"content/source_snapshots/math/002_quadratic_through_three_points.md").read_bytes())
        (source/"999_stale_draft.md").write_text("Bad draft with no reviewed mapping.\n@question stale\n")
        for item in review["inputs"]:
            if item["root"] == "source":
                continue
            origin = (ROOT if item["root"] == "repo" else reviewed)/item["path"]
            name = item["id"] + origin.suffix
            (mapping/name).write_bytes(origin.read_bytes());item.update(root="review", path=name)
        review_path = mapping/"review.json"; review_path.write_bytes(encoded(review))
        out, duplicate = temp/"prepared", temp/"duplicate"
        prepare(review_path, source, args.parser_root, out)
        prepare(review_path, source, args.parser_root, duplicate)
        before = capture(out)
        check(before == capture(duplicate), "Full parser audit and prepared documents reproduce byte for byte")
        check(before["audit/source.md"] == (source/"002_quadratic_through_three_points.md").read_bytes(), "Full source bytes retained outside the learner documents")
        document = before["documents/chapter.paths.md"].decode()
        check("## Attempt" not in document and "not attempted" not in document and "stale" not in document, "Learner state and unselected drafts excluded")
        exact_math(document)
        rejected = 0
        def refused(code):
            nonlocal rejected
            try:
                prepare(review_path, source, args.parser_root, out)
            except ExportError as error:
                check(error.code == code, f"Expected {code}, got {error.code}")
                if code == "source.stale":
                    check(bool(error.diagnostics[0]["file"]) and error.diagnostics[0]["field"].endswith(".sha256"), "Stale input is identified by file and field")
            else:
                raise AssertionError("Invalid source mapping was accepted")
            check(capture(out) == before, "Rejected preparation preserves the earlier result")
            rejected += 1
        for item in review["inputs"]:
            file = (source if item["root"] == "source" else mapping)/item["path"]
            original = file.read_bytes();file.write_bytes(original+b"\nChanged after review.\n")
            refused("source.stale");file.write_bytes(original)
        for field, value, code in (("status", "draft", "source.review"), ("parser_version", "0.0.0", "parser.version")):
            changed = dict(review);changed[field] = value;review_path.write_bytes(encoded(changed));refused(code)
        review_path.write_bytes(encoded(review))
        print(run(args.model, "--source-lesson", out/"documents").strip())
        store, package = temp/"store", temp/"package"
        result = json.loads(run(sys.executable, "-B", ROOT/"tools/export_learning.py", "publish", out,
                                "--target", args.target, "--store", store, "--output", package))
        check(result["published"], "Prepared lesson publishes through the existing exporter")
        check("audit/source.md" not in capture(package), "Raw source and learner state are not in the portable runtime pack")
        print(run(args.model, "--source-store", store).strip())
        active = (store/"active.json").read_bytes()
        frozen = capture(package)
        payload = package/"documents/chapter.paths.md";payload.write_bytes(payload.read_bytes()+b"\nChanged after export.\n")
        failure = subprocess.run([sys.executable, "-B", str(ROOT/"tools/export_learning.py"), "install", str(package),
                                  "--target", str(args.target), "--store", str(store)], capture_output=True, text=True, timeout=90)
        check(failure.returncode != 0 and (store/"active.json").read_bytes() == active, "Changed exported text cannot replace the active library")
        check(frozen["documents/chapter.paths.md"] != payload.read_bytes(), "Tamper probe exercised changed bytes")
        print(json.dumps(dict(source_lesson=True, deterministic=True, stale_or_draft_rejections=rejected,
                              arithmetic="independent exact fractions", publication=True, tamper_rejected=True,
                              native_windows=0, captures=0)))


if __name__ == "__main__":
    main()
