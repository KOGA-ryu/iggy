// MA4 s1 GATE 1 -- capability classes as DATA: the class->cost-row mapping + the unusable-edge skip
// predicate, the string parse, the loader `capability` column (fail-closed), and the load-carry (the
// class rides the profile catalog, no saved/hashed field).

#include "content/FixtureScenarioLoader.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/ai/ReasoningRoute.hpp"
#include "runtime/movement/MovementCapability.hpp"
#include "runtime/session/Session.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::size_t climbIndex() {
  return static_cast<std::size_t>(iggy3d::ReasoningEdgeKind::climb);
}

bool hasDiagnosticCode(const iggy3d::ScenarioLoadResult& result, std::string_view code) {
  for (const iggy3d::Diagnostic& diagnostic : result.diagnostics) {
    if (diagnostic.code == code) {
      return true;
    }
  }
  return false;
}

// A minimal warden scenario with an optional `capability` line injected into the profile block.
std::string scenarioWithCapability(std::string_view capabilityLine) {
  return std::string(
             "[scenario]\n"
             "id = \"cap_test\"\n"
             "[defaults]\n"
             "fixed_tick_rate_hz = 20\n"
             "interaction_range_meters = 1.500\n"
             "movement_distance_meters = 3.000\n"
             "slow_time_scale = 0.250\n"
             "initial_clock = \"Normal\"\n"
             "default_realtime_camera = \"ThirdPerson\"\n"
             "default_tactical_camera = \"TacticalOverhead\"\n"
             "[[players]]\n"
             "slot = 0\n"
             "kind = \"Local\"\n"
             "actor = \"player\"\n"
             "[[entities]]\n"
             "stable_name = \"player\"\n"
             "kind = \"Player\"\n"
             "active = true\n"
             "persistent = true\n"
             "position = [1.000, 0.000, 1.000]\n"
             "bounds_min = [-0.250, 0.000, -0.250]\n"
             "bounds_max = [0.250, 1.800, 0.250]\n"
             "targetable = false\n"
             "target_actions = []\n"
             "combatant = true\n"
             "faction_id = 1\n"
             "hit_points = 10\n"
             "max_hit_points = 10\n"
             "[[entities]]\n"
             "stable_name = \"guard\"\n"
             "kind = \"Npc\"\n"
             "active = true\n"
             "persistent = true\n"
             "position = [9.000, 0.000, 9.000]\n"
             "bounds_min = [-0.250, 0.000, -0.250]\n"
             "bounds_max = [0.250, 1.200, 0.250]\n"
             "targetable = true\n"
             "target_actions = [\"Attack\", \"Inspect\"]\n"
             "combatant = true\n"
             "faction_id = 2\n"
             "hit_points = 3\n"
             "max_hit_points = 3\n"
             "[[ai_actors]]\n"
             "actor = \"guard\"\n"
             "behavior_profile_id = \"scaler\"\n"
             "[[behavior_profiles]]\n"
             "id = \"scaler\"\n"
             "engagement_policy = \"hostile\"\n") +
         std::string(capabilityLine) +
         "perception_radius_meters = 7.500\n"
         "[[objectives]]\n"
         "id = \"guard_the_room\"\n"
         "initial_status = \"Active\"\n"
         "condition = \"InventoryContains\"\n"
         "player_slot = 0\n"
         "item_id = \"phantom_key\"\n"
         "item_count = 1\n"
         "complete_status = \"Complete\"\n";
}

bool capabilityCostRowsAndSkipPredicate() {
  const iggy3d::TravelCostConfig grounded =
      iggy3d::travelCostConfigForCapability(iggy3d::MovementCapabilityClass::grounded);
  const iggy3d::TravelCostConfig climber =
      iggy3d::travelCostConfigForCapability(iggy3d::MovementCapabilityClass::climber);
  const iggy3d::TravelCostConfig leaper =
      iggy3d::travelCostConfigForCapability(iggy3d::MovementCapabilityClass::leaper);
  const iggy3d::TravelCostConfig flier =
      iggy3d::travelCostConfigForCapability(iggy3d::MovementCapabilityClass::flier);

  bool ok =
      expect(grounded.edgeKindMultipliers[climbIndex()] == iggy3d::kUnusableEdgeKindMultiplier,
             "grounded marks climb UNUSABLE (the sentinel)") &&
      expect(climber.edgeKindMultipliers[climbIndex()] == 1.0F,
             "climber leaves climb usable (multiplier 1.0)") &&
      expect(leaper.edgeKindMultipliers[climbIndex()] == iggy3d::kUnusableEdgeKindMultiplier &&
                 flier.edgeKindMultipliers[climbIndex()] == iggy3d::kUnusableEdgeKindMultiplier,
             "reserved leaper/flier fall through to grounded (no v1 behavior)");

  // The skip predicate: grounded skips climb but not walkable; climber + the default {} traverse all.
  ok = ok && expect(!iggy3d::isTraversableEdgeKind(grounded, iggy3d::ReasoningEdgeKind::climb),
                    "grounded SKIPS climb edges") &&
       expect(iggy3d::isTraversableEdgeKind(grounded, iggy3d::ReasoningEdgeKind::walkable),
              "grounded still walks walkable edges") &&
       expect(iggy3d::isTraversableEdgeKind(climber, iggy3d::ReasoningEdgeKind::climb),
              "climber traverses climb edges") &&
       expect(iggy3d::isTraversableEdgeKind(iggy3d::TravelCostConfig{},
                                            iggy3d::ReasoningEdgeKind::climb),
              "the default config traverses every kind (pre-MA4 behavior preserved)");
  return ok;
}

bool capabilityStringMapping() {
  iggy3d::MovementCapabilityClass parsed = iggy3d::MovementCapabilityClass::grounded;
  bool ok = expect(iggy3d::movementCapabilityClassFromString("climber", parsed) &&
                       parsed == iggy3d::MovementCapabilityClass::climber,
                   "\"climber\" parses to climber") &&
            expect(!iggy3d::movementCapabilityClassFromString("birdperson", parsed),
                   "an unknown class fails closed") &&
            expect(iggy3d::movementCapabilityClassName(iggy3d::MovementCapabilityClass::grounded) ==
                       "grounded",
                   "grounded names round-trip");
  return ok;
}

bool loaderParsesCapabilityFailClosed() {
  // Explicit climber.
  const iggy3d::ScenarioLoadResult climber =
      iggy3d::parseScenarioText(scenarioWithCapability("capability = \"climber\"\n"));
  bool ok = expect(climber.status == iggy3d::ScenarioLoadStatus::Ok, "climber scenario parses") &&
            expect(!climber.seed.behaviorProfiles.empty() &&
                       climber.seed.behaviorProfiles.back().capability ==
                           iggy3d::MovementCapabilityClass::climber,
                   "the profile carries capability climber");

  // Absent ⇒ grounded (every existing profile unchanged).
  const iggy3d::ScenarioLoadResult absent =
      iggy3d::parseScenarioText(scenarioWithCapability(""));
  ok = ok && expect(absent.status == iggy3d::ScenarioLoadStatus::Ok, "no-capability scenario parses") &&
       expect(!absent.seed.behaviorProfiles.empty() &&
                  absent.seed.behaviorProfiles.back().capability ==
                      iggy3d::MovementCapabilityClass::grounded,
              "absent capability defaults to grounded");

  // Unknown ⇒ fail-closed invalid_enum.
  const iggy3d::ScenarioLoadResult unknown =
      iggy3d::parseScenarioText(scenarioWithCapability("capability = \"teleporter\"\n"));
  ok = ok && expect(unknown.status != iggy3d::ScenarioLoadStatus::Ok,
                    "an unknown capability is rejected") &&
       expect(hasDiagnosticCode(unknown, "scenario.invalid_enum"),
              "unknown capability yields scenario.invalid_enum");
  return ok;
}

bool capabilityCarriesAcrossLoad() {
  const iggy3d::ScenarioLoadResult parsed =
      iggy3d::parseScenarioText(scenarioWithCapability("capability = \"climber\"\n"));
  iggy3d::SessionCreateRequest request;
  request.packageId = "iggy3d.cap_test";
  request.config = parsed.seed.config;
  request.seed = parsed.seed;
  iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(request);
  if (!expect(created.status == iggy3d::ResultStatus::Ok, "climber session creates")) {
    return false;
  }
  iggy3d::Session session = std::move(created.value);

  const auto climberInCatalog = [&session]() {
    const iggy3d::NpcBehaviorProfileResolveResult resolved =
        iggy3d::resolveNpcBehaviorProfile({&session.state().behaviorProfileCatalog, "scaler"});
    return resolved.ok && resolved.profile.capability == iggy3d::MovementCapabilityClass::climber;
  };
  bool ok = expect(climberInCatalog(), "the session catalog carries climber");

  // Model a load whose catalog is EMPTY (never serialized); replaceStateFromLoad must CARRY the live
  // climber catalog -- the class rides the profile, no saved/hashed field.
  iggy3d::SessionState loaded = session.state();
  loaded.behaviorProfileCatalog = {};
  const iggy3d::SessionLoadResult load = session.replaceStateFromLoad(std::move(loaded));
  ok = ok && expect(load.status == iggy3d::SessionLoadStatus::Ok, "replaceStateFromLoad ok") &&
       expect(climberInCatalog(), "climber survives the load carry");
  return ok;
}

}  // namespace

int main() {
  const bool ok = capabilityCostRowsAndSkipPredicate() && capabilityStringMapping() &&
                  loaderParsesCapabilityFailClosed() && capabilityCarriesAcrossLoad();
  return ok ? 0 : 1;
}
