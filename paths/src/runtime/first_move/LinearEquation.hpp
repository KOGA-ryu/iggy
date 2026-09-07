#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <variant>

namespace iggy3d::first_move {

// Reduced exact values; parser and operations enforce |numerator|, denominator <= 1e9.
struct ExactNumber {
  std::int64_t numerator=0, denominator=1;
  bool operator==(const ExactNumber&) const = default;
};
struct LinearExpression {
  ExactNumber coefficient, constant;
  bool operator==(const LinearExpression&) const = default;
};
struct LinearEquation {
  LinearExpression left, right;
  std::string display;
  bool expandable=false, isolated=false;
};
struct AugmentedMatrix {
  std::array<std::array<ExactNumber,3>,2> rows;
  std::string display;
};
enum class MathWorkingModel : std::uint8_t { LinearEquation, RowReduction };
using MathWorkingValue=std::variant<LinearEquation,AugmentedMatrix>;
enum class MathOperation : std::uint8_t {
  Expand, Simplify, Add, Subtract, Multiply, Divide,
  SwapRows, DivideRow1, DivideRow2, AddRow1ToRow2, AddRow2ToRow1
};
enum class RowMove { Swap, Divide, Add };
struct RowOperation {
  MathOperation operation;
  std::string_view key, conceptId;
  RowMove kind;
  std::size_t target, source;
};
inline constexpr std::array rowOperations{
  RowOperation{MathOperation::SwapRows,"swap_rows","row_swap",RowMove::Swap,0,1},
  RowOperation{MathOperation::DivideRow1,"divide_row_1","row_scaling",RowMove::Divide,0,0},
  RowOperation{MathOperation::DivideRow2,"divide_row_2","row_scaling",RowMove::Divide,1,1},
  RowOperation{MathOperation::AddRow1ToRow2,"add_row_1_to_2","row_addition",RowMove::Add,1,0},
  RowOperation{MathOperation::AddRow2ToRow1,"add_row_2_to_1","row_addition",RowMove::Add,0,1}
};
inline constexpr std::size_t kMathReferenceCapacity=16;
struct MathReference {
  std::string id, title, definition, rule;
  std::uint32_t version=0;
  MathOperation operation=MathOperation::SwapRows;
  std::string operand, example;
};
struct MathReferenceExample {
  std::array<std::array<std::string,3>,2> before, after;
  std::array<std::string,3> calculations;
  std::string operation;
};
// A separate authored example, projected by the same exact row rules as play.
// Invalid metadata or examples return no projection. No player evidence changes.
[[nodiscard]] std::optional<MathReferenceExample> mathReferenceExample(const MathReference&);
struct LinearEquationResult {
  std::optional<LinearEquation> equation;
  std::string_view error;
};
// Bounded grammar: x, exact integers/decimals/fractions, (), + - * / and
// implicit multiplication. Variable products/divisors are refused, even if
// later cancellation would remove them. No evaluation of code or functions.
[[nodiscard]] LinearEquationResult parseLinearEquation(std::string_view text);
template<class Value> struct CheckedMathMove {
  std::optional<Value> result;
  std::string operation, explanation;
  std::string_view feedback;
};
using MathMoveCheck=CheckedMathMove<LinearEquation>;
[[nodiscard]] MathMoveCheck checkMathMove(const LinearEquation& before,
    MathOperation operation, std::string_view operand, std::string_view entry);
inline constexpr std::size_t kMathMoveChoiceCapacity=6, kMathResultChoiceCount=4;
struct MathMoveChoice {
  MathOperation operation;
  std::string operand, label;
  std::array<std::string,kMathResultChoiceCount> results;
  std::string_view conceptId;
};
// Bounded presentation projection: concrete moves and distinct result choices,
// without a public answer key. Submission still goes through checkMathMove.
[[nodiscard]] std::vector<MathMoveChoice> availableMathMoves(const LinearEquation& before);
[[nodiscard]] std::string verifyMathSolution(const LinearEquation& original,
                                              const LinearEquation& result);
[[nodiscard]] CheckedMathMove<AugmentedMatrix> parseAugmentedMatrix(std::string_view text);
[[nodiscard]] CheckedMathMove<MathWorkingValue> prepareMathWorking(MathWorkingModel, std::string_view text);
[[nodiscard]] CheckedMathMove<AugmentedMatrix> checkMathMove(const AugmentedMatrix& before,
    MathOperation operation, std::string_view operand, std::string_view entry);
[[nodiscard]] std::vector<MathMoveChoice> availableMathMoves(const AugmentedMatrix& before);
[[nodiscard]] std::string verifyMathSolution(const AugmentedMatrix& original,const AugmentedMatrix& result);
} // namespace iggy3d::first_move
