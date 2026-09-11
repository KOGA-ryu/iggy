"""Exact finite rational-equation certificates on actual compiled fields.

Reuse the frozen mathematical-field syntax adapter read-only. New code owns
only this family's rational polynomial/LCD/domain checks; no Markdown parser.
"""
import argparse
import ast
import copy
from fractions import Fraction as F
import importlib.util
from itertools import zip_longest
import json
from math import gcd, isqrt, lcm
from pathlib import Path
import re
from question_workflow import exact

SOURCE = Path(__file__).resolve().parent
ROOT = SOURCE.parents[4]
HELPER = SOURCE.parents[1]/"wave03/algebra/certificate_tests.py"
spec = importlib.util.spec_from_file_location("frozen_quadratic_arithmetic", HELPER)
quadratic = importlib.util.module_from_spec(spec)
spec.loader.exec_module(quadratic)
math_tree = quadratic.ast_math
PREFIX = "prod04_algebra_rational_equations"
DOMAIN = ("Work over real x where every original denominator is nonzero. E lists excluded "
          "inputs, C lists polynomial candidates, and S is the complete solution set. "
          "Lambda denotes the chosen LCD. L(t) and R(t) evaluate the original sides at t. "
          "V lists L(r),R(r) for each solution r in increasing order.")


def need(condition, message):
    if not condition:
        raise ValueError(message)


def trim(p):
    p = list(p)
    while len(p) > 1 and not p[-1]:
        p.pop()
    return tuple(p)


Z, ONE, X = (F(0),), (F(1),), (F(0), F(1))


def plus(p, q):
    return trim(a+b for a, b in zip_longest(p, q, fillvalue=F(0)))


def scale(p, c):
    return trim(v*c for v in p)


def minus(p, q):
    return plus(p, scale(q, -1))


def times(p, q):
    # Degree four is needed only for exact rational cross-products, not inputs.
    need(len(p)+len(q)-2 <= 4, "bounded intermediate degree")
    result = [F(0)]*(len(p)+len(q)-1)
    for i, a in enumerate(p):
        for j, b in enumerate(q):
            result[i+j] += a*b
    return trim(result)


def division(p, q):
    need(q != Z, "zero polynomial divisor")
    remainder = p
    result = [F(0)]*max(1, len(p)-len(q)+1)
    while remainder != Z and len(remainder) >= len(q):
        shift = len(remainder)-len(q)
        coefficient = remainder[-1]/q[-1]
        result[shift] += coefficient
        remainder = minus(remainder, (F(0),)*shift+scale(q, coefficient))
    return trim(result), remainder


def quotient(p, q):
    result, remainder = division(p, q)
    need(remainder == Z, "denominator does not divide chosen multiplier")
    return result


def common(p, q):
    while q != Z:
        p, q = q, division(p, q)[1]
    return scale(p, 1/p[-1]) if p != Z else ONE


def polynomial(node):
    """Evaluate only rational polynomial operations of bounded degree."""
    if isinstance(node, ast.Constant):
        need(type(node.value) is int and abs(node.value) <= 10000, "bounded integer literal")
        return (exact(node.value),)
    if isinstance(node, ast.Name):
        need(node.id == "x", "only x is a variable")
        return X
    if isinstance(node, ast.UnaryOp):
        need(isinstance(node.op, (ast.USub, ast.UAdd)), "signed arithmetic only")
        return scale(polynomial(node.operand), -1 if isinstance(node.op, ast.USub) else 1)
    need(isinstance(node, ast.BinOp), "rational polynomial operation required")
    p = polynomial(node.left)
    if isinstance(node.op, ast.Pow):
        need(isinstance(node.right, ast.Constant) and node.right.value == 2, "squaring only")
        return times(p, p)
    q = polynomial(node.right)
    if isinstance(node.op, ast.Div):
        need(len(q) == 1 and q != Z, "polynomial has nonconstant or zero denominator")
        return scale(p, 1/q[0])
    operation = {ast.Add: plus, ast.Sub: minus, ast.Mult: times}.get(type(node.op))
    need(operation is not None, "unsupported polynomial operation")
    return operation(p, q)


def poly(text):
    return polynomial(math_tree(text))


def terms(node):
    if isinstance(node, ast.BinOp) and isinstance(node.op, (ast.Add, ast.Sub)):
        right = terms(node.right)
        if isinstance(node.op, ast.Sub):
            right = [(scale(n, -1), d) for n, d in right]
        return terms(node.left)+right
    if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
        return [(scale(n, -1), d) for n, d in terms(node.operand)]
    if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Div):
        n, d = polynomial(node.left), polynomial(node.right)
        need(d != Z, "original zero denominator")
        return [(scale(n, -1), scale(d, -1))] if d[-1] < 0 else [(n, d)]
    return [(polynomial(node), ONE)]


def parse_equation(text):
    need(text.count("=") == 1, "one equality required")
    left, right = (math_tree(s) for s in text.split("="))
    return left, right, terms(left), terms(right)


def roots(p):
    """All real roots when rational, or an explicit irrational-pair signature."""
    if p == Z:
        return None
    if len(p) == 1:
        return frozenset()
    if len(p) == 2:
        return frozenset((-p[0]/p[1],))
    need(len(p) == 3, "cleared polynomial exceeds degree two")
    c, b, a = p
    discriminant = b*b-4*a*c
    if discriminant < 0:
        return frozenset()
    n, d = isqrt(discriminant.numerator), isqrt(discriminant.denominator)
    if n*n != discriminant.numerator or d*d != discriminant.denominator:
        return ("irrational_pair", scale(p, 1/a))
    radical = F(n, d)
    return frozenset(((-b-radical)/(2*a), (-b+radical)/(2*a)))


def evaluate(p, x):
    total = F(0)
    for v in reversed(p):
        total = total*x+v
    return total


def exclusions(items):
    result = set()
    for _, d in items:
        r = roots(d)
        need(isinstance(r, frozenset), "denominator must split over rational factors")
        result.update(r)
    return frozenset(result)


def lcd(items):
    factors, contents = {}, []
    for _, d in items:
        need(d != Z and all(v.denominator == 1 for v in d), "integer original denominator")
        r = roots(d)
        need(isinstance(r, frozenset), "rational denominator roots required")
        need(len(r) == len(d)-1, "denominator must have complete distinct rational linear factors")
        product = ONE
        for root in sorted(r):
            factor = (F(-root.numerator), F(root.denominator))
            factors[root] = factor
            product = times(product, factor)
        content = d[-1]/product[-1]
        need(content.denominator == 1 and 1 <= content <= 6, "denominator content bound")
        need(scale(product, content) == d, "denominator factor identity")
        contents.append(int(content))
    need(len(factors) <= 2, "at most two distinct denominator factors")
    result = (F(lcm(*contents)),)
    for root in sorted(factors):
        result = times(result, factors[root])
    return result


def cleared(items, multiplier):
    result = Z
    for n, d in items:
        result = plus(result, times(n, quotient(multiplier, d)))
    return result


def reduced_fraction(n, d):
    g = common(n, d)
    n, d = quotient(n, g), quotient(d, g)
    return scale(n, 1/d[-1]), scale(d, 1/d[-1])


def sum_fraction(items):
    numerator, denominator = Z, ONE
    for n, d in items:
        numerator, denominator = reduced_fraction(
            plus(times(numerator, d), times(n, denominator)), times(denominator, d))
    return numerator, denominator


def relation(left, right, retained):
    n, d = sum_fraction(left+[(scale(n, -1), d) for n, d in right])
    excluded = retained | exclusions(left+right)
    candidates = roots(n)
    if candidates is None:
        return "cofinite", excluded
    if not isinstance(candidates, frozenset):
        return candidates
    return "finite", candidates-excluded


def original(case):
    _, _, left, right = parse_equation(case["given"])
    for side, declared in ((left, case["left"]), (right, case["right"])):
        need(len(side) == len(declared) <= 2, "original additive term count")
        for (n, d), vector in zip(side, declared):
            need(n == trim(exact(v) for v in vector["numerator"]) and
                 d == trim(exact(v) for v in vector["denominator"]),
                 "actual original differs from independent term expansion")
            need(len(n) <= 3 and len(d) <= 3 and
                 all(v.denominator == 1 and abs(v) <= 30 for v in n+d),
                 "original polynomial coefficient/degree bound")
    excluded = exclusions(left+right)
    multiplier = lcd(left+right)
    cl, cr = cleared(left, multiplier), cleared(right, multiplier)
    difference = minus(cl, cr)
    need(len(difference) <= 3 and all(abs(v) <= 100 for v in difference), "cleared bound")
    candidates = roots(difference)
    need(candidates is None or isinstance(candidates, frozenset), "finite pool rational roots")
    for root in excluded | (candidates or frozenset()):
        need(abs(root.numerator) <= 30 and root.denominator <= 6, "root bound")
    solution = ("cofinite", excluded) if candidates is None else ("finite", candidates-excluded)
    need(relation(left, right, excluded) == solution, "independent rational difference disagrees")
    checks, rejection = [], []
    for root in sorted(candidates or ()):
        denominators = [evaluate(d, root) for _, d in left+right]
        if root in excluded:
            need(0 in denominators, "excluded candidate lacks original zero denominator")
            rejection.append({"candidate":root, "original_denominators":denominators})
        else:
            values = [sum(evaluate(n, root)/evaluate(d, root) for n, d in side)
                      for side in (left, right)]
            need(values[0] == values[1], "root fails original sides")
            checks.extend(values)
    reduced = [[(quotient(n, common(n,d)), quotient(d, common(n,d))) for n,d in side]
               for side in (left, right)]
    reduced_lcd = lcd(reduced[0]+reduced[1])
    return {"left":left, "right":right, "excluded":excluded, "lcd":multiplier,
            "cleared":(cl,cr), "difference":difference, "candidates":candidates,
            "solution":solution, "checks":tuple(checks), "rejected":rejection,
            "reduced":reduced, "reduced_cleared":tuple(cleared(s,reduced_lcd) for s in reduced)}


def finite_set(text):
    if text == r"\varnothing":
        return frozenset()
    need(text.startswith(r"\{") and text.endswith(r"\}"), "finite set braces")
    values = []
    for s in text[2:-2].split(","):
        p = poly(s)
        need(len(p) == 1, "rational set member required")
        values.append(p[0])
    need(len(values) == len(set(values)), "duplicate set member")
    return frozenset(values)


def set_label(text, prefix):
    need(text.startswith(prefix+"="), "set label")
    body = text[len(prefix)+1:].replace(r"\left", "").replace(r"\right", "")
    if body == r"\mathbb{R}":
        return "cofinite", frozenset()
    if body.startswith(r"\mathbb{R}\setminus"):
        return "cofinite", finite_set(body[len(r"\mathbb{R}\setminus"):])
    return "finite", finite_set(body)


def check_vector(text):
    need(text.startswith("V="), "original check label")
    body = text[2:].replace(r"\left", "").replace(r"\right", "")
    need(body.startswith("(") and body.endswith(")"), "ordered value list")
    values = [poly(s) for s in body[1:-1].split(",")]
    need(all(len(p) == 1 for p in values), "rational values required")
    return tuple(p[0] for p in values)


def factor_shape(node):
    return (isinstance(node, ast.BinOp) and isinstance(node.op, ast.Mult)
            and len(polynomial(node.left)) == len(polynomial(node.right)) == 2)


def classify(text, goal, data):
    if goal in ("excluded", "candidates", "solutions"):
        prefix = {"excluded":"E", "candidates":"C", "solutions":"S"}[goal]
        actual = set_label(text, prefix)
        target = (("finite",data["excluded"]) if goal == "excluded" else
                  ("finite",data["candidates"]) if goal == "candidates" else data["solution"])
        return actual == target, True, actual
    if goal == "check":
        actual = check_vector(text)
        return actual == data["checks"], True, actual
    if goal == "lcd":
        need(text.startswith(r"\Lambda="), "LCD label")
        actual = poly(text[len(r"\Lambda="):])
        truth = actual != Z and all(division(actual,d)[1] == Z for _,d in data["left"]+data["right"])
        return truth, actual == data["lcd"], actual
    l, r, left, right = parse_equation(text)
    truth = relation(left, right, data["excluded"]) == data["solution"]
    fractions = tuple(sum_fraction(s) for s in (left,right))
    polynomial_sides = all(d == ONE for _,d in fractions)
    values = tuple(n for n,_ in fractions)
    if goal == "cancel":
        target = tuple(sum_fraction(s) for s in data["reduced"])
        shape = (fractions == target and len(left) == len(right) == 1
                 and len(left[0][0]) == len(left[0][1]) == 2
                 and isinstance(l, ast.BinOp) and isinstance(l.op, ast.Div))
    elif goal in ("clear", "clear_reduced", "clear_factored"):
        target = data["reduced_cleared"] if goal == "clear_reduced" else data["cleared"]
        no_divisions = not any(isinstance(n,ast.Div) for tree in (l,r) for n in ast.walk(tree))
        shape = polynomial_sides and values == target and no_divisions
        if goal == "clear_factored":
            shape = shape and factor_shape(l) and factor_shape(r)
    elif goal == "constant":
        shape = (polynomial_sides and values == (data["difference"], Z)
                 and len(values[0]) == 1)
    else:
        need(goal == "factor", "unknown local goal")
        standard = scale(data["difference"], 1 if data["difference"][-1] > 0 else -1)
        shape = polynomial_sides and values == (standard,Z) and factor_shape(l)
    # Distinct forms may demonstrate an explicit goal contrast; identical
    # normalized sides with identical goal-form status are duplicate choices.
    return truth, bool(shape), (fractions, bool(shape))


def reached(text, goal, data):
    if goal == "check":
        roots_text, values = text.split(r",\quad ")
        need(set_label(roots_text,"S") == data["solution"], "final solution changed")
        result = classify(values, goal, data)
    else:
        result = classify(text, goal, data)
    need(result[0] and result[1], "false or goal-missing reached state")


def audit(routes, pool):
    need(routes["accepted"] is True and routes["windows"] == 0, "accepted headless model required")
    need(pool["domain"] == DOMAIN, "original real domain contract changed")
    need(pool["bounds"] == {"term_count":2,"polynomial_degree":2,"original_coefficient":30,
                           "constant_denominator":6,"cleared_coefficient":100,
                           "root_numerator":30,"root_denominator":6,
                           "distinct_variable_factors":2}, "finite bounds changed")
    ids = [f"{PREFIX}_q{i:02}" for i in range(1,13)]
    need([c["id"] for c in pool["cases"]] == ids == routes["question_ids"], "identity/order")
    need([q["id"] for q in routes["questions"]] == ids, "compiled question identities")
    need([c["group"] for c in pool["cases"]] ==
         ["introductory"]*4+["practice"]*4+["mixed"]*4, "group coverage")
    records, positions, seeds = [], [], set()
    for case, row in zip(pool["cases"], routes["questions"]):
        source = case["source"]
        need(source["source_id"] == "precalc_rational" and source["changes"] and
             source["original_givens"] and source["adaptation_kind"], "traceable adaptation")
        exercise = int(re.search(r"Exercise (\d+) ", source["locator"])[1])
        need(exercise in range(5,19), "unapproved seed")
        seeds.add(exercise)
        data, q = original(case), row["question"]
        need(q["equation"] == case["given"] and q["description"].endswith(" "+DOMAIN),
             "compiled original/domain mismatch")
        steps, states = q["steps"], q["working_states"]
        need(len(steps) == len(case["goals"]) and len(states) == len(steps)+1 and
             states[0]["display"] == case["given"], "complete route states")
        decisions = []
        for i,(step,goal) in enumerate(zip(steps,case["goals"])):
            need(step["semantics"]["before"] == states[i]["id"] and
                 step["semantics"]["after"] == states[i+1]["id"], "working continuity")
            valid, signatures, checks = [], [], []
            need(len(step["options"]) == 3, "three choices")
            for option in step["options"]:
                truth, form, signature = classify(option["label"], goal, data)
                signatures.append(signature)
                checks.append({"id":option["id"], "mathematical_truth":truth,"requested_form":form})
                if truth and form:
                    valid.append(option["id"])
            need(len(set(signatures)) == 3, "equivalent duplicate options")
            need(len(valid) == 1 and step["accepted_option_ids"] == valid, "false or nonunique key")
            for option in step["options"]:
                need("wrong_feedback" not in option if option["id"] in valid else
                     bool(option.get("wrong_feedback")), "specific feedback attachment")
            need(step["prompt"] and step["explanation"] and step["wrong_hint"], "teaching field")
            need("\\" not in step["explanation"], "history must be ordinary prose")
            reached(states[i+1]["display"], goal, data)
            if i == 0:
                positions.append(next(j+1 for j,o in enumerate(step["options"]) if o["id"] in valid))
            decisions.append({"goal":goal,"options":checks,"valid_id":valid[0],"reached_verified":True})
        records.append({"id":case["id"],"source_exercise":exercise,"original":case["given"],
                        "excluded":sorted(data["excluded"]),"lcd_coefficients":data["lcd"],
                        "cleared_sides":data["cleared"],"cleared_difference":data["difference"],
                        "candidates":sorted(data["candidates"]) if data["candidates"] is not None else "identity",
                        "solution_kind":data["solution"][0],"solution_members_or_exclusions":sorted(data["solution"][1]),
                        "original_values":data["checks"],"rejected_candidates":data["rejected"],
                        "decisions":decisions})
    need(len(seeds) >= 6 and sorted(positions) == [1]*4+[2]*4+[3]*4,"source/position coverage")
    count = sum(len(r["decisions"]) for r in records)
    need(count == 53 and routes["wrong_choices"] == 106, "complete decision count")
    return {"questions":12,"readings":1,"steps":count,"options":159,"wrong_choices":106,
            "source_questions":12,"distinct_seed_prompts":len(seeds),"seed_exercises":sorted(seeds),
            "first_key_positions":positions,"cases":records}


def controls(routes,pool):
    results = []
    def reject(name, action):
        try:
            action()
        except (ValueError,AssertionError,SyntaxError,ZeroDivisionError) as error:
            results.append({"control":name,"rejected":True,"reason":str(error)})
        else:
            raise AssertionError("negative control accepted: "+name)
    def mutation(qi,si,label,mode):
        changed = copy.deepcopy(routes)
        q = changed["questions"][qi]["question"]
        step = q["steps"][si]
        if mode in ("choice","both"):
            next(o for o in step["options"] if o["id"] in step["accepted_option_ids"])["label"] = label
        if mode in ("state","both"):
            before = q["working_states"][si+1]["display"]
            q["working_states"][si+1]["display"] = (
                before.split(r",\quad ")[0]+r",\quad "+label
                if pool["cases"][qi]["goals"][si]=="check" else label)
        return audit(changed,pool)
    faults = [
        (0,1,"2x-3=x+5","false cleared arithmetic"),
        (11,2,"2(x-5)+1=3x-1","omitted LCD term"),
        (8,1,r"\frac{x-1}{x+1}=\frac{3}{2}","false cancellation"),
        (5,3,r"S=\{-2,1\}","extraneous denominator-zero root"),
        (4,3,r"S=\{1\}","zero-over-zero candidate"),
        (9,3,r"S=\mathbb{R}","identity lost original domain"),
        (9,3,r"S=\mathbb{R}\setminus\{2\}","canceled exclusion lost"),
        (0,0,r"E=\{4\}","wrong restriction"),
        (6,2,"2x^2+10x-12=0","true but unfactored result"),
        (9,2,"x^2-x-6=x^2-x-6","true but products not factored"),
        (0,1,"x=7","true but not LCD multiplication result"),
        (9,1,r"\Lambda=(x-2)(x+2)^2","common but not least denominator"),
        (0,1,r"\frac{4x-6}{2}=x+4","true but uncleared displayed constant denominator"),
        (8,1,r"\frac{(x-1)(x+3)}{(x-1)(x+1)}=\frac{3}{2}","true but uncanceled original"),
        (0,3,r"V=\left(2,1\right)","false original side evaluation")]
    for mode in ("choice","state","both"):
        for qi,si,label,name in faults:
            reject(name+" "+mode, lambda q=qi,s=si,t=label,m=mode: mutation(q,s,t,m))
    bad = copy.deepcopy(routes)
    bad["questions"][0]["question"]["steps"][0]["accepted_option_ids"]=[12]
    reject("false key", lambda: audit(bad,pool))
    for qi,si,label in ((4,0,r"E=\{1,-1\}"),(1,0,r"E=\{\frac{2}{6}\}")):
        bad = copy.deepcopy(routes)
        step = bad["questions"][qi]["question"]["steps"][si]
        next(o for o in step["options"] if o["id"] not in step["accepted_option_ids"])["label"]=label
        reject("equivalent duplicate "+str(qi),lambda b=bad:audit(b,pool))
    bad = copy.deepcopy(routes)
    bad["questions"][0]["question"]["description"]=bad["questions"][0]["question"]["description"].replace("real x","complex x")
    reject("compiled domain change",lambda:audit(bad,pool))
    for expression,numerator,denominator in (
            (r"\frac{1}{0}=1",[1],[0]),
            (r"\frac{1}{x^2+1}=1",[1],[1,0,1]),
            (r"\frac{1}{(x-1)^2}=1",[1],[1,-2,1]),
            (r"\frac{1}{x^3-1}=1",[1],[-1,0,0,1]),
            (r"\frac{31}{x-1}=1",[31],[-1,1])):
        fake = copy.deepcopy(pool["cases"][0])
        fake["given"]=expression
        fake["left"]=[{"numerator":numerator,"denominator":denominator}]
        fake["right"]=[{"numerator":[1],"denominator":[1]}]
        # Matching independent vectors ensure the intended input bound is tested.
        reject("unsupported input "+expression, lambda c=fake:original(c))
    alternatives = [
        (4,"excluded",r"E=\{1,-1\}"),
        (1,"excluded",r"E=\{\frac{2}{6}\}"),
        (1,"clear","x-5=6x-2"),
        (6,"factor","(x+6)(2x-2)=0"),
        (5,"candidates",r"C=\{1,-2\}"),
        (7,"solutions",r"S=\{\frac{6}{4},-3\}"),
        (9,"lcd",r"\Lambda=(x+2)(x-2)"),
        (9,"clear_factored","(x+2)(x-3)=(x-3)(x+2)"),
        (9,"solutions",r"S=\mathbb{R}\setminus\{2,-2\}"),
        (11,"check",r"V=\left(\frac{2}{4},\frac{3}{6}\right)")]
    for qi,goal,text in alternatives:
        truth,form,_=classify(text,goal,original(pool["cases"][qi]))
        need(truth and form,"valid alternative refused: "+text)
        results.append({"control":"valid "+goal+" "+text,"accepted":True})
    return results


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--routes",required=True,type=Path)
    parser.add_argument("--edited-routes",type=Path)
    args=parser.parse_args()
    pool=json.loads((SOURCE/"cases.json").read_text())
    routes=json.loads(args.routes.read_text())
    result=audit(routes,pool)
    result["controls"]=controls(routes,pool)
    if args.edited_routes:
        edited=json.loads(args.edited_routes.read_text())
        need(audit(edited,pool)==audit(routes,pool),"edit changed mathematical certificates")
        old=routes["questions"][0]["question"]["steps"][0]
        new=edited["questions"][0]["question"]["steps"][0]
        need(old["prompt"]!=new["prompt"],"real Markdown prompt edit missing")
        old_option=next(o for o in old["options"] if o["id"]==12)
        new_option=next(o for o in new["options"] if o["id"]==12)
        need(old_option["wrong_feedback"]!=new_option["wrong_feedback"],"real Markdown feedback edit missing")
        new["prompt"]=old["prompt"]
        new_option["wrong_feedback"]=old_option["wrong_feedback"]
        need(edited==routes,"other compiled fields changed")
        result["markdown_edit_proof"]="only q01 step10 prompt and option12 feedback changed; exact mathematics unchanged"
    result["scope"]="all twelve pinned adapted equations; rational degree-two inputs with degree-four cross-product intermediates; no sampled identities"
    print(json.dumps(result,indent=2,default=str))


if __name__=="__main__":
    main()
