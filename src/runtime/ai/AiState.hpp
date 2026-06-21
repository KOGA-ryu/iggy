#pragma once

#include <cstdint>
#include <vector>

#include "core/ids/EntityId.hpp"

namespace iggy3d {

struct AiActorState {
  EntityId actor;
  std::uint64_t nextDecisionTick = 0;
  std::uint32_t deterministicPolicy = 0;
  bool enabled = true;
};

struct AiState {
  std::vector<AiActorState> actors;
};

}  // namespace iggy3d
