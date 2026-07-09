#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "content/NpcBehaviorProfileId.hpp"
#include "runtime/ai/NpcAlertSystem.hpp"
#include "runtime/ai/NpcBehaviorSystem.hpp"
#include "runtime/ai/NpcPersonalityWeights.hpp"
#include "runtime/ai/NpcSoundPerception.hpp"

namespace iggy3d {

struct NpcBehaviorProfile {
  NpcBehaviorProfileId id;
  NpcEngagementPolicy engagementPolicy = NpcEngagementPolicy::Hostile;
  float perceptionRadiusMeters = 6.0F;
  float chaseStopDistanceMeters = 1.25F;
  float attackRangeMeters = 1.5F;
  float chaseStepMeters = 1.0F;
  float visionHalfAngleDegrees = 60.0F;
  float verticalHalfAngleDegrees = 30.0F;
  float guardEyeHeightMeters = 1.6F;
  float targetStandEyeHeightMeters = 1.6F;
  float targetSneakEyeHeightMeters = 0.9F;
  float occlusionMarginMeters = 0.05F;
  std::int32_t attackDamage = 1;
  std::uint32_t decisionIntervalTicks = 1;
  std::uint32_t attackCooldownTicks = 2;
  // Graded-alert tuning (slice 5) rides on the behavior profile. The default
  // construction is valid; resolve validates it via isValidAlertProfile.
  AlertProfile alertProfile;
  // Sound-hearing tuning (a1s2, L1) sibling to alertProfile: the attenuation
  // kernel's per-guard thresholds/losses. Reference defaults from a1s1.
  SoundPerceptionConfig soundConfig;
  // L6 personality weights (a5s1, A9 seam). ALL NEUTRAL (1.0) in v1 -- invisible to behavior; the
  // guard-decision kernel multiplies by them, A9's cards later ship non-neutral values.
  NpcPersonalityWeights personalityWeights;
};

struct NpcBehaviorProfileCatalog {
  std::vector<NpcBehaviorProfile> profiles;
};

enum class NpcBehaviorProfileResolveStatus : std::uint8_t {
  Resolved,
  Missing,
  InvalidId,
  InvalidConfig,
  CatalogEmpty,
};

std::string_view npcBehaviorProfileResolveStatusName(
    NpcBehaviorProfileResolveStatus status);

struct NpcBehaviorProfileResolveRequest {
  const NpcBehaviorProfileCatalog* catalog = nullptr;
  std::string_view profileId;
};

struct NpcBehaviorProfileResolveResult {
  bool ok = false;
  NpcBehaviorProfileResolveStatus status = NpcBehaviorProfileResolveStatus::Missing;
  std::string_view reasonCode = "profile_missing";
  NpcBehaviorProfile profile;
  NpcBehaviorConfig config;
};

NpcBehaviorConfig configFromNpcBehaviorProfile(const NpcBehaviorProfile& profile);
NpcBehaviorProfileCatalog makeBuiltInNpcBehaviorProfileCatalog();
NpcBehaviorProfileResolveResult resolveNpcBehaviorProfile(
    const NpcBehaviorProfileResolveRequest& request);

}  // namespace iggy3d
