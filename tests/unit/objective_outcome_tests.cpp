// a8a Commit 2: the objective->outcome mapping is DATA. Pin the default table so it keeps
// reproducing the two formerly-hardcoded rules exactly (a change here would move the golden hash /
// receipts -- the defaults ARE the output-preserving contract).

#include "runtime/objective/ObjectiveOutcome.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool defaultTableReproducesTheTwoRules() {
  const iggy3d::ObjectiveOutcomeTable table = iggy3d::buildObjectiveOutcomeTable();
  bool ok = expect(table.rules.size() == 2U, "default outcome table has exactly two rules");
  if (table.rules.size() != 2U) {
    return false;
  }
  // Order is load-bearing: exit_ (prefix) FIRST for precedence, then the exact gold-key rule.
  ok = ok && expect(table.rules[0].match == "exit_" && table.rules[0].isPrefix &&
                        table.rules[0].outcome == iggy3d::SessionOutcome::Victory,
                    "rule 0: prefix exit_ -> Victory (precedence)");
  ok = ok && expect(table.rules[1].match == "collect_gold_key" && !table.rules[1].isPrefix &&
                        table.rules[1].outcome == iggy3d::SessionOutcome::DemoComplete,
                    "rule 1: exact collect_gold_key -> DemoComplete");
  return ok;
}

}  // namespace

int main() {
  return defaultTableReproducesTheTwoRules() ? 0 : 1;
}
