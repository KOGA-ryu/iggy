#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "core/ids/EntityId.hpp"

namespace iggy3d {

enum class AiBehaviorKind : std::uint8_t {
  None,
  Idle,
  Alert,
  Chasing,
  Attacking,
  Defeated,
};

enum class AiIntentKind : std::uint8_t {
  None,
  Wait,
  MoveTowardTarget,
  AttackTarget,
};

inline std::string_view aiBehaviorKindName(AiBehaviorKind kind) {
  switch (kind) {
    case AiBehaviorKind::None:
      return "none";
    case AiBehaviorKind::Idle:
      return "idle";
    case AiBehaviorKind::Alert:
      return "alert";
    case AiBehaviorKind::Chasing:
      return "chasing";
    case AiBehaviorKind::Attacking:
      return "attacking";
    case AiBehaviorKind::Defeated:
      return "defeated";
  }
  return "none";
}

inline std::string_view aiIntentKindName(AiIntentKind kind) {
  switch (kind) {
    case AiIntentKind::None:
      return "none";
    case AiIntentKind::Wait:
      return "wait";
    case AiIntentKind::MoveTowardTarget:
      return "move_toward_target";
    case AiIntentKind::AttackTarget:
      return "attack_target";
  }
  return "none";
}

struct AiActorState {
  EntityId actor;
  std::uint64_t nextDecisionTick = 0;
  std::uint32_t deterministicPolicy = 0;
  bool enabled = true;
  EntityId target;
  AiBehaviorKind behavior = AiBehaviorKind::Idle;
  AiIntentKind lastIntent = AiIntentKind::None;
  std::uint32_t cooldownTicksRemaining = 0;
};

struct AiState {
  std::vector<AiActorState> actors;
};

}  // namespace iggy3d
