#include "runtime/ai/NpcBehaviorProfile.hpp"

namespace iggy3d {

std::string_view npcBehaviorProfileResolveStatusName(
    NpcBehaviorProfileResolveStatus status) {
  switch (status) {
    case NpcBehaviorProfileResolveStatus::Resolved:
      return "profile_resolved";
    case NpcBehaviorProfileResolveStatus::Missing:
      return "profile_missing";
    case NpcBehaviorProfileResolveStatus::InvalidId:
      return "profile_invalid_id";
    case NpcBehaviorProfileResolveStatus::InvalidConfig:
      return "profile_invalid_config";
    case NpcBehaviorProfileResolveStatus::CatalogEmpty:
      return "profile_catalog_empty";
  }
  return "profile_invalid_id";
}

NpcBehaviorConfig configFromNpcBehaviorProfile(const NpcBehaviorProfile& profile) {
  NpcBehaviorConfig config;
  config.engagementPolicy = profile.engagementPolicy;
  config.perceptionRadiusMeters = profile.perceptionRadiusMeters;
  config.chaseStopDistanceMeters = profile.chaseStopDistanceMeters;
  config.attackRangeMeters = profile.attackRangeMeters;
  config.chaseStepMeters = profile.chaseStepMeters;
  config.visionHalfAngleDegrees = profile.visionHalfAngleDegrees;
  config.verticalHalfAngleDegrees = profile.verticalHalfAngleDegrees;
  config.guardEyeHeightMeters = profile.guardEyeHeightMeters;
  config.targetStandEyeHeightMeters = profile.targetStandEyeHeightMeters;
  config.targetSneakEyeHeightMeters = profile.targetSneakEyeHeightMeters;
  config.occlusionMarginMeters = profile.occlusionMarginMeters;
  config.attackDamage = profile.attackDamage;
  config.decisionIntervalTicks = profile.decisionIntervalTicks;
  config.attackCooldownTicks = profile.attackCooldownTicks;
  return config;
}

NpcBehaviorProfileCatalog makeBuiltInNpcBehaviorProfileCatalog() {
  NpcBehaviorProfileCatalog catalog;

  NpcBehaviorProfile defaultProfile;
  defaultProfile.id.value = "default";
  catalog.profiles.push_back(defaultProfile);

  NpcBehaviorProfile meleeTraining;
  meleeTraining.id.value = "melee_training";
  catalog.profiles.push_back(meleeTraining);

  NpcBehaviorProfile passive;
  passive.id.value = "passive";
  passive.engagementPolicy = NpcEngagementPolicy::Passive;
  catalog.profiles.push_back(passive);

  return catalog;
}

NpcBehaviorProfileResolveResult resolveNpcBehaviorProfile(
    const NpcBehaviorProfileResolveRequest& request) {
  NpcBehaviorProfileResolveResult result;

  if (!isValidNpcBehaviorProfileId(request.profileId)) {
    result.status = NpcBehaviorProfileResolveStatus::InvalidId;
    result.reasonCode = npcBehaviorProfileResolveStatusName(result.status);
    return result;
  }
  if (request.catalog == nullptr || request.catalog->profiles.empty()) {
    result.status = NpcBehaviorProfileResolveStatus::CatalogEmpty;
    result.reasonCode = npcBehaviorProfileResolveStatusName(result.status);
    return result;
  }

  for (const NpcBehaviorProfile& profile : request.catalog->profiles) {
    if (profile.id.value != request.profileId) {
      continue;
    }
    result.profile = profile;
    result.config = configFromNpcBehaviorProfile(profile);
    if (!isValidNpcBehaviorConfig(result.config) ||
        !isValidAlertProfile(profile.alertProfile)) {
      result.status = NpcBehaviorProfileResolveStatus::InvalidConfig;
      result.reasonCode = npcBehaviorProfileResolveStatusName(result.status);
      return result;
    }
    result.ok = true;
    result.status = NpcBehaviorProfileResolveStatus::Resolved;
    result.reasonCode = npcBehaviorProfileResolveStatusName(result.status);
    return result;
  }

  result.status = NpcBehaviorProfileResolveStatus::Missing;
  result.reasonCode = npcBehaviorProfileResolveStatusName(result.status);
  return result;
}

}  // namespace iggy3d
