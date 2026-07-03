#include "runtime/objective/ObjectiveOutcome.hpp"

namespace iggy3d {

ObjectiveOutcomeTable buildObjectiveOutcomeTable() {
  ObjectiveOutcomeTable table;
  // Order is load-bearing: the exit_ prefix rule is evaluated first (as the old loop was), then the
  // exact gold-key rule -- so a completed exit objective wins over the gold key.
  table.rules.push_back({"exit_", /*isPrefix=*/true, SessionOutcome::Victory});
  table.rules.push_back({"collect_gold_key", /*isPrefix=*/false, SessionOutcome::DemoComplete});
  return table;
}

}  // namespace iggy3d
