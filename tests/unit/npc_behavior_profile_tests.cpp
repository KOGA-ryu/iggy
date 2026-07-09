#include "runtime/ai/NpcBehaviorProfile.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

#include "runtime/ai/AiState.hpp"

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(float lhs, float rhs) {
  return std::fabs(lhs - rhs) <= 0.0001F;
}

bool expectDefaultConfig(const iggy3d::NpcBehaviorConfig& config,
                         iggy3d::NpcEngagementPolicy policy) {
  return expect(config.engagementPolicy == policy, "engagement policy") &&
         expect(near(config.perceptionRadiusMeters, 6.0F), "perception default") &&
         expect(near(config.chaseStopDistanceMeters, 1.25F), "stop default") &&
         expect(near(config.attackRangeMeters, 1.5F), "attack range default") &&
         expect(near(config.chaseStepMeters, 1.0F), "chase step default") &&
         expect(near(config.visionHalfAngleDegrees, 60.0F), "vision default") &&
         expect(near(config.verticalHalfAngleDegrees, 30.0F), "vertical default") &&
         expect(near(config.guardEyeHeightMeters, 1.6F), "guard eye default") &&
         expect(near(config.targetStandEyeHeightMeters, 1.6F),
                "target stand eye default") &&
         expect(near(config.targetSneakEyeHeightMeters, 0.9F),
                "target sneak eye default") &&
         expect(near(config.occlusionMarginMeters, 0.05F),
                "occlusion margin default") &&
         expect(config.attackDamage == 1, "damage default") &&
         expect(config.decisionIntervalTicks == 1U, "decision interval default") &&
         expect(config.attackCooldownTicks == 2U, "cooldown default");
}

iggy3d::NpcBehaviorProfileResolveResult resolveBuiltIn(std::string_view id) {
  const iggy3d::NpcBehaviorProfileCatalog catalog =
      iggy3d::makeBuiltInNpcBehaviorProfileCatalog();
  return iggy3d::resolveNpcBehaviorProfile({&catalog, id});
}

bool builtInDefaultProfileResolvesToV01Defaults() {
  const iggy3d::NpcBehaviorProfileResolveResult result = resolveBuiltIn("default");
  return expect(result.ok, "default resolves") &&
         expect(result.status == iggy3d::NpcBehaviorProfileResolveStatus::Resolved,
                "default status") &&
         expect(result.reasonCode == "profile_resolved", "default reason") &&
         expect(result.profile.id.value == "default", "default id") &&
         expect(iggy3d::isValidNpcBehaviorConfig(result.config), "default valid") &&
         expectDefaultConfig(result.config, iggy3d::NpcEngagementPolicy::Hostile);
}

bool meleeTrainingProfileResolvesDeterministically() {
  const iggy3d::NpcBehaviorProfileResolveResult result =
      resolveBuiltIn("melee_training");
  return expect(result.ok, "melee resolves") &&
         expect(result.profile.id.value == "melee_training", "melee id") &&
         expect(iggy3d::isValidNpcBehaviorConfig(result.config), "melee valid") &&
         expectDefaultConfig(result.config, iggy3d::NpcEngagementPolicy::Hostile);
}

bool passiveProfileResolvesToExplicitPassivePolicy() {
  const iggy3d::NpcBehaviorProfileResolveResult result = resolveBuiltIn("passive");
  return expect(result.ok, "passive resolves") &&
         expect(result.profile.id.value == "passive", "passive id") &&
         expect(iggy3d::isValidNpcBehaviorConfig(result.config), "passive valid") &&
         expectDefaultConfig(result.config, iggy3d::NpcEngagementPolicy::Passive);
}

bool missingInvalidIdAndEmptyCatalogRejectDeterministically() {
  const iggy3d::NpcBehaviorProfileCatalog catalog =
      iggy3d::makeBuiltInNpcBehaviorProfileCatalog();
  const iggy3d::NpcBehaviorProfileResolveResult missing =
      iggy3d::resolveNpcBehaviorProfile({&catalog, "missing"});
  const iggy3d::NpcBehaviorProfileResolveResult invalid =
      iggy3d::resolveNpcBehaviorProfile({&catalog, "Bad-Id"});
  const iggy3d::NpcBehaviorProfileCatalog empty;
  const iggy3d::NpcBehaviorProfileResolveResult emptyResult =
      iggy3d::resolveNpcBehaviorProfile({&empty, "default"});

  return expect(!missing.ok, "missing rejects") &&
         expect(missing.status == iggy3d::NpcBehaviorProfileResolveStatus::Missing,
                "missing status") &&
         expect(missing.reasonCode == "profile_missing", "missing reason") &&
         expect(!invalid.ok, "invalid id rejects") &&
         expect(invalid.status == iggy3d::NpcBehaviorProfileResolveStatus::InvalidId,
                "invalid id status") &&
         expect(invalid.reasonCode == "profile_invalid_id", "invalid id reason") &&
         expect(!emptyResult.ok, "empty catalog rejects") &&
         expect(emptyResult.status ==
                    iggy3d::NpcBehaviorProfileResolveStatus::CatalogEmpty,
                "empty catalog status") &&
         expect(emptyResult.reasonCode == "profile_catalog_empty", "empty reason");
}

bool invalidProfileNumbersRejectDeterministically() {
  iggy3d::NpcBehaviorProfileCatalog catalog;
  iggy3d::NpcBehaviorProfile profile;
  profile.id.value = "bad";

  auto reject = [&](auto mutate, std::string_view message) {
    iggy3d::NpcBehaviorProfile invalid = profile;
    mutate(invalid);
    catalog.profiles = {invalid};
    const iggy3d::NpcBehaviorProfileResolveResult result =
        iggy3d::resolveNpcBehaviorProfile({&catalog, "bad"});
    return expect(!result.ok, message) &&
           expect(result.status == iggy3d::NpcBehaviorProfileResolveStatus::InvalidConfig,
                  "invalid config status") &&
           expect(result.reasonCode == "profile_invalid_config", "invalid config reason");
  };

  bool ok = reject([](iggy3d::NpcBehaviorProfile& p) {
                     p.perceptionRadiusMeters = 0.0F;
                   },
                   "zero perception rejects");
  ok = ok && reject([](iggy3d::NpcBehaviorProfile& p) {
                     p.chaseStopDistanceMeters = -1.0F;
                   },
                   "negative stop rejects");
  ok = ok && reject([](iggy3d::NpcBehaviorProfile& p) {
                     p.attackRangeMeters =
                         std::numeric_limits<float>::infinity();
                   },
                   "infinite attack range rejects");
  ok = ok && reject([](iggy3d::NpcBehaviorProfile& p) {
                     p.chaseStepMeters = 0.0F;
                   },
                   "zero chase step rejects");
  ok = ok && reject([](iggy3d::NpcBehaviorProfile& p) {
                     p.attackDamage = 0;
                   },
                   "zero damage rejects");
  ok = ok && reject([](iggy3d::NpcBehaviorProfile& p) {
                     p.decisionIntervalTicks = 0;
                   },
                   "zero decision interval rejects");
  ok = ok && reject([](iggy3d::NpcBehaviorProfile& p) {
                     p.engagementPolicy =
                         static_cast<iggy3d::NpcEngagementPolicy>(255);
                   },
                   "invalid policy rejects");
  return ok;
}

bool catalogOrderingAndNamesAreDeterministic() {
  const iggy3d::NpcBehaviorProfileCatalog catalog =
      iggy3d::makeBuiltInNpcBehaviorProfileCatalog();
  return expect(catalog.profiles.size() == 3U, "built-in profile count") &&
         expect(catalog.profiles[0].id.value == "default", "default first") &&
         expect(catalog.profiles[1].id.value == "melee_training", "melee second") &&
         expect(catalog.profiles[2].id.value == "passive", "passive third") &&
         expect(iggy3d::npcBehaviorProfileResolveStatusName(
                    iggy3d::NpcBehaviorProfileResolveStatus::Resolved) ==
                    "profile_resolved",
                "resolved lower snake") &&
         expect(iggy3d::npcBehaviorProfileResolveStatusName(
                    iggy3d::NpcBehaviorProfileResolveStatus::CatalogEmpty) ==
                    "profile_catalog_empty",
                "empty lower snake") &&
         expect(iggy3d::npcEngagementPolicyName(
                    iggy3d::NpcEngagementPolicy::Passive) == "passive",
                "passive policy name");
}

bool defaultConstructedProfileCarriesPerceptionConfigDefaults() {
  const iggy3d::NpcBehaviorConfig config =
      iggy3d::configFromNpcBehaviorProfile(iggy3d::NpcBehaviorProfile{});
  return expectDefaultConfig(config, iggy3d::NpcEngagementPolicy::Hostile);
}

bool profilePerceptionConfigFieldsThreadThroughDerivation() {
  iggy3d::NpcBehaviorProfile profile;
  profile.visionHalfAngleDegrees = 30.0F;
  profile.verticalHalfAngleDegrees = 45.0F;
  profile.guardEyeHeightMeters = 1.75F;
  profile.targetStandEyeHeightMeters = 1.65F;
  profile.targetSneakEyeHeightMeters = 0.8F;
  profile.occlusionMarginMeters = 0.125F;

  const iggy3d::NpcBehaviorConfig config =
      iggy3d::configFromNpcBehaviorProfile(profile);

  return expect(near(config.visionHalfAngleDegrees, 30.0F),
                "vision half-angle threaded") &&
         expect(near(config.verticalHalfAngleDegrees, 45.0F),
                "vertical half-angle threaded") &&
         expect(near(config.guardEyeHeightMeters, 1.75F),
                "guard eye threaded") &&
         expect(near(config.targetStandEyeHeightMeters, 1.65F),
                "target stand eye threaded") &&
         expect(near(config.targetSneakEyeHeightMeters, 0.8F),
                "target sneak eye threaded") &&
         expect(near(config.occlusionMarginMeters, 0.125F),
                "occlusion margin threaded");
}

}  // namespace

int main() {
  const bool ok = builtInDefaultProfileResolvesToV01Defaults() &&
                  meleeTrainingProfileResolvesDeterministically() &&
                  passiveProfileResolvesToExplicitPassivePolicy() &&
                  missingInvalidIdAndEmptyCatalogRejectDeterministically() &&
                  invalidProfileNumbersRejectDeterministically() &&
                  catalogOrderingAndNamesAreDeterministic() &&
                  defaultConstructedProfileCarriesPerceptionConfigDefaults() &&
                  profilePerceptionConfigFieldsThreadThroughDerivation();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
