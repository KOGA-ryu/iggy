#!/usr/bin/env python3
"""Author fixed algebra cards and prepared bracket/graph/matrix practice. No runtime generation."""
import argparse
import hashlib
import json
import re
from fractions import Fraction
from pathlib import Path
from generate_matrix_practice import assemble_catalogue, matrix_practice_outputs, publish_outputs, unique_fields


def shifted(expression, offset):
    return f"{expression} {'+' if offset > 0 else '-'} {abs(offset)}"


def fixture():
    equations, solutions = [], {}
    # A fixed permutation gives each family 25 solutions, -12 through 12.
    # Interleave all four forms. IDs deliberately do not encode home indices.
    for index in range(25):
        solution = (index * 7) % 25 - 12
        offset = (index % 9 + 1) * (-1 if index % 2 else 1)
        coefficient = (index % 7 + 2) * (-1 if index % 3 == 0 else 1)
        forms = [
            f"{shifted('x', offset)} = {solution + offset}",
            f"{coefficient}x = {coefficient * solution}",
            f"{shifted(f'{coefficient}x', offset)} = {coefficient * solution + offset}",
            f"{coefficient}({shifted('x', offset)}) = {coefficient * (solution + offset)}",
        ]
        inverse = f"{'Subtract' if offset > 0 else 'Add'} {abs(offset)} {'from' if offset > 0 else 'to'} both sides."
        hints = [
            f"Algebra: x is the unknown. {inverse}",
            f"Algebra: isolate x by dividing both sides by {coefficient}.",
            f"Algebra: {inverse} Then divide both sides by {coefficient}.",
            f"Algebra: divide both sides by {coefficient} first. Then {inverse[0].lower() + inverse[1:]}",
        ]
        for text, hint in zip(forms, hints):
            home = len(equations)
            identity = 1001 + (home * 37) % 100
            equations.append({"id": identity, "home_index": home, "text": text, "subject": "algebra", "hint": hint})
            if home == 3:
                equations[-1]["solve_pack"] = "linear_bracket_pack.json"
            solutions[str(identity)] = solution
    assert len({e['text'] for e in equations}) == 100
    return {"schema_version": 1, "equations": equations}, solutions


def bracket_outputs(document):
    """Validate a bounded recipe batch, then expand it with exact arithmetic.

    Work and output are O(N), with at most 16 recipes and four decisions each.
    Fraction reduces arithmetic choices before deduplication.
    """
    def require(condition, field, reason):
        if not condition:
            raise ValueError(f"{field}: {reason}")

    require(isinstance(document, dict), "", "expected an object")
    require(set(document) == {"schema_version", "method", "recipes"}, "", "expected schema_version, method and recipes")
    require(type(document["schema_version"]) is int and document["schema_version"] == 1,
            "/schema_version", "expected version 1")
    require(document["method"] == "divide_then_shift", "/method", "only divide_then_shift is supported")
    recipes = document["recipes"]
    require(isinstance(recipes, list) and 1 <= len(recipes) <= 16, "/recipes", "expected 1 through 16 recipes")
    identities, sorter_ids, packs, equations = set(), set(), set(), set()
    for index, recipe in enumerate(recipes):
        field = f"/recipes/{index}"
        require(isinstance(recipe, dict) and set(recipe) == {
            "id", "content_version", "sorter_id", "pack", "coefficient", "offset", "rhs"},
            field, "expected id, content_version, sorter_id, pack, coefficient, offset and rhs")
        for key in ("id", "pack"):
            require(isinstance(recipe[key], str) and re.fullmatch(r"[a-z][a-z0-9_]{0,63}", recipe[key]),
                    field + "/" + key, "expected a lowercase name of at most 64 characters")
        require(recipe["id"].startswith("sorter_"), field + "/id", "generated cards use the sorter_ namespace")
        require(recipe["pack"] == "linear_bracket_pack" or
                (recipe["pack"].startswith("bracket_") and recipe["pack"] != "bracket_practice_v1"),
                field + "/pack", "expected a bracket pack name, excluding bracket_practice_v1")
        for key in ("sorter_id", "content_version"):
            require(type(recipe[key]) is int and 1 <= recipe[key] <= 4294967295,
                    field + "/" + key, "expected a positive 32-bit integer")
        for key, low, high in (("coefficient", 2, 12), ("offset", 1, 12), ("rhs", 0, 144)):
            require(type(recipe[key]) is int and low <= abs(recipe[key]) <= high,
                    field + "/" + key, f"expected an integer with magnitude {low} through {high}")
        for key, seen in (("id", identities), ("sorter_id", sorter_ids), ("pack", packs)):
            require(recipe[key] not in seen, field + "/" + key, "duplicate identity")
            seen.add(recipe[key])
        if recipe["id"] == "sorter_linear_bracket" or recipe["pack"] == "linear_bracket_pack":
            require(recipe["id"] == "sorter_linear_bracket" and recipe["pack"] == "linear_bracket_pack" and
                    recipe["sorter_id"] == 1012, field, "the original question and its pack retain sorter identity 1012")
        values = tuple(recipe[key] for key in ("coefficient", "offset", "rhs"))
        require(values not in equations, field, "duplicate equation")
        equations.add(values)

    def arithmetic_options(answer, candidates):
        unique = [answer]
        for value in candidates:
            value = Fraction(value)
            if value not in unique:
                unique.append(value)
            if len(unique) == 4:
                return [str(value) for value in unique]
        raise ValueError("cannot form four distinct arithmetic choices")

    outputs, prepared = {}, []
    for recipe in recipes:
        a, b, c = (recipe[key] for key in ("coefficient", "offset", "rhs"))
        quotient = Fraction(c, a)
        answer = quotient - b
        require(a * (answer + b) == c, "/recipes", "final substitution does not agree")
        bracket = f"({shifted('x', b)})"
        equation = f"{a}{bracket} = {c}"
        divisor = f"({a})" if a < 0 else str(a)
        division = f"{c} / {divisor}"
        calculation = shifted(str(quotient), -b)
        operation, preposition, undoing = ("Subtract", "from", "adding") if b > 0 else ("Add", "to", "subtracting")
        inverse = f"{operation} {abs(b)} {preposition} both sides"
        displays = [equation, f"{shifted('x', b)} = {division}", f"{shifted('x', b)} = {quotient}",
                    f"x = {calculation}", f"x = {answer}"]
        verification = (f"Substitute x = {answer} into the original equation: "
                        f"{a}({shifted(str(answer), b)}) = {a}({quotient}) = {c}. Both sides equal {c}.")
        decisions = [
            ("REMOVE THE OUTSIDE FACTOR", "Both sides: remove the factor",
             [f"÷ {divisor}", f"× {divisor}", f"- {abs(a)}", f"+ {abs(a)}"],
             f"Dividing both sides by the nonzero factor {a} preserves equality and removes the outside factor.",
             "What is multiplying the entire bracket? Use the inverse of multiplication.",
             f"Divide both sides by {a}. This leaves {displays[1]}.", "operation_choice"),
            ("CALCULATE THE DIVISION", f"What is {c} divided by {a}?",
             arithmetic_options(quotient, [-quotient, c - a, c * a, quotient + 1, quotient - 1, quotient + 2]),
             f"{division} = {quotient}. The equation is now {displays[2]}.",
             "Divide the magnitudes, then check the signs. Keep a non-integer result as an exact fraction.",
             f"{division} = {quotient}, so {displays[2]}.", "calculation"),
            ("REMOVE THE CONSTANT", "Both sides: isolate x",
             [f"{'-' if b > 0 else '+'} {abs(b)}", f"{'+' if b > 0 else '-'} {abs(b)}", f"× {abs(a)}", f"÷ {abs(a)}"],
             f"{inverse} to preserve equality and isolate x.",
             f"The goal is to leave x alone. Which operation undoes {undoing} {abs(b)}?",
             f"{inverse}. This leaves {displays[3]}.", "operation_choice"),
            ("CALCULATE THE ANSWER", f"What is {calculation}?",
             arithmetic_options(answer, [quotient + b, -answer, quotient, answer + 1, answer - 1, answer + 2]),
             verification,
             f"{'Subtracting' if b > 0 else 'Adding'} {abs(b)} {'moves left' if b > 0 else 'moves right'} on the number line. Use a common denominator if needed.",
             f"{calculation} = {answer}, so {displays[4]}.", "calculation"),
        ]
        states = [{"id": (i + 1) * 10, "display": display} for i, display in enumerate(displays)]
        states[0]["highlights"] = [{"offset": 0, "length": len(str(a)), "label": "Outside factor"},
                                    {"offset": len(str(a)), "length": len(bracket), "label": "Whole bracket"}]
        steps = []
        for i, (name, prompt, options, explanation, hint, next_move, purpose) in enumerate(decisions):
            steps.append({"id": i + 1, "layer_name": name, "prompt": prompt,
                          "options": [{"id": 101 + 7 * j, "label": label} for j, label in enumerate(options)],
                          "accepted_option_ids": [101], "wrong_hint": "", "explanation": explanation,
                          "hint": hint, "next_move": next_move,
                          "semantics": {"purpose": purpose, "completion": "any_accepted",
                                        "before": states[i]["id"], "after": states[i + 1]["id"]}})
        identity, version = recipe["id"], recipe["content_version"]
        recipe_hash = hashlib.sha256(json.dumps(recipe, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
        outputs[f"content/cards/{identity}.json"] = {
            "schema_version": 1, "id": identity, "content_version": version, "equation": equation,
            "skill": "linear_equation", "description": f"The outside factor {a} multiplies the whole bracket {bracket}. "
            "Choose your own valid moves in the workspace. The prepared arcade sequence uses division first.",
            "working_model": "linear_moves",
            "preparation": {"method": document["method"], "generator_version": 3, "recipe_sha256": recipe_hash},
            "working_states": states, "steps": steps}
        outputs[f"content/sorter/{recipe['pack']}.json"] = {
            "schema_version": 1, "questions": [f"../cards/{identity}.json"],
            "decks": {"solve": [{"question_id": identity, "content_version": version}]}}
        prepared.append({"id": recipe["sorter_id"], "home_index": len(prepared), "text": equation,
                         "subject": "algebra", "hint": f"Divide both sides by {a} first. Then {inverse.lower()}.",
                         "solve_pack": f"{recipe['pack']}.json",
                         "study": {"chapter": "Linear equations", "type": "Bracket equations", "form": "a(x + b) = c"}})

    # Keep the 100-slot sorter contract. Prepared cards lead a separate pack;
    # original fixture/mixed-pack text, identities and slots remain unchanged.
    baseline, _ = fixture()
    by_id = {e["id"]: e for e in baseline["equations"]}
    for i, record in enumerate(prepared):
        require(record["id"] not in by_id or record["text"] == by_id[record["id"]]["text"],
                f"/recipes/{i}/sorter_id", "cannot reuse an existing sorter identity for a different equation")
    texts = {e["text"] for e in prepared}
    remaining = [e for e in baseline["equations"] if e["id"] not in sorter_ids and e["text"] not in texts]
    for record in remaining:
        record.pop("solve_pack", None)
    records = (prepared + remaining)[:100]
    require(len(records) == 100, "/recipes", "not enough distinct cards to fill the practice pack")
    for i, record in enumerate(records):
        record["home_index"] = i
    outputs["content/sorter/bracket_practice_v1.json"] = {"schema_version": 1, "equations": records}
    return outputs


def line_equation(rise, run, intercept, scale=1):
    slope = Fraction(rise * scale, run)
    term = {Fraction(1): "x", Fraction(-1): "-x", Fraction(0): ""}.get(slope)
    if term is None:
        term = f"{slope}x" if slope.denominator == 1 else f"({slope})x"
    constant = intercept * scale
    expression = str(constant) if not term else shifted(term, constant) if constant else term
    return f"{'y' if scale == 1 else str(scale) + 'y'} = {expression}"


def line_graph_outputs(document, bracket):
    """Prepare four graph reveals; runtime never parses an equation or answer label."""
    if (not isinstance(document, dict) or set(document) != {"schema_version", "recipes"} or
            type(document["schema_version"]) is not int or document["schema_version"] != 1 or
            not isinstance(document["recipes"], list) or not 1 <= len(document["recipes"]) <= 16):
        raise ValueError("line graphs: expected schema version 1 and 1 through 16 recipes")
    outputs, prepared = {}, []
    seen_ids, seen_sorter = set(), set()
    for recipe in document["recipes"]:
        if (not isinstance(recipe, dict) or set(recipe) != {
                "id", "sorter_id", "content_version", "rise", "run", "intercept"} or
                not isinstance(recipe["id"], str) or not re.fullmatch(r"sorter_graph_[a-z_]{1,40}", recipe["id"])):
            raise ValueError("line graphs: invalid recipe fields or identity")
        for key, low, high in (("rise", -8, 8), ("run", 1, 8), ("intercept", -8, 8),
                               ("sorter_id", 4000, 4999), ("content_version", 1, 4294967295)):
            if type(recipe[key]) is not int or not low <= recipe[key] <= high:
                raise ValueError(f"line graphs: invalid {key}")
        identity, sorter_id = recipe["id"], recipe["sorter_id"]
        if identity in seen_ids or sorter_id in seen_sorter:
            raise ValueError("line graphs: duplicate identity")
        seen_ids.add(identity); seen_sorter.add(sorter_id)
        rise, run, b = (recipe[key] for key in ("rise", "run", "intercept"))
        slope = Fraction(rise, run)
        if (slope.numerator, slope.denominator) != (rise, run):
            raise ValueError("line graphs: rise/run must be in lowest terms")
        equation = line_equation(rise, run, b)
        point = lambda x, y: f"({x}, {y})"
        origin, tip = point(0, b), point(run, b + rise)
        states = ["m = ?    b = ?", f"b = {b}    {origin}", f"{origin}    run = {run}",
                  f"run = {run}    rise = {rise}", f"{origin} -> {tip}    m = {rise}/{run}"]
        answers = [str(b), str(run), str(rise), tip]
        candidates = [[str(v) for v in (b, -b, b+1, b-1, b+2)],
                      [str(v) for v in (run, -run, run+1, run+2)],
                      [str(v) for v in (rise, -rise, rise+1, rise-1, rise+2)],
                      [tip, point(run, b-rise), point(run, b+rise+1), point(-run, b+rise), point(run, b+rise-1)]]
        prompts = ["x = 0    y = ?", f"m = {rise}/{run}    run = ?", f"run = {run}    rise = ?",
                   f"x = {run}    (x, y) = ?"]
        hints = ["Set x to zero. The remaining constant is the y-intercept.",
                 "The denominator of the displayed slope is the horizontal run to the right.",
                 "The numerator gives the vertical change. A negative rise goes down; zero stays level.",
                 "Start at the intercept, move right by the run, then add the rise to y."]
        explanations = [f"At x = 0, y = {b}. The intercept is {origin}.",
                        f"m = rise/run = {rise}/{run}. Move right {run} from {origin}.",
                        f"The vertical change is {rise}. This reaches {tip}.",
                        f"At x = {run}, y = ({slope})({run}) + ({b}) = {b+rise}. "
                        f"Both {origin} and {tip} lie on {equation}; their slope is {rise}/{run}."]
        steps = []
        for i, answer in enumerate(answers):
            options = list(dict.fromkeys(candidates[i]))[:4]
            if len(options) != 4 or options[0] != answer:
                raise ValueError("line graphs: need four distinct choices")
            steps.append({"id": i+1, "layer_name": ("INTERCEPT", "RUN", "RISE", "SECOND POINT")[i],
                          "prompt": prompts[i], "options": [{"id": 101+7*j, "label": value} for j, value in enumerate(options)],
                          "accepted_option_ids": [101], "wrong_hint": "", "hint": hints[i],
                          "next_move": explanations[i], "explanation": explanations[i],
                          "semantics": {"purpose": "graph_choice", "completion": "any_accepted",
                                        "before": (i+1)*10, "after": (i+2)*10}})
        pack = identity.removeprefix("sorter_") + "_pack"
        outputs[f"content/cards/{identity}.json"] = {
            "schema_version": 1, "id": identity, "content_version": recipe["content_version"], "equation": equation,
            "skill": "line_graph", "description": "Build the line from its intercept and slope. Each accepted step adds to the same graph.",
            "line_graph": {"rise": rise, "run": run, "intercept": b, "x_min": -4, "x_max": max(5, run+2),
                           "y_min": min(-3, b-2, b+rise-2), "y_max": max(4, b+2, b+rise+2)},
            "working_states": [{"id": (i+1)*10, "display": display, "graph_stage": stage} for i, (display, stage) in
                               enumerate(zip(states, ("grid", "intercept", "run", "rise", "line")))], "steps": steps}
        outputs[f"content/sorter/{pack}.json"] = {
            "schema_version": 1, "questions": [f"../cards/{identity}.json"],
            "decks": {"solve": [{"question_id": identity, "content_version": recipe["content_version"]}]}}
        prepared.append({"id": sorter_id, "text": equation, "subject": "algebra", "hint": hints[0],
                         "solve_pack": f"{pack}.json", "study": {"chapter": "Straight lines", "type": "Slope and intercept", "form": "y = mx + b"}})
    outputs["content/sorter/study_practice_v1.json"] = assemble_catalogue(
        bracket["content/sorter/bracket_practice_v1.json"], prepared)
    return outputs


def system_graph_outputs(document, existing):
    """Prepare exact two-line systems, including parallel and coincident cases."""
    if (not isinstance(document, dict) or set(document) != {"schema_version", "recipes"} or
            type(document["schema_version"]) is not int or document["schema_version"] != 1 or
            not isinstance(document["recipes"], list) or not 1 <= len(document["recipes"]) <= 16):
        raise ValueError("systems: expected version 1 and 1 through 16 recipes")
    outputs, prepared, identities, sorter_ids = {}, [], set(), set()
    for recipe in document["recipes"]:
        if (not isinstance(recipe, dict) or set(recipe) != {"id", "sorter_id", "content_version", "first", "second"} or
                not isinstance(recipe["id"], str) or not re.fullmatch(r"sorter_system_[a-z_]{1,40}", recipe["id"])):
            raise ValueError("systems: invalid recipe fields or identity")
        identity, sorter_id, version = (recipe[key] for key in ("id", "sorter_id", "content_version"))
        if type(sorter_id) is not int or not 5000 <= sorter_id <= 5999 or type(version) is not int or not 1 <= version <= 4294967295:
            raise ValueError("systems: invalid sorter_id or content_version")
        if identity in identities or sorter_id in sorter_ids:
            raise ValueError("systems: duplicate identity")
        identities.add(identity); sorter_ids.add(sorter_id)
        for key in ("first", "second"):
            line = recipe[key]
            if not isinstance(line, dict) or set(line) != {"rise", "run", "intercept", "scale"}:
                raise ValueError("systems: expected rise, run, intercept and scale")
            for field, low, high in (("rise", -8, 8), ("run", 1, 8), ("intercept", -8, 8), ("scale", 1, 4)):
                if type(line[field]) is not int or not low <= line[field] <= high:
                    raise ValueError(f"systems: invalid {key}/{field}")
            slope = Fraction(line["rise"], line["run"])
            if (slope.numerator, slope.denominator) != (line["rise"], line["run"]):
                raise ValueError("systems: slopes must be in lowest terms")
        a, b = recipe["first"], recipe["second"]
        m1, m2 = Fraction(a["rise"], a["run"]), Fraction(b["rise"], b["run"])
        eq1, eq2 = (line_equation(line["rise"], line["run"], line["intercept"], line["scale"]) for line in (a, b))
        count = "1" if m1 != m2 else "0" if a["intercept"] != b["intercept"] else "∞"
        x = Fraction(b["intercept"] - a["intercept"], m1 - m2) if m1 != m2 else Fraction(0)
        y = m1*x+a["intercept"]
        pair = lambda x, y: f"({x},{y})"
        last_answer = pair(x, y) if count == "1" else str(a["intercept"] - b["intercept"])
        final = (f"(x,y) = {pair(x,y)}" if count == "1" else
                 "N = 0    m1 = m2    b1 != b2" if count == "0" else "N = inf    m1 = m2    b1 = b2")
        check = (f"At x = {x}: line 1 gives ({m1})({x}) + ({a['intercept']}) = {y}; "
                 f"line 2 gives ({m2})({x}) + ({b['intercept']}) = {y}. "
                 f"The point {pair(x,y)} satisfies both original equations." if count == "1" else
                 f"Both slopes equal {m1}, but their intercepts differ by {last_answer}. The parallel lines never meet, so there is no solution." if count == "0" else
                 f"Both equations reduce to {eq1}. Their slopes and intercepts agree, so every point on this line satisfies both: infinitely many solutions.")
        numeric_options = lambda value: list(dict.fromkeys(str(v) for v in (value, -value, value+1, value-1, value+2)))[:4]
        final_options = ([pair(x,y), pair(x,y+1), pair(x+1,y), pair(0,0)] if count == "1" else numeric_options(Fraction(last_answer)))
        if len(set(final_options)) != 4:
            final_options = list(dict.fromkeys(final_options + [pair(x+2,y+2), pair(x-1,y-1)]))[:4]
        answers = [str(m1), str(m2), count, last_answer]
        choices = [numeric_options(m1), numeric_options(m2), [count] + [v for v in ("0", "1", "2", "∞") if v != count], final_options]
        prompts = ["m1 = ?", "m2 = ?", "N = ?", "(x,y) = ?" if count == "1" else "b1 - b2 = ?"]
        hints = ["For the first equation, divide the x coefficient by the y coefficient to find its slope.",
                 "Find the slope of the second equation in the same way. Both lines use the same axes.",
                 "N counts shared solutions. Cross once: 1. Parallel: 0. Same line: infinity. Move the guide to compare y-values.",
                 "Find the point where the guide shows the same y-value on both lines." if count == "1" else
                 "Subtract the second y-intercept from the first. Equal slopes need equal intercepts to describe the same line."]
        explanations = [f"Line 1 has slope {m1}. Its graph is shown in teal.", f"Line 2 has slope {m2}. Its graph is shown in violet.",
                        "The lines cross once, so N = 1." if count == "1" else
                        "Their slopes match but their intercepts differ, so N = 0." if count == "0" else
                        "The equations describe the same line, so N is infinite.", check]
        displays = ["m1 = ?    m2 = ?", f"m1 = {m1}    m2 = ?", f"m1 = {m1}    m2 = {m2}",
                    f"N = {'inf' if count == '∞' else count}", final]
        steps = []
        for i in range(4):
            if len(choices[i]) != 4 or len(set(choices[i])) != 4 or choices[i][0] != answers[i]:
                raise ValueError("systems: need four distinct choices with an exact accepted value")
            steps.append({"id": i+1, "layer_name": ("FIRST LINE", "SECOND LINE", "SOLUTION COUNT", "CHECK BOTH")[i],
                          "prompt": prompts[i], "options": [{"id": 101+7*j, "label": value} for j,value in enumerate(choices[i])],
                          "accepted_option_ids": [101], "wrong_hint": "", "hint": hints[i], "next_move": explanations[i],
                          "explanation": explanations[i], "semantics": {"purpose": "graph_choice", "completion": "any_accepted", "before": (i+1)*10, "after": (i+2)*10}})
        graph = {k: a[k] for k in ("rise", "run", "intercept")}
        graph.update(x_min=min(-3, x.__floor__()-2), x_max=max(a["run"]+2, x.__ceil__()+2),
                     y_min=min(-2, a["intercept"]-2, a["intercept"]+a["rise"]-2, b["intercept"]-2, y.__floor__()-2),
                     y_max=max(3, a["intercept"]+2, a["intercept"]+a["rise"]+2, b["intercept"]+2, y.__ceil__()+2),
                     second={k:b[k] for k in ("rise", "run", "intercept")})
        if min(graph["x_min"],graph["y_min"]) < -20 or max(graph["x_max"],graph["y_max"]) > 20:
            raise ValueError("systems: intersection exceeds bounded graph range")
        pack = identity.removeprefix("sorter_") + "_pack"
        equation = f"1: {eq1}; 2: {eq2}"
        outputs[f"content/cards/{identity}.json"] = {
            "schema_version": 1, "id": identity, "content_version": version, "equation": equation, "skill": "linear_system_graph",
            "description": "Read both slopes, compare the lines, then check their shared solutions. N is the number of solutions.",
            "line_graph": graph, "working_states": [{"id": (i+1)*10, "display": display, "graph_stage": stage} for i,(display,stage) in
            enumerate(zip(displays,("grid","first_line","both_lines","classified","system_solution")))], "steps": steps}
        outputs[f"content/sorter/{pack}.json"] = {"schema_version": 1, "questions": [f"../cards/{identity}.json"],
            "decks": {"solve": [{"question_id": identity, "content_version": version}]}}
        prepared.append({"id": sorter_id, "text": equation, "subject": "algebra", "hint": hints[0], "solve_pack": f"{pack}.json",
            "study": {"chapter": "Simultaneous equations", "type": "Two straight lines", "form": "y = m1 x + b1; y = m2 x + b2"}})
    outputs["content/sorter/study_practice_v1.json"] = assemble_catalogue(
        existing["content/sorter/study_practice_v1.json"], prepared)
    return outputs


def matrix_study_outputs(card, existing):
    """Link the authored row-reduction question into the existing study catalogue."""
    identity, version = card["id"], card["content_version"]
    if identity != "sorter_matrix_rows" or card["working_model"] != "matrix_rows":
        raise ValueError("matrix question: expected sorter_matrix_rows with matrix_rows working")
    record = {"id": 6001, "home_index": 0, "text": card["equation"], "subject": "linear_algebra",
              "hint": "Each row gives x, y and the right-hand value. Use row operations to make the left block the identity matrix.",
              "solve_pack": "matrix_rows_pack.json", "study": {"chapter": "Matrices and systems",
              "type": "Row reduction", "form": "Ax = b; [A | b] -> [I | x]"}}
    return {"content/sorter/study_practice_v1.json": assemble_catalogue(existing["content/sorter/study_practice_v1.json"], [record]),
            "content/sorter/matrix_rows_pack.json": {"schema_version": 1, "questions": [f"../cards/{identity}.json"],
            "reference_library": "../references/row_operations.json",
            "decks": {"solve": [{"question_id": identity, "content_version": version}]}}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="verify committed files without writing")
    parser.add_argument("--recipes", type=Path, help="prepared bracket recipe file")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    content, solutions = fixture()
    outputs = [("content/sorter/equations_v1.json", content),
               ("tests/fixtures/equation_sorter_v1.solutions.json", solutions)]
    recipe_source = args.recipes or root / "content/authoring/linear_bracket_recipes.json"
    sources = [recipe_source]
    active_source = recipe_source
    try:
        if recipe_source.stat().st_size > 65536:
            raise ValueError("file exceeds 64 KiB")
        generated = bracket_outputs(json.loads(recipe_source.read_text(), object_pairs_hook=unique_fields))
        outputs.extend(generated.items())
        study_path = "content/sorter/study_practice_v1.json"
        for relative, prepare in (("content/authoring/line_graph_recipes.json", line_graph_outputs),
                                  ("content/authoring/system_graph_recipes.json", system_graph_outputs),
                                  ("content/cards/sorter_matrix_rows.json", matrix_study_outputs)):
            active_source = root / relative; sources.append(active_source)
            generated = prepare(json.loads(active_source.read_text(), object_pairs_hook=unique_fields), generated)
            outputs.extend((path, data) for path, data in generated.items() if path != study_path)
        active_source = root / "content/authoring/matrix_practice_recipes.json"
        sources.append(active_source)
        chapter_document = json.loads(active_source.read_text(), object_pairs_hook=unique_fields)
        chapter = matrix_practice_outputs(chapter_document, generated["content/sorter/study_practice_v1.json"])
        if len(chapter_document["recipes"]) != 12:
            raise ValueError("matrix practice: publication requires all twelve recipes")
        outputs.extend(chapter.items())
        outputs.append((study_path, chapter["content/sorter/matrix_practice_v1.json"]))
        publish_outputs(root, outputs, args.check, sources=sources)
    except (OSError, ValueError) as error:
        parser.error(f"{active_source}:{error}")
    count = sum("working_states" in value for _, value in outputs)
    print(f"100-card study pack and {count} generated bracket/graph/matrix sequences plus one authored matrix question: " + ("unchanged" if args.check else "written"))


if __name__ == "__main__":
    main()
