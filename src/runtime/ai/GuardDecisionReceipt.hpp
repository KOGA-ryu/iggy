#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "core/ids/EntityId.hpp"

namespace iggy3d {

// Deck §18 reasoning receipt for the L6 guard decision -- split into this light header so state
// (AiActorState) can carry the latest receipt without pulling the whole decision kernel (graph +
// physics). The kernel (GuardDecision.hpp) includes this and fills it.

// One SIGNED contribution to a decision score (no black boxes).
struct GuardDecisionFactor {
  std::string_view name;
  float value;
};

// The full, inspectable record of a decision. The chosen node's factors are, in order,
// {"suspicion", "strategic", "travel"(<=0), "ally"(0)}, each already multiplied by its personality
// weight so the receipt shows the ACTUAL contribution. hasChoice=false + empty factors when no
// candidate remains.
struct GuardDecisionReceipt {
  EntityId actor;
  std::uint64_t tick = 0;
  bool hasChoice = false;
  std::uint32_t chosenNodeId = 0;
  float totalScore = 0.0F;
  std::vector<GuardDecisionFactor> factors;
  bool hasExcluded = false;
  std::uint32_t excludedNodeId = 0;
};

}  // namespace iggy3d
