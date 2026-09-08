"""Source fidelity and deterministic publication checks; no rendering or imports from Documents."""
import copy
from fractions import Fraction
import importlib.util
from itertools import combinations
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("toc", ROOT / "tools/generate_corpus_toc.py")
toc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(toc)


class CorpusTests(unittest.TestCase):
    def test_source_coverage_and_fidelity(self):
        data = json.loads(toc.OUTPUT.read_text())
        self.assertEqual(len(data["entries"]), 930)
        for subject in toc.SUBJECTS:
            path = "sections/" + subject + ".md"
            lines = (toc.SNAPSHOT / path).read_text().splitlines()
            terms = {i + 1 for i, line in enumerate(lines) if line.startswith("#### Term: ")}
            entries = [e for e in data["entries"] if e["source"] == path]
            self.assertEqual({e["first_line"] for e in entries if e["kind"] == "term"}, terms)
            for e in entries:
                self.assertEqual(e["body"], "\n".join(lines[e["first_line"]:e["last_line"]]).strip())
        self.assertTrue(all(not re.match(r"Round \d", t["title"]) for t in data["topics"]))

    def test_publication_and_stable_identity(self):
        subjects, entries, sources = toc.extract()
        mapping = json.loads(toc.MAPPING.read_text())
        output = toc.publish(mapping, entries, sources)
        self.assertEqual(output, toc.publish(mapping, list(reversed(entries)), sources))
        revised = copy.deepcopy(entries)
        revised[0]["title"] = "A renamed teaching title"
        revised[0]["topic_title"] = "A new teaching chapter"
        update = toc.publish(mapping, revised, sources)
        self.assertEqual(output["entries"][0]["id"], update["entries"][0]["id"])
        self.assertEqual(output["entries"][0]["topic"], update["entries"][0]["topic"])
        self.assertEqual(len(subjects), 6)
        with self.assertRaises(ValueError):
            toc.publish(mapping, entries, [])
        with self.assertRaises((KeyError, ValueError)):
            toc.publish(mapping, entries[:-1], sources)
        bad = copy.deepcopy(mapping)
        bad["entries"][1]["id"] = bad["entries"][0]["id"]
        with self.assertRaises(ValueError):
            toc.publish(bad, entries, sources)

    def test_review_publication_and_stale_sources(self):
        subjects, entries, sources = toc.extract()
        catalogue = toc.publish(json.loads(toc.MAPPING.read_text()), entries, sources)
        reviews = json.loads(toc.REVIEWS.read_text())
        base = json.loads((ROOT / "content/references/math_notation.json").read_text())
        original = copy.deepcopy(catalogue)
        notation = toc.apply_reviews(catalogue, reviews, base)
        self.assertEqual(toc.encoded(catalogue), toc.OUTPUT.read_bytes())
        self.assertEqual(toc.encoded(notation), toc.NOTATION.read_bytes())
        self.assertEqual({e["id"] for e in catalogue["entries"] if "review" in e},
                         {"corpus_00511", "corpus_00512", "corpus_00513", "corpus_00514"})
        for before, after in zip(original["entries"], catalogue["entries"]):
            self.assertEqual(before, {k: v for k, v in after.items() if k not in ("review", "related")})
        for edit in (lambda r: r["reviews"].append(r["reviews"][0]),
                     lambda r: r["reviews"][0].update(source_sha256="0" * 64),
                     lambda r: r["reviews"][0].update(source_ids=[]),
                     lambda r: r["reviews"][0].update(reviewed_on="2026-02-30"),
                     lambda r: r["reviews"][0].update(content_version=0),
                     lambda r: r["reviews"][0].update(definition="x" * 481)):
            bad = copy.deepcopy(reviews); edit(bad)
            with self.assertRaises(ValueError):
                toc.apply_reviews(copy.deepcopy(original), bad, base)
        for key in ("body", "title", "kind"):
            changed = copy.deepcopy(original)
            next(e for e in changed["entries"] if e["id"] == "corpus_00511")[key] += " changed"
            with self.assertRaises(ValueError):
                toc.apply_reviews(changed, reviews, base)
        for e in catalogue["entries"]:
            if "review" in e:
                term = next(t for t in notation["terms"] if t["id"] == e["id"])
                for field in ("meaning", "definition", "example", "content_version"):
                    self.assertEqual(term[field], e["review"][field])
        for kind in ("terms", "lessons"):
            revised = copy.deepcopy(notation[kind]);revised[-1]["title"] += " edited"
            with self.assertRaises(ValueError):
                toc.require_versions(notation[kind], revised)
            revised[-1]["content_version"] += 1
            toc.require_versions(notation[kind], revised)

    def test_reviewed_reading_links(self):
        _, entries, sources = toc.extract()
        catalogue = toc.publish(json.loads(toc.MAPPING.read_text()), entries, sources)
        reviews = json.loads(toc.REVIEWS.read_text())
        base = json.loads((ROOT / "content/references/math_notation.json").read_text())
        linked = copy.deepcopy(catalogue)
        notation = toc.apply_reviews(linked, reviews, base)
        self.assertEqual({e["id"]:e["related"] for e in linked["entries"] if "related" in e}, reviews["reading_links"])
        legacy = copy.deepcopy(reviews); legacy.pop("reading_links")
        self.assertEqual(notation, toc.apply_reviews(catalogue, legacy, base))
        self.assertEqual(catalogue, {**linked, "entries":[{k:v for k,v in e.items() if k != "related"} for e in linked["entries"]]})
        for links in ([], {"unknown":["corpus_00511"]}, {"corpus_00864":["corpus_00511"]},
                      {"corpus_00511":[]}, {"corpus_00511":"corpus_00512"},
                      {"corpus_00511":[None]}, {"corpus_00511":["unknown"]},
                      {"corpus_00511":["corpus_00864"]}, {"corpus_00511":["corpus_00511"]},
                      {"corpus_00511":["corpus_00512"]*2}, {"corpus_00511":["corpus_00512"]*5}):
            bad = copy.deepcopy(reviews);bad["reading_links"] = links
            with self.assertRaises(ValueError):
                toc.apply_reviews(copy.deepcopy(catalogue), bad, base)

    def test_reviewed_examples_with_independent_arithmetic(self):
        notes = json.loads(toc.REVIEWS.read_text())["reviews"]
        for note in notes:
            if "example_case" not in note:
                self.assertIn("system_cases", note)
                continue
            case = note["example_case"]
            rows = [[Fraction(v) for v in row] for row in case["before"]]
            solution = [Fraction(v) for v in case["solution"]]
            for move in case["moves"]:
                k = Fraction(move["operand"])
                if move["operation"] == "divide_row_2":
                    self.assertNotEqual(k, 0); rows[1] = [v/k for v in rows[1]]
                elif move["operation"] == "add_row_1_to_2":
                    rows[1] = [a+k*b for a,b in zip(rows[1],rows[0])]
                elif move["operation"] == "add_row_2_to_1":
                    rows[0] = [a+k*b for a,b in zip(rows[0],rows[1])]
                else:
                    self.fail("Unknown example operation")
                self.assertEqual(rows, [[Fraction(v) for v in row] for row in move["after"]])
                for a,b,c in rows:
                    self.assertEqual(a*solution[0]+b*solution[1], c)
        # A unique reduced form may represent many solutions or none.
        for x in map(Fraction, [-2,0,3]):
            self.assertEqual(x+(2-x), 2)  # [1,1|2] [0,0|0]
        self.assertNotEqual(Fraction(0), 1)  # [0,0|1] is inconsistent.

    def test_rank_nullity_and_consistency_examples(self):
        # Independent exact minor determinants, not the game's row-reduction kernel.
        def determinant(rows):
            if not rows:
                return Fraction(1)
            return sum((-1)**j * value * determinant([row[:j]+row[j+1:] for row in rows[1:]])
                       for j, value in enumerate(rows[0]))

        def rank(rows):
            for size in range(min(len(rows), len(rows[0])), 0, -1):
                for rr in combinations(range(len(rows)), size):
                    for cc in combinations(range(len(rows[0])), size):
                        if determinant([[rows[r][c] for c in cc] for r in rr]):
                            return size
            return 0

        def check(case):
            a = [[Fraction(v) for v in row] for row in case["coefficients"]]
            b = list(map(Fraction, case["rhs"]))
            m, n = len(a), len(a[0])
            self.assertEqual(len(b), m)
            self.assertTrue(all(len(row) == n for row in a))
            r, augmented = rank(a), rank([row+[value] for row, value in zip(a, b)])
            self.assertEqual(r, case["coefficient_rank"])
            self.assertEqual(augmented, case["augmented_rank"])
            self.assertEqual(n-r, case["nullity"])
            basis = [list(map(Fraction, v)) for v in case["kernel_basis"]]
            self.assertEqual(len(basis), n-r)
            self.assertTrue(all(len(v) == n for v in basis))
            self.assertEqual(rank(basis) if basis else 0, len(basis))
            for vector in basis:
                self.assertTrue(all(sum(x*y for x,y in zip(row,vector)) == 0 for row in a))
            if "particular" in case:
                particular = list(map(Fraction, case["particular"]))
                self.assertEqual(len(particular), n)
                self.assertEqual(r, augmented)
                self.assertEqual([sum(x*y for x,y in zip(row,particular)) for row in a], b)
                # Independent kernel vectors of dimension n-r plus a particular
                # solution certify the complete affine family, not just samples.
                for t in map(Fraction, ["-2", "0", "1/3", "5"]):
                    x = [value+sum((j+1)*t*v[i] for j,v in enumerate(basis)) for i,value in enumerate(particular)]
                    self.assertEqual([sum(c*y for c,y in zip(row,x)) for row in a], b)
            else:
                self.assertGreater(augmented, r)
                witness = list(map(Fraction, case["inconsistency_witness"]))
                self.assertEqual(len(witness), m)
                self.assertTrue(all(sum(witness[i]*a[i][j] for i in range(m)) == 0 for j in range(n)))
                self.assertNotEqual(sum(x*y for x,y in zip(witness,b)), 0)

        notes = [note for note in json.loads(toc.REVIEWS.read_text())["reviews"] if "system_cases" in note]
        self.assertEqual([n["entry_id"] for n in notes], ["corpus_00513", "corpus_00514"])
        for note in notes:
            case = note["system_cases"][0]
            displayed = " ".join("["+",".join(row)+"|"+b+"]" for row,b in zip(case["coefficients"],case["rhs"]))
            self.assertIn(displayed, note["example"])
            for case in note["system_cases"]:
                check(case)
        # A full set of coefficient pivots still cannot cancel a contradiction;
        # other boundaries cover nonleading pivots, fractions, zero rank and uniqueness.
        for case in [
            {"coefficients":[["1"],["2"]],"rhs":["1","3"],"coefficient_rank":1,"augmented_rank":2,"nullity":0,"kernel_basis":[],"inconsistency_witness":["-2","1"]},
            {"coefficients":[["0","1","1/2"],["0","2","1"]],"rhs":["3","6"],"coefficient_rank":1,"augmented_rank":1,"nullity":2,"kernel_basis":[["1","0","0"],["0","-1/2","1"]],"particular":["0","3","0"]},
            {"coefficients":[["0","0"],["0","0"]],"rhs":["0","0"],"coefficient_rank":0,"augmented_rank":0,"nullity":2,"kernel_basis":[["1","0"],["0","1"]],"particular":["0","0"]},
            {"coefficients":[["1","2"],["3","5"]],"rhs":["4","11"],"coefficient_rank":2,"augmented_rank":2,"nullity":0,"kernel_basis":[],"particular":["2","1"]},
        ]:
            check(case)


if __name__ == "__main__":
    unittest.main()
