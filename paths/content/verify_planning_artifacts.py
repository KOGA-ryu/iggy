#!/usr/bin/env python3
"""Validate authored planning data; never import or execute the game or source programs."""

import hashlib
import json
import re
from datetime import datetime, timezone
from fractions import Fraction as F
from pathlib import Path

ROOT = Path(__file__).resolve().parent
RECOVERY = "That choice does not fit this step. You can try again or see the answer and why."
EXPECTED = {
    "002": [2, 3, 4, 1, 3, 2, 4, 1, 3, 2, 4, 1, 3],
    "013": [4, 2, 1, 3, 4, 2, 1, 4, 3, 2, 1, 4, 2, 3],
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def question_section(text):
    match = re.search(r"^## Question\n(.*?)(?=^## |\Z)", text, re.M | re.S)
    require(match is not None, "Source has no Question section")
    return match.group(1).strip()


def selected_text(card, step_id):
    step = next(s for s in card["steps"] if s["id"] == step_id)
    return next(o["text"] for o in step["options"] if o["id"] == step["correct_option_id"])


def check_cards():
    manifest = json.loads((ROOT / "source_manifest.json").read_text())
    for entry in manifest["snapshots"]:
        data = (ROOT / entry["snapshot"]).read_bytes()
        require(hashlib.sha256(data).hexdigest() == entry["sha256"],
                "Snapshot hash differs: " + entry["snapshot"])
    cards = {}
    kinds = {"vocabulary", "role", "structure", "condition", "setup", "method",
             "mechanics", "result", "justification", "verification"}
    question_ids = set()
    for page_id, expected in EXPECTED.items():
        card = json.loads((ROOT / "authoring" / f"{page_id}_guided.json").read_text())
        require(card["schema_version"] == 1 and card["content_version"] == 1, "Version mismatch")
        require(card["question_id"] not in question_ids, "Duplicate question ID")
        question_ids.add(card["question_id"])
        source = (ROOT / card["source"]["snapshot"]).read_bytes()
        require(card["source"]["sha256"] == hashlib.sha256(source).hexdigest(), "Card source hash mismatch")
        require(card["source_question_verbatim"] == question_section(source.decode()), "Question was altered")
        require(len(card["steps"]) == len(expected), "Unexpected step count")
        require(0 < len(card["steps"]) <= 32, "Step limit")
        step_ids = set()
        for i, step in enumerate(card["steps"]):
            require(step["id"] not in step_ids, "Duplicate step ID")
            step_ids.add(step["id"])
            require(step["kind"] in kinds, "Unknown layer kind")
            require(len(step["options"]) == 4, "Choice count")
            option_ids = [o["id"] for o in step["options"]]
            require(len(set(option_ids)) == 4, "Duplicate option ID")
            require(step["correct_option_id"] == option_ids[expected[i] - 1], "Reviewed key mismatch")
            require(step["recovery_text"] == RECOVERY, "Recovery discloses more than the shared neutral copy")
            require(bool(step["prompt"]) and bool(step["explanation"]) and bool(step["workspace"]), "Missing content")
            for option in step["options"]:
                require(bool(option["text"]), "Empty choice")
                if option["id"] == step["correct_option_id"]:
                    require(option["misconception"] is None, "Correct option marked as misconception")
                else:
                    require(bool(option["misconception"]), "Distractor has no authored reason")
        script = (ROOT / "reference_cases" / f"{page_id}_all_correct.planned.script").read_text().splitlines()
        require(script[:2] == ["mode_guided", "guided_open_question " + card["question_id"]], "Planned script identity mismatch")
        expected_tail = [line for n in expected for line in [f"guided_select {n}", "guided_check", "guided_next"]]
        require(script[2:] == expected_tail, "Planned script differs from authored key")
        cards[page_id] = card
    return cards, len(manifest["snapshots"])


def solve_exact(matrix, rhs):
    """Independent small Gaussian elimination, all arithmetic rational."""
    n = len(rhs)
    rows = [[F(x) for x in row] + [F(value)] for row, value in zip(matrix, rhs)]
    for col in range(n):
        pivot = next(r for r in range(col, n) if rows[r][col] != 0)
        rows[col], rows[pivot] = rows[pivot], rows[col]
        divisor = rows[col][col]
        rows[col] = [x / divisor for x in rows[col]]
        for r in range(n):
            if r != col:
                factor = rows[r][col]
                rows[r] = [x - factor * y for x, y in zip(rows[r], rows[col])]
    return tuple(row[-1] for row in rows)


def check_002(card):
    inputs, heights = [-1, 0, 1], [1, 0, 2]
    matrix = [[x*x, x, 1] for x in inputs]
    a, b, c = solve_exact(matrix, heights)
    require((a, b, c) == (F(3, 2), F(1, 2), F(0)), "002 coefficients")
    determinant = sum(
        matrix[0][j] *
        (matrix[1][(j+1) % 3] * matrix[2][(j+2) % 3] -
         matrix[1][(j+2) % 3] * matrix[2][(j+1) % 3])
        for j in range(3)
    )
    require(determinant == -2, "002 determinant")
    values = [a*x*x + b*x + c for x in inputs]
    require(values == list(map(F, heights)), "002 original observations")
    require(selected_text(card, "solve_a") == f"a = {a}", "002 selected a does not match calculation")
    require(selected_text(card, "solve_b") == f"b = {b}", "002 selected b does not match calculation")
    require(selected_text(card, "state_function") == f"f(x) = ({a})*x^2 + ({b})*x", "002 selected function")
    return {"coefficients": list(map(str, (a,b,c))), "determinant": determinant,
            "observations": [str(x) for x in values], "arithmetic": "exact rational",
            "tolerance": "not applicable; no floating point", "result": "passed"}


def check_013(card):
    rows = []
    offsets = [F(-3), F(-1,10), F(0), F(1,10), F(3)]
    for m in [F(0), F(2), F(-1,2)]:
        planted = F(17,10)
        for a, b in [(F(1),F(3)), (F(-2),F(5)), (F(0),F(0)), (planted,m*planted)]:
            denominator = 1 + m*m
            require(denominator > 0, "013 nonzero direction")
            t = (a+m*b)/denominator
            q = (t,m*t)
            residual_dot = (a-q[0]) + m*(b-q[1])
            require(residual_dot == 0, "013 residual perpendicularity")
            # g(s)=A*s^2+B*s+C. Compute its unique stationary point independently.
            A, B, C = F(1)+m*m, -2*a-2*m*b, a*a+b*b
            t_calculus = -B/(2*A)
            require(t == t_calculus and 2*A > 0, "013 calculus agreement")
            baseline = (t-a)**2 + (m*t-b)**2
            for offset in offsets:
                s = t+offset
                distance = (s-a)**2 + (m*s-b)**2
                require(distance == A*s*s+B*s+C, "013 squared-distance expansion")
                require(distance-baseline == denominator*(s-t)**2, "013 distance-gap identity")
                require((distance == baseline) == (s == t), "013 unique minimum in compared cases")
            if b == m*a:
                require(q == (a,b), "013 point already on the line")
            rows.append({"m":str(m),"p":[str(a),str(b)],"t":str(t),
                         "q":list(map(str,q)),"distance_squared":str(baseline)})
    require(len(rows) == 12, "013 case count")
    require(selected_text(card, "assemble_point") == "q = (t,m*t)", "013 coordinate assembly")
    return {"cases": rows,"distance_gap_comparisons":60,"arithmetic":"exact rational",
            "tolerance":"not applicable; no floating point","result":"passed",
            "limit":"Finite examples, not a proof for all real parameters. See SOURCE_CARD_ARCHITECTURE.md section 8 for the general argument."}


def check_reference_cases():
    obj = json.loads((ROOT / "reference_cases" / "FM002_REFERENCE_CASES.json").read_text())
    require(obj["answer_option_indices"] == [1,2,1,2,0,1,3], "FM002 reviewed key mismatch")
    require(len(obj["cases"]) == 8, "Reference case count")
    for case in obj["cases"]:
        n = len(case["actions"])
        rejected = case["expected_rejected_action_indices"]
        require(len(set(rejected)) == len(rejected) and all(0 <= x < n for x in rejected), "Reference action index")
        s = case["expected"]["summary"]
        require(s["assisted"] == (s["shown_answers"] > 0), "Reference assistance inconsistency")
        resolved = s["correct_on_first_try"] + s["corrected_after_retry"] + s["shown_answers"]
        require(0 <= resolved <= 7, "Reference resolution partition")
        if case["expected"]["completed"]:
            require(resolved == 7, "Reference completion inconsistency")
    return {"cases":len(obj["cases"]),"status":"internally consistent specifications",
            "limit":"Not executed against the C++ model in this planning check."}


def main():
    cards, snapshot_count = check_cards()
    receipt = {
        "schema_version":1,
        "checked_at_utc":datetime.now(timezone.utc).isoformat(),
        "status":"passed",
        "scope":"Planning artifacts and the explicitly listed mathematics only",
        "snapshot_hashes_checked":snapshot_count,
        "authored_cards":2,
        "authored_layers":sum(len(c["steps"]) for c in cards.values()),
        "card_002":check_002(cards["002"]),
        "card_013":check_013(cards["013"]),
        "fm002_reference_cases":check_reference_cases(),
        "not_performed":["Engine build or runtime tests", "Execution of source-page programs", "Source-page writeback", "Verification of the remaining source-card corpus", "Automated verification of every distractor's prose"]
    }
    (ROOT / "planning_validation.json").write_text(json.dumps(receipt,indent=2)+"\n")
    print(f"PASS: {snapshot_count} source snapshots; 2 authored cards / {receipt['authored_layers']} layers; exact 002 answer; 12 exact 013 cases / 60 distance gaps; 8 internally consistent FM002 reference cases.")
    print("Engine implementation was not executed by this check.")


if __name__ == "__main__":
    main()
