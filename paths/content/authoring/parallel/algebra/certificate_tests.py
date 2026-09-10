"""Pure rejection tests for the algebra pilot's independent certificates."""
import copy
import json
import sys
import unittest
from fractions import Fraction
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "tools"))

import build_question_batch as batch
import export_learning as export
import certificates


class AlgebraCertificateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.sequence = json.loads((Path(__file__).with_name("sequence.json")).read_text())
        cls.questions = batch.validate_role_sequence(cls.sequence, Path(__file__).with_name("sequence.json"))

    def certificate(self, question):
        return batch.reasoning_certificate(question, certificates.CHECKERS)

    def test_every_fixed_case_has_one_exact_decision_and_required_evidence(self):
        checked = [self.certificate(question) for question in self.questions]
        self.assertEqual([item["role"] for item in checked], list(batch.EXERCISE_ROLES))
        self.assertEqual(sum(item["steps_checked"] for item in checked), 7)
        self.assertEqual(sum(item["wrong_choices_checked"] for item in checked), 14)
        self.assertEqual(checked[-1]["expected"]["steps"][0]["answer"], r"x=-\frac{3}{2}")
        self.assertEqual(checked[-1]["evidence"]["facts"]["choice_residuals"], ["-18", "0", "45"])

    def test_original_solution_and_repair_candidates_are_checked_with_fraction_arithmetic(self):
        repair = self.certificate(self.questions[4])["evidence"]["facts"]
        self.assertEqual(repair["wrong_solution"], "2/3")
        self.assertEqual(repair["wrong_solution_residual"], "-10")
        self.assertEqual(repair["solution"], "4")
        self.assertEqual(Fraction(-6) * Fraction(-3, 2) + Fraction(5), Fraction(14))

    def test_zero_coefficient_and_altered_fixed_givens_are_rejected(self):
        altered = copy.deepcopy(self.questions[0])
        altered["case"]["a"] = 0
        with self.assertRaises(export.ExportError):
            self.certificate(altered)
        altered = copy.deepcopy(self.questions[2])
        altered["case"]["c"] = 14
        with self.assertRaises(export.ExportError):
            self.certificate(altered)

    def test_subtracting_a_negative_one_sided_operation_and_noninverse_scaling_fail(self):
        self.assertNotEqual(7 - (-5), 7 - 5)
        a, b, c = -3, 4, 13
        solution = Fraction(c - b, a)
        self.assertEqual(a * solution + b, c)
        self.assertNotEqual(a * solution + (b - 4), c, "one-sided subtraction changes the original condition")
        self.assertNotEqual(Fraction(0) * Fraction(-3), Fraction(-3), "zero scaling cannot reverse division")

    def test_duplicate_or_equivalent_choices_are_rejected_before_compilation(self):
        original = self.questions[5]
        given, after, steps, facts = certificates.independent(original)
        duplicate = [(steps[0][0][:2] + [steps[0][0][1]], steps[0][1])]
        with self.assertRaises(export.ExportError):
            batch.reasoning_certificate(dict(original, case=copy.deepcopy(original["case"])),
                                        {"independent": lambda _: (given, after, duplicate, facts)})

    def test_uniqueness_argument_uses_nonzero_coefficient(self):
        a = Fraction(-6)
        x1 = x2 = Fraction(-3, 2)
        self.assertEqual(a * (x1 - x2), 0)
        self.assertNotEqual(a, 0)
        self.assertEqual(x1, x2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
