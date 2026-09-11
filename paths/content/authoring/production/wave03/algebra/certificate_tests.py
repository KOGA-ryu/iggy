"""Finite quadratic arithmetic on actual compiled model fields, not Markdown.

Only degree <=2 polynomials, rational constants and bounded integer radicals
are understood. No writer/generator, answer table, eval, floats or CAS is used.
"""
import argparse
import ast
import copy
from fractions import Fraction as F
import json
from math import isqrt, lcm
from pathlib import Path
import re

from question_workflow import exact

SOURCE = Path(__file__).resolve().parent
PREFIX = "prod03_algebra_quadratic_roots"
DOMAIN = ("Work over the real numbers. S is the complete solution set. L(t) and R(t) "
          "evaluate the original left and right sides at t. At the final check, V lists "
          "L(r),R(r) for each distinct root r in increasing order.")


def require(test, message):
    if not test:
        raise ValueError(message)


# A radical scalar is an immutable sum of rational multiples of sqrt(squarefree).
# Component 1 is rational; an empty tuple is exactly zero.
def scalar(n):
    n = exact(str(n))
    return ((1, n),) if n else ()


def clean(d):
    return tuple(sorted((k, v) for k, v in d.items() if v))


def add(a, b):
    d = dict(a)
    for k, v in b:
        d[k] = d.get(k, F(0)) + v
    return clean(d)


def neg(a):
    return tuple((k, -v) for k, v in a)


def sqrt_parts(n):
    require(type(n) is int and 0 <= n <= 10000, "bounded radical product")
    if not n:
        return 0, 1
    for q in range(isqrt(n), 0, -1):
        if n % (q*q) == 0:
            return q, n // (q*q)
    raise AssertionError("unreachable")


def radical(n):
    require(type(n) is int and 0 <= n <= 100, "real bounded radical input")
    q, d = sqrt_parts(n)
    return ((d, F(q)),) if q else ()


def mul(a, b):
    result = ()
    for d, v in a:
        for e, w in b:
            q, core = sqrt_parts(d*e)
            result = add(result, ((core, v*w*q),))
    return result


def rational(a):
    require(all(k == 1 for k, _ in a), "rational quantity required")
    return dict(a).get(1, F(0))


def divide(a, b):
    divisor = rational(b)
    require(divisor != 0, "nonzero constant divisor required")
    return tuple((k, v/divisor) for k, v in a)


ZERO = ((), (), ())
X = ((), scalar(1), ())


def constant(a):
    return (a, (), ())


def padd(a, b):
    return tuple(add(v, w) for v, w in zip(a, b))


def pneg(a):
    return tuple(neg(v) for v in a)


def psub(a, b):
    return padd(a, pneg(b))


def pmul(a, b):
    result = [()] * 5
    for i, v in enumerate(a):
        for j, w in enumerate(b):
            result[i+j] = add(result[i+j], mul(v, w))
    require(not any(result[3:]), "degree exceeds finite quadratic domain")
    return tuple(result[:3])


def degree(p):
    return max((i for i, v in enumerate(p) if v), default=-1)


def ast_math(s):
    require(len(s) <= 512, "bounded expression length")
    s = s.replace(r"\left", "").replace(r"\right", "").replace(r"\,", "")
    s = s.replace(r"\ ", "").replace(" ", "").replace("^{2}", "^2")
    s = re.sub(r"\\sqrt\{(\d+)\}", r"sqrt(\1)", s)
    s = re.sub(r"\\frac\{([^{}]+)\}\{([^{}]+)\}", r"((\1)/(\2))", s)
    require("\\" not in s and "{" not in s and "}" not in s, "unsupported math syntax")
    s = re.sub(r"(?<=[0-9])(?=x|\(|sqrt)", "*", s)
    s = re.sub(r"(?<=x)(?=\()", "*", s)
    s = re.sub(r"(?<=\))(?=x|\(|sqrt)", "*", s).replace("^", "**")
    tree = ast.parse(s, mode="eval").body
    require(sum(1 for _ in ast.walk(tree)) <= 80, "bounded arithmetic tree")
    return tree


def value(n, x=X):
    if isinstance(n, ast.Constant):
        require(type(n.value) is int and abs(n.value) <= 10000, "integer literal required")
        return constant(scalar(n.value))
    if isinstance(n, ast.Name):
        require(n.id == "x", "only the variable x is allowed")
        return x
    if isinstance(n, ast.UnaryOp) and isinstance(n.op, (ast.UAdd, ast.USub)):
        v = value(n.operand, x)
        return pneg(v) if isinstance(n.op, ast.USub) else v
    if isinstance(n, ast.Call):
        require(isinstance(n.func, ast.Name) and n.func.id == "sqrt"
                and len(n.args) == 1 and not n.keywords, "bounded square root only")
        arg = n.args[0]
        require(isinstance(arg, ast.Constant), "integer radicand literal required")
        return constant(radical(arg.value))
    require(isinstance(n, ast.BinOp), "unsupported arithmetic node")
    a = value(n.left, x)
    if isinstance(n.op, ast.Pow):
        require(isinstance(n.right, ast.Constant) and n.right.value == 2,
                "only squaring is supported")
        return pmul(a, a)
    b = value(n.right, x)
    if isinstance(n.op, ast.Add):
        return padd(a, b)
    if isinstance(n.op, ast.Sub):
        return psub(a, b)
    if isinstance(n.op, ast.Mult):
        return pmul(a, b)
    require(isinstance(n.op, ast.Div) and degree(b) <= 0, "fixed constant denominator only")
    return tuple(divide(v, b[0]) for v in a)


def poly(s):
    return value(ast_math(s))


def scalar_text(s):
    p = poly(s)
    require(degree(p) <= 0, "constant expression required")
    return p[0]


def equation(s):
    require(s.count("=") == 1, "one equality required")
    left, right = s.split("=")
    l, r = ast_math(left), ast_math(right)
    return l, r, value(l), value(r)


def roots_of(p):
    c, b, a = (rational(v) for v in p)
    require(a > 0, "positive normalized quadratic coefficient")
    d = b*b - 4*a*c
    require(d.denominator == 1 and abs(d) <= 100, "bounded integer discriminant")
    if d < 0:
        return [], d
    root = radical(int(d))
    minus = divide(add(scalar(-b), neg(root)), scalar(2*a))
    plus = divide(add(scalar(-b), root), scalar(2*a))
    return [minus] if minus == plus else [minus, plus], d


def original(case):
    l, r, left, right = equation(case["given"])
    for p in (left, right):
        for v in p:
            f = rational(v)
            require(abs(f.numerator) <= 12 and f.denominator <= 6, "original coefficient bound")
    require([rational(v) for v in left] == [exact(str(v)) for v in case["left"]]
            and [rational(v) for v in right] == [exact(str(v)) for v in case["right"]],
            "original expressions differ from independent expansion")
    diff = psub(left, right)
    require(degree(diff) == 2, "original must have nonzero quadratic coefficient")
    coefficients = [rational(v) for v in diff]
    scale = lcm(*(v.denominator for v in coefficients))
    scale *= 1 if coefficients[2] > 0 else -1
    standard = tuple(mul(v, scalar(scale)) for v in diff)
    require(all(abs(rational(v)) <= 12 for v in standard), "standard coefficient bound")
    roots, d = roots_of(standard)
    require(all(k in (1, 2, 3, 5) for root in roots for k, _ in root), "root core bound")
    checks = []
    for root in roots:
        lv, rv = value(l, constant(root))[0], value(r, constant(root))[0]
        require(lv == rv, "independently derived root fails original substitution")
        checks.extend((lv, rv))
    # Exact completion identity, valid for all x (not point sampling).
    c, b, a = [rational(v) for v in standard]
    base = (scalar(b/(2*a)), scalar(1), ())
    completed = psub(pmul(constant(scalar(a)), pmul(base, base)),
                     constant(scalar(d/(4*a))))
    require(completed == standard, "completion identity failure")
    return {"left": left, "right": right, "difference": diff, "standard": standard,
            "roots": roots, "D": d, "checks": checks, "a": a, "b": b, "c": c}


def terms(n):
    if isinstance(n, ast.BinOp) and isinstance(n.op, (ast.Add, ast.Sub)):
        return terms(n.left) + terms(n.right)
    return [n]


def standard_shape(n):
    entries = [value(t) for t in terms(n)]
    degrees = [degree(p) for p in entries]
    return (degrees == sorted(set(degrees), reverse=True) and degrees[0] == 2
            and all(sum(bool(v) for v in p) == 1 for p in entries)
            and not any(isinstance(t, (ast.Div, ast.Call)) for t in ast.walk(n)))


def linear_square(n):
    return (isinstance(n, ast.BinOp) and isinstance(n.op, ast.Pow)
            and isinstance(n.right, ast.Constant) and n.right.value == 2
            and degree(value(n.left)) == 1 and value(n.left)[1] == scalar(1))


def factored_shape(n):
    if linear_square(n):
        return True
    return (isinstance(n, ast.BinOp) and isinstance(n.op, ast.Mult)
            and degree(value(n.left)) == degree(value(n.right)) == 1)


def root_set(s):
    require(s.startswith("S="), "solution-set label required")
    body = s[2:].strip()
    if body == r"\varnothing":
        return frozenset()
    require(body.startswith(r"\{") and body.endswith(r"\}"), "root braces required")
    members = [scalar_text(v) for v in body[2:-2].split(",")]
    require(len(members) == len(set(members)), "repeat root appears twice")
    require(1 <= len(members) <= 2, "quadratic root-set size")
    return frozenset(members)


def vector(s, prefix="V="):
    require(s.startswith(prefix), "ordered list label")
    body = s[len(prefix):].replace(r"\left", "").replace(r"\right", "")
    require(body.startswith("(") and body.endswith(")"), "ordered list parentheses")
    return tuple(scalar_text(v) for v in body[1:-1].split(","))


def branches(s):
    equations = s.split(r"\text{or}")
    members, sides = [], []
    for text in equations:
        _, _, l, r = equation(text)
        p = psub(l, r)
        require(degree(p) == 1, "linear branches required")
        members.append(divide(neg(p[0]), p[1]))
        sides.append((l, r))
    require(len(members) == len(set(members)), "duplicate branch")
    return frozenset(members), sides


def formula(s, data):
    require(s.startswith("x=") and s.count(r"\pm") == 1, "two-sign formula required")
    expression = s[2:]
    values = frozenset(scalar_text(expression.replace(r"\pm", sign)) for sign in ("-", "+"))
    node = ast_math(expression.replace(r"\pm", "+"))
    shape = isinstance(node, ast.BinOp) and isinstance(node.op, ast.Div)
    if shape:
        num = node.left
        shape = (isinstance(num, ast.BinOp) and isinstance(num.op, ast.Add)
                 and value(num.left) == constant(scalar(-data["b"]))
                 and isinstance(num.right, ast.Call)
                 and isinstance(num.right.func, ast.Name) and num.right.func.id == "sqrt"
                 and len(num.right.args) == 1 and isinstance(num.right.args[0], ast.Constant)
                 and num.right.args[0].value == data["D"]
                 and value(node.right) == constant(scalar(2*data["a"])))
    return values, bool(shape)


def classify(label, goal, data):
    """Return truth, requested form, and a semantic duplicate key."""
    expected = frozenset(data["roots"])
    if goal == "roots":
        result = root_set(label)
        return result == expected, True, result
    if goal == "check":
        result = vector(label)
        return result == tuple(data["checks"]), True, result
    if goal == "coefficients":
        result = vector(label, "(a,b,c)=")
        target = tuple(scalar(data[k]) for k in ("a", "b", "c"))
        return result == target, True, result
    if goal == "discriminant":
        require(label.startswith("D="), "discriminant label")
        result = scalar_text(label[2:])
        return result == scalar(data["D"]), True, result
    if goal == "formula":
        result, shape = formula(label, data)
        return result == expected, shape, result
    if goal in ("branches", "square_branches"):
        result, sides = branches(label)
        shape = len(sides) == len(expected)
        if goal == "branches":
            shape = shape and all(r == ZERO for _, r in sides)
        else:
            base = (scalar(data["b"]/(2*data["a"])), scalar(1), ())
            shape = shape and all(l == base and degree(r) <= 0 for l, r in sides)
        return result == expected, shape, result
    if goal == "positive_witness":
        node = ast_math(label)
        p = value(node)
        shape = (isinstance(node, ast.BinOp) and isinstance(node.op, ast.Add)
                 and linear_square(node.left) and degree(value(node.right)) <= 0
                 and rational(value(node.right)[0]) > 0)
        return p == data["difference"], bool(shape), (p, bool(shape))
    l, r, left, right = equation(label)
    difference = psub(left, right)
    if goal == "standard":
        truth = difference == data["standard"]
        shape = standard_shape(l) and right == ZERO and isinstance(r, ast.Constant) and r.value == 0
    elif goal in ("factor", "factor_method"):
        truth = difference == data["standard"]
        shape = factored_shape(l) and right == ZERO and isinstance(r, ast.Constant) and r.value == 0
    else:
        require(goal == "square_method", "unknown local goal")
        truth = difference == tuple(divide(v, scalar(data["a"])) for v in data["standard"])
        shape = linear_square(l) and degree(right) <= 0
    return truth, bool(shape), (difference, bool(shape))


def reached(label, goal, data):
    if goal == "check":
        roots, values = label.split(r",\quad ")
        require(root_set(roots) == frozenset(data["roots"]), "final roots lost or changed")
        result = classify(values, goal, data)
    elif goal == "positive_witness":
        require(label.startswith("L(x)-R(x)="), "original witness label")
        expression, roots = label[len("L(x)-R(x)="):].split(r">0,\quad ")
        require(root_set(roots) == frozenset(), "negative-D conclusion must be empty")
        result = classify(expression, goal, data)
    else:
        result = classify(label, goal, data)
    require(result[0] and result[1], "reached work fails truth or requested form")


def audit(routes, pool):
    require(routes["accepted"] is True and routes["windows"] == 0, "headless accepted model")
    require(pool["domain"] == DOMAIN and pool["bounds"]["domain"] == "real", "real domain changed")
    require(pool["bounds"] == {"coefficient_numerator": 12, "coefficient_denominator": 6,
                              "standard_coefficient": 12, "discriminant_magnitude": 100,
                              "radicand_max": 100, "root_squarefree_cores": [2,3,5],
                              "domain": "real"}, "finite bounds changed")
    cases = pool["cases"]
    require(len(cases) == 12 and [c["group"] for c in cases] ==
            ["introductory"]*4+["practice"]*4+["mixed"]*4, "finite group contract")
    ids = [f"{PREFIX}_q{i:02}" for i in range(1, 13)]
    require([c["id"] for c in cases] == ids == routes["question_ids"], "reserved identity order")
    require([q["id"] for q in routes["questions"]] == ids, "compiled identity order")
    records, positions = [], []
    for case, row in zip(cases, routes["questions"]):
        q = row["question"]
        require(q["equation"] == case["given"] and q["description"].endswith(" "+DOMAIN),
                "compiled original/domain does not match finite inputs")
        data = original(case)
        require(len(q["steps"]) == len(case["goals"]), "local goal count")
        states = q["working_states"]
        require(len(states) == len(q["steps"])+1 and states[0]["display"] == q["equation"],
                "complete original/reached states")
        decisions = []
        for i, (step, goal) in enumerate(zip(q["steps"], case["goals"])):
            require(step["semantics"]["before"] == states[i]["id"] and
                    step["semantics"]["after"] == states[i+1]["id"], "working continuity")
            require(len(step["options"]) == 3, "three choices required")
            valid, signatures, checks = [], [], []
            for option in step["options"]:
                truth, shape, signature = classify(option["label"], goal, data)
                signatures.append(signature)
                checks.append({"id": option["id"], "truth": truth, "requested_form": shape})
                if truth and shape:
                    valid.append(option["id"])
            require(len(set(signatures)) == 3, "semantic duplicate choices")
            require(len(valid) == 1 and step["accepted_option_ids"] == valid, "false or nonunique key")
            for option in step["options"]:
                require("wrong_feedback" not in option if option["id"] in valid else
                        bool(option.get("wrong_feedback")), "specific correction attachment")
            require(step["prompt"] and step["explanation"] and step["wrong_hint"], "teaching field absent")
            require("\\" not in step["explanation"], "raw TeX in history explanation")
            reached(states[i+1]["display"], goal, data)
            if i == 0:
                positions.append(next(j+1 for j, o in enumerate(step["options"]) if o["id"] in valid))
            decisions.append({"goal": goal, "options": checks, "valid_id": valid[0],
                              "reached_verified": True})
        records.append({"id": case["id"], "original": case["given"], "standard_coefficients":
                        [rational(v) for v in data["standard"]], "D": data["D"],
                        "roots_squarefree_components": data["roots"],
                        "original_values": data["checks"], "completion_identity": True,
                        "decisions": decisions})
    require(sorted(positions) == [1]*4+[2]*4+[3]*4, "first key position balance")
    require(sum(len(r["decisions"]) for r in records) == 50 and routes["wrong_choices"] == 100,
            "complete route count")
    return {"questions": 12, "steps": 50, "options": 150, "wrong_choices": 100,
            "first_key_positions": positions, "cases": records}


def controls(routes, pool):
    outcomes = []

    def reject(name, action):
        try:
            action()
        except (ValueError, AssertionError, SyntaxError, ZeroDivisionError) as error:
            outcomes.append({"control": name, "rejected": True, "reason": str(error)})
        else:
            raise AssertionError("negative control passed: "+name)

    def mutate(qi, si, text, mode):
        modified = copy.deepcopy(routes)
        q = modified["questions"][qi]["question"]
        step = q["steps"][si]
        if mode in ("choice", "both"):
            next(o for o in step["options"] if o["id"] in step["accepted_option_ids"])["label"] = text
        if mode in ("state", "both"):
            q["working_states"][si+1]["display"] = text
        return audit(modified, pool)

    for mode in ("choice", "state", "both"):
        reject("false standard "+mode, lambda m=mode: mutate(0, 0, "x^2-5x+7=0", m))
        reject("true but unexpanded standard "+mode,
               lambda m=mode: mutate(0, 0, "(x-2)(x-3)=0", m))
        reject("true but unfactored "+mode,
               lambda m=mode: mutate(0, 1, "x^2-5x+6=0", m))
        reject("true but unsquared method "+mode,
               lambda m=mode: mutate(9, 0, "x(x-2)=2", m))
        reject("lost zero root "+mode, lambda m=mode: mutate(3, 2, r"S=\{2\}", m))
        reject("lost radical minus branch "+mode,
               lambda m=mode: mutate(6, 3, r"S=\{2+\sqrt{5}\}", m))
        reject("evaluated rather than substituted formula "+mode,
               lambda m=mode: mutate(6, 2, r"x=2\pm\sqrt{5}", m))
        reject("isolated x instead of requested x-1 branches "+mode,
               lambda m=mode: mutate(9, 1, r"x=1+\sqrt{3}\ \text{or}\ x=1-\sqrt{3}", m))
    bad = copy.deepcopy(routes)
    bad["questions"][0]["question"]["steps"][0]["accepted_option_ids"] = [12]
    reject("false authored key", lambda: audit(bad, pool))
    for qi, si, duplicate in ((0, 2, r"S=\{3,2\}"),
                               (6, 3, r"S=\{2+\frac{\sqrt{20}}{2},2-\frac{\sqrt{20}}{2}\}")):
        bad = copy.deepcopy(routes)
        step = bad["questions"][qi]["question"]["steps"][si]
        next(o for o in step["options"] if o["id"] not in step["accepted_option_ids"])["label"] = duplicate
        reject("equivalent duplicate "+str(qi), lambda b=bad: audit(b, pool))
    for given, left, right in (("x=1", [0,1,0], [1,0,0]),
                               ("13x^2=0", [0,0,13], [0,0,0]),
                               ("x^2/7=0", [0,0,"1/7"], [0,0,0]),
                               ("x^2/x=1", [0,1,0], [1,0,0]),
                               ("x^2/0=1", [0,0,1], [1,0,0]),
                               ("x^3=1", [0,0,1], [1,0,0]),
                               ("12x^2+12x-12=0", [-12,12,12], [0,0,0]),
                               ("x^2=7", [0,0,1], [7,0,0])):
        case = {**pool["cases"][0], "given": given, "left": left, "right": right}
        reject("invalid input "+given, lambda c=case: original(c))
    bad = copy.deepcopy(routes)
    bad["questions"][0]["question"]["description"] = bad["questions"][0]["question"]["description"].replace("real numbers", "complex numbers")
    reject("compiled domain changed", lambda: audit(bad, pool))
    reject("negative radicand", lambda: scalar_text(r"\sqrt{-8}"))
    reject("radicand above bound", lambda: scalar_text(r"\sqrt{101}"))
    reject("duplicate repeated root", lambda: root_set(r"S=\{2,2\}"))
    for qi, goal, text in (
            (0, "factor", "(x-3)(x-2)=0"),
            (0, "standard", "(x^2)-5*x+6=0"),
            (0, "roots", r"S=\{3,2\}"),
            (6, "roots", r"S=\{2+\frac{\sqrt{20}}{2},2-\frac{\sqrt{20}}{2}\}"),
            (6, "formula", r"x=(4\pm\sqrt{20})/(2)"),
            (9, "square_method", "((x-1))^2=3"),
            (9, "square_branches", r"x-1=-\sqrt{3}\ \text{or}\ x-1=\sqrt{3}"),
            (10, "positive_witness", "(x+1)^2+(2)")):
        data = original(pool["cases"][qi])
        truth, shape, _ = classify(text, goal, data)
        require(truth and shape, "valid alternative rejected: "+text)
        outcomes.append({"control": "valid "+goal+" "+text, "accepted": True})
    return outcomes


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--routes", required=True, type=Path)
    parser.add_argument("--edited-routes", type=Path)
    args = parser.parse_args()
    pool = json.loads((SOURCE/"cases.json").read_text())
    routes = json.loads(args.routes.read_text())
    result = audit(routes, pool)
    result["controls"] = controls(routes, pool)
    if args.edited_routes:
        edited = json.loads(args.edited_routes.read_text())
        require(audit(edited, pool) == audit(routes, pool), "edit changed mathematics")
        target = edited["questions"][0]["question"]["steps"][0]
        old = routes["questions"][0]["question"]["steps"][0]
        require(target["prompt"] != old["prompt"], "real prompt edit absent")
        old_option = next(o for o in old["options"] if o["id"] == 12)
        option = next(o for o in target["options"] if o["id"] == 12)
        require(option["wrong_feedback"] != old_option["wrong_feedback"], "real feedback edit absent")
        target["prompt"] = old["prompt"]
        option["wrong_feedback"] = old_option["wrong_feedback"]
        require(edited == routes, "unintended compiled field changed")
        result["markdown_edit_proof"] = "exactly q01 step10 prompt and option12 wrong_feedback changed; math unchanged"
    result["scope"] = "all 12 finite cases; bounded degree-two rational/radical identities; no point samples"
    print(json.dumps(result, indent=2, default=str))


if __name__ == "__main__":
    main()
