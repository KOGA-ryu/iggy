#include "app/iggy3d/world/PackageSessionSeed.hpp"
#include "runtime/session/Session.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

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

iggy3d::PackageLoadResult loadPackageFixture(std::string_view path) {
  return iggy3d::loadPackage({std::string(path)});
}

const iggy3d::ScenarioEntitySeed* findEntity(const iggy3d::FixtureScenarioSeed& seed,
                                             std::string_view stableName) {
  for (const iggy3d::ScenarioEntitySeed& entity : seed.entities) {
    if (entity.stableName == stableName) {
      return &entity;
    }
  }
  return nullptr;
}

const iggy3d::ScenarioAiActorSeed* findAiActorSeed(
    const iggy3d::FixtureScenarioSeed& seed,
    std::string_view actorStableName) {
  for (const iggy3d::ScenarioAiActorSeed& aiActor : seed.aiActors) {
    if (aiActor.actorStableName == actorStableName) {
      return &aiActor;
    }
  }
  return nullptr;
}

const iggy3d::ScenarioAiGuardAnchorSeed* findAiGuardAnchorSeed(
    const iggy3d::FixtureScenarioSeed& seed,
    std::string_view actorStableName) {
  for (const iggy3d::ScenarioAiGuardAnchorSeed& guard : seed.aiGuardAnchors) {
    if (guard.actorStableName == actorStableName) {
      return &guard;
    }
  }
  return nullptr;
}

const iggy3d::AiActorState* findAiActorState(const iggy3d::AiState& ai,
                                             iggy3d::EntityId actor) {
  for (const iggy3d::AiActorState& actorState : ai.actors) {
    if (actorState.actor == actor) {
      return &actorState;
    }
  }
  return nullptr;
}

bool expectGuardHome(const iggy3d::AiActorState* aiActor,
                     const iggy3d::EntityState* anchor,
                     std::string_view anchorStableName,
                     float leashRadius,
                     float returnRadius,
                     float tolerance,
                     std::string_view message) {
  return expect(aiActor != nullptr, message) &&
         expect(aiActor != nullptr && aiActor->hasHomePosition,
                "ai guard home configured") &&
         expect(aiActor != nullptr && anchor != nullptr &&
                    iggy3d::nearlyEqual(aiActor->homePosition,
                                        anchor->transform.position),
                "ai guard home position") &&
         expect(aiActor != nullptr && aiActor->homeStableName == anchorStableName,
                "ai guard home stable name") &&
         expect(aiActor != nullptr && near(aiActor->leashRadiusMeters, leashRadius),
                "ai guard leash") &&
         expect(aiActor != nullptr && near(aiActor->returnRadiusMeters, returnRadius),
                "ai guard return radius") &&
         expect(aiActor != nullptr && near(aiActor->homeToleranceMeters, tolerance),
                "ai guard tolerance");
}

const iggy3d::RoomAnchorAsset* findAnchor(const iggy3d::RoomAsset& room,
                                          std::string_view id) {
  for (const iggy3d::RoomAnchorAsset& anchor : room.anchors) {
    if (anchor.id == id) {
      return &anchor;
    }
  }
  return nullptr;
}

iggy3d::RoomAnchorAsset anchor(std::string_view id,
                               std::string_view kind,
                               iggy3d::Vec3 position) {
  iggy3d::RoomAnchorAsset anchor;
  anchor.id = std::string(id);
  anchor.kind = std::string(kind);
  anchor.positionMeters = position;
  return anchor;
}

bool authoredFirstRoomScenarioIsPreserved() {
  const iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/first_room/package.iggy3d.toml");
  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package);
  return expect(package.status == iggy3d::PackageLoadStatus::Ok, "package ok") &&
         expect(result.ok, "seed ok") &&
         expect(result.status == "product_package_seed_ready", "seed status") &&
         expect(!result.synthesizedFromRoomAnchors, "not synthesized") &&
         expect(result.playerCount == package.scenario.players.size(), "player count") &&
         expect(result.entityCount == package.scenario.entities.size(), "entity count") &&
         expect(result.objectiveCount == package.scenario.objectives.size(),
                "objective count") &&
         expect(result.seed.scenarioId == package.scenario.scenarioId,
                "scenario id preserved") &&
         expect(result.seed.entities.size() == package.scenario.entities.size(),
                "entities preserved") &&
         expect(result.seed.players.size() == package.scenario.players.size(),
                "players preserved") &&
         expect(result.seed.objectives.size() == package.scenario.objectives.size(),
                "objectives preserved");
}

bool asciiPackageSynthesizesSeedAndCreatesSession() {
  const iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/ascii_training_room/package.iggy3d.toml");
  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package);
  if (!expect(package.status == iggy3d::PackageLoadStatus::Ok, "package ok") ||
      !expect(result.ok, "seed ok") || !expect(!package.rooms.empty(), "room exists")) {
    return false;
  }

  const iggy3d::RoomAnchorAsset* spawn =
      findAnchor(package.rooms.front(), "marker_player_spawn_r1_c1");
  const iggy3d::ScenarioEntitySeed* player = findEntity(result.seed, "player");
  const iggy3d::ScenarioEntitySeed* npc =
      findEntity(result.seed, "marker_npc_spawn_r1_c4");
  const iggy3d::ScenarioEntitySeed* pickup =
      findEntity(result.seed, "marker_treasure_r2_c4");
  const iggy3d::ScenarioEntitySeed* door =
      findEntity(result.seed, "marker_door_r2_c2");
  const iggy3d::ScenarioEntitySeed* exit =
      findEntity(result.seed, "marker_exit_r3_c3");
  const iggy3d::ScenarioAiActorSeed* npcAiSeed =
      findAiActorSeed(result.seed, "marker_npc_spawn_r1_c4");

  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.seed = result.seed;
  create.config = result.seed.config;
  const iggy3d::Result<iggy3d::Session> session = iggy3d::Session::create(create);

  return expect(result.status == "product_package_seed_ready", "seed status") &&
         expect(result.synthesizedFromRoomAnchors, "synthesized") &&
         expect(result.sourceRoomId == "training_room_ascii", "source room") &&
         expect(result.roomCount == 1U, "room count") &&
         expect(result.anchorCount == 5U, "anchor count") &&
         expect(result.playerCount == 1U, "player count") &&
         expect(result.entityCount == 5U, "entity count") &&
         expect(result.npcCount == 1U, "npc count") &&
         expect(result.pickupCount == 1U, "pickup count") &&
         expect(result.doorCount == 1U, "door count") &&
         expect(result.markerEntityCount == 1U, "marker entity count") &&
         expect(result.objectiveCount == 2U, "objective count") &&
         expect(result.seed.aiActors.size() == 1U, "default npc ai seed count") &&
         expect(npcAiSeed != nullptr && npcAiSeed->behaviorProfileId == "default",
                "default npc ai seed profile") &&
         expect(result.seed.scenarioId == "ascii_training_room.runtime_loop",
                "scenario id") &&
         expect(result.seed.players.size() == 1U &&
                    result.seed.players[0].slot == 0 &&
                    result.seed.players[0].kind == iggy3d::PlayerSlotKind::Local &&
                    result.seed.players[0].actorStableName == "player",
                "player slot") &&
         expect(spawn != nullptr && player != nullptr, "spawn and player") &&
         expect(player->kind == iggy3d::EntityKind::Player, "player kind") &&
         expect(player->targeting.targetable &&
                    iggy3d::isTargetActionSupported(player->targeting,
                                                    iggy3d::TargetAction::Attack) &&
                    iggy3d::isTargetActionSupported(player->targeting,
                                                    iggy3d::TargetAction::Inspect) &&
                    player->combatantEnabled &&
                    player->combatant.factionId == 1U &&
                    player->combatant.hitPoints == 10 &&
                    player->combatant.maxHitPoints == 10,
                "player combat target") &&
         expect(near(player->transform.position.x, spawn->positionMeters.x) &&
                    near(player->transform.position.y, spawn->positionMeters.y) &&
                    near(player->transform.position.z, spawn->positionMeters.z),
                "player position from spawn") &&
         expect(npc != nullptr && npc->kind == iggy3d::EntityKind::Npc,
                "npc entity") &&
         expect(npc->targeting.targetable &&
                    iggy3d::isTargetActionSupported(npc->targeting,
                                                    iggy3d::TargetAction::Attack) &&
                    npc->combatantEnabled && npc->combatant.factionId == 2U &&
                    npc->combatant.hitPoints == 3 && npc->combatant.maxHitPoints == 3,
                "npc combat target") &&
         expect(pickup != nullptr && pickup->kind == iggy3d::EntityKind::Pickup,
                "pickup entity") &&
         expect(pickup->targeting.targetable &&
                    iggy3d::isTargetActionSupported(pickup->targeting,
                                                    iggy3d::TargetAction::Interact) &&
                    pickup->interaction.kind == iggy3d::InteractionKind::Pickup &&
                    pickup->interaction.primaryEffect ==
                        iggy3d::InteractionEffectKind::AddItemToInventory &&
                    pickup->interaction.itemId == pickup->stableName &&
                    pickup->interaction.objectiveId == "collect_marker_treasure_r2_c4",
                "pickup interaction") &&
         expect(result.seed.objectives.size() == 2U &&
                    result.seed.objectives[0].id == "collect_marker_treasure_r2_c4" &&
                    result.seed.objectives[0].itemId == pickup->interaction.itemId &&
                    result.seed.objectives[0].itemCount == 1U,
                "pickup objective") &&
         expect(door != nullptr && door->kind == iggy3d::EntityKind::Door &&
                    door->interaction.kind == iggy3d::InteractionKind::OpenDoor &&
                    door->interaction.primaryEffect ==
                        iggy3d::InteractionEffectKind::EmitEventOnly &&
                    door->interaction.deactivateTargetOnSuccess,
                "door entity") &&
         expect(exit != nullptr && exit->kind == iggy3d::EntityKind::Marker &&
                    iggy3d::isTargetActionSupported(exit->targeting,
                                                    iggy3d::TargetAction::Interact) &&
                    iggy3d::isTargetActionSupported(exit->targeting,
                                                    iggy3d::TargetAction::Move) &&
                    exit->interaction.kind == iggy3d::InteractionKind::ObjectiveTrigger &&
                    exit->interaction.primaryEffect ==
                        iggy3d::InteractionEffectKind::CompleteObjective &&
                    exit->interaction.objectiveId == "exit_marker_exit_r3_c3" &&
                    exit->interaction.requiredItemId == pickup->interaction.itemId &&
                    exit->interaction.requiredItemCount == 1U &&
                    result.seed.objectives[1].id == "exit_marker_exit_r3_c3",
                "exit marker") &&
         expect(session.status == iggy3d::ResultStatus::Ok, "session create ok") &&
         expect(session.value.state().world.size() == 5U, "session world count") &&
         expect(session.value.state().identity.packageId == "iggy3d.ascii_training_room",
                "session package id") &&
         expect(session.value.state().identity.scenarioId ==
                    "ascii_training_room.runtime_loop",
                "session scenario id");
}

bool missingRoomAndSpawnRejectDeterministically() {
  iggy3d::PackageLoadResult missingRoom;
  missingRoom.status = iggy3d::PackageLoadStatus::Ok;
  missingRoom.scenario.scenarioId = "missing_room";
  const auto missingRoomResult = iggy3d::buildProductPackageSessionSeed(missingRoom);

  iggy3d::PackageLoadResult missingSpawn;
  missingSpawn.status = iggy3d::PackageLoadStatus::Ok;
  missingSpawn.scenario.scenarioId = "missing_spawn";
  iggy3d::RoomAsset room;
  room.id = "anchor_room";
  iggy3d::RoomAnchorAsset marker;
  marker.id = "marker_only";
  marker.kind = "marker";
  room.anchors.push_back(marker);
  missingSpawn.rooms.push_back(room);
  const auto missingSpawnResult = iggy3d::buildProductPackageSessionSeed(missingSpawn);

  iggy3d::PackageLoadResult notLoaded;
  notLoaded.status = iggy3d::PackageLoadStatus::PackageReadFailed;
  const auto notLoadedResult = iggy3d::buildProductPackageSessionSeed(notLoaded);

  return expect(!missingRoomResult.ok, "missing room rejected") &&
         expect(missingRoomResult.reasonCode == "product_package_seed_missing_room",
                "missing room reason") &&
         expect(!missingSpawnResult.ok, "missing spawn rejected") &&
         expect(missingSpawnResult.reasonCode ==
                    "product_package_seed_missing_spawn_anchor",
                "missing spawn reason") &&
         expect(!notLoadedResult.ok, "not loaded rejected") &&
         expect(notLoadedResult.reasonCode == "product_package_seed_package_not_loaded",
                "not loaded reason");
}

bool asciiSemanticAnchorsGateSecretDoorAndExit() {
  iggy3d::PackageLoadResult package;
  package.status = iggy3d::PackageLoadStatus::Ok;
  package.manifest.packageId = "iggy3d.semantic_room";
  package.scenario.scenarioId = "semantic_room";
  iggy3d::RoomAsset room;
  room.id = "semantic_room";
  room.anchors = {
      anchor("marker_player_spawn_r1_c1", "spawn", {1.0F, 0.0F, 1.0F}),
      anchor("marker_key_r1_c2", "key", {2.0F, 0.0F, 1.0F}),
      anchor("marker_secret_door_r1_c3", "secret_door", {3.0F, 0.0F, 1.0F}),
      anchor("marker_treasure_r1_c4", "treasure", {4.0F, 0.0F, 1.0F}),
      anchor("marker_exit_r1_c5", "exit", {5.0F, 0.0F, 1.0F}),
  };
  package.rooms.push_back(room);

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package);
  const iggy3d::ScenarioEntitySeed* key =
      findEntity(result.seed, "marker_key_r1_c2");
  const iggy3d::ScenarioEntitySeed* secretDoor =
      findEntity(result.seed, "marker_secret_door_r1_c3");
  const iggy3d::ScenarioEntitySeed* treasure =
      findEntity(result.seed, "marker_treasure_r1_c4");
  const iggy3d::ScenarioEntitySeed* exit =
      findEntity(result.seed, "marker_exit_r1_c5");

  return expect(result.ok, "semantic seed ok") &&
         expect(result.pickupCount == 2U, "key and treasure are pickups") &&
         expect(result.doorCount == 1U, "secret door count") &&
         expect(result.markerEntityCount == 1U, "exit marker count") &&
         expect(result.objectiveCount == 3U, "key treasure exit objectives") &&
         expect(key != nullptr && key->kind == iggy3d::EntityKind::Pickup &&
                    key->interaction.itemId == "marker_key_r1_c2",
                "key pickup item") &&
         expect(treasure != nullptr && treasure->kind == iggy3d::EntityKind::Pickup &&
                    treasure->interaction.itemId == "marker_treasure_r1_c4",
                "treasure pickup item") &&
         expect(secretDoor != nullptr && secretDoor->kind == iggy3d::EntityKind::Door &&
                    secretDoor->interaction.requiredItemId ==
                        key->interaction.itemId &&
                    secretDoor->interaction.requiredItemCount == 1U,
                "secret door requires key") &&
         expect(exit != nullptr && exit->kind == iggy3d::EntityKind::Marker &&
                    exit->interaction.kind == iggy3d::InteractionKind::ObjectiveTrigger &&
                    exit->interaction.primaryEffect ==
                        iggy3d::InteractionEffectKind::CompleteObjective &&
                    exit->interaction.requiredItemId ==
                        treasure->interaction.itemId &&
                    exit->interaction.requiredItemCount == 1U &&
                    exit->interaction.objectiveId == "exit_marker_exit_r1_c5",
                "exit requires treasure");
}

bool npcProfileAssignmentAppliesToSynthesizedNpcStableName() {
  const iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/ascii_training_room/package.iggy3d.toml");
  iggy3d::ProductNpcProfileAssignmentTable assignments;
  assignments.assignments.push_back({"marker_npc_spawn_r1_c4", "passive"});

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package, &assignments);
  const iggy3d::ScenarioAiActorSeed* npcAiSeed =
      findAiActorSeed(result.seed, "marker_npc_spawn_r1_c4");

  return expect(result.ok, "assigned seed ok") &&
         expect(result.seed.aiActors.size() == 1U, "assigned ai seed count") &&
         expect(npcAiSeed != nullptr, "assigned npc ai seed exists") &&
         expect(npcAiSeed != nullptr && npcAiSeed->behaviorProfileId == "passive",
                "assigned npc passive profile");
}

bool validUnknownNpcProfileAssignmentIsPreserved() {
  const iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/ascii_training_room/package.iggy3d.toml");
  iggy3d::ProductNpcProfileAssignmentTable assignments;
  assignments.assignments.push_back({"marker_npc_spawn_r1_c4", "ghost_profile"});

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package, &assignments);
  const iggy3d::ScenarioAiActorSeed* npcAiSeed =
      findAiActorSeed(result.seed, "marker_npc_spawn_r1_c4");

  return expect(result.ok, "unknown profile seed ok") &&
         expect(npcAiSeed != nullptr, "unknown profile npc ai seed exists") &&
         expect(npcAiSeed != nullptr && npcAiSeed->behaviorProfileId == "ghost_profile",
                "unknown profile preserved");
}

bool invalidNpcProfileAssignmentRejectsSeedConstruction() {
  const iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/ascii_training_room/package.iggy3d.toml");
  iggy3d::ProductNpcProfileAssignmentTable assignments;
  assignments.assignments.push_back({"marker_npc_spawn_r1_c4", "Bad-Id"});

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package, &assignments);

  return expect(!result.ok, "invalid assignment rejects") &&
         expect(result.reasonCode == "product_package_seed_npc_profile_assignment_invalid",
                "invalid assignment reason") &&
         expect(result.seed.aiActors.empty(), "invalid assignment no partial ai seed");
}

bool glyphLikeNpcProfileAssignmentHasNoAsciiMeaning() {
  const iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/ascii_training_room/package.iggy3d.toml");
  iggy3d::ProductNpcProfileAssignmentTable assignments;
  assignments.assignments.push_back({"N", "passive"});

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package, &assignments);
  const iggy3d::ScenarioAiActorSeed* npcAiSeed =
      findAiActorSeed(result.seed, "marker_npc_spawn_r1_c4");

  return expect(result.ok, "glyph-like assignment seed ok") &&
         expect(npcAiSeed != nullptr, "glyph-like assignment npc ai seed exists") &&
         expect(npcAiSeed != nullptr && npcAiSeed->behaviorProfileId == "default",
                "glyph-like assignment does not match npc stable name");
}

bool scenarioAiActorsApplyToSynthesizedRoomNpc() {
  iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/ascii_training_room/package.iggy3d.toml");
  package.scenario.aiActors.push_back({"marker_npc_spawn_r1_c4", "passive"});

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package);
  const iggy3d::ScenarioAiActorSeed* npcAiSeed =
      findAiActorSeed(result.seed, "marker_npc_spawn_r1_c4");

  return expect(result.ok, "scenario ai synthesized seed ok") &&
         expect(result.seed.aiActors.size() == 1U,
                "scenario ai synthesized seed count") &&
         expect(npcAiSeed != nullptr && npcAiSeed->behaviorProfileId == "passive",
                "scenario ai synthesized passive profile");
}

bool scenarioAiActorGlyphLikeNameDoesNotMatchSynthesizedNpc() {
  iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/ascii_training_room/package.iggy3d.toml");
  package.scenario.aiActors.push_back({"N", "passive"});

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package);
  const iggy3d::ScenarioAiActorSeed* npcAiSeed =
      findAiActorSeed(result.seed, "marker_npc_spawn_r1_c4");

  return expect(result.ok, "scenario glyph-like seed ok") &&
         expect(npcAiSeed != nullptr && npcAiSeed->behaviorProfileId == "default",
                "scenario glyph-like name has no ascii meaning");
}

bool explicitNpcAssignmentOverridesScenarioAiActors() {
  iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/ascii_training_room/package.iggy3d.toml");
  package.scenario.aiActors.push_back({"marker_npc_spawn_r1_c4", "passive"});
  iggy3d::ProductNpcProfileAssignmentTable explicitAssignments;
  explicitAssignments.assignments.push_back({"marker_npc_spawn_r1_c4", "ghost_profile"});

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package, &explicitAssignments);
  const iggy3d::ScenarioAiActorSeed* npcAiSeed =
      findAiActorSeed(result.seed, "marker_npc_spawn_r1_c4");

  return expect(result.ok, "explicit override seed ok") &&
         expect(npcAiSeed != nullptr && npcAiSeed->behaviorProfileId == "ghost_profile",
                "explicit assignment overrides scenario metadata");
}

bool authoredScenarioAiActorsPreserveAndMapThroughSessionCreate() {
  iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/first_room/package.iggy3d.toml");
  // first_room now authors training_dummy=passive in the fixture; clear and
  // re-author here so this test exercises an explicit assignment independent of
  // the fixture's contents (rather than duplicating the actor).
  package.scenario.aiActors.clear();
  package.scenario.aiActors.push_back({"training_dummy", "passive"});

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package);
  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.seed = result.seed;
  create.config = result.seed.config;
  const iggy3d::Result<iggy3d::Session> session = iggy3d::Session::create(create);
  const iggy3d::EntityState* npc =
      session.status == iggy3d::ResultStatus::Ok
          ? session.value.state().world.findByStableName("training_dummy")
          : nullptr;
  const iggy3d::AiActorState* aiActor =
      npc == nullptr ? nullptr : findAiActorState(session.value.state().ai, npc->id);

  return expect(result.ok, "authored ai seed ok") &&
         expect(!result.synthesizedFromRoomAnchors, "authored scenario not synthesized") &&
         expect(result.seed.aiActors.size() == 1U,
                "authored scenario ai actor preserved") &&
         expect(result.seed.aiActors[0].actorStableName == "training_dummy" &&
                    result.seed.aiActors[0].behaviorProfileId == "passive",
                "authored scenario ai actor values") &&
         expect(session.status == iggy3d::ResultStatus::Ok,
                "authored ai session create ok") &&
         expect(npc != nullptr, "authored ai npc resolved") &&
         expect(aiActor != nullptr && aiActor->behaviorProfileId == "passive",
                "authored ai session profile mapped");
}

bool authoredScenarioGuardAnchorsPreserveAndMapThroughSessionCreate() {
  iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/first_room/package.iggy3d.toml");
  // Drop the fixture's authored training_dummy=passive profile so this guard-
  // anchor test exercises the default behavior profile it asserts below.
  package.scenario.aiActors.clear();
  package.scenario.aiGuardAnchors.push_back(
      {"training_dummy", "tactical_marker_alpha", 6.0F, 1.0F, 0.25F});

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package);
  const iggy3d::ScenarioAiGuardAnchorSeed* guardSeed =
      findAiGuardAnchorSeed(result.seed, "training_dummy");

  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.seed = result.seed;
  create.config = result.seed.config;
  const iggy3d::Result<iggy3d::Session> session = iggy3d::Session::create(create);
  const iggy3d::EntityState* npc =
      session.status == iggy3d::ResultStatus::Ok
          ? session.value.state().world.findByStableName("training_dummy")
          : nullptr;
  const iggy3d::EntityState* anchor =
      session.status == iggy3d::ResultStatus::Ok
          ? session.value.state().world.findByStableName("tactical_marker_alpha")
          : nullptr;
  const iggy3d::AiActorState* aiActor =
      npc == nullptr ? nullptr : findAiActorState(session.value.state().ai, npc->id);
  const iggy3d::AiActorState* baselineAiActor =
      npc == nullptr ? nullptr : findAiActorState(session.value.state().baseline.ai, npc->id);

  return expect(result.ok, "authored guard seed ok") &&
         expect(!result.synthesizedFromRoomAnchors, "authored guard not synthesized") &&
         expect(result.seed.aiGuardAnchors.size() == 1U,
                "authored guard seed preserved") &&
         expect(guardSeed != nullptr &&
                    guardSeed->actorStableName == "training_dummy" &&
                    guardSeed->anchorStableName == "tactical_marker_alpha" &&
                    near(guardSeed->leashRadiusMeters, 6.0F) &&
                    near(guardSeed->returnRadiusMeters, 1.0F) &&
                    near(guardSeed->homeToleranceMeters, 0.25F),
                "authored guard values") &&
         expect(session.status == iggy3d::ResultStatus::Ok,
                "authored guard session create ok") &&
         expect(aiActor != nullptr && aiActor->behaviorProfileId == "default",
                "authored guard default profile") &&
         expectGuardHome(aiActor, anchor, "tactical_marker_alpha", 6.0F, 1.0F,
                         0.25F, "authored guard ai actor") &&
         expectGuardHome(baselineAiActor, anchor, "tactical_marker_alpha", 6.0F,
                         1.0F, 0.25F, "authored guard baseline ai actor");
}

bool synthesizedScenarioGuardAnchorsMapThroughSessionCreate() {
  iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/ascii_training_room/package.iggy3d.toml");
  package.scenario.aiGuardAnchors.push_back(
      {"marker_npc_spawn_r1_c4", "marker_exit_r3_c3", 5.0F, 1.25F, 0.5F});

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package);

  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.seed = result.seed;
  create.config = result.seed.config;
  const iggy3d::Result<iggy3d::Session> session = iggy3d::Session::create(create);
  const iggy3d::EntityState* npc =
      session.status == iggy3d::ResultStatus::Ok
          ? session.value.state().world.findByStableName("marker_npc_spawn_r1_c4")
          : nullptr;
  const iggy3d::EntityState* anchor =
      session.status == iggy3d::ResultStatus::Ok
          ? session.value.state().world.findByStableName("marker_exit_r3_c3")
          : nullptr;
  const iggy3d::AiActorState* aiActor =
      npc == nullptr ? nullptr : findAiActorState(session.value.state().ai, npc->id);

  return expect(result.ok, "synthesized guard seed ok") &&
         expect(result.synthesizedFromRoomAnchors, "synthesized guard room") &&
         expect(result.seed.aiGuardAnchors.size() == 1U,
                "synthesized guard seed preserved") &&
         expect(session.status == iggy3d::ResultStatus::Ok,
                "synthesized guard session create ok") &&
         expect(aiActor != nullptr && aiActor->behaviorProfileId == "default",
                "synthesized guard default profile") &&
         expectGuardHome(aiActor, anchor, "marker_exit_r3_c3", 5.0F, 1.25F,
                         0.5F, "synthesized guard ai actor");
}

bool scenarioProfileAndGuardMergeIntoOneAiActor() {
  iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/ascii_training_room/package.iggy3d.toml");
  package.scenario.aiActors.push_back({"marker_npc_spawn_r1_c4", "passive"});
  package.scenario.aiGuardAnchors.push_back(
      {"marker_npc_spawn_r1_c4", "marker_exit_r3_c3", 6.0F, 1.0F, 0.25F});

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package);
  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.seed = result.seed;
  create.config = result.seed.config;
  const iggy3d::Result<iggy3d::Session> session = iggy3d::Session::create(create);
  const iggy3d::EntityState* npc =
      session.status == iggy3d::ResultStatus::Ok
          ? session.value.state().world.findByStableName("marker_npc_spawn_r1_c4")
          : nullptr;
  const iggy3d::EntityState* anchor =
      session.status == iggy3d::ResultStatus::Ok
          ? session.value.state().world.findByStableName("marker_exit_r3_c3")
          : nullptr;
  const iggy3d::AiActorState* aiActor =
      npc == nullptr ? nullptr : findAiActorState(session.value.state().ai, npc->id);

  return expect(result.ok, "profile guard merge seed ok") &&
         expect(result.seed.aiActors.size() == 1U, "profile guard ai seed count") &&
         expect(result.seed.aiGuardAnchors.size() == 1U,
                "profile guard seed count") &&
         expect(session.status == iggy3d::ResultStatus::Ok,
                "profile guard session create ok") &&
         expect(session.value.state().ai.actors.size() == 1U,
                "profile guard one runtime ai actor") &&
         expect(aiActor != nullptr && aiActor->behaviorProfileId == "passive",
                "profile guard preserves profile") &&
         expectGuardHome(aiActor, anchor, "marker_exit_r3_c3", 6.0F, 1.0F,
                         0.25F, "profile guard home");
}

bool explicitProfileOverrideDoesNotEraseGuardMetadata() {
  iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/ascii_training_room/package.iggy3d.toml");
  package.scenario.aiActors.push_back({"marker_npc_spawn_r1_c4", "passive"});
  package.scenario.aiGuardAnchors.push_back(
      {"marker_npc_spawn_r1_c4", "marker_exit_r3_c3", 6.0F, 1.0F, 0.25F});
  iggy3d::ProductNpcProfileAssignmentTable explicitAssignments;
  explicitAssignments.assignments.push_back({"marker_npc_spawn_r1_c4", "ghost_profile"});

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package, &explicitAssignments);
  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.seed = result.seed;
  create.config = result.seed.config;
  const iggy3d::Result<iggy3d::Session> session = iggy3d::Session::create(create);
  const iggy3d::EntityState* npc =
      session.status == iggy3d::ResultStatus::Ok
          ? session.value.state().world.findByStableName("marker_npc_spawn_r1_c4")
          : nullptr;
  const iggy3d::EntityState* anchor =
      session.status == iggy3d::ResultStatus::Ok
          ? session.value.state().world.findByStableName("marker_exit_r3_c3")
          : nullptr;
  const iggy3d::AiActorState* aiActor =
      npc == nullptr ? nullptr : findAiActorState(session.value.state().ai, npc->id);

  return expect(result.ok, "explicit profile guard seed ok") &&
         expect(session.status == iggy3d::ResultStatus::Ok,
                "explicit profile guard session create ok") &&
         expect(aiActor != nullptr && aiActor->behaviorProfileId == "ghost_profile",
                "explicit profile override preserved") &&
         expectGuardHome(aiActor, anchor, "marker_exit_r3_c3", 6.0F, 1.0F,
                         0.25F, "explicit profile guard home");
}

bool glyphLikeGuardNamesHaveNoAsciiMeaning() {
  iggy3d::PackageLoadResult package =
      loadPackageFixture("fixtures/demos/ascii_training_room/package.iggy3d.toml");
  package.scenario.aiGuardAnchors.push_back({"N", "G", 6.0F, 1.0F, 0.25F});

  const iggy3d::ProductPackageSessionSeedResult result =
      iggy3d::buildProductPackageSessionSeed(package);
  const iggy3d::ScenarioEntitySeed* generatedNpc =
      findEntity(result.seed, "marker_npc_spawn_r1_c4");
  const iggy3d::ScenarioAiGuardAnchorSeed* glyphGuard =
      findAiGuardAnchorSeed(result.seed, "N");

  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.seed = result.seed;
  create.config = result.seed.config;
  const iggy3d::Result<iggy3d::Session> session = iggy3d::Session::create(create);

  return expect(result.ok, "glyph guard seed construction ok") &&
         expect(generatedNpc != nullptr, "glyph guard generated npc exists") &&
         expect(glyphGuard != nullptr && glyphGuard->anchorStableName == "G",
                "glyph guard preserved as stable-name strings") &&
         expect(session.status == iggy3d::ResultStatus::Error,
                "glyph guard session create rejects") &&
         expect(session.error.code == "session.guard_seed_missing_actor",
                "glyph guard missing actor error");
}

}  // namespace

int main() {
  const bool ok = authoredFirstRoomScenarioIsPreserved() &&
                  asciiPackageSynthesizesSeedAndCreatesSession() &&
                  missingRoomAndSpawnRejectDeterministically() &&
                  asciiSemanticAnchorsGateSecretDoorAndExit() &&
                  npcProfileAssignmentAppliesToSynthesizedNpcStableName() &&
                  validUnknownNpcProfileAssignmentIsPreserved() &&
                  invalidNpcProfileAssignmentRejectsSeedConstruction() &&
                  glyphLikeNpcProfileAssignmentHasNoAsciiMeaning() &&
                  scenarioAiActorsApplyToSynthesizedRoomNpc() &&
                  scenarioAiActorGlyphLikeNameDoesNotMatchSynthesizedNpc() &&
                  explicitNpcAssignmentOverridesScenarioAiActors() &&
                  authoredScenarioAiActorsPreserveAndMapThroughSessionCreate() &&
                  authoredScenarioGuardAnchorsPreserveAndMapThroughSessionCreate() &&
                  synthesizedScenarioGuardAnchorsMapThroughSessionCreate() &&
                  scenarioProfileAndGuardMergeIntoOneAiActor() &&
                  explicitProfileOverrideDoesNotEraseGuardMetadata() &&
                  glyphLikeGuardNamesHaveNoAsciiMeaning();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
