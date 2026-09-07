"""Independent arithmetic and publication-boundary checks for prepared recipes."""
import copy
from fractions import Fraction
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
SPEC = importlib.util.spec_from_file_location("generator", ROOT / "tools/generate_sorter_fixture.py")
GENERATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GENERATOR)
RECIPES = json.loads((ROOT / "content/authoring/linear_bracket_recipes.json").read_text())


class BracketRecipes(unittest.TestCase):
    def check_math(self, outputs):
        for path, card in outputs.items():
            if "working_states" not in card:
                continue
            self.assertEqual(card["working_model"], "linear_moves")
            # Decode the displayed equation, independently of the recipe fields.
            match = re.fullmatch(r"(-?\d+)\(x ([+-]) (\d+)\) = (-?\d+)", card["equation"])
            self.assertIsNotNone(match, path)
            a, b, c = int(match[1]), int(match[3]) * (1 if match[2] == "+" else -1), int(match[4])
            quotient, answer = Fraction(c, a), Fraction(c - a * b, a)
            self.assertEqual(a * (answer + b), c)
            self.assertEqual(card["working_states"][-1]["display"], f"x = {answer}")
            for i, step in enumerate(card["steps"]):
                self.assertTrue(step["hint"] and step["next_move"] and step["explanation"])
                choices = {option["id"]: option["label"] for option in step["options"]}
                self.assertEqual(len(choices), 4)
                self.assertEqual(len(set(choices.values())), 4)
                self.assertEqual(step["semantics"]["before"], card["working_states"][i]["id"])
                self.assertEqual(step["semantics"]["after"], card["working_states"][i + 1]["id"])
                if i % 2:
                    values = {identity: Fraction(label) for identity, label in choices.items()}
                    self.assertEqual(len(set(values.values())), 4)
                    expected = quotient if i == 1 else answer
                    self.assertEqual([identity for identity, value in values.items() if value == expected],
                                     step["accepted_option_ids"])
                else:
                    self.assertIn("Both sides", step["prompt"])
                    before = (Fraction(a), Fraction(a * b), Fraction(c)) if i == 0 else (Fraction(1), Fraction(b), quotient)
                    after = (Fraction(1), Fraction(b), quotient) if i == 0 else (Fraction(1), Fraction(0), answer)
                    accepted = []
                    for identity, label in choices.items():
                        symbol = re.fullmatch(r"([÷×+\-]) (\(-\d+\)|\d+)", label)
                        self.assertIsNotNone(symbol, "operation buttons contain symbols with parenthesized negative operands")
                        operand = Fraction(symbol[2].strip("()"))
                        slope, constant, rhs = before
                        transformed = {
                            "÷": tuple(value / operand for value in before),
                            "×": tuple(value * operand for value in before),
                            "-": (slope, constant - operand, rhs - operand),
                            "+": (slope, constant + operand, rhs + operand),
                        }[symbol[1]]
                        if transformed == after:
                            accepted.append(identity)
                    self.assertEqual(accepted, step["accepted_option_ids"], "displayed operation produces the prepared next equation")
            self.assertIn(f"= {c}. Both sides equal {c}.", card["steps"][-1]["explanation"])

    def test_reviewed_examples_and_reproducibility(self):
        outputs = GENERATOR.bracket_outputs(RECIPES)
        self.check_math(outputs)
        self.assertEqual(outputs, GENERATOR.bracket_outputs(copy.deepcopy(RECIPES)))
        pack = outputs["content/sorter/bracket_practice_v1.json"]["equations"]
        self.assertEqual(len(pack), 100)
        self.assertEqual(len({e["id"] for e in pack}), 100)
        self.assertEqual(len({e["text"] for e in pack}), 100)
        self.assertEqual([e["home_index"] for e in pack], list(range(100)))
        self.assertEqual([e["id"] for e in pack if "solve_pack" in e], [1012, 3001, 3002, 3003, 3004, 3005])
        mixed = json.loads((ROOT / "content/sorter/mixed_foundations_v1.json").read_text())
        existing = {e["id"]: e["text"] for e in mixed["equations"]}
        for record in pack:
            if record["id"] in existing:
                self.assertEqual(record["text"], existing[record["id"]], "shared card IDs retain the same equation across packs")
        reversed_input = copy.deepcopy(RECIPES)
        reversed_input["recipes"].reverse()
        reordered = GENERATOR.bracket_outputs(reversed_input)
        for path, card in outputs.items():
            if "working_states" in card:
                self.assertEqual(card, reordered[path], "recipe ordering cannot change question identity or content")

    def test_signs_zero_fractions_and_bounds(self):
        doc = copy.deepcopy(RECIPES)
        doc["recipes"] = [doc["recipes"][1]]
        recipe = doc["recipes"][0]
        for a in (-12, -2, 2, 12):
            for b in (-12, -1, 1, 12):
                for c in (-144, -1, 0, 1, 144, a * b):
                    recipe.update(coefficient=a, offset=b, rhs=c)
                    self.check_math(GENERATOR.bracket_outputs(doc))

    def test_refusal_fields(self):
        for key, invalid in (("coefficient", [0, 1, -1, 13, 2.0, True, "2"]),
                             ("offset", [0, 13, -13, None]), ("rhs", [145, -145, False, "10"]),
                             ("content_version", [0, -1, True, 4294967296]),
                             ("sorter_id", [0, 4294967296, 1001]),
                             ("id", ["../source", "source_002"]),
                             ("pack", ["../escape", "equations_v1", "bracket_practice_v1"])):
            for value in invalid:
                doc = copy.deepcopy(RECIPES)
                doc["recipes"][1][key] = value
                with self.assertRaisesRegex(ValueError, f"/recipes/1/{key}"):
                    GENERATOR.bracket_outputs(doc)
        for key in ("id", "sorter_id", "pack"):
            doc = copy.deepcopy(RECIPES)
            doc["recipes"][1][key] = doc["recipes"][0][key]
            with self.assertRaisesRegex(ValueError, f"/recipes/1/{key}"):
                GENERATOR.bracket_outputs(doc)
        for key, value in (("method", "expand_first"), ("schema_version", True), ("recipes", []),
                           ("recipes", RECIPES["recipes"] * 3)):
            doc = copy.deepcopy(RECIPES)
            doc[key] = value
            with self.assertRaisesRegex(ValueError, "/" + key):
                GENERATOR.bracket_outputs(doc)

    def test_command_refuses_before_writing(self):
        # Run an isolated copy of the real publisher, never write source fixtures.
        with tempfile.TemporaryDirectory(prefix="paths-bracket-") as directory:
            root = Path(directory)
            (root / "tools").mkdir()
            script = root / "tools/generate_sorter_fixture.py"
            script.write_bytes((ROOT / "tools/generate_sorter_fixture.py").read_bytes())
            (root / "tools/generate_matrix_practice.py").write_bytes((ROOT / "tools/generate_matrix_practice.py").read_bytes())
            graph_source = root / "content/authoring/line_graph_recipes.json"
            graph_source.parent.mkdir(parents=True)
            graph_source.write_bytes((ROOT / "content/authoring/line_graph_recipes.json").read_bytes())
            system_source = root / "content/authoring/system_graph_recipes.json"
            system_source.write_bytes((ROOT / "content/authoring/system_graph_recipes.json").read_bytes())
            (root / "content/authoring/matrix_practice_recipes.json").write_bytes((ROOT / "content/authoring/matrix_practice_recipes.json").read_bytes())
            matrix_source = root / "content/cards/sorter_matrix_rows.json"
            matrix_source.parent.mkdir(parents=True)
            matrix_source.write_bytes((ROOT / "content/cards/sorter_matrix_rows.json").read_bytes())
            source = root / "recipes.json"
            source.write_text(json.dumps(RECIPES))
            command = [sys.executable, str(script), "--recipes", str(source)]
            subprocess.run(command, check=True, capture_output=True)
            files = {p.relative_to(root): p.read_bytes() for p in root.rglob("*.json") if p != source}
            def unchanged():
                self.assertEqual(files, {p.relative_to(root): p.read_bytes() for p in root.rglob("*.json") if p != source})
            for identity in ("sorter_graph_positive", "sorter_system_integer", "sorter_matrix_rows", "sorter_matrix_practice_6101"):
                with self.subTest(collision=identity):
                    broken = copy.deepcopy(RECIPES)
                    broken["recipes"][0].update(id=identity, pack="bracket_collision_probe", sorter_id=3501, content_version=10)
                    source.write_text(json.dumps(broken))
                    result = subprocess.run(command, capture_output=True, text=True)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn("publication path already owned", result.stderr)
                    self.assertIn(identity + ".json", result.stderr)
                    unchanged()
            broken = copy.deepcopy(RECIPES)
            broken["recipes"][-1]["coefficient"] = 0
            source.write_text(json.dumps(broken))
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("/recipes/5/coefficient", result.stderr)
            unchanged()
            source.write_text(json.dumps(RECIPES).replace('"coefficient": -2', '"coefficient": 4, "coefficient": -2'))
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("duplicate JSON field: coefficient", result.stderr)
            unchanged()
            broken = copy.deepcopy(RECIPES)
            broken["recipes"][1]["rhs"] = 22
            source.write_text(json.dumps(broken))
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("/content_version", result.stderr)
            unchanged()
            source.write_text(json.dumps(RECIPES))
            subprocess.run(command + ["--check"], check=True, capture_output=True)
            unchanged()


class PublicationAssembly(unittest.TestCase):
    def test_chapters_preserve_playable_questions_and_refuse_overflow(self):
        full = {"schema_version": 1, "equations": [
            {"id": 9000+i, "home_index": i, "text": f"x = {10000+i}", "subject": "algebra",
             "solve_pack": f"retained_{i}.json", "study": {"chapter": "Retained", "type": "Equations", "form": "x = n"}}
            for i in range(100)]}
        del full["equations"][0]["study"]  # Playable questions without Contents metadata also survive.
        line = json.loads((ROOT / "content/authoring/line_graph_recipes.json").read_text())
        system = json.loads((ROOT / "content/authoring/system_graph_recipes.json").read_text())
        matrix = json.loads((ROOT / "content/cards/sorter_matrix_rows.json").read_text())
        study_path = "content/sorter/study_practice_v1.json"
        for prepare, document, source, identities in (
                (GENERATOR.line_graph_outputs, line, "content/sorter/bracket_practice_v1.json", {r["sorter_id"] for r in line["recipes"]}),
                (GENERATOR.system_graph_outputs, system, study_path, {r["sorter_id"] for r in system["recipes"]}),
                (GENERATOR.matrix_study_outputs, matrix, study_path, {6001})):
            with self.subTest(chapter=prepare.__name__):
                before = copy.deepcopy(full)
                with self.assertRaisesRegex(ValueError, "cannot discard a playable question"):
                    prepare(document, {source: full})
                self.assertEqual(full, before)
                room = copy.deepcopy(full)
                for record in room["equations"][-len(identities):]:
                    del record["solve_pack"], record["study"]
                before = copy.deepcopy(room)
                records = prepare(document, {source: room})[study_path]["equations"]
                self.assertEqual(room, before, "successful assembly also leaves its source catalogue untouched")
                self.assertEqual(len(records), 100)
                self.assertEqual([r["id"] for r in records[:-len(identities)]], [r["id"] for r in room["equations"][:-len(identities)]])
                self.assertEqual({r["id"] for r in records[-len(identities):]}, identities)
                self.assertEqual([r["home_index"] for r in records], list(range(100)))


class LineGraphs(unittest.TestCase):
    def test_equations_points_and_choices(self):
        recipes = json.loads((ROOT / "content/authoring/line_graph_recipes.json").read_text())
        baseline = GENERATOR.bracket_outputs(RECIPES)
        unchanged = copy.deepcopy(baseline)
        outputs = GENERATOR.line_graph_outputs(recipes, baseline)
        self.assertEqual(baseline, unchanged)
        self.assertEqual(outputs, GENERATOR.line_graph_outputs(copy.deepcopy(recipes), baseline))
        slopes = []
        for path, card in outputs.items():
            if "working_states" not in card:
                continue
            # Decode the printed equation independently of its graph metadata.
            expression = card["equation"].removeprefix("y = ")
            if "x" in expression:
                coefficient, constant = expression.split("x")
                slope = Fraction({"": "1", "-": "-1"}.get(coefficient, coefficient.strip("()")))
                intercept = Fraction(constant.replace(" ", "") or "0")
            else:
                slope, intercept = Fraction(0), Fraction(expression)
            slopes.append(slope)
            graph = card["line_graph"]
            run, rise = slope.denominator, slope.numerator
            self.assertEqual((graph["rise"], graph["run"], graph["intercept"]), (rise, run, intercept))
            expected = [intercept, run, rise, (run, slope * run + intercept)]
            for i, step in enumerate(card["steps"]):
                self.assertEqual(step["semantics"]["purpose"], "graph_choice")
                values = {o["id"]: tuple(Fraction(v) for v in o["label"].strip("()").split(",")) if i == 3
                          else Fraction(o["label"]) for o in step["options"]}
                self.assertEqual(len(set(values.values())), 4)
                self.assertEqual([key for key, value in values.items() if value == expected[i]], step["accepted_option_ids"])
            self.assertEqual([s["graph_stage"] for s in card["working_states"]], ["grid", "intercept", "run", "rise", "line"])
            self.assertLess(graph["x_min"], 0); self.assertGreater(graph["x_max"], run)
            self.assertLess(graph["y_min"], min(0, intercept, intercept+rise))
            self.assertGreater(graph["y_max"], max(0, intercept, intercept+rise))
        self.assertEqual(slopes, [Fraction(2), Fraction(-1), Fraction(1, 2), Fraction(0)])
        pack = outputs["content/sorter/study_practice_v1.json"]["equations"]
        self.assertEqual(sum("study" in e for e in pack), 10)
        for key in ("id", "home_index", "text"):
            self.assertEqual(len({e[key] for e in pack}), 100)
        for key, bad in (("run", 0), ("rise", True), ("intercept", 100), ("content_version", 0)):
            invalid = copy.deepcopy(recipes); invalid["recipes"][0][key] = bad
            with self.assertRaises(ValueError):
                GENERATOR.line_graph_outputs(invalid, baseline)


class SimultaneousGraphs(unittest.TestCase):
    def setUp(self):
        self.recipes = json.loads((ROOT / "content/authoring/system_graph_recipes.json").read_text())
        self.existing = GENERATOR.bracket_outputs(RECIPES)
        self.existing.update(GENERATOR.line_graph_outputs(json.loads((ROOT / "content/authoring/line_graph_recipes.json").read_text()), self.existing))

    def test_exact_systems_and_all_answer_options(self):
        baseline = copy.deepcopy(self.existing)
        outputs = GENERATOR.system_graph_outputs(self.recipes, self.existing)
        self.assertEqual(self.existing, baseline)
        outcomes = []
        for path, card in outputs.items():
            if "working_states" not in card:
                continue
            parsed = []
            # Independently decode the printed equations, including scaled y.
            for text in card["equation"].split("; "):
                lhs, rhs = text.split(": ", 1)[1].split(" = ")
                scale = Fraction(lhs.removesuffix("y") or "1")
                if "x" in rhs:
                    coefficient, constant = rhs.split("x")
                    slope = Fraction({"": "1", "-": "-1"}.get(coefficient, coefficient.strip("()"))) / scale
                    intercept = Fraction(constant.replace(" ", "") or "0") / scale
                else:
                    slope, intercept = Fraction(0), Fraction(rhs) / scale
                parsed.append((slope, intercept))
            (m1,b1),(m2,b2) = parsed
            graph = card["line_graph"]
            self.assertEqual((Fraction(graph["rise"],graph["run"]),graph["intercept"]), parsed[0])
            self.assertEqual((Fraction(graph["second"]["rise"],graph["second"]["run"]),graph["second"]["intercept"]), parsed[1])
            count = "1" if m1 != m2 else "0" if b1 != b2 else "∞"
            outcomes.append(count)
            if count == "1":
                x = (b2-b1)/(m1-m2); y = m1*x+b1
                self.assertEqual(y,m2*x+b2)
                self.assertTrue(graph["x_min"] < x < graph["x_max"] and graph["y_min"] < y < graph["y_max"])
                final = (x,y)
            else:
                final = b1-b2
                for x in (-3,0,Fraction(3,4),2):
                    self.assertEqual(m1*x+b1 == m2*x+b2,count == "∞")
            expected = [m1,m2,count,final]
            for i,step in enumerate(card["steps"]):
                def value(label):
                    if i == 2:return label
                    if i == 3 and count == "1":return tuple(Fraction(v) for v in label.strip("()").split(","))
                    return Fraction(label)
                choices = {o["id"]:value(o["label"]) for o in step["options"]}
                self.assertEqual(len(set(choices.values())),4)
                self.assertEqual([key for key,v in choices.items() if v == expected[i]],step["accepted_option_ids"])
            self.assertEqual([s["graph_stage"] for s in card["working_states"]],["grid","first_line","both_lines","classified","system_solution"])
        self.assertEqual(outcomes,["1","1","0","∞"])
        pack = outputs["content/sorter/study_practice_v1.json"]["equations"]
        self.assertEqual(sum("study" in e for e in pack),14)
        self.assertEqual(pack[:10],baseline["content/sorter/study_practice_v1.json"]["equations"][:10])
        self.assertEqual(len({e["text"] for e in pack}),100)
        self.assertEqual(outputs,GENERATOR.system_graph_outputs(copy.deepcopy(self.recipes),self.existing))

    def test_invalid_and_off_board_systems(self):
        for key,bad in (("run",0),("scale",0),("rise",True),("intercept",100)):
            invalid=copy.deepcopy(self.recipes);invalid["recipes"][0]["second"][key]=bad
            with self.assertRaises(ValueError):GENERATOR.system_graph_outputs(invalid,self.existing)
        invalid=copy.deepcopy(self.recipes)
        invalid["recipes"][0]["first"].update(rise=7,run=8,intercept=-8)
        invalid["recipes"][0]["second"].update(rise=6,run=7,intercept=8)
        with self.assertRaisesRegex(ValueError,"range"):GENERATOR.system_graph_outputs(invalid,self.existing)


class MatrixQuestion(unittest.TestCase):
    def test_authored_rows_and_catalogue(self):
        card = json.loads((ROOT / "content/cards/sorter_matrix_rows.json").read_text())
        def values(display):
            return [list(map(Fraction, (a, b, c))) for a, b, c in re.findall(r"\[([^,]+),([^|]+)\|([^\]]+)\]", display)]
        expected = [
            [[2, 1, 7], [1, -1, -1]], [[1, -1, -1], [2, 1, 7]],
            [[1, -1, -1], [0, 3, 9]], [[1, -1, -1], [0, 1, 3]], [[1, 0, 2], [0, 1, 3]]]
        self.assertEqual(values(card["equation"]), expected[0])
        self.assertEqual([values(s["display"]) for s in card["working_states"]], expected)
        for index, step in enumerate(card["steps"]):
            self.assertEqual([o["id"] for o in step["options"] if values(o["label"]) == expected[index + 1]], step["accepted_option_ids"])
        for row in expected[0]:
            self.assertEqual(row[0] * 2 + row[1] * 3, row[2])
        existing = GENERATOR.bracket_outputs(RECIPES)
        existing.update(GENERATOR.line_graph_outputs(json.loads((ROOT / "content/authoring/line_graph_recipes.json").read_text()), existing))
        existing.update(GENERATOR.system_graph_outputs(json.loads((ROOT / "content/authoring/system_graph_recipes.json").read_text()), existing))
        result = GENERATOR.matrix_study_outputs(card, existing)
        before = existing["content/sorter/study_practice_v1.json"]["equations"]
        records = result["content/sorter/study_practice_v1.json"]["equations"]
        self.assertEqual(records[:14], before[:14])
        self.assertEqual(len(records), 100)
        self.assertEqual(sum("study" in r for r in records), 15)
        self.assertEqual(records[14]["subject"], "linear_algebra")
        self.assertEqual(records[14]["text"], card["equation"])
        self.assertEqual(result, GENERATOR.matrix_study_outputs(copy.deepcopy(card), existing))


if __name__ == "__main__":
    unittest.main()
