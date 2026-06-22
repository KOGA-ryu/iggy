#include "content/PackageLoader.hpp"
#include "content/PackageValidator.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

std::string validPackage() {
  return R"([package]
id = "iggy3d.first_room"
schema_version = 1
required_runtime_schema = 1
scenario = "scenario.iggy3d.toml"
)";
}

std::string validScenario() {
  return R"([scenario]
id = "first_room.runtime_loop"

[defaults]
fixed_tick_rate_hz = 20
interaction_range_meters = 1.500
movement_distance_meters = 3.000
slow_time_scale = 0.250
initial_clock = "Normal"
default_realtime_camera = "ThirdPerson"
default_tactical_camera = "TacticalOverhead"

[[players]]
slot = 0
kind = "Local"
actor = "player"

[[entities]]
stable_name = "player"
kind = "Player"
active = true
persistent = true
position = [0.000, 0.000, 0.000]
bounds_min = [-0.250, 0.000, -0.250]
bounds_max = [0.250, 1.800, 0.250]
targetable = false
target_actions = []
combatant = true
faction_id = 1
hit_points = 10
max_hit_points = 10

[[entities]]
stable_name = "gold_key"
kind = "Pickup"
active = true
persistent = true
position = [3.000, 0.000, 0.000]
bounds_min = [-0.100, 0.000, -0.100]
bounds_max = [0.100, 0.100, 0.100]
targetable = true
target_actions = ["Interact", "Inspect"]
item_id = "gold_key"
item_count = 1
interaction = "Pickup"
deactivate_on_success = true
objective_ref = "collect_gold_key"

[[entities]]
stable_name = "tactical_marker_alpha"
kind = "Marker"
active = true
persistent = true
position = [2.000, 0.000, 1.000]
bounds_min = [-0.100, 0.000, -0.100]
bounds_max = [0.100, 0.100, 0.100]
targetable = true
target_actions = ["Move", "Inspect"]

[[entities]]
stable_name = "training_dummy"
kind = "Npc"
active = true
persistent = true
position = [2.000, 0.000, 2.000]
bounds_min = [-0.250, 0.000, -0.250]
bounds_max = [0.250, 1.200, 0.250]
targetable = true
target_actions = ["Attack", "Inspect"]
combatant = true
faction_id = 2
hit_points = 3
max_hit_points = 3

[[objectives]]
id = "collect_gold_key"
initial_status = "Active"
condition = "InventoryContains"
player_slot = 0
item_id = "gold_key"
item_count = 1
complete_status = "Complete"
)";
}

std::string replaceFirst(std::string text, std::string_view from, std::string_view to) {
  const std::size_t pos = text.find(from);
  if (pos != std::string::npos) {
    text.replace(pos, from.size(), to);
  }
  return text;
}

std::string eraseFirst(std::string text, std::string_view value) {
  const std::size_t pos = text.find(value);
  if (pos != std::string::npos) {
    text.erase(pos, value.size());
  }
  return text;
}

iggy3d::PackageLoadResult validLoad() {
  return iggy3d::parsePackageText(validPackage(), validScenario(), "fixtures/demos/first_room");
}

bool parseValidPackageAndScenarioText() {
  const iggy3d::PackageLoadResult load = validLoad();
  return expect(load.status == iggy3d::PackageLoadStatus::Ok, "valid load status") &&
         expect(load.manifest.packageId == "iggy3d.first_room", "manifest id") &&
         expect(load.manifest.scenarioPath == "scenario.iggy3d.toml", "scenario path") &&
         expect(load.scenario.scenarioId == "first_room.runtime_loop", "scenario id") &&
         expect(load.scenario.entities.size() == 4U, "entity count") &&
         expect(load.scenario.objectives.size() == 1U, "objective count");
}

bool scenarioSeedContainsDefaults() {
  const iggy3d::FixtureScenarioSeed seed = validLoad().scenario;
  return expect(seed.config.fixedTickRateHz == 20U, "tick rate") &&
         expect(seed.config.interactionRangeMeters == 1.500F, "range") &&
         expect(seed.config.movementDistanceMeters == 3.000F, "movement") &&
         expect(seed.config.slowTimeScale == 0.250F, "slow scale") &&
         expect(seed.initialClockMode == iggy3d::ClockMode::Normal, "initial clock") &&
         expect(seed.defaultRealtimeCamera == iggy3d::CameraMode::ThirdPerson, "realtime camera") &&
         expect(seed.defaultTacticalCamera == iggy3d::CameraMode::TacticalOverhead, "tactical camera");
}

bool scenarioSeedContainsEntities() {
  const iggy3d::FixtureScenarioSeed seed = validLoad().scenario;
  const iggy3d::ScenarioEntitySeed& player = seed.entities[0];
  const iggy3d::ScenarioEntitySeed& gold = seed.entities[1];
  const iggy3d::ScenarioEntitySeed& marker = seed.entities[2];
  const iggy3d::ScenarioEntitySeed& dummy = seed.entities[3];
  return expect(player.stableName == "player" && player.kind == iggy3d::EntityKind::Player,
                "player entity") &&
         expect(iggy3d::nearlyEqual(player.transform.position, iggy3d::Vec3{0.0F, 0.0F, 0.0F}),
                "player position") &&
         expect(player.combatantEnabled && player.combatant.factionId == 1U &&
                    player.combatant.hitPoints == 10 && player.combatant.maxHitPoints == 10,
                "player combatant") &&
         expect(gold.stableName == "gold_key" && gold.kind == iggy3d::EntityKind::Pickup,
                "gold entity") &&
         expect(iggy3d::nearlyEqual(gold.transform.position, iggy3d::Vec3{3.0F, 0.0F, 0.0F}),
                "gold position") &&
         expect(iggy3d::isTargetActionSupported(gold.targeting, iggy3d::TargetAction::Interact),
                "gold interact") &&
         expect(iggy3d::isTargetActionSupported(gold.targeting, iggy3d::TargetAction::Inspect),
                "gold inspect") &&
         expect(gold.interaction.kind == iggy3d::InteractionKind::Pickup, "pickup kind") &&
         expect(gold.interaction.primaryEffect == iggy3d::InteractionEffectKind::AddItemToInventory,
                "pickup effect") &&
         expect(gold.interaction.itemId == "gold_key" && gold.interaction.itemCount == 1U,
                "pickup item") &&
         expect(gold.interaction.objectiveId == "collect_gold_key", "pickup objective") &&
         expect(!gold.interaction.repeatable && gold.interaction.deactivateTargetOnSuccess,
                "pickup flags") &&
         expect(marker.stableName == "tactical_marker_alpha" &&
                    iggy3d::isTargetActionSupported(marker.targeting, iggy3d::TargetAction::Move) &&
                    marker.interaction.itemId.empty(),
                "marker entity") &&
         expect(dummy.stableName == "training_dummy" &&
                    dummy.kind == iggy3d::EntityKind::Npc &&
                    iggy3d::isTargetActionSupported(dummy.targeting, iggy3d::TargetAction::Attack) &&
                    iggy3d::isTargetActionSupported(dummy.targeting, iggy3d::TargetAction::Inspect),
                "dummy attack target") &&
         expect(dummy.combatantEnabled && dummy.combatant.factionId == 2U &&
                    dummy.combatant.hitPoints == 3 && dummy.combatant.maxHitPoints == 3,
                "dummy combatant");
}

bool validatorAcceptsAndRejects() {
  const iggy3d::PackageLoadResult load = validLoad();
  iggy3d::PackageValidationRequest request{load.manifest, load.scenario};
  bool ok = expect(iggy3d::validatePackage(request).status == iggy3d::PackageValidationStatus::Ok,
                   "validator accepts");
  request.manifest.packageId.clear();
  auto result = iggy3d::validatePackage(request);
  ok = ok && expect(result.status == iggy3d::PackageValidationStatus::MissingPackageId,
                   "missing package id") &&
       expect(!result.diagnostics.empty() && result.diagnostics[0].code == "package.missing_id",
              "missing package id code");
  request = {load.manifest, load.scenario};
  request.scenario.scenarioId.clear();
  ok = ok && expect(iggy3d::validatePackage(request).status ==
                        iggy3d::PackageValidationStatus::MissingScenarioId,
                    "missing scenario id");
  request = {load.manifest, load.scenario};
  request.scenario.entities[2].stableName = "gold_key";
  result = iggy3d::validatePackage(request);
  ok = ok && expect(result.status == iggy3d::PackageValidationStatus::DuplicateStableName,
                   "duplicate stable name") &&
       expect(result.diagnostics[0].code == "scenario.duplicate_stable_name", "duplicate code") &&
       expect(result.diagnostics[0].message.find("gold_key") != std::string::npos, "duplicate message");
  request = {load.manifest, load.scenario};
  request.scenario.entities[2].stableName.clear();
  ok = ok && expect(iggy3d::validatePackage(request).status ==
                        iggy3d::PackageValidationStatus::MissingStableName,
                    "empty stable name");
  request = {load.manifest, load.scenario};
  request.scenario.players.clear();
  ok = ok && expect(iggy3d::validatePackage(request).status ==
                        iggy3d::PackageValidationStatus::MissingPlayerBinding,
                    "missing player binding");
  request = {load.manifest, load.scenario};
  request.scenario.entities[0].transform.position.x = std::numeric_limits<float>::infinity();
  ok = ok && expect(iggy3d::validatePackage(request).status ==
                        iggy3d::PackageValidationStatus::NonFiniteTransform,
                    "non finite transform");
  request = {load.manifest, load.scenario};
  request.scenario.entities[0].localBounds.min.x = 2.0F;
  request.scenario.entities[0].localBounds.max.x = 1.0F;
  ok = ok && expect(iggy3d::validatePackage(request).status ==
                        iggy3d::PackageValidationStatus::InvalidBounds,
                    "invalid bounds");
  request = {load.manifest, load.scenario};
  request.scenario.entities[1].interaction.objectiveId.clear();
  ok = ok && expect(iggy3d::validatePackage(request).status ==
                        iggy3d::PackageValidationStatus::MissingGoldKeyInteraction,
                    "missing gold interaction");
  request = {load.manifest, load.scenario};
  request.scenario.objectives[0].itemId = "silver_key";
  ok = ok && expect(iggy3d::validatePackage(request).status ==
                        iggy3d::PackageValidationStatus::InvalidObjectiveCondition,
                    "invalid objective");
  request = {load.manifest, load.scenario};
  request.manifest.assets.push_back({"debug", "assets/old_iggy_dependency.mesh"});
  ok = ok && expect(iggy3d::validatePackage(request).status ==
                        iggy3d::PackageValidationStatus::OldIggyDependency,
                    "old dependency");
  return ok;
}

bool loaderRejectsInvalidInputs() {
  bool ok = true;
  auto result = iggy3d::parsePackageText(
      replaceFirst(validPackage(), "scenario.iggy3d.toml", std::string{"/Users/kogaryu/"} + "iggy/foo.toml"),
      validScenario(), "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::InvalidPath, "old path");
  result = iggy3d::parsePackageText(validPackage() + "extra = true\n", validScenario(),
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::UnsupportedKey, "unsupported package key");
  result = iggy3d::parsePackageText(validPackage(), validScenario() + "extra = true\n",
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::UnsupportedKey, "unsupported scenario key");
  result = iggy3d::parsePackageText(replaceFirst(validPackage(), "scenario.iggy3d.toml", "../scenario.iggy3d.toml"),
                                    validScenario(), "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::InvalidPath, "invalid path");
  result = iggy3d::parsePackageText(validPackage(), eraseFirst(validScenario(), "bounds_min = [-0.100, 0.000, -0.100]\n"),
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::MissingRequiredKey, "missing required");
  result = iggy3d::parsePackageText(validPackage(), eraseFirst(validScenario(), "objective_ref = \"collect_gold_key\"\n"),
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::MissingRequiredKey,
                   "missing objective_ref");
  result = iggy3d::parsePackageText(validPackage(), replaceFirst(validScenario(), "interaction = \"Pickup\"", "interaction = \"InspectOnly\""),
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::InvalidEnum, "inspect only");
  result = iggy3d::parsePackageText(validPackage(), replaceFirst(validScenario(), "initial_clock = \"Normal\"", "initial_clock = \"Paused\""),
                                    "inline/scenario_invalid_initial_clock.iggy3d.toml");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::InvalidEnum, "paused initial clock") &&
       expect(!result.diagnostics.empty() && result.diagnostics[0].domain == iggy3d::DiagnosticDomain::Content,
              "clock diagnostic domain") &&
       expect(result.diagnostics[0].code == "scenario.invalid_enum", "clock diagnostic code") &&
       expect(result.diagnostics[0].location.file == "inline/scenario_invalid_initial_clock.iggy3d.toml",
              "clock diagnostic file") &&
       expect(result.diagnostics[0].location.line == 9U, "clock diagnostic line") &&
       expect(result.diagnostics[0].location.column == 1U, "clock diagnostic column") &&
       expect(result.diagnostics[0].message.find("Paused") != std::string::npos &&
                  result.diagnostics[0].message.find("Normal") != std::string::npos,
              "clock diagnostic message");
  result = iggy3d::parsePackageText(validPackage(), replaceFirst(validScenario(), "movement_distance_meters = 3.000", "movement_distance_meters = \"far\""),
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::InvalidNumber, "invalid number");
  result = iggy3d::parsePackageText(validPackage(), eraseFirst(validScenario(), "id = \"first_room.runtime_loop\"\n"),
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::MissingScenarioId, "missing scenario id");
  result = iggy3d::parsePackageText(validPackage(),
                                    replaceFirst(validScenario(), "target_actions = [\"Attack\", \"Inspect\"]",
                                                 "target_actions = [\"AttackOnly\"]"),
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::InvalidEnum, "invalid attack action");
  result = iggy3d::parsePackageText(validPackage(),
                                    eraseFirst(validScenario(), "combatant = true\n"),
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::MissingRequiredKey,
                   "stray combat field");
  result = iggy3d::parsePackageText(validPackage(),
                                    replaceFirst(validScenario(), "hit_points = 3", "hit_points = -1"),
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::InvalidNumber, "negative hp");
  result = iggy3d::parsePackageText(validPackage(),
                                    replaceFirst(validScenario(), "max_hit_points = 3", "max_hit_points = 0"),
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::InvalidNumber, "zero max hp");
  result = iggy3d::parsePackageText(validPackage(),
                                    replaceFirst(validScenario(), "hit_points = 3", "hit_points = 4"),
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::InvalidNumber, "hp over max");
  result = iggy3d::parsePackageText(validPackage(),
                                    replaceFirst(validScenario(), "hit_points = 3", "hit_points = 0"),
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::InvalidNumber, "starting defeated");
  return ok;
}

bool loadPackageSmoke() {
  const iggy3d::PackageLoadResult result =
      iggy3d::loadPackage({"fixtures/demos/first_room/package.iggy3d.toml"});
  return expect(result.status == iggy3d::PackageLoadStatus::Ok, "loadPackage smoke");
}

}  // namespace

int main() {
  const bool ok = parseValidPackageAndScenarioText() && scenarioSeedContainsDefaults() &&
                  scenarioSeedContainsEntities() && validatorAcceptsAndRejects() &&
                  loaderRejectsInvalidInputs() && loadPackageSmoke();
  return ok ? 0 : 1;
}
