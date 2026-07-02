#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

enum class AiBehaviorKind : std::uint8_t {
  None,
  Idle,
  Alert,
  Chasing,
  Attacking,
  Defeated,
  Returning,
};

enum class AiIntentKind : std::uint8_t {
  None,
  Wait,
  MoveTowardTarget,
  AttackTarget,
  ReturnToAnchor,
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
    case AiBehaviorKind::Returning:
      return "returning";
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
    case AiIntentKind::ReturnToAnchor:
      return "return_to_anchor";
  }
  return "none";
}

struct AiActorState {
  EntityId actor;
  std::uint64_t nextDecisionTick = 0;
  std::uint32_t deterministicPolicy = 0;
  bool enabled = true;
  std::string behaviorProfileId = "default";
  EntityId target;
  AiBehaviorKind behavior = AiBehaviorKind::Idle;
  AiIntentKind lastIntent = AiIntentKind::None;
  std::uint32_t cooldownTicksRemaining = 0;
  bool hasHomePosition = false;
  Vec3 homePosition;
  std::string homeStableName;
  float leashRadiusMeters = 0.0F;
  float returnRadiusMeters = 0.0F;
  float homeToleranceMeters = 0.0F;
  // Horizontal gaze direction (normalized) the NPC's vision cone originates
  // from. Transient runtime state: recomputed each decision tick by the AI,
  // not serialized or hashed. Defaults to canonical forward until driven.
  Vec3 facingDirection{0.0F, 0.0F, 1.0F};
  // Last-decision perception outcome, mirrored for read-only observability
  // (debug snapshot / tests / vision overlay). Not part of the decision inputs.
  bool lastTargetInRadius = false;
  bool lastTargetInVisionCone = false;
  bool lastTargetHasLineOfSight = false;
  float lastSightRangeMeters = 0.0F;
};

struct AiState {
  std::vector<AiActorState> actors;
};

}  // namespace iggy3d
