"""Independent mathematics, coverage and publication checks for the P037 chapter."""
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
SPEC = importlib.util.spec_from_file_location("matrix_generator", ROOT / "tools/generate_matrix_practice.py")
GENERATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GENERATOR)
DOCUMENT = json.loads((ROOT / "content/authoring/matrix_practice_recipes.json").read_text())
BASE = json.loads((ROOT / "content/sorter/study_practice_v1.json").read_text())
IDS = list(range(6101, 6113))
ANSWERS = [(3, 2), (4, 3), (1, 3), (2, 3), (4, -3), (-4, 3), (-7, 1), (3, -2),
           (3, 2), (4, 2), (Fraction(1, 2), 1), (Fraction(2, 3), Fraction(-1, 3))]


def matrix(text):
    rows = re.fullmatch(r"\[([^,]+),([^|]+)\|([^\]]+)\]\s*\[([^,]+),([^|]+)\|([^\]]+)\]", text)
    if rows is None:
        raise AssertionError(f"not a displayed two-row augmented matrix: {text}")
    cells = [Fraction(value.strip()) for value in rows.groups()]
    return [cells[:3], cells[3:]]


def apply(rows, key, operand):
    # Independent six-cell row arithmetic; no generator helper or live checker.
    if key == "swap_rows":
        if operand:
            raise AssertionError("swap operand must be empty")
        return [rows[1][:], rows[0][:]]
    value = Fraction(operand)
    if key in {"divide_row_1", "divide_row_2"}:
        target = {"divide_row_1": 0, "divide_row_2": 1}[key]
        return [[cell / value for cell in row] if index == target else row[:]
                for index, row in enumerate(rows)]
    target, source = {"add_row_1_to_2": (1, 0), "add_row_2_to_1": (0, 1)}[key]
    return [[rows[target][column] + value * rows[source][column] for column in range(3)]
            if index == target else row[:] for index, row in enumerate(rows)]


def apply_symbol(rows, symbol):
    if symbol == "R1 <-> R2":
        return [rows[1][:], rows[0][:]]
    divide = re.fullmatch(r"R([12]) ÷ (.+)", symbol)
    if divide:
        return apply(rows, "divide_row_" + divide[1], divide[2].strip("()"))
    add = re.fullmatch(r"R([12]) ([+-]) (.*?)R([12])", symbol)
    if add is None or add[1] == add[4]:
        raise AssertionError(f"unknown displayed row operation: {symbol}")
    factor = Fraction(add[3].strip("()") or "1") * (1 if add[2] == "+" else -1)
    return apply(rows, f"add_row_{add[4]}_to_{add[1]}", str(factor))


class MatrixPracticeRecipes(unittest.TestCase):
    def setUp(self):
        self.outputs = GENERATOR.matrix_practice_outputs(DOCUMENT, BASE)

    def test_twelve_exact_original_solutions_and_skill_coverage(self):
        self.assertEqual([recipe["sorter_id"] for recipe in DOCUMENT["recipes"]], IDS)
        fractional_answers = 0
        for index, recipe in enumerate(DOCUMENT["recipes"]):
            card = self.outputs[f"content/cards/sorter_matrix_practice_{recipe['sorter_id']}.json"]
            a, b, c = matrix(card["equation"])[0]
            d, e, f = matrix(card["equation"])[1]
            determinant = a * e - b * d
            self.assertNotEqual(determinant, 0)
            x, y = (c * e - b * f) / determinant, (a * f - c * d) / determinant
            self.assertEqual((x, y), ANSWERS[index], "independent determinant solution agrees with the result sheet")
            self.assertEqual(a * x + b * y, c)
            self.assertEqual(d * x + e * y, f)
            self.assertEqual(recipe["answer"], [str(x), str(y)])
            self.assertEqual(card["content_version"], 1)
            self.assertEqual(card["working_model"], "matrix_rows")
            self.assertEqual(card["concept_ids"], ["row_swap", "row_scaling", "row_addition"])
            states = [matrix(state["display"]) for state in card["working_states"]]
            self.assertEqual(states[-1], [[1, 0, x], [0, 1, y]])
            if index < 4:
                self.assertEqual(recipe["group"], "integers")
                self.assertTrue(all(cell.denominator == 1 for state in states for row in state for cell in row))
            elif index < 8:
                self.assertEqual(recipe["group"], "signed")
                self.assertIn("swap_rows", [move["operation"] for move in recipe["routes"][0]["moves"]])
                self.assertTrue(any(cell < 0 for row in states[0] for cell in row))
            else:
                self.assertEqual(recipe["group"], "fractions")
                self.assertTrue(any(cell.denominator > 1 for state in states for row in state for cell in row))
                fractional_answers += x.denominator > 1 or y.denominator > 1
        self.assertGreaterEqual(fractional_answers, 2)

    def test_authored_symbols_steps_options_and_two_independent_routes(self):
        for recipe in DOCUMENT["recipes"]:
            with self.subTest(id=recipe["sorter_id"]):
                card = self.outputs[f"content/cards/sorter_matrix_practice_{recipe['sorter_id']}.json"]
                states = {state["id"]: matrix(state["display"]) for state in card["working_states"]}
                self.assertEqual(states[10], matrix(card["equation"]))
                self.assertEqual(len(states), len(card["steps"]) + 1)
                for index, step in enumerate(card["steps"]):
                    self.assertEqual(step["semantics"], {"purpose": "calculation", "completion": "any_accepted",
                                                        "before": (index + 1) * 10, "after": (index + 2) * 10})
                    expected = apply_symbol(states[step["semantics"]["before"]], step["prompt"])
                    self.assertEqual(expected, states[step["semantics"]["after"]])
                    self.assertEqual(step["prompt"], step["next_move"])
                    accepted = [option["id"] for option in step["options"] if matrix(option["label"]) == expected]
                    self.assertEqual(accepted, step["accepted_option_ids"])
                    self.assertEqual(len({option["label"] for option in step["options"]}), len(step["options"]))
                    self.assertTrue(step["hint"] and step["wrong_hint"] and step["explanation"])
                paths = []
                for route in recipe["routes"]:
                    working = matrix(card["equation"])
                    path = [working]
                    for move in route["moves"]:
                        working = apply(working, move["operation"], move["operand"])
                        self.assertNotIn(working, path, "alternate routes must do useful work without cycling")
                        path.append(working)
                    self.assertEqual(working, states[max(states)])
                    paths.append(path)
                self.assertNotEqual(paths[0][1], paths[1][1], "routes start with genuinely different working")
                self.assertIn(f"x = {recipe['answer'][0]}, y = {recipe['answer'][1]}", card["steps"][-1]["explanation"])

    def test_catalogue_preserves_old_questions_and_only_replaces_tail_filler(self):
        base_copy = copy.deepcopy(BASE)
        GENERATOR.matrix_practice_outputs(DOCUMENT, BASE)
        records = self.outputs[GENERATOR.CATALOGUE]["equations"]
        self.assertEqual(BASE, base_copy)
        self.assertEqual(len(records), 100)
        self.assertEqual(len({record["id"] for record in records}), 100)
        self.assertEqual(len({record["text"] for record in records}), 100)
        self.assertEqual([record["home_index"] for record in records], list(range(100)))
        old = [record for record in BASE["equations"] if record["id"] not in IDS]
        playable = [record for record in old if "solve_pack" in record or "study" in record]
        filler = [record for record in old if "solve_pack" not in record and "study" not in record]
        self.assertEqual(len(playable), 15)
        expected_ids = [record["id"] for record in playable] + IDS + [record["id"] for record in filler[:73]]
        self.assertEqual([record["id"] for record in records], expected_ids)
        actual = {record["id"]: record for record in records}
        for record in old:
            if record["id"] in actual:
                self.assertEqual({k: v for k, v in record.items() if k != "home_index"},
                                 {k: v for k, v in actual[record["id"]].items() if k != "home_index"})
            else:
                self.assertIn(record, filler[73:])
        self.assertEqual(actual[6001]["study"], {"chapter": "Matrices and systems", "type": "Row reduction",
                                              "form": "Ax = b; [A | b] -> [I | x]"})
        self.assertEqual({actual[identity]["study"]["type"] for identity in IDS},
                         {"Integer foundations", "Swaps and negatives", "Exact fractions"})
        self.assertEqual(GENERATOR.matrix_practice_outputs(DOCUMENT, self.outputs[GENERATOR.CATALOGUE]), self.outputs)

    def test_scoped_output_files_references_and_byte_reproducibility(self):
        expected_paths = {GENERATOR.CATALOGUE}
        for identity in IDS:
            expected_paths.add(f"content/cards/sorter_matrix_practice_{identity}.json")
            path = f"content/sorter/sorter_matrix_practice_{identity}_pack.json"
            expected_paths.add(path)
            self.assertEqual(self.outputs[path], {
                "schema_version": 1, "questions": [f"../cards/sorter_matrix_practice_{identity}.json"],
                "reference_library": "../references/row_operations.json",
                "decks": {"solve": [{"question_id": f"sorter_matrix_practice_{identity}", "content_version": 1}]}})
        self.assertEqual(set(self.outputs), expected_paths)
        self.assertEqual(self.outputs, GENERATOR.matrix_practice_outputs(copy.deepcopy(DOCUMENT), copy.deepcopy(BASE)))
        for path, expected in self.outputs.items():
            self.assertEqual((ROOT / path).read_text(), json.dumps(expected, indent=2) + "\n", path)
        subprocess.run([sys.executable, str(ROOT / "tools/generate_matrix_practice.py"), "--check"], check=True, capture_output=True)

    def test_recipe_refusal_boundaries(self):
        def bad(edit):
            document = copy.deepcopy(DOCUMENT)
            edit(document)
            with self.assertRaises(ValueError):
                GENERATOR.matrix_practice_outputs(document, BASE)
        bad(lambda doc: doc.update(schema_version=True))
        bad(lambda doc: doc["recipes"][1].update(sorter_id=6101))
        bad(lambda doc: doc["recipes"][0].update(content_version=0))
        bad(lambda doc: doc["recipes"][0].update(group="fractions"))
        bad(lambda doc: doc["recipes"][0].update(rows=[["1", "1", "2"], ["2", "2", "4"]]))
        bad(lambda doc: doc["recipes"][0].update(answer=["2", "3"]))
        bad(lambda doc: doc["recipes"][0]["rows"][0].__setitem__(0, "2/2"))
        bad(lambda doc: doc["recipes"][0]["rows"][0].__setitem__(0, "1/0"))
        bad(lambda doc: doc["recipes"][0]["rows"][0].__setitem__(0, "1000000001"))
        bad(lambda doc: doc["recipes"][0]["rows"][0].__setitem__(0, 1.0))
        bad(lambda doc: doc["recipes"][0]["routes"][0]["moves"][0].update(operation="multiply_row_1"))
        bad(lambda doc: doc["recipes"][0]["routes"][0]["moves"][0].update(operation="divide_row_1", operand="0"))
        bad(lambda doc: doc["recipes"][0]["routes"][1]["moves"][0].update(operand="1"))
        bad(lambda doc: doc["recipes"][0]["routes"].__setitem__(1, copy.deepcopy(doc["recipes"][0]["routes"][0])))
        bad(lambda doc: doc["recipes"][0]["routes"][0]["moves"].append({"operation": "swap_rows", "operand": ""}))
        with self.assertRaises(ValueError):
            json.loads('{"schema_version": 1, "schema_version": 2}', object_pairs_hook=GENERATOR.unique_fields)

    def test_catalogue_collision_and_playable_capacity_refusals(self):
        entries = GENERATOR.chapter_entries(DOCUMENT)
        with self.assertRaisesRegex(ValueError, "chapter ID collides"):
            GENERATOR.assemble_catalogue(BASE, entries)
        self.assertEqual(GENERATOR.assemble_catalogue(BASE, entries, replace_existing=True), BASE,
                         "refreshing an existing chapter requires an explicit same-pack replacement")
        collision = copy.deepcopy(BASE)
        collision["equations"][-1]["id"] = 6101
        with self.assertRaises(ValueError):
            GENERATOR.matrix_practice_outputs(DOCUMENT, collision)
        full = copy.deepcopy(BASE)
        for record in full["equations"]:
            if record["id"] in IDS:
                record["id"] += 10000
            record["solve_pack"] = "retained_playable.json"
        with self.assertRaises(ValueError):
            GENERATOR.matrix_practice_outputs(DOCUMENT, full)

    def test_first_example_staging_and_version_guard_preserve_existing_files(self):
        with tempfile.TemporaryDirectory(prefix="paths-matrix-publication-") as directory:
            output = Path(directory)
            command = [sys.executable, str(ROOT / "tools/generate_matrix_practice.py"), "--first-example", "--output-root", directory]
            subprocess.run(command, check=True, capture_output=True)
            files = list(output.rglob("*.json"))
            self.assertEqual(len(files), 3)
            target = output / "content/cards/sorter_matrix_practice_6101.json"
            old = json.loads(target.read_text())
            old["description"] = "Saved question with the same version and different teaching content."
            target.write_text(json.dumps(old, indent=2) + "\n")
            before = {str(path): path.read_bytes() for path in files}
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("increase content_version", result.stderr)
            self.assertEqual(before, {str(path): path.read_bytes() for path in files})


if __name__ == "__main__":
    unittest.main()
