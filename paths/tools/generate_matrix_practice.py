#!/usr/bin/env python3
"""Publish P037 matrix questions and a separately loadable 100-card catalogue."""
import argparse
import copy
from fractions import Fraction
import json
from pathlib import Path
import re
import tempfile


PREFIX = "sorter_matrix_practice_"
GROUPS = {"integers": "Integer foundations", "signed": "Swaps and negatives", "fractions": "Exact fractions"}
OPERATIONS = {"swap_rows", "divide_row_1", "divide_row_2", "add_row_1_to_2", "add_row_2_to_1"}
CATALOGUE = "content/sorter/matrix_practice_v1.json"


def unique_fields(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON field: {key}")
        result[key] = value
    return result


def exact(value):
    if not isinstance(value, str) or not re.fullmatch(r"-?\d+(?:/[1-9]\d*)?", value):
        raise ValueError("matrix practice: use an exact integer or fraction string")
    number = Fraction(value)
    if str(number) != value or abs(number.numerator) > 10**9 or number.denominator > 10**9:
        raise ValueError("matrix practice: use reduced fractions within the live numeric limit")
    return number


def display(rows):
    cells = [[str(value) for value in row] for row in rows]
    widths = [max(len(row[column]) for row in cells) for column in range(3)]
    return "\n".join("[" + ", ".join(row[i].rjust(widths[i]) for i in range(2)) +
                     " | " + row[2].rjust(widths[2]) + "]" for row in cells)


def operation_label(operation, operand):
    if operation == "swap_rows":
        return "R1 <-> R2"
    value = exact(operand)
    if operation.startswith("divide_row_"):
        return f"R{operation[-1]} ÷ " + (f"({value})" if value < 0 else str(value))
    target, source = (2, 1) if operation == "add_row_1_to_2" else (1, 2)
    magnitude = abs(value)
    factor = "" if magnitude == 1 else str(magnitude) if magnitude.denominator == 1 else f"({magnitude})"
    return f"R{target} {'-' if value < 0 else '+'} {factor}R{source}"


def transform(rows, move):
    if not isinstance(move, dict) or set(move) != {"operation", "operand"} or move["operation"] not in OPERATIONS:
        raise ValueError("matrix practice: invalid row move")
    operation, operand = move["operation"], move["operand"]
    result = [row[:] for row in rows]
    if operation == "swap_rows":
        if operand != "":
            raise ValueError("matrix practice: a swap has no operand")
        result.reverse()
    else:
        number = exact(operand)
        if number == 0:
            raise ValueError("matrix practice: a row operand must be nonzero")
        if operation.startswith("divide_row_"):
            target = int(operation[-1]) - 1
            result[target] = [value / number for value in rows[target]]
        else:
            target, source = (1, 0) if operation == "add_row_1_to_2" else (0, 1)
            result[target] = [a + number * b for a, b in zip(rows[target], rows[source])]
    if result == rows:
        raise ValueError("matrix practice: a move must change the matrix")
    for row in result:
        for value in row:
            exact(str(value))
    if len(display(result)) > 160:
        raise ValueError("matrix practice: result exceeds the live matrix text limit")
    return result


def validated_recipes(document):
    if (not isinstance(document, dict) or set(document) != {"schema_version", "recipes"} or
            type(document["schema_version"]) is not int or document["schema_version"] != 1 or
            not isinstance(document["recipes"], list) or not 1 <= len(document["recipes"]) <= 12):
        raise ValueError("matrix practice: expected version 1 and up to twelve recipes")
    expected_ids = list(range(6101, 6101 + len(document["recipes"])))
    if [recipe.get("sorter_id") for recipe in document["recipes"] if isinstance(recipe, dict)] != expected_ids:
        raise ValueError("matrix practice: ordered card IDs must start at 6101 and end by 6112")
    result = []
    for recipe in document["recipes"]:
        if set(recipe) != {"sorter_id", "content_version", "group", "skill", "rows", "answer", "routes"}:
            raise ValueError("matrix practice: unexpected recipe fields")
        identity, version = recipe["sorter_id"], recipe["content_version"]
        if type(identity) is not int or type(version) is not int or not 1 <= version <= 4294967295:
            raise ValueError("matrix practice: invalid identity or content version")
        expected_group = ("integers", "signed", "fractions")[(identity - 6101) // 4]
        if recipe["group"] != expected_group or not isinstance(recipe["skill"], str) or not 1 <= len(recipe["skill"]) <= 100:
            raise ValueError("matrix practice: invalid skill group or description")
        raw = recipe["rows"]
        if not isinstance(raw, list) or len(raw) != 2 or any(not isinstance(row, list) or len(row) != 3 for row in raw):
            raise ValueError("matrix practice: expected two augmented rows")
        rows = [[exact(value) for value in row] for row in raw]
        if rows[0][0] * rows[1][1] == rows[0][1] * rows[1][0]:
            raise ValueError("matrix practice: each problem needs exactly one solution")
        raw_answer = recipe["answer"]
        if not isinstance(raw_answer, list) or len(raw_answer) != 2:
            raise ValueError("matrix practice: expected x and y answers")
        x, y = map(exact, raw_answer)
        if any(a*x + b*y != c for a, b, c in rows):
            raise ValueError("matrix practice: answers must satisfy both original equations")
        solved = [[Fraction(1), Fraction(0), x], [Fraction(0), Fraction(1), y]]
        if rows == solved:
            raise ValueError("matrix practice: question must start unsolved")
        routes = recipe["routes"]
        if not isinstance(routes, list) or len(routes) != 2:
            raise ValueError("matrix practice: provide two distinct complete routes")
        states_by_route = []
        for route in routes:
            if (not isinstance(route, dict) or set(route) != {"name", "moves"} or
                    not isinstance(route["name"], str) or not 1 <= len(route["name"]) <= 80 or
                    not isinstance(route["moves"], list) or not 1 <= len(route["moves"]) <= 16):
                raise ValueError("matrix practice: invalid route")
            states = [rows]
            for move in route["moves"]:
                if states[-1] == solved:
                    raise ValueError("matrix practice: route continues after completion")
                states.append(transform(states[-1], move))
            if states[-1] != solved or len({display(state) for state in states}) != len(states):
                raise ValueError("matrix practice: route must reach the checked answer without cycling")
            states_by_route.append(states)
        if routes[0]["moves"] == routes[1]["moves"] or routes[0]["name"] == routes[1]["name"]:
            raise ValueError("matrix practice: routes must be distinct")
        result.append((recipe, states_by_route))
    return result


def chapter_entries(document):
    entries = []
    for recipe, states in validated_recipes(document):
        identity = PREFIX + str(recipe["sorter_id"])
        entries.append({"id": recipe["sorter_id"], "home_index": 0,
                        "text": display(states[0][0]).replace("\n", " "), "subject": "linear_algebra",
                        "hint": "Each row gives x, y and the right-hand value. Make the left block the identity matrix.",
                        "solve_pack": f"{identity}_pack.json",
                        "study": {"chapter": "Matrix practice", "type": GROUPS[recipe["group"]],
                                  "form": "Ax = b; [A | b] -> [I | x]"}})
    return entries


def assemble_catalogue(base_catalogue, entries):
    """Retain existing playable records; replace only the sorting-only tail."""
    if (not isinstance(base_catalogue, dict) or base_catalogue.get("schema_version") != 1 or
            not isinstance(base_catalogue.get("equations"), list) or len(base_catalogue["equations"]) != 100):
        raise ValueError("matrix practice: base catalogue must have exactly 100 records")
    old = copy.deepcopy(base_catalogue["equations"])
    chapter_ids = {entry["id"] for entry in entries}
    for record in old:
        if record.get("id") in chapter_ids and record.get("solve_pack") != f"{PREFIX}{record['id']}_pack.json":
            raise ValueError("matrix practice: chapter ID collides with existing content")
    retained = [record for record in old if record.get("id") not in chapter_ids]
    playable = [record for record in retained if "solve_pack" in record or "study" in record]
    filler = [record for record in retained if "solve_pack" not in record and "study" not in record]
    if len(playable) + len(entries) > 100:
        raise ValueError("matrix practice: cannot discard a playable question to fit the catalogue")
    records = playable + copy.deepcopy(entries) + filler[:100-len(playable)-len(entries)]
    if (len(records) != 100 or len({record["id"] for record in records}) != 100 or
            len({record["text"] for record in records}) != 100):
        raise ValueError("matrix practice: catalogue needs 100 distinct IDs and statements")
    for index, record in enumerate(records):
        record["home_index"] = index
    return {"schema_version": 1, "equations": records}


def matrix_practice_outputs(document, base_catalogue):
    outputs = {}
    for recipe, routes in validated_recipes(document):
        identity, states = PREFIX + str(recipe["sorter_id"]), routes[0]
        original = display(states[0])
        x, y = recipe["answer"]
        steps = []
        for index, (move, after) in enumerate(zip(recipe["routes"][0]["moves"], states[1:])):
            label = operation_label(move["operation"], move["operand"])
            wrong = [row[:] for row in after]
            target = 0 if move["operation"] == "swap_rows" else 1 if move["operation"] in {"divide_row_2", "add_row_1_to_2"} else 0
            wrong[target][2] += 1
            explanation = "Apply the row operation to all three columns."
            if index == len(states) - 2:
                checks = [f"({a})({x}) + ({b})({y}) = {c}" for a, b, c in recipe["rows"]]
                explanation = f"x = {x}, y = {y}. The original rows check: " + " and ".join(checks) + "."
            steps.append({"id": index+1, "layer_name": "ROW REDUCTION", "prompt": label,
                          "options": [{"id": 101, "label": display(after)}, {"id": 108, "label": display(wrong)}],
                          "accepted_option_ids": [101], "wrong_hint": "Check every value in the changed row.",
                          "explanation": explanation, "hint": "Keep the other row unchanged, except when swapping rows.",
                          "next_move": label, "semantics": {"purpose": "calculation", "completion": "any_accepted",
                                                            "before": (index+1)*10, "after": (index+2)*10}})
        outputs[f"content/cards/{identity}.json"] = {
            "schema_version": 1, "id": identity, "content_version": recipe["content_version"],
            "equation": original.replace("\n", " "), "skill": "row_reduction",
            "description": recipe["skill"] + ". Each augmented row contains the x coefficient, y coefficient and right-hand value. Make the left block the identity matrix.",
            "working_model": "matrix_rows", "working_states": [{"id": (index+1)*10, "display": display(state)} for index, state in enumerate(states)],
            "steps": steps, "concept_ids": ["row_swap", "row_scaling", "row_addition"]}
        outputs[f"content/sorter/{identity}_pack.json"] = {
            "schema_version": 1, "questions": [f"../cards/{identity}.json"],
            "reference_library": "../references/row_operations.json",
            "decks": {"solve": [{"question_id": identity, "content_version": recipe["content_version"]}]}}
    outputs[CATALOGUE] = assemble_catalogue(base_catalogue, chapter_entries(document))
    return outputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="compare every generated file without writing")
    parser.add_argument("--first-example", action="store_true", help="publish only 6101 for the initial live-logic proof")
    parser.add_argument("--output-root", type=Path, help="write a separate generated artifact tree")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    try:
        document = json.loads((root / "content/authoring/matrix_practice_recipes.json").read_text(), object_pairs_hook=unique_fields)
        if args.first_example:
            document["recipes"] = document["recipes"][:1]
        elif len(document["recipes"]) != 12:
            raise ValueError("matrix practice: publication requires all twelve recipes")
        base = json.loads((root / "content/sorter/study_practice_v1.json").read_text(), object_pairs_hook=unique_fields)
        outputs = matrix_practice_outputs(document, base)
        destination = args.output_root or root
        for relative, data in outputs.items():
            path = destination / relative
            if "working_states" in data and path.is_file():
                previous = json.loads(path.read_text())
                if previous != data and data["content_version"] <= previous["content_version"]:
                    raise ValueError(f"{path}: increase content_version before changing an existing question")
        for relative, data in outputs.items():
            path, expected = destination / relative, json.dumps(data, indent=2) + "\n"
            if args.check:
                if not path.is_file() or path.read_text() != expected:
                    raise ValueError(f"generated file differs: {path}")
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=path.parent, prefix=path.name + ".", suffix=".tmp", delete=False) as temporary:
                    temporary.write(expected)
                Path(temporary.name).replace(path)
    except (OSError, ValueError, TypeError) as error:
        parser.error(str(error))
    print(f"{len(document['recipes'])} matrix questions and 100-card standalone catalogue: " + ("unchanged" if args.check else "written"))


if __name__ == "__main__":
    main()
