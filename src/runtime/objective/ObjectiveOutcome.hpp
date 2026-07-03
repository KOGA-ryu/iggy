#pragma once

#include <string>
#include <vector>

#include "runtime/session/SessionOutcome.hpp"

namespace iggy3d {

// Objective -> terminal-outcome mapping as DATA (A8a groundwork for the deck kernel). One rule maps
// a completed-objective id (exact or by prefix) to a SessionOutcome. Rules are evaluated in order;
// the FIRST rule with a matching complete objective decides. This is a session-level TRANSIENT
// table (rebuilt at create/load; NOT persisted or hashed) -- the deck validator will grow the rule
// set later; v1's defaults reproduce the two previously-hardcoded rules exactly.
struct ObjectiveOutcomeRule {
  std::string match;
  bool isPrefix = false;
  SessionOutcome outcome = SessionOutcome::None;
};

struct ObjectiveOutcomeTable {
  std::vector<ObjectiveOutcomeRule> rules;
};

// Default table: reproduces the two hardcoded rules BYTE-IDENTICALLY --
//   1. any complete objective whose id begins with "exit_"  -> Victory
//   2. the complete objective "collect_gold_key" (exact)    -> DemoComplete
// evaluated in that order (rule 1 has precedence, matching the old code path).
ObjectiveOutcomeTable buildObjectiveOutcomeTable();

}  // namespace iggy3d
