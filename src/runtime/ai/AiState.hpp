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
  // Graded-alert rungs (slice 5), appended so existing serialized values keep
  // their numbers. Ordering low->high is derived by alertBehaviorForLevel, not
  // by enum value.
  Observant,
  Suspicious,
  Searching,
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
    case AiBehaviorKind::Observant:
      return "observant";
    case AiBehaviorKind::Suspicious:
      return "suspicious";
    case AiBehaviorKind::Searching:
      return "searching";
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
  // Graded alert FSM (slice 5). alertLevel is normalized 0..1 (combat = 1.0);
  // behavior is always re-derived from it, never set directly.
  float alertLevel = 0.0F;
  std::uint64_t lastRiseTick = 0;         // dead-time anchor; resets on any rise
  std::uint8_t maxAlertIndexThisEngagement = 0;  // high-water band (calm-down)
  std::uint64_t graceUntilTick = 0;       // up-hysteresis window end
  float graceThreshold = 0.0F;            // level when the window opened
  std::uint32_t graceCount = 0;           // swallowed rises this window
};

struct AiState {
  std::vector<AiActorState> actors;
};

}  // namespace iggy3d
