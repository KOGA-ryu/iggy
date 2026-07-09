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

std::string scenarioWithAiActor(std::string_view actor, std::string_view profileId) {
  std::string scenario = validScenario();
  scenario += "\n[[ai_actors]]\nactor = \"";
  scenario += actor;
  scenario += "\"\nbehavior_profile_id = \"";
  scenario += profileId;
  scenario += "\"\n";
  return scenario;
}

std::string scenarioWithGuardAnchor(std::string_view actor = "training_dummy",
                                    std::string_view anchor = "tactical_marker_alpha",
                                    std::string_view leash = "6.0",
                                    std::string_view returnRadius = "1.0",
                                    std::string_view tolerance = "0.25") {
  std::string scenario = validScenario();
  scenario += "\n[[ai_guard_anchors]]\nactor = \"";
  scenario += actor;
  scenario += "\"\nanchor = \"";
  scenario += anchor;
  scenario += "\"\nleash_radius_meters = ";
  scenario += leash;
  scenario += "\nreturn_radius_meters = ";
  scenario += returnRadius;
  scenario += "\nhome_tolerance_meters = ";
  scenario += tolerance;
  scenario += "\n";
  return scenario;
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
         expect(seed.initialClockMode == iggy3d::ScenarioClockMode::Normal, "initial clock") &&
         expect(seed.defaultRealtimeCamera == iggy3d::ScenarioCameraMode::ThirdPerson, "realtime camera") &&
         expect(seed.defaultTacticalCamera == iggy3d::ScenarioCameraMode::TacticalOverhead, "tactical camera");
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

bool scenarioSeedContainsAiActors() {
  const iggy3d::PackageLoadResult passive = iggy3d::parsePackageText(
      validPackage(), scenarioWithAiActor("training_dummy", "passive"),
      "fixtures/demos/first_room");
  const iggy3d::PackageLoadResult unknown = iggy3d::parsePackageText(
      validPackage(), scenarioWithAiActor("training_dummy", "ghost_profile"),
      "fixtures/demos/first_room");

  return expect(passive.status == iggy3d::PackageLoadStatus::Ok,
                "passive ai actor parses") &&
         expect(passive.scenario.aiActors.size() == 1U,
                "passive ai actor count") &&
         expect(passive.scenario.aiActors[0].actorStableName == "training_dummy",
                "passive ai actor stable name") &&
         expect(passive.scenario.aiActors[0].behaviorProfileId == "passive",
                "passive ai actor profile") &&
         expect(unknown.status == iggy3d::PackageLoadStatus::Ok,
                "unknown ai actor parses") &&
         expect(unknown.scenario.aiActors.size() == 1U,
                "unknown ai actor count") &&
         expect(unknown.scenario.aiActors[0].behaviorProfileId == "ghost_profile",
                "unknown ai actor profile preserved");
}

bool scenarioAiActorRejectsInvalidInputs() {
  std::string missingActor = validScenario();
  missingActor += "\n[[ai_actors]]\nbehavior_profile_id = \"passive\"\n";
  std::string missingProfile = validScenario();
  missingProfile += "\n[[ai_actors]]\nactor = \"training_dummy\"\n";
  std::string invalidProfile = scenarioWithAiActor("training_dummy", "Bad-Id");
  std::string unsupported = scenarioWithAiActor("training_dummy", "passive");
  unsupported += "extra = \"ignored\"\n";

  iggy3d::PackageLoadResult result =
      iggy3d::parsePackageText(validPackage(), missingActor, "fixtures/demos/first_room");
  bool ok = expect(result.status == iggy3d::PackageLoadStatus::MissingRequiredKey,
                   "missing ai actor actor rejects");
  result = iggy3d::parsePackageText(validPackage(), missingProfile,
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::MissingRequiredKey,
                    "missing ai actor profile rejects");
  result = iggy3d::parsePackageText(validPackage(), invalidProfile,
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::InvalidEnum,
                    "invalid ai actor profile rejects");
  result = iggy3d::parsePackageText(validPackage(), unsupported,
                                    "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::UnsupportedKey,
                    "unsupported ai actor key rejects");
  result = iggy3d::parsePackageText(
      validPackage(),
      replaceFirst(scenarioWithAiActor("training_dummy", "passive"),
                   "actor = \"training_dummy\"", "actor = training_dummy"),
      "fixtures/demos/first_room");
  ok = ok && expect(result.status == iggy3d::PackageLoadStatus::ParseError,
                    "invalid ai actor string rejects");
  return ok;
}

bool scenarioAiActorParsesPatrolRoute() {
  // Optional patrol keys: repeated `waypoint` builds an ordered route; `patrol_mode`
  // selects loop/ping_pong. An actor with no patrol keys yields an empty route.
  std::string routed = scenarioWithAiActor("training_dummy", "passive");
  routed += "waypoint = [1.0, 0.0, 2.0]\n";
  routed += "waypoint = [3.0, 0.0, 4.0]\n";
  routed += "patrol_mode = \"ping_pong\"\n";
  const iggy3d::ScenarioLoadResult route = iggy3d::parseScenarioText(routed);

  bool ok = expect(route.status == iggy3d::ScenarioLoadStatus::Ok, "patrol route parses") &&
            expect(route.seed.aiActors.size() == 1U, "patrol actor count") &&
            expect(route.seed.aiActors[0].patrolWaypoints.size() == 2U,
                   "patrol waypoint count") &&
            expect(route.seed.aiActors[0].patrolWaypoints[0].x == 1.0F &&
                       route.seed.aiActors[0].patrolWaypoints[0].z == 2.0F,
                   "first waypoint ordered") &&
            expect(route.seed.aiActors[0].patrolWaypoints[1].x == 3.0F &&
                       route.seed.aiActors[0].patrolWaypoints[1].z == 4.0F,
                   "second waypoint ordered") &&
            expect(route.seed.aiActors[0].patrolMode == iggy3d::ScenarioPatrolMode::PingPong,
                   "patrol mode ping_pong");

  // Bad patrol_mode value fails InvalidEnum.
  std::string badMode = scenarioWithAiActor("training_dummy", "passive");
  badMode += "patrol_mode = \"diagonal\"\n";
  const iggy3d::ScenarioLoadResult bad = iggy3d::parseScenarioText(badMode);
  ok = ok && expect(bad.status == iggy3d::ScenarioLoadStatus::InvalidEnum,
                    "invalid patrol mode rejects");

  // Bad waypoint value fails InvalidNumber.
  std::string badWaypoint = scenarioWithAiActor("training_dummy", "passive");
  badWaypoint += "waypoint = [1.0, 0.0]\n";  // wrong arity
  const iggy3d::ScenarioLoadResult badWp = iggy3d::parseScenarioText(badWaypoint);
  ok = ok && expect(badWp.status == iggy3d::ScenarioLoadStatus::InvalidNumber,
                    "invalid waypoint rejects");

  // Back-compat: no patrol keys -> empty route.
  const iggy3d::ScenarioLoadResult plain =
      iggy3d::parseScenarioText(scenarioWithAiActor("training_dummy", "passive"));
  ok = ok && expect(plain.status == iggy3d::ScenarioLoadStatus::Ok, "plain ai actor parses") &&
       expect(plain.seed.aiActors.size() == 1U && plain.seed.aiActors[0].patrolWaypoints.empty(),
              "no patrol keys yields empty route");
  return ok;
}


bool scenarioSeedContainsAiGuardAnchors() {
  const iggy3d::ScenarioLoadResult result =
      iggy3d::parseScenarioText(scenarioWithGuardAnchor());
  const iggy3d::ScenarioLoadResult glyphLike =
      iggy3d::parseScenarioText(scenarioWithGuardAnchor("N", "G"));

  return expect(result.status == iggy3d::ScenarioLoadStatus::Ok,
                "guard anchor parses") &&
         expect(result.seed.aiGuardAnchors.size() == 1U,
                "guard anchor count") &&
         expect(result.seed.aiGuardAnchors[0].actorStableName == "training_dummy",
                "guard actor stable name") &&
         expect(result.seed.aiGuardAnchors[0].anchorStableName == "tactical_marker_alpha",
                "guard anchor stable name") &&
         expect(result.seed.aiGuardAnchors[0].leashRadiusMeters == 6.0F,
                "guard leash parsed") &&
         expect(result.seed.aiGuardAnchors[0].returnRadiusMeters == 1.0F,
                "guard return parsed") &&
         expect(result.seed.aiGuardAnchors[0].homeToleranceMeters == 0.25F,
                "guard tolerance parsed") &&
         expect(glyphLike.status == iggy3d::ScenarioLoadStatus::Ok,
                "glyph-like guard names parse") &&
         expect(glyphLike.seed.aiGuardAnchors.size() == 1U,
                "glyph-like guard count") &&
         expect(glyphLike.seed.aiGuardAnchors[0].actorStableName == "N" &&
                    glyphLike.seed.aiGuardAnchors[0].anchorStableName == "G",
                "glyph-like guard strings preserved");
}

bool scenarioAiGuardAnchorRejectsInvalidInputs() {
  const std::string valid = scenarioWithGuardAnchor();
  bool ok = true;
  auto result = iggy3d::parseScenarioText(
      eraseFirst(valid, "actor = \"training_dummy\"\n"));
  ok = ok && expect(result.status == iggy3d::ScenarioLoadStatus::MissingRequiredKey,
                    "missing guard actor rejects");
  result = iggy3d::parseScenarioText(
      eraseFirst(valid, "anchor = \"tactical_marker_alpha\"\n"));
  ok = ok && expect(result.status == iggy3d::ScenarioLoadStatus::MissingRequiredKey,
                    "missing guard anchor rejects");
  result = iggy3d::parseScenarioText(
      eraseFirst(valid, "leash_radius_meters = 6.0\n"));
  ok = ok && expect(result.status == iggy3d::ScenarioLoadStatus::MissingRequiredKey,
                    "missing guard leash rejects");
  result = iggy3d::parseScenarioText(
      eraseFirst(valid, "return_radius_meters = 1.0\n"));
  ok = ok && expect(result.status == iggy3d::ScenarioLoadStatus::MissingRequiredKey,
                    "missing guard return rejects");
  result = iggy3d::parseScenarioText(
      eraseFirst(valid, "home_tolerance_meters = 0.25\n"));
  ok = ok && expect(result.status == iggy3d::ScenarioLoadStatus::MissingRequiredKey,
                    "missing guard tolerance rejects");

  result = iggy3d::parseScenarioText(
      replaceFirst(valid, "actor = \"training_dummy\"", "actor = training_dummy"));
  ok = ok && expect(result.status == iggy3d::ScenarioLoadStatus::ParseError,
                    "invalid guard actor string rejects");
  result = iggy3d::parseScenarioText(
      replaceFirst(valid, "anchor = \"tactical_marker_alpha\"", "anchor = tactical_marker_alpha"));
  ok = ok && expect(result.status == iggy3d::ScenarioLoadStatus::ParseError,
                    "invalid guard anchor string rejects");

  result = iggy3d::parseScenarioText(
      replaceFirst(valid, "leash_radius_meters = 6.0", "leash_radius_meters = far"));
  ok = ok && expect(result.status == iggy3d::ScenarioLoadStatus::InvalidNumber,
                    "invalid guard leash number rejects");
  result = iggy3d::parseScenarioText(
      replaceFirst(valid, "return_radius_meters = 1.0", "return_radius_meters = near"));
  ok = ok && expect(result.status == iggy3d::ScenarioLoadStatus::InvalidNumber,
                    "invalid guard return number rejects");
  result = iggy3d::parseScenarioText(
      replaceFirst(valid, "home_tolerance_meters = 0.25", "home_tolerance_meters = maybe"));
  ok = ok && expect(result.status == iggy3d::ScenarioLoadStatus::InvalidNumber,
                    "invalid guard tolerance number rejects");

  std::string unsupported = valid;
  unsupported += "extra = true\n";
  result = iggy3d::parseScenarioText(unsupported);
  return ok && expect(result.status == iggy3d::ScenarioLoadStatus::UnsupportedKey,
                      "unsupported guard key rejects");
}

bool expectGuardValidation(iggy3d::PackageValidationRequest request,
                           iggy3d::PackageValidationStatus status,
                           std::string_view code,
                           std::string_view message) {
  const iggy3d::PackageValidationResult result = iggy3d::validatePackage(request);
  return expect(result.status == status, message) &&
         expect(!result.diagnostics.empty() && result.diagnostics[0].code == code,
                "guard validation diagnostic code");
}

bool validatorAcceptsAndRejectsAiGuardAnchors() {
  const iggy3d::PackageLoadResult load = validLoad();
  iggy3d::PackageValidationRequest request{load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "tactical_marker_alpha", 6.0F, 1.0F, 0.25F});
  bool ok = expect(iggy3d::validatePackage(request).status ==
                       iggy3d::PackageValidationStatus::Ok,
                   "valid guard anchor accepted");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back({"N", "G", 6.0F, 1.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::GuardActorNotFound,
                                    "scenario.guard_actor_not_found",
                                    "glyph-like guard actor rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"", "tactical_marker_alpha", 6.0F, 1.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::MissingGuardActor,
                                    "scenario.missing_guard_actor",
                                    "empty guard actor rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"missing_dummy", "tactical_marker_alpha", 6.0F, 1.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::GuardActorNotFound,
                                    "scenario.guard_actor_not_found",
                                    "missing guard actor rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"player", "tactical_marker_alpha", 6.0F, 1.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::NonNpcGuardActor,
                                    "scenario.non_npc_guard_actor",
                                    "non npc guard actor rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "tactical_marker_alpha", 6.0F, 1.0F, 0.25F});
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "tactical_marker_alpha", 5.0F, 1.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::DuplicateGuardActor,
                                    "scenario.duplicate_guard_actor",
                                    "duplicate guard actor rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back({"training_dummy", "", 6.0F, 1.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::MissingGuardAnchor,
                                    "scenario.missing_guard_anchor",
                                    "empty guard anchor rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "missing_marker", 6.0F, 1.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::GuardAnchorNotFound,
                                    "scenario.guard_anchor_not_found",
                                    "missing guard anchor rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back({"training_dummy", "gold_key", 6.0F, 1.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::NonMarkerGuardAnchor,
                                    "scenario.non_marker_guard_anchor",
                                    "non marker pickup guard anchor rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "training_dummy", 6.0F, 1.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::NonMarkerGuardAnchor,
                                    "scenario.non_marker_guard_anchor",
                                    "non marker npc guard anchor rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "tactical_marker_alpha", 0.0F, 1.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::InvalidGuardLeashRadius,
                                    "scenario.invalid_guard_leash_radius",
                                    "zero guard leash rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "tactical_marker_alpha", -1.0F, 1.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::InvalidGuardLeashRadius,
                                    "scenario.invalid_guard_leash_radius",
                                    "negative guard leash rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "tactical_marker_alpha",
       std::numeric_limits<float>::infinity(), 1.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::InvalidGuardLeashRadius,
                                    "scenario.invalid_guard_leash_radius",
                                    "nonfinite guard leash rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "tactical_marker_alpha", 6.0F, 0.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::InvalidGuardReturnRadius,
                                    "scenario.invalid_guard_return_radius",
                                    "zero guard return rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "tactical_marker_alpha", 6.0F, -1.0F, 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::InvalidGuardReturnRadius,
                                    "scenario.invalid_guard_return_radius",
                                    "negative guard return rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "tactical_marker_alpha", 6.0F,
       std::numeric_limits<float>::infinity(), 0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::InvalidGuardReturnRadius,
                                    "scenario.invalid_guard_return_radius",
                                    "nonfinite guard return rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "tactical_marker_alpha", 6.0F, 1.0F, -0.25F});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::InvalidGuardHomeTolerance,
                                    "scenario.invalid_guard_home_tolerance",
                                    "negative guard tolerance rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "tactical_marker_alpha", 6.0F, 1.0F,
       std::numeric_limits<float>::infinity()});
  ok = ok && expectGuardValidation(request,
                                    iggy3d::PackageValidationStatus::InvalidGuardHomeTolerance,
                                    "scenario.invalid_guard_home_tolerance",
                                    "nonfinite guard tolerance rejected");

  request = {load.manifest, load.scenario};
  request.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "tactical_marker_alpha", 1.0F, 2.0F, 0.25F});
  return ok && expectGuardValidation(request,
                                      iggy3d::PackageValidationStatus::GuardReturnExceedsLeash,
                                      "scenario.guard_return_exceeds_leash",
                                      "guard return exceeds leash rejected");
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
  request.scenario.aiActors.push_back({"training_dummy", "passive"});
  result = iggy3d::validatePackage(request);
  ok = ok && expect(result.status == iggy3d::PackageValidationStatus::Ok,
                    "valid ai actor accepted");
  request = {load.manifest, load.scenario};
  request.scenario.aiActors.push_back({"training_dummy", "ghost_profile"});
  result = iggy3d::validatePackage(request);
  ok = ok && expect(result.status == iggy3d::PackageValidationStatus::Ok,
                    "unknown ai profile accepted");
  request = {load.manifest, load.scenario};
  request.scenario.aiActors.push_back({"", "passive"});
  result = iggy3d::validatePackage(request);
  ok = ok && expect(result.status == iggy3d::PackageValidationStatus::MissingAiActorEntity,
                    "missing ai actor entity") &&
       expect(!result.diagnostics.empty() &&
                  result.diagnostics[0].code == "scenario.missing_ai_actor_entity",
              "missing ai actor code");
  request = {load.manifest, load.scenario};
  request.scenario.aiActors.push_back({"missing_dummy", "passive"});
  result = iggy3d::validatePackage(request);
  ok = ok && expect(result.status == iggy3d::PackageValidationStatus::MissingAiActorEntity,
                    "unknown ai actor entity") &&
       expect(!result.diagnostics.empty() &&
                  result.diagnostics[0].code == "scenario.missing_ai_actor_entity",
              "unknown ai actor code");
  request = {load.manifest, load.scenario};
  request.scenario.aiActors.push_back({"player", "passive"});
  result = iggy3d::validatePackage(request);
  ok = ok && expect(result.status == iggy3d::PackageValidationStatus::NonNpcAiActor,
                    "non npc ai actor") &&
       expect(!result.diagnostics.empty() &&
                  result.diagnostics[0].code == "scenario.non_npc_ai_actor",
              "non npc ai actor code");
  request = {load.manifest, load.scenario};
  request.scenario.aiActors.push_back({"training_dummy", "passive"});
  request.scenario.aiActors.push_back({"training_dummy", "default"});
  result = iggy3d::validatePackage(request);
  ok = ok && expect(result.status ==
                        iggy3d::PackageValidationStatus::DuplicateAiActorBinding,
                    "duplicate ai actor") &&
       expect(!result.diagnostics.empty() &&
                  result.diagnostics[0].code == "scenario.duplicate_ai_actor",
              "duplicate ai actor code");
  request = {load.manifest, load.scenario};
  request.scenario.aiActors.push_back({"training_dummy", "Bad-Id"});
  result = iggy3d::validatePackage(request);
  ok = ok && expect(result.status ==
                        iggy3d::PackageValidationStatus::InvalidAiActorProfileId,
                    "invalid ai actor profile") &&
       expect(!result.diagnostics.empty() &&
                  result.diagnostics[0].code == "scenario.invalid_ai_actor_profile_id",
              "invalid ai actor profile code");
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
                  scenarioSeedContainsEntities() && scenarioSeedContainsAiActors() &&
                  scenarioAiActorRejectsInvalidInputs() && scenarioAiActorParsesPatrolRoute() &&
                  scenarioSeedContainsAiGuardAnchors() &&
                  scenarioAiGuardAnchorRejectsInvalidInputs() && validatorAcceptsAndRejects() &&
                  validatorAcceptsAndRejectsAiGuardAnchors() && loaderRejectsInvalidInputs() &&
                  loadPackageSmoke();
  return ok ? 0 : 1;
}
