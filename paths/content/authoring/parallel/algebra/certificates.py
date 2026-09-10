"""Independent exact certificates for the bounded balanced-linear pilot.

Each checker starts from its immutable original case, not from rendered Markdown
or an answer key.  The shared authoring checker compares these results with the
real compiled choices.v1 document.
"""
from fractions import Fraction

import build_question_batch as batch


def _tex(value):
    return batch.tex(Fraction(value))


def _equation(a, b, c):
    """Render the fixed one-variable form ax+b=c without hiding its sign."""
    coefficient = _tex(a) + "x"
    added = ("+" if b >= 0 else "-") + _tex(abs(b))
    return coefficient + added + "=" + _tex(c)


def _fixed_case(question, expected):
    case = question["case"]
    batch.require(set(case) == set(expected), f"{question['id']}: unexpected case fields")
    batch.require(case == expected, f"{question['id']}: fixed pilot given was altered")
    batch.require(all(type(value) is int for value in case.values()),
                  f"{question['id']}: original coefficients and constants must be integers")
    if "a" in case:
        batch.require(case["a"] != 0 and abs(case["a"]) <= 20,
                      f"{question['id']}: unique-solution coefficient must be nonzero and bounded")
    return case


def _solution(a, b, c):
    batch.require(a != 0, "A unique-solution linear equation needs a nonzero coefficient")
    return Fraction(c - b, a)


def _residual(a, b, c, candidate):
    return Fraction(a) * Fraction(candidate) + Fraction(b) - Fraction(c)


def _unique_facts(a, b, c, answer):
    batch.require(_residual(a, b, c, answer) == 0, "Derived solution fails the original equation")
    # If x1 and x2 both solve ax+b=c, subtraction gives a(x1-x2)=0.
    # For nonzero a, multiplying by 1/a gives x1-x2=0, hence x1=x2.
    return {
        "coefficient": str(a),
        "added_constant": str(b),
        "right_hand_constant": str(c),
        "solution": str(answer),
        "substitution_residual": str(_residual(a, b, c, answer)),
        "uniqueness_argument": "If x1 and x2 solve ax+b=c, subtraction gives a(x1-x2)=0; a is nonzero, so x1=x2.",
    }


def read_notation(question):
    case = _fixed_case(question, {"a": -4, "b": 7, "c": 19})
    a, b, c = case["a"], case["b"], case["c"]
    answer = rf"({_tex(a)},{_tex(b)},{_tex(c)})"
    choices = [answer, rf"({_tex(abs(a))},{_tex(b)},{_tex(c)})", rf"({_tex(a)},{_tex(c)},{_tex(b)})"]
    facts = _unique_facts(a, b, c, _solution(a, b, c))
    facts.update({"notation_order": "(a,b,c)", "equation": _equation(a, b, c)})
    return _equation(a, b, c), [rf"(a,b,c)={answer}"], [(choices, answer)], facts


def worked_check(question):
    case = _fixed_case(question, {"a": 5, "b": -6, "c": 9})
    a, b, c = case["a"], case["b"], case["c"]
    reached = c - b
    batch.require(reached == 15, "The fixed worked calculation must reach 15")
    facts = _unique_facts(a, b, c, _solution(a, b, c))
    facts.update({"balance_operation": "add 6 to both complete sides", "reached_right_hand_side": str(reached)})
    return _equation(a, b, c), [rf"{_tex(a)}x={_tex(reached)}"], [(["9", "15", "3"], "15")], facts


def choose_next_step(question):
    case = _fixed_case(question, {"a": -3, "b": 4, "c": 13})
    a, b, c = case["a"], case["b"], case["c"]
    reached = c - b
    correct = r"-4\ \text{ to both sides}"
    choices = [r"+4\ \text{ to both sides}", r"-4\ \text{ on the left only}", correct]
    batch.require(reached == 9, "The cancelling move must reach -3x=9")
    facts = _unique_facts(a, b, c, _solution(a, b, c))
    facts.update({
        "goal": "remove the added constant while retaining the coefficient and equality",
        "correct_operation": "subtract 4 from both complete sides",
        "valid_but_not_goal": "adding 4 to both sides preserves equality but produces -3x+8=17",
        "one_sided_failure": "subtracting 4 only on the left changes the solution set",
    })
    return _equation(a, b, c), [rf"{_tex(a)}x={_tex(reached)}"], [(choices, correct)], facts


def explain_step(question):
    case = _fixed_case(question, {"a": -4, "rhs": 12})
    a, rhs = case["a"], case["rhs"]
    answer = r"\times -4\ \text{ on both sides}"
    choices = [r"\div -4\ \text{ on both sides}", r"\times 0\ \text{ on both sides}", answer]
    recovered = Fraction(rhs, a) * a
    batch.require(recovered == rhs, "Division by the coefficient must recover the starting equation")
    batch.require(a != 0, "Division has no inverse when the coefficient is zero")
    facts = {
        "given": rf"{_tex(a)}x={_tex(rhs)}",
        "inverse_operation": "divide both complete sides by -4",
        "recovered_right_hand_side": str(recovered),
        "zero_scaling_failure": "Multiplying by zero erases the restriction and cannot be inverted.",
        "repeat_scaling_failure": "Multiplying by -4 repeats rather than reverses the division.",
        "nonzero_condition": "-4 is nonzero, so division by -4 is defined and reversible.",
    }
    given = rf"{_tex(a)}x={_tex(rhs)}"
    return given, [given], [(choices, answer)], facts


def repair_error(question):
    case = _fixed_case(question, {"a": 3, "b": -5, "c": 7})
    a, b, c = case["a"], case["b"], case["c"]
    corrected_rhs = c - b
    wrong_answer = Fraction(2, 3)
    correct_answer = _solution(a, b, c)
    batch.require(corrected_rhs == 12 and correct_answer == 4, "The repaired calculation must reach 3x=12 and x=4")
    batch.require(_residual(a, b, c, wrong_answer) != 0, "The student's incorrect value must fail the original equation")
    facts = _unique_facts(a, b, c, correct_answer)
    facts.update({
        "first_error": "L_1",
        "signed_subtraction": "7-(-5)=12",
        "wrong_solution": str(wrong_answer),
        "wrong_solution_residual": str(_residual(a, b, c, wrong_answer)),
        "corrected_reached_equation": "3x=12",
    })
    given = (r"\begin{gathered}3x-5=7\\"
             r"\begin{aligned}L_1 &: 3x=7-5\\L_2 &: 3x=2\\L_3 &: x=\frac{2}{3}\end{aligned}\end{gathered}")
    return given, [r"L_1:\quad 7-(-5)=12", r"3x=12"], [
        ([r"L_1", r"L_2", r"L_3"], r"L_1"),
        (["2", "7", "12"], "12"),
    ], facts


def independent(question):
    case = _fixed_case(question, {"a": -6, "b": 5, "c": 14})
    a, b, c = case["a"], case["b"], case["c"]
    answer = _solution(a, b, c)
    candidates = [Fraction(3, 2), Fraction(-3, 2), Fraction(-9)]
    residuals = [_residual(a, b, c, candidate) for candidate in candidates]
    batch.require(answer == Fraction(-3, 2) and residuals == [Fraction(-18), Fraction(0), Fraction(45)],
                  "Independent candidates must have residuals -18, 0 and 45")
    facts = _unique_facts(a, b, c, answer)
    facts.update({"choice_residuals": [str(value) for value in residuals]})
    choices = [r"x=\frac{3}{2}", r"x=-\frac{3}{2}", r"x=-9"]
    return _equation(a, b, c), [r"x=-\frac{3}{2}"], [(choices, choices[1])], facts


CHECKERS = {
    "read_notation": read_notation,
    "worked_check": worked_check,
    "choose_next_step": choose_next_step,
    "explain_step": explain_step,
    "repair_error": repair_error,
    "independent": independent,
}
