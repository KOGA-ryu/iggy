#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/save/SaveLoad.hpp"
#include "runtime/session/Session.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

std::string eraseFirst(std::string text, std::string_view value) {
  const std::size_t pos = text.find(value);
  if (pos != std::string::npos) {
    text.erase(pos, value.size());
  }
  return text;
}

std::string eraseLineStartingWith(std::string text, std::string_view prefix) {
  const std::size_t pos = text.find(prefix);
  if (pos == std::string::npos) {
    return text;
  }
  const std::size_t end = text.find('\n', pos);
  const std::size_t count = end == std::string::npos ? text.size() - pos : end - pos + 1U;
  text.erase(pos, count);
  return text;
}

iggy3d::Transform3 transformAt(float x, float y, float z) {
  iggy3d::Transform3 transform = iggy3d::identityTransform3();
  transform.position = {x, y, z};
  return transform;
}

iggy3d::ScenarioEntitySeed playerSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "player";
  seed.kind = iggy3d::EntityKind::Player;
  seed.transform = transformAt(0.0F, 0.0F, 0.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  seed.active = true;
  seed.persistent = true;
  seed.combatantEnabled = true;
  seed.combatant.factionId = 1;
  seed.combatant.hitPoints = 10;
  seed.combatant.maxHitPoints = 10;
  return seed;
}

iggy3d::ScenarioEntitySeed goldKeySeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "gold_key";
  seed.kind = iggy3d::EntityKind::Pickup;
  seed.transform = transformAt(3.0F, 0.0F, 0.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.1F, 0.0F, -0.1F}, {0.1F, 0.1F, 0.1F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Interact, iggy3d::TargetAction::Inspect};
  seed.interaction.kind = iggy3d::InteractionKind::Pickup;
  seed.interaction.primaryEffect = iggy3d::InteractionEffectKind::AddItemToInventory;
  seed.interaction.itemId = "gold_key";
  seed.interaction.itemCount = 1;
  seed.interaction.objectiveId = "collect_gold_key";
  seed.interaction.deactivateTargetOnSuccess = true;
  return seed;
}

iggy3d::ScenarioEntitySeed markerSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "tactical_marker_alpha";
  seed.kind = iggy3d::EntityKind::Marker;
  seed.transform = transformAt(2.0F, 0.0F, 1.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.1F, 0.0F, -0.1F}, {0.1F, 0.1F, 0.1F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Move, iggy3d::TargetAction::Inspect};
  return seed;
}

iggy3d::ScenarioEntitySeed dummySeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "training_dummy";
  seed.kind = iggy3d::EntityKind::Npc;
  seed.transform = transformAt(2.0F, 0.0F, 2.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.2F, 0.25F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Attack, iggy3d::TargetAction::Inspect};
  seed.combatantEnabled = true;
  seed.combatant.factionId = 2;
  seed.combatant.hitPoints = 3;
  seed.combatant.maxHitPoints = 3;
  return seed;
}

iggy3d::ScenarioEntitySeed lockedDoorSeed() {
  iggy3d::ScenarioEntitySeed seed;
  seed.stableName = "secret_door";
  seed.kind = iggy3d::EntityKind::Door;
  seed.transform = transformAt(1.0F, 0.0F, 3.0F);
  seed.localBounds = iggy3d::makeAabb3({-0.25F, 0.0F, -0.25F},
                                       {0.25F, 1.8F, 0.25F});
  seed.active = true;
  seed.persistent = true;
  seed.targeting.targetable = true;
  seed.targeting.actions = {iggy3d::TargetAction::Interact,
                            iggy3d::TargetAction::Inspect};
  seed.interaction.kind = iggy3d::InteractionKind::OpenDoor;
  seed.interaction.primaryEffect = iggy3d::InteractionEffectKind::EmitEventOnly;
  seed.interaction.requiredItemId = "gold_key";
  seed.interaction.requiredItemCount = 1;
  seed.interaction.deactivateTargetOnSuccess = true;
  return seed;
}

iggy3d::FixtureScenarioSeed firstRoomSeed() {
  iggy3d::FixtureScenarioSeed seed;
  seed.scenarioId = "first_room.runtime_loop";
  seed.config = iggy3d::makeDefaultRuntimeConfig();
  seed.initialClockMode = iggy3d::ClockMode::Normal;
  seed.defaultRealtimeCamera = iggy3d::CameraMode::ThirdPerson;
  seed.defaultTacticalCamera = iggy3d::CameraMode::TacticalOverhead;
  seed.players.push_back({0, iggy3d::PlayerSlotKind::Local, "player"});
  seed.entities = {playerSeed(), goldKeySeed(), markerSeed(), dummySeed()};
  iggy3d::ScenarioObjectiveSeed objective;
  objective.id = "collect_gold_key";
  objective.initialStatus = iggy3d::ObjectiveStatusSeed::Active;
  objective.condition = "InventoryContains";
  objective.playerSlot = 0;
  objective.itemId = "gold_key";
  objective.itemCount = 1;
  objective.completeStatus = iggy3d::ObjectiveStatusSeed::Complete;
  seed.objectives.push_back(objective);
  return seed;
}

iggy3d::Session makeSession() {
  iggy3d::SessionCreateRequest request;
  request.config = iggy3d::makeDefaultRuntimeConfig();
  request.seed = firstRoomSeed();
  return iggy3d::Session::create(request).value;
}

iggy3d::Session makeSessionWithConfig(iggy3d::RuntimeConfig config) {
  iggy3d::SessionCreateRequest request;
  request.config = config;
  request.seed = firstRoomSeed();
  request.seed.config = config;
  return iggy3d::Session::create(request).value;
}

iggy3d::Session makeSessionWithRequiredItemDoor() {
  iggy3d::SessionCreateRequest request;
  request.config = iggy3d::makeDefaultRuntimeConfig();
  request.seed = firstRoomSeed();
  request.seed.entities.push_back(lockedDoorSeed());
  return iggy3d::Session::create(request).value;
}

iggy3d::CommandRecord submittedInteract() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Interact;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = {2};
  return command;
}

iggy3d::CommandRecord submittedMove(iggy3d::Vec3 point) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

iggy3d::CommandRecord submittedRetry(iggy3d::CommandId sourceCommandId) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Retry;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.retrySourceCommandId = sourceCommandId;
  return command;
}

iggy3d::CommandRecord submittedAttack() {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Attack;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = {4};
  command.payload.attackDamage = 3;
  return command;
}

std::uint32_t goldKeyCount(const iggy3d::InventoryState& inventory) {
  const iggy3d::PlayerInventory* player = iggy3d::findInventory(inventory, 0);
  if (player == nullptr) {
    return 0;
  }
  for (const iggy3d::InventoryStack& stack : player->stacks) {
    if (stack.itemId == "gold_key") {
      return stack.count;
    }
  }
  return 0;
}

iggy3d::Session completedCurrentLoopSession() {
  iggy3d::Session session = makeSession();
  (void)session.submitCommand(submittedInteract());
  (void)session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  (void)session.tick();
  (void)session.submitCommand(submittedRetry(1));
  (void)session.tick();
  return session;
}

iggy3d::SaveCompatibilityRequest compatibilityFor(const iggy3d::SaveEnvelope& envelope) {
  return {envelope, "iggy3d.first_room", "first_room.runtime_loop"};
}

bool envelopeMappingPreservesDurableState() {
  iggy3d::Session session = completedCurrentLoopSession();
  const iggy3d::SaveStateResult saved = iggy3d::saveSessionState(session.state());
  const iggy3d::SaveEnvelope& envelope = saved.envelope;
  return expect(saved.status == iggy3d::SaveLoadStatus::Ok, "save status") &&
         expect(envelope.session.nextCommandId == 4U, "next command id saved") &&
         expect(envelope.commandLog.records.size() == 3U, "command records saved") &&
         expect(envelope.commandLog.nextSequence == 4U, "next sequence saved") &&
         expect(envelope.commandLog.epoch == session.state().commandLog.epoch(), "epoch saved") &&
         expect(envelope.world.entities[1].stableName == "gold_key", "key order saved") &&
         expect(!envelope.world.entities[1].active, "key inactive saved") &&
         expect(envelope.inventory.players[0].stacks[0].itemId == "gold_key", "item id saved") &&
         expect(envelope.inventory.players[0].stacks[0].count == 1U, "item count saved") &&
         expect(envelope.objectives.objectives[0].status == iggy3d::ObjectiveStatus::Complete,
                "objective saved") &&
         expect(envelope.clock.tickIndex == 2U, "tick saved") &&
         expect(envelope.camera.activeMode == iggy3d::CameraMode::ThirdPerson, "camera saved") &&
         expect(envelope.session.outcome == iggy3d::SessionOutcome::DemoComplete, "outcome saved") &&
         expect(envelope.metadata.savedStateHash == session.stateHash(), "hash saved");
}

bool encodedSaveRoundtripLoadsFreshSession() {
  iggy3d::Session session = completedCurrentLoopSession();
  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(session.state());
  bool ok = expect(saved.status == iggy3d::SaveLoadStatus::Ok, "encoded save status") &&
            expect(saved.encodedSaveText.starts_with("iggy3d.save_envelope.v1\n"),
                   "encoded header") &&
            expect(!saved.encodedSaveText.empty() && saved.encodedSaveText.back() == '\n',
                   "encoded newline");

  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(saved.encodedSaveText);
  ok = ok && expect(decoded.status == iggy3d::SaveCodecStatus::Ok, "decode status") &&
       expect(decoded.envelope.metadata.savedStateHash == saved.savedStateHash, "decode hash");

  iggy3d::Session loaded = makeSession();
  const iggy3d::LoadStateResult load =
      iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText, compatibilityFor(saved.envelope));
  ok = ok && expect(load.status == iggy3d::SaveLoadStatus::Ok, "load encoded status") &&
       expect(loaded.stateHash() == session.stateHash(), "loaded hash") &&
       expect(goldKeyCount(loaded.state().inventory) == 1U, "loaded inventory") &&
       expect(!loaded.state().world.findByStableName("gold_key")->active, "loaded key inactive") &&
       expect(iggy3d::objectiveComplete(loaded.state().objectives, "collect_gold_key"),
              "loaded objective") &&
       expect(loaded.state().outcome == iggy3d::SessionOutcome::DemoComplete, "loaded outcome");
  return ok;
}

bool interactionRequiredItemFactsRoundTripThroughSaveCodec() {
  iggy3d::Session session = makeSessionWithRequiredItemDoor();
  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(session.state());
  bool ok = expect(saved.status == iggy3d::SaveLoadStatus::Ok,
                   "required item save status") &&
            expect(saved.envelope.world.entities.size() == 5U,
                   "required item world size");

  const iggy3d::SaveEntityRecord& record = saved.envelope.world.entities.back();
  ok = ok && expect(record.stableName == "secret_door", "required door record") &&
       expect(record.interactionRequiredItemId == "gold_key",
              "required item id envelope") &&
       expect(record.interactionRequiredItemCount == 1U,
              "required item count envelope") &&
       expect(saved.encodedSaveText.find(
                  "world.entity.4.interactionRequiredItemId=gold_key\n") !=
                  std::string::npos,
              "required item id encoded") &&
       expect(saved.encodedSaveText.find(
                  "world.entity.4.interactionRequiredItemCount=1\n") !=
                  std::string::npos,
              "required item count encoded");

  iggy3d::Session loaded = makeSessionWithRequiredItemDoor();
  const iggy3d::LoadStateResult load =
      iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText,
                                         compatibilityFor(saved.envelope));
  const iggy3d::EntityState* loadedDoor =
      loaded.state().world.findByStableName("secret_door");
  ok = ok && expect(load.status == iggy3d::SaveLoadStatus::Ok,
                    "required item load status") &&
       expect(loadedDoor != nullptr, "loaded required door present") &&
       expect(loadedDoor != nullptr &&
                  loadedDoor->interaction.requiredItemId == "gold_key" &&
                  loadedDoor->interaction.requiredItemCount == 1U,
              "required item facts loaded");

  std::string oldStyle = saved.encodedSaveText;
  oldStyle = eraseLineStartingWith(oldStyle, "world.entity.4.interactionRequiredItemId=");
  oldStyle = eraseLineStartingWith(oldStyle, "world.entity.4.interactionRequiredItemCount=");
  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(oldStyle);
  return ok && expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                      "old save without required item facts decodes") &&
         expect(decoded.envelope.world.entities.back().interactionRequiredItemId.empty(),
                "old save required item id defaults empty") &&
         expect(decoded.envelope.world.entities.back().interactionRequiredItemCount == 0U,
                "old save required item count defaults zero");
}

bool productMetadataRoundTripsThroughSaveCodec() {
  iggy3d::Session session = makeSession();
  iggy3d::SaveStateResult saved = iggy3d::saveSessionState(session.state());
  saved.envelope.metadata.saveId = "save_001";
  saved.envelope.metadata.worldId = "world_0001";
  saved.envelope.metadata.worldTitle = "World 100% = ready\nLine 2";
  saved.envelope.metadata.saveTitle = "Manual Save = 1%";
  saved.envelope.metadata.saveType = "manual";
  saved.envelope.metadata.createdAtUtc = "2026-06-24T00:00:00Z";
  saved.envelope.metadata.savedAtUtc = "2026-06-24T01:02:03Z";

  const iggy3d::SaveEncodeResult encoded = iggy3d::encodeSaveEnvelope(saved.envelope);
  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(encoded.encodedText);

  return expect(encoded.status == iggy3d::SaveCodecStatus::Ok, "product metadata encode status") &&
         expect(encoded.encodedText.find("metadata.saveId=") != std::string::npos,
                "save id key encoded") &&
         expect(encoded.encodedText.find("metadata.worldId=") != std::string::npos,
                "world id key encoded") &&
         expect(encoded.encodedText.find("metadata.worldTitle=") != std::string::npos,
                "world title key encoded") &&
         expect(encoded.encodedText.find("metadata.saveTitle=") != std::string::npos,
                "save title key encoded") &&
         expect(encoded.encodedText.find("metadata.saveType=") != std::string::npos,
                "save type key encoded") &&
         expect(encoded.encodedText.find("metadata.createdAtUtc=") != std::string::npos,
                "created utc key encoded") &&
         expect(encoded.encodedText.find("metadata.savedAtUtc=") != std::string::npos,
                "saved utc key encoded") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok, "product metadata decode status") &&
         expect(decoded.envelope.metadata.saveId == "save_001", "save id round trip") &&
         expect(decoded.envelope.metadata.worldId == "world_0001", "world id round trip") &&
         expect(decoded.envelope.metadata.worldTitle == "World 100% = ready\nLine 2",
                "world title escaped round trip") &&
         expect(decoded.envelope.metadata.saveTitle == "Manual Save = 1%",
                "save title escaped round trip") &&
         expect(decoded.envelope.metadata.saveType == "manual", "save type round trip") &&
         expect(decoded.envelope.metadata.createdAtUtc == "2026-06-24T00:00:00Z",
                "created utc round trip") &&
         expect(decoded.envelope.metadata.savedAtUtc == "2026-06-24T01:02:03Z",
                "saved utc round trip");
}

bool oldSaveWithoutProductMetadataStillDecodes() {
  iggy3d::Session session = makeSession();
  iggy3d::SaveStateResult saved = iggy3d::saveSessionState(session.state());
  saved.envelope.metadata.saveId = "save_001";
  saved.envelope.metadata.worldId = "world_0001";
  saved.envelope.metadata.worldTitle = "World Title";
  saved.envelope.metadata.saveTitle = "Save Title";
  saved.envelope.metadata.saveType = "manual";
  saved.envelope.metadata.createdAtUtc = "2026-06-24T00:00:00Z";
  saved.envelope.metadata.savedAtUtc = "2026-06-24T01:02:03Z";

  const iggy3d::SaveEncodeResult encoded = iggy3d::encodeSaveEnvelope(saved.envelope);
  std::string oldStyle = encoded.encodedText;
  oldStyle = eraseLineStartingWith(oldStyle, "metadata.saveId=");
  oldStyle = eraseLineStartingWith(oldStyle, "metadata.worldId=");
  oldStyle = eraseLineStartingWith(oldStyle, "metadata.worldTitle=");
  oldStyle = eraseLineStartingWith(oldStyle, "metadata.saveTitle=");
  oldStyle = eraseLineStartingWith(oldStyle, "metadata.saveType=");
  oldStyle = eraseLineStartingWith(oldStyle, "metadata.createdAtUtc=");
  oldStyle = eraseLineStartingWith(oldStyle, "metadata.savedAtUtc=");

  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(oldStyle);
  return expect(encoded.status == iggy3d::SaveCodecStatus::Ok, "old metadata encode status") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                "old save without product metadata decodes") &&
         expect(decoded.envelope.metadata.saveId.empty(), "old save id defaults empty") &&
         expect(decoded.envelope.metadata.worldId.empty(), "old world id defaults empty") &&
         expect(decoded.envelope.metadata.worldTitle.empty(), "old world title defaults empty") &&
         expect(decoded.envelope.metadata.saveTitle.empty(), "old save title defaults empty") &&
         expect(decoded.envelope.metadata.saveType.empty(), "old save type defaults empty") &&
         expect(decoded.envelope.metadata.createdAtUtc.empty(), "old created utc defaults empty") &&
         expect(decoded.envelope.metadata.savedAtUtc.empty(), "old saved utc defaults empty");
}

bool encodedSaveRoundtripPreservesRuntimeConfig() {
  iggy3d::RuntimeConfig config;
  config.fixedTickRateHz = 60;
  config.interactionRangeMeters = 2.250F;
  config.movementDistanceMeters = 4.500F;
  config.slowTimeScale = 0.500F;
  iggy3d::Session session = makeSessionWithConfig(config);
  (void)session.submitCommand(submittedMove({2.0F, 0.0F, 0.0F}));
  (void)session.tick();

  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(session.state());
  iggy3d::Session loaded = makeSession();
  const iggy3d::LoadStateResult load =
      iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText,
                                         compatibilityFor(saved.envelope));

  return expect(saved.status == iggy3d::SaveLoadStatus::Ok, "config save status") &&
         expect(saved.envelope.session.fixedTickRateHz == 60U, "config tick saved") &&
         expect(saved.envelope.session.interactionRangeMeters == 2.250F,
                "config interaction saved") &&
         expect(saved.envelope.session.movementDistanceMeters == 4.500F,
                "config movement saved") &&
         expect(saved.envelope.session.slowTimeScale == 0.500F, "config slow saved") &&
         expect(saved.encodedSaveText.find("session.fixedTickRateHz=60\n") !=
                    std::string::npos,
                "config tick encoded") &&
         expect(saved.encodedSaveText.find("session.slowTimeScale=0.500\n") !=
                    std::string::npos,
                "config slow encoded") &&
         expect(load.status == iggy3d::SaveLoadStatus::Ok, "config load status") &&
         expect(loaded.state().config.fixedTickRateHz == 60U, "config tick loaded") &&
         expect(loaded.state().config.interactionRangeMeters == 2.250F,
                "config interaction loaded") &&
         expect(loaded.state().config.movementDistanceMeters == 4.500F,
                "config movement loaded") &&
         expect(loaded.state().config.slowTimeScale == 0.500F, "config slow loaded") &&
         expect(loaded.stateHash() == session.stateHash(), "config loaded hash");
}

bool authoredRoomSectionRoundTripsThroughSaveCodec() {
  iggy3d::Session session = makeSession();
  iggy3d::SaveStateResult saved = iggy3d::saveSessionState(session.state());
  saved.envelope.authoredRoom.present = true;
  saved.envelope.authoredRoom.id = "movement_test_room";
  saved.envelope.authoredRoom.sourceFile = "save_file";
  iggy3d::SaveAuthoredRoomFloorRecord floor;
  floor.id = "edit_floor_7";
  floor.storyIndex = 1;
  floor.centerMeters = {2.0F, 0.0F, 3.0F};
  floor.sizeMeters = {4.0F, 0.1F, 5.0F};
  floor.semantics.materialId = "debug_floor";
  floor.semantics.walkable = true;
  floor.semantics.traversalTags = {"walkable"};
  floor.semantics.gameplayTags = {"safe_zone"};
  floor.locked = true;
  floor.hidden = true;
  saved.envelope.authoredRoom.floors.push_back(floor);
  iggy3d::SaveAuthoredRoomWallRecord wall;
  wall.id = "edit_wall_3";
  wall.storyIndex = 2;
  wall.startMeters = {-1.0F, 0.0F, 1.0F};
  wall.endMeters = {1.0F, 0.0F, 1.0F};
  wall.bottomY = 0.25F;
  wall.heightMeters = 1.5F;
  wall.thicknessMeters = 0.2F;
  wall.semantics.materialId = "debug_wall";
  wall.semantics.blocksActor = true;
  wall.semantics.blocksProjectile = true;
  wall.semantics.traversalTags = {"clamber"};
  wall.semantics.gameplayTags = {"training"};
  wall.locked = true;
  wall.hidden = true;
  saved.envelope.authoredRoom.walls.push_back(wall);
  iggy3d::SaveAuthoredRoomMarkerRecord marker;
  marker.id = "marker_treasure_r2_c4";
  marker.tag = "treasure";
  marker.glyph = "$";
  marker.row = 2;
  marker.column = 4;
  marker.positionMeters = {4.0F, 0.05F, 2.0F};
  marker.sourceLine = 3;
  marker.sourceColumn = 5;
  saved.envelope.authoredRoom.markers.push_back(marker);

  const iggy3d::SaveEncodeResult encoded = iggy3d::encodeSaveEnvelope(saved.envelope);
  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(encoded.encodedText);
  const iggy3d::SaveStateResult oldStyle = iggy3d::saveSessionStateEncoded(session.state());
  const iggy3d::SaveDecodeResult oldDecoded =
      iggy3d::decodeSaveEnvelope(oldStyle.encodedSaveText);

  return expect(encoded.status == iggy3d::SaveCodecStatus::Ok, "authored encode status") &&
         expect(encoded.encodedText.find("authoredRoom.present=true\n") != std::string::npos,
                "authored present encoded") &&
         expect(encoded.encodedText.find("authoredRoom.floor.0.id=edit_floor_7\n") !=
                    std::string::npos,
                "authored floor encoded") &&
         expect(encoded.encodedText.find(
                    "authoredRoom.marker.0.id=marker_treasure_r2_c4\n") !=
                    std::string::npos,
                "authored marker encoded") &&
         expect(decoded.status == iggy3d::SaveCodecStatus::Ok, "authored decode status") &&
         expect(decoded.envelope.authoredRoom.present, "authored present decoded") &&
         expect(decoded.envelope.authoredRoom.floors.size() == 1U, "authored floor count") &&
         expect(decoded.envelope.authoredRoom.floors[0].id == "edit_floor_7",
                "authored floor id") &&
         expect(decoded.envelope.authoredRoom.floors[0].storyIndex == 1,
                "authored floor story") &&
         expect(decoded.envelope.authoredRoom.floors[0].centerMeters.x == 2.0F,
                "authored floor center") &&
         expect(decoded.envelope.authoredRoom.floors[0].sizeMeters.z == 5.0F,
                "authored floor size") &&
         expect(decoded.envelope.authoredRoom.floors[0].semantics.materialId ==
                    "debug_floor",
                "authored floor material") &&
         expect(decoded.envelope.authoredRoom.floors[0].semantics.walkable,
                "authored floor walkable") &&
         expect(decoded.envelope.authoredRoom.floors[0].semantics.traversalTags[0] ==
                    "walkable",
                "authored floor traversal") &&
         expect(decoded.envelope.authoredRoom.floors[0].semantics.gameplayTags[0] ==
                    "safe_zone",
                "authored floor gameplay tag") &&
         expect(decoded.envelope.authoredRoom.floors[0].locked, "authored floor locked") &&
         expect(decoded.envelope.authoredRoom.floors[0].hidden, "authored floor hidden") &&
         expect(decoded.envelope.authoredRoom.walls.size() == 1U, "authored wall count") &&
         expect(decoded.envelope.authoredRoom.walls[0].id == "edit_wall_3",
                "authored wall id") &&
         expect(decoded.envelope.authoredRoom.walls[0].storyIndex == 2,
                "authored wall story") &&
         expect(decoded.envelope.authoredRoom.walls[0].startMeters.x == -1.0F,
                "authored wall start") &&
         expect(decoded.envelope.authoredRoom.walls[0].endMeters.x == 1.0F,
                "authored wall end") &&
         expect(decoded.envelope.authoredRoom.walls[0].bottomY == 0.25F,
                "authored wall bottom") &&
         expect(decoded.envelope.authoredRoom.walls[0].heightMeters == 1.5F,
                "authored wall height") &&
         expect(decoded.envelope.authoredRoom.walls[0].thicknessMeters == 0.2F,
                "authored wall thickness") &&
         expect(decoded.envelope.authoredRoom.walls[0].semantics.blocksActor,
                "authored wall actor blocker") &&
         expect(decoded.envelope.authoredRoom.walls[0].semantics.blocksProjectile,
                "authored wall projectile blocker") &&
         expect(decoded.envelope.authoredRoom.walls[0].semantics.traversalTags[0] == "clamber",
                "authored wall traversal tag") &&
         expect(decoded.envelope.authoredRoom.walls[0].semantics.gameplayTags[0] ==
                    "training",
                "authored wall gameplay tag") &&
         expect(decoded.envelope.authoredRoom.walls[0].locked, "authored wall locked") &&
         expect(decoded.envelope.authoredRoom.walls[0].hidden, "authored wall hidden") &&
         expect(decoded.envelope.authoredRoom.markers.size() == 1U,
                "authored marker count") &&
         expect(decoded.envelope.authoredRoom.markers[0].id ==
                    "marker_treasure_r2_c4",
                "authored marker id") &&
         expect(decoded.envelope.authoredRoom.markers[0].tag == "treasure",
                "authored marker tag") &&
         expect(decoded.envelope.authoredRoom.markers[0].glyph == "$",
                "authored marker glyph") &&
         expect(decoded.envelope.authoredRoom.markers[0].row == 2U,
                "authored marker row") &&
         expect(decoded.envelope.authoredRoom.markers[0].column == 4U,
                "authored marker column") &&
         expect(decoded.envelope.authoredRoom.markers[0].positionMeters.x == 4.0F,
                "authored marker x") &&
         expect(decoded.envelope.authoredRoom.markers[0].sourceLine == 3U,
                "authored marker source line") &&
         expect(decoded.envelope.authoredRoom.markers[0].sourceColumn == 5U,
                "authored marker source column") &&
         expect(!oldDecoded.envelope.authoredRoom.present, "old saves omit authored room");
}

bool postLoadCommandIdDoesNotCollide() {
  iggy3d::Session source = completedCurrentLoopSession();
  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(source.state());
  iggy3d::Session loaded = makeSession();
  const iggy3d::LoadStateResult load =
      iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText, compatibilityFor(saved.envelope));
  bool ok = expect(load.status == iggy3d::SaveLoadStatus::Ok, "load before post command");

  const iggy3d::SessionCommandResult move =
      loaded.submitCommand(submittedMove({2.0F, 0.0F, 1.0F}));
  ok = ok && expect(move.appendStatus == iggy3d::CommandLogAppendStatus::Ok, "post append ok") &&
       expect(move.command.commandId == 4U, "post command id") &&
       expect(move.command.sequence == 4U, "post sequence") &&
       expect(loaded.state().nextCommandId == 5U, "post cursor advanced");
  return ok;
}

bool invalidCursorIsRejected() {
  iggy3d::Session source = completedCurrentLoopSession();
  iggy3d::SaveStateResult saved = iggy3d::saveSessionState(source.state());
  saved.envelope.session.nextCommandId = 1;
  iggy3d::Session destination = makeSession();
  const iggy3d::StateHashValue before = destination.stateHash();
  const iggy3d::LoadStateResult load =
      iggy3d::loadEnvelopeIntoSession(destination, saved.envelope, compatibilityFor(saved.envelope));
  return expect(load.status == iggy3d::SaveLoadStatus::InvalidEnvelope, "invalid cursor status") &&
         expect(destination.stateHash() == before, "invalid cursor unchanged");
}

bool duplicateCommandIdIsRejectedByCommandLogRestore() {
  iggy3d::Session source = completedCurrentLoopSession();
  iggy3d::SaveStateResult saved = iggy3d::saveSessionState(source.state());
  saved.envelope.commandLog.records[1].commandId = saved.envelope.commandLog.records[0].commandId;
  iggy3d::Session destination = makeSession();
  const iggy3d::LoadStateResult load =
      iggy3d::loadEnvelopeIntoSession(destination, saved.envelope, compatibilityFor(saved.envelope));
  return expect(load.status == iggy3d::SaveLoadStatus::InvalidCommandLog, "duplicate id status");
}

bool malformedEncodedSaveFailsDecodeAndLeavesDestinationUnchanged() {
  iggy3d::Session destination = makeSession();
  const iggy3d::StateHashValue before = destination.stateHash();
  const iggy3d::SaveStateResult saved = iggy3d::saveSessionState(destination.state());
  const iggy3d::LoadStateResult load = iggy3d::loadEncodedSaveIntoSession(
      destination, "iggy3d.save_envelope.v1\nmetadata.schemaVersion=bad\n",
      compatibilityFor(saved.envelope));
  return expect(load.status == iggy3d::SaveLoadStatus::DecodeFailed, "decode failure status") &&
         expect(destination.stateHash() == before, "decode failure unchanged");
}

bool attackDamageAndCombatRoundTrip() {
  iggy3d::Session session = makeSession();
  (void)session.submitCommand(submittedMove({2.0F, 0.0F, 1.0F}));
  (void)session.tick();
  const iggy3d::SessionCommandResult attack = session.submitCommand(submittedAttack());
  (void)session.tick();
  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(session.state());
  bool ok = expect(attack.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                   "attack accepted") &&
            expect(saved.status == iggy3d::SaveLoadStatus::Ok, "attack save status") &&
            expect(saved.envelope.commandLog.records.size() == 2U, "attack command count") &&
            expect(saved.envelope.commandLog.records[1].attackDamage == 3,
                   "attack damage envelope") &&
            expect(saved.encodedSaveText.find("commandLog.record.1.attackDamage=3\n") !=
                       std::string::npos,
                   "attack damage encoded");
  iggy3d::Session loaded = makeSession();
  const iggy3d::LoadStateResult load =
      iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText, compatibilityFor(saved.envelope));
  ok = ok && expect(load.status == iggy3d::SaveLoadStatus::Ok, "attack load status") &&
       expect(loaded.stateHash() == session.stateHash(), "attack loaded hash") &&
       expect(loaded.state().combat.combatants[1].hitPoints == 0, "dummy hp loaded") &&
       expect(loaded.state().combat.combatants[1].defeated, "dummy defeated loaded");

  const std::string oldStyle = eraseFirst(saved.encodedSaveText,
                                          "commandLog.record.0.attackDamage=0\n");
  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(oldStyle);
  ok = ok && expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                    "old non attack missing damage decodes") &&
       expect(decoded.envelope.commandLog.records[0].attackDamage == 0,
              "old non attack damage default");
  return ok;
}

bool invalidCombatStateRejectedOnLoad() {
  iggy3d::Session source = makeSession();
  iggy3d::SaveStateResult saved = iggy3d::saveSessionState(source.state());
  bool ok = true;

  iggy3d::Session destination = makeSession();
  iggy3d::SaveEnvelope bad = saved.envelope;
  bad.combat.combatants[0].entity = {99};
  iggy3d::LoadStateResult load =
      iggy3d::loadEnvelopeIntoSession(destination, bad, compatibilityFor(saved.envelope));
  ok = ok && expect(load.status == iggy3d::SaveLoadStatus::InvalidReference,
                    "missing combat entity rejected");

  destination = makeSession();
  bad = saved.envelope;
  bad.combat.combatants[1].entity = bad.combat.combatants[0].entity;
  load = iggy3d::loadEnvelopeIntoSession(destination, bad, compatibilityFor(saved.envelope));
  ok = ok && expect(load.status == iggy3d::SaveLoadStatus::InvalidReference,
                    "duplicate combat entity rejected");

  destination = makeSession();
  bad = saved.envelope;
  bad.combat.combatants[1].hitPoints = 0;
  bad.combat.combatants[1].defeated = false;
  load = iggy3d::loadEnvelopeIntoSession(destination, bad, compatibilityFor(saved.envelope));
  ok = ok && expect(load.status == iggy3d::SaveLoadStatus::InvalidReference,
                    "combat invariant rejected");
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = envelopeMappingPreservesDurableState() && ok;
  ok = encodedSaveRoundtripLoadsFreshSession() && ok;
  ok = interactionRequiredItemFactsRoundTripThroughSaveCodec() && ok;
  ok = productMetadataRoundTripsThroughSaveCodec() && ok;
  ok = oldSaveWithoutProductMetadataStillDecodes() && ok;
  ok = encodedSaveRoundtripPreservesRuntimeConfig() && ok;
  ok = authoredRoomSectionRoundTripsThroughSaveCodec() && ok;
  ok = postLoadCommandIdDoesNotCollide() && ok;
  ok = invalidCursorIsRejected() && ok;
  ok = duplicateCommandIdIsRejectedByCommandLogRestore() && ok;
  ok = malformedEncodedSaveFailsDecodeAndLeavesDestinationUnchanged() && ok;
  ok = attackDamageAndCombatRoundTrip() && ok;
  ok = invalidCombatStateRejectedOnLoad() && ok;
  return ok ? 0 : 1;
}
