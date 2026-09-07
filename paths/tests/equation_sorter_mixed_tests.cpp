#include "content/EquationSorterContentIO.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <iterator>
#include <numbers>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>

using namespace paths;
using Json = nlohmann::json;
using Integer = long long;
void expect(bool value, const std::string& reason) { if (!value) throw std::runtime_error(reason); }
Integer integer(const std::string& text) {
  std::size_t read = 0;
  const auto value = std::stoll(text, &read);
  expect(read == text.size() && value >= -1000000 && value <= 1000000, "bounded integer expected");
  return value;
}

// These are bounded, test-only interpretations of the actual displayed strings.
// The runtime neither evaluates these expressions nor knows their subject.
bool trig(const std::string& text) {
  static const std::regex pattern(R"(^(sin|cos|tan)\((pi(?:/[1-9][0-9]*)?|-?[0-9]+) (deg|rad)\) = (-?[0-9]+)(?:/([1-9][0-9]*))?$)");
  std::smatch m;
  if (!std::regex_match(text, m, pattern)) return false;
  const std::string angle = m[2];
  double radians = angle.starts_with("pi") ? std::numbers::pi /
      (angle.size() == 2 ? 1.0 : static_cast<double>(integer(angle.substr(3)))) : static_cast<double>(integer(angle));
  if (m[3] == "deg") radians *= std::numbers::pi / 180;
  constexpr std::array<std::pair<std::string_view, double (*)(double)>, 3> functions{{
      {"sin", std::sin}, {"cos", std::cos}, {"tan", std::tan}}};
  const auto function = std::find_if(functions.begin(), functions.end(), [&](const auto& item) { return item.first == m[1].str(); });
  if (m[1] == "tan" && std::abs(std::cos(radians)) < 1e-12) return false;
  const double expected = static_cast<double>(integer(m[4])) / (m[5].matched ? integer(m[5]) : 1);
  return std::abs(function->second(radians) - expected) < 1e-12;
}

struct Monomial { Integer coefficient, power; };
Monomial monomial(const std::string& text) {
  static const std::regex variable(R"(^(?:(-?[0-9]+)\*)?x(?:\^([0-9]+))?$)");
  static const std::regex constant(R"(^-?[0-9]+$)");
  std::smatch m;
  if (std::regex_match(text, m, variable)) {
    const auto coefficient = m[1].matched ? integer(m[1]) : 1;
    const auto power = m[2].matched ? integer(m[2]) : 1;
    expect(std::abs(coefficient) <= 100 && power <= 8, "bounded monomial expected");
    return {coefficient, power};
  }
  expect(std::regex_match(text, constant), "monomial notation expected");
  return {integer(text), 0};
}
Integer power(Integer base, Integer exponent) {
  Integer value = 1;
  for (Integer i = 0; i < exponent; ++i) value *= base;
  return value;
}
bool calculus(const std::string& text) {
  static const std::regex derivative(R"(^d/dx \((.+)\) = (.+)$)");
  static const std::regex integral(R"(^integral\[(-?[0-9]+),(-?[0-9]+)\] (.+) dx = (-?[0-9]+)$)");
  std::smatch m;
  if (std::regex_match(text, m, derivative)) {
    const auto lhs = monomial(m[1]), rhs = monomial(m[2]);
    return rhs.coefficient == lhs.coefficient * lhs.power &&
        (rhs.coefficient == 0 || rhs.power == lhs.power - 1);
  }
  if (std::regex_match(text, m, integral)) {
    const auto lower = integer(m[1]), upper = integer(m[2]);
    expect(std::abs(lower) <= 10 && std::abs(upper) <= 10, "bounded integration interval expected");
    const auto term = monomial(m[3]);
    // Cross multiplication checks the exact rational integral without floating point.
    return term.coefficient * (power(upper, term.power + 1) - power(lower, term.power + 1)) ==
        integer(m[4]) * (term.power + 1);
  }
  return false;
}
std::vector<Integer> vector(const std::string& text) {
  const auto value = Json::parse(text);
  expect(value.is_array() && value.size() == 2, "two-component vector expected");
  std::vector<Integer> result;
  for (const auto& item : value) {
    expect(item.is_number_integer(), "integer vector components expected");
    result.push_back(integer(item.dump()));
    expect(std::abs(result.back()) <= 100, "bounded vector component expected");
  }
  return result;
}
std::array<std::vector<Integer>, 2> matrix(const std::string& text) {
  const auto value = Json::parse(text);
  expect(value.is_array() && value.size() == 2, "two matrix rows expected");
  return {vector(value[0].dump()), vector(value[1].dump())};
}
bool linear(const std::string& text) {
  static const std::regex pair(R"(^(\[-?[0-9]+,-?[0-9]+\]) (dot|\+) (\[-?[0-9]+,-?[0-9]+\]) = (.+)$)");
  static const std::regex scale(R"(^(-?[0-9]+) \* (\[-?[0-9]+,-?[0-9]+\]) = (.+)$)");
  static const std::regex determinant(R"(^det\((\[\[.+\]\])\) = (-?[0-9]+)$)");
  static const std::regex product(R"(^(\[\[.+\]\]) \* (\[-?[0-9]+,-?[0-9]+\]) = (.+)$)");
  std::smatch m;
  if (std::regex_match(text, m, pair)) {
    const auto a = vector(m[1]), b = vector(m[3]);
    if (m[2] == "dot") return a[0] * b[0] + a[1] * b[1] == integer(m[4]);
    return vector(m[4]) == std::vector<Integer>{a[0] + b[0], a[1] + b[1]};
  }
  if (std::regex_match(text, m, scale)) {
    const auto a = integer(m[1]); const auto b = vector(m[2]);
    return vector(m[3]) == std::vector<Integer>{a * b[0], a * b[1]};
  }
  if (std::regex_match(text, m, determinant)) {
    const auto a = matrix(m[1]);
    return a[0][0] * a[1][1] - a[0][1] * a[1][0] == integer(m[2]);
  }
  if (std::regex_match(text, m, product)) {
    const auto a = matrix(m[1]); const auto b = vector(m[2]);
    return vector(m[3]) == std::vector<Integer>{a[0][0] * b[0] + a[0][1] * b[1], a[1][0] * b[0] + a[1][1] * b[1]};
  }
  return false;
}
std::set<Integer> set(std::string text) {
  expect(text.front() == '{' && text.back() == '}', "set braces expected");
  text.front() = '['; text.back() = ']';
  const auto values = Json::parse(text);
  expect(values.is_array() && values.size() <= 20, "bounded set expected");
  std::set<Integer> result;
  for (const auto& value : values) expect(result.insert(integer(value.dump())).second, "unique set elements expected");
  return result;
}
bool discrete(const std::string& text) {
  static const std::regex sets(R"(^(\{.*\}) (union|intersect) (\{.*\}) = (\{.*\})$)");
  static const std::regex choose(R"(^choose\(([0-9]+),([0-9]+)\) = ([0-9]+)$)");
  static const std::regex logic(R"(^(NOT )?\((true|false) (AND|OR) (true|false)\) = (true|false)$)");
  std::smatch m;
  if (std::regex_match(text, m, sets)) {
    const auto a = set(m[1]), b = set(m[3]);
    std::set<Integer> result;
    if (m[2] == "union") std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::inserter(result, result.end()));
    else std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(result, result.end()));
    return result == set(m[4]);
  }
  if (std::regex_match(text, m, choose)) {
    const auto n = integer(m[1]), k = integer(m[2]);
    expect(k <= n && n <= 12, "bounded combination expected");
    // Enumerate subsets independently of the factorial/binomial formula used to author the card.
    Integer count = 0;
    for (unsigned mask = 0; mask < (1U << n); ++mask) {
      unsigned bits = mask, selected = 0;
      while (bits) { selected += bits & 1; bits >>= 1; }
      if (selected == k) ++count;
    }
    return count == integer(m[3]);
  }
  if (std::regex_match(text, m, logic)) {
    const bool a = m[2] == "true", b = m[4] == "true";
    bool result = m[3] == "AND" ? a && b : a || b;
    if (m[1].matched) result = !result;
    return result == (m[5] == "true");
  }
  return false;
}
bool checked(bool (*verify)(const std::string&), const std::string& text) {
  try { return verify(text); } catch (const std::exception&) { return false; }
}
std::string wrongResult(const std::string& text) {
  const auto split = text.rfind(" = ");
  auto result = text.substr(split + 3);
  if (result == "true" || result == "false") result = result == "true" ? "false" : "true";
  else if (result.front() == '[') {
    auto values = Json::parse(result); values[0] = values[0].get<Integer>() + 1; result = values.dump();
  } else if (result.front() == '{') result = "{}";
  else result = "123";
  return text.substr(0, split + 3) + result;
}
int main() {
  try {
    const auto base = loadSorterContent(SORTER_FIXTURE), mixed = loadSorterContent(SORTER_MIXED_FIXTURE);
    constexpr std::array checks{trig, calculus, linear, discrete};
    std::array<unsigned, 4> subjectCounts{};
    unsigned retained = 0;
    EquationSorterSession session(mixed);
    const auto send = [&](SorterActionKind kind, SorterEquationId id = 0) {
      expect(session.dispatch({kind, SorterBucket::A, id, session.view().revision}).accepted, "mixed pack uses the existing session route");
    };
    send(SorterActionKind::SelectBucket);
    for (const auto& card : mixed) {
      if (card.id < 2000) {
        const auto original = std::find_if(base.begin(), base.end(), [&](const auto& e) { return e.id == card.id; });
        expect(original != base.end() && original->homeIndex < 80 && original->text == card.text, "algebra IDs/text preserved from first 80 originals");
        expect(card.subject == SorterSubject::Algebra && card.hint == original->hint, "retained cards keep prepared algebra guidance");
        ++retained;
        continue;
      }
      const auto subject = card.id / 100 - 20;
      expect(subject < 4 && card.id % 100 >= 1 && card.id % 100 <= 5, "declared subject fixture ID");
      ++subjectCounts[subject];
      expect(card.subject == static_cast<SorterSubject>(subject + 1) && !card.hint.empty(), "new cards declare their verified subject and a hint");
      expect(checked(checks[subject], card.text), "invalid mathematical content in card " + std::to_string(card.id) + ": " + card.text);
      expect(!checked(checks[subject], wrongResult(card.text)), "checker must reject a wrong displayed result");
      send(SorterActionKind::ActivateEquation, card.id);
      send(SorterActionKind::ActivateEquation, card.id);
    }
    expect(retained == 80 && subjectCounts == std::array<unsigned,4>{5,5,5,5}, "80 algebra plus five per new subject");
    expect(session.view().counts[1] == 20, "all new subjects sort without subject-specific runtime code");
    send(SorterActionKind::SelectBucket);
    const auto slots = session.view().inventorySlots;
    send(SorterActionKind::ActivateEquation, slots[0]); send(SorterActionKind::ActivateEquation, slots[0]);
    send(SorterActionKind::Undo);
    send(SorterActionKind::RequestEmpty); send(SorterActionKind::ConfirmEmpty); send(SorterActionKind::Undo);
    expect(session.view().counts[1] == 20 && session.view().inventorySlots == slots, "mixed return and bulk Undo preserve membership and slots");
    EquationSorterSession automatic(mixed);
    expect(automatic.dispatch({SorterActionKind::AutoSort, SorterBucket::A, 0, automatic.view().revision}).changed,
        "prepared mixed pack automatically groups in one transaction");
    expect(automatic.view().counts == std::array<std::size_t, 7>{0,80,5,5,5,5,0} && automatic.view().undoDepth == 1,
        "five maths subjects have distinct groups; Dump stays empty");
    expect(automatic.view().nextStep.find("All grouped") != std::string::npos, "completion explains the next review action");
    (void)automatic.dispatch({SorterActionKind::SelectBucket, SorterBucket::E});
    expect(automatic.view().inventory && automatic.view().slotCount == 5, "after completion one group click opens review");
    std::cout << "80 retained algebra cards; 20 new equations independently checked and wrong-result mutations rejected; sorting and Undo passed\n";
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
