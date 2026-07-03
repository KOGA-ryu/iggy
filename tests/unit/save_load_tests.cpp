#include "runtime/ai/NpcAlertSystem.hpp"
#include "runtime/inventory/InventorySystem.hpp"
#include "runtime/objective/ObjectiveSystem.hpp"
#include "runtime/replay/StateHash.hpp"
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

std::string eraseAllLinesContaining(std::string text, std::string_view prefix) {
  std::string previous;
  while (previous != text) {
    previous = text;
    text = eraseLineStartingWith(text, prefix);
  }
  return text;
}

std::string replaceFirst(std::string text,
                         std::string_view value,
                         std::string_view replacement) {
  const std::size_t pos = text.find(value);
  if (pos != std::string::npos) {
    text.replace(pos, value.size(), replacement);
  }
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

iggy3d::AiActorState authoredAiActorState() {
  iggy3d::AiActorState actor;
  actor.actor = {4};
  actor.nextDecisionTick = 17;
  actor.deterministicPolicy = 3;
  actor.enabled = false;
  actor.behaviorProfileId = "passive";
  actor.target = {1};
  actor.behavior = iggy3d::AiBehaviorKind::Returning;
  actor.lastIntent = iggy3d::AiIntentKind::ReturnToAnchor;
  actor.cooldownTicksRemaining = 5;
  actor.hasHomePosition = true;
  actor.homePosition = {2.0F, 0.0F, 4.0F};
  actor.homeStableName = "guard_post_alpha";
  actor.leashRadiusMeters = 6.0F;
  actor.returnRadiusMeters = 1.0F;
  actor.homeToleranceMeters = 0.25F;
  return actor;
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
         expect(envelope.session.nextCommandId == session.state().nextCommandId,
                "next command id saved") &&
         expect(envelope.commandLog.records.size() == session.state().commandLog.records().size(),
                "command records saved") &&
         expect(envelope.commandLog.nextSequence == session.state().commandLog.nextSequence(),
                "next sequence saved") &&
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
  iggy3d::SaveAuthoredRoomObjectRecord object;
  object.id = "object_crate_r2_c3";
  object.assetId = "wood_crate_proxy";
  object.storyIndex = 3;
  object.positionMeters = {3.0F, 0.4F, 2.0F};
  object.sizeMeters = {0.8F, 0.8F, 0.8F};
  object.yawDegrees = 90.0F;
  object.semantics.materialId = "wood_crate_proxy";
  object.semantics.blocksActor = true;
  object.semantics.blocksProjectile = true;
  object.semantics.traversalTags = {"object", "prop", "crate"};
  object.semantics.gameplayTags = {"object", "prop", "crate"};
  object.locked = true;
  object.hidden = true;
  object.glyph = "C";
  object.row = 2;
  object.column = 3;
  object.sourceLine = 3;
  object.sourceColumn = 4;
  saved.envelope.authoredRoom.objects.push_back(object);
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
         expect(encoded.encodedText.find(
                    "authoredRoom.object.0.id=object_crate_r2_c3\n") !=
                    std::string::npos,
                "authored object encoded") &&
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
         expect(decoded.envelope.authoredRoom.objects.size() == 1U,
                "authored object count") &&
         expect(decoded.envelope.authoredRoom.objects[0].id == "object_crate_r2_c3",
                "authored object id") &&
         expect(decoded.envelope.authoredRoom.objects[0].assetId == "wood_crate_proxy",
                "authored object asset") &&
         expect(decoded.envelope.authoredRoom.objects[0].storyIndex == 3,
                "authored object story") &&
         expect(decoded.envelope.authoredRoom.objects[0].positionMeters.y == 0.4F,
                "authored object y") &&
         expect(decoded.envelope.authoredRoom.objects[0].sizeMeters.x == 0.8F,
                "authored object size") &&
         expect(decoded.envelope.authoredRoom.objects[0].yawDegrees == 90.0F,
                "authored object yaw") &&
         expect(decoded.envelope.authoredRoom.objects[0].semantics.blocksActor,
                "authored object actor blocker") &&
         expect(decoded.envelope.authoredRoom.objects[0].semantics.blocksProjectile,
                "authored object projectile blocker") &&
         expect(decoded.envelope.authoredRoom.objects[0].semantics.traversalTags[2] ==
                    "crate",
                "authored object traversal tag") &&
         expect(decoded.envelope.authoredRoom.objects[0].glyph == "C",
                "authored object glyph") &&
         expect(decoded.envelope.authoredRoom.objects[0].row == 2U,
                "authored object row") &&
         expect(decoded.envelope.authoredRoom.objects[0].column == 3U,
                "authored object column") &&
         expect(decoded.envelope.authoredRoom.objects[0].sourceLine == 3U,
                "authored object source line") &&
         expect(decoded.envelope.authoredRoom.objects[0].sourceColumn == 4U,
                "authored object source column") &&
         expect(decoded.envelope.authoredRoom.objects[0].locked,
                "authored object locked") &&
         expect(decoded.envelope.authoredRoom.objects[0].hidden,
                "authored object hidden") &&
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

  const iggy3d::CommandId expectedCommandId = loaded.state().nextCommandId;
  const iggy3d::CommandSequence expectedSequence =
      loaded.state().commandLog.nextSequence();
  const iggy3d::SessionCommandResult move =
      loaded.submitCommand(submittedMove({2.0F, 0.0F, 1.0F}));
  ok = ok && expect(move.appendStatus == iggy3d::CommandLogAppendStatus::Ok, "post append ok") &&
       expect(move.command.commandId == expectedCommandId, "post command id") &&
       expect(move.command.sequence == expectedSequence, "post sequence") &&
       expect(loaded.state().nextCommandId == expectedCommandId + 1U,
              "post cursor advanced");
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
  const iggy3d::SaveCommandRecord* savedAttack = nullptr;
  for (const iggy3d::SaveCommandRecord& record : saved.envelope.commandLog.records) {
    if (record.commandId == attack.command.commandId) {
      savedAttack = &record;
      break;
    }
  }

  bool ok = expect(attack.command.admission == iggy3d::CommandAdmissionStatus::Accepted,
                   "attack accepted") &&
            expect(saved.status == iggy3d::SaveLoadStatus::Ok, "attack save status") &&
            expect(savedAttack != nullptr &&
                       savedAttack->kind == iggy3d::CommandKind::Attack,
                   "attack command saved") &&
            expect(savedAttack != nullptr && savedAttack->attackDamage == 3,
                   "attack damage envelope") &&
            expect(saved.encodedSaveText.find("attackDamage=3\n") !=
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

bool defaultAiActorStateHasPassiveDefaults() {
  const iggy3d::AiActorState actor;
  return expect(!iggy3d::isValid(actor.target), "default ai target invalid") &&
         expect(actor.behavior == iggy3d::AiBehaviorKind::Idle,
                "default ai behavior idle") &&
         expect(actor.behaviorProfileId == "default",
                "default ai behavior profile default") &&
         expect(actor.lastIntent == iggy3d::AiIntentKind::None,
                "default ai last intent none") &&
         expect(actor.cooldownTicksRemaining == 0U, "default ai cooldown zero") &&
         expect(!actor.hasHomePosition, "default ai no home") &&
         expect(iggy3d::nearlyEqual(actor.homePosition, iggy3d::Vec3{}),
                "default ai home position zero") &&
         expect(actor.homeStableName.empty(), "default ai home stable name empty") &&
         expect(actor.leashRadiusMeters == 0.0F, "default ai leash zero") &&
         expect(actor.returnRadiusMeters == 0.0F, "default ai return zero") &&
         expect(actor.homeToleranceMeters == 0.0F, "default ai tolerance zero") &&
         expect(actor.enabled, "default ai enabled");
}

bool aiStateRoundTripsThroughSaveLoadAndCodec() {
  iggy3d::Session source = makeSession();
  iggy3d::SessionState sourceState = source.state();
  sourceState.ai.actors.push_back(authoredAiActorState());

  const iggy3d::SaveStateResult saved =
      iggy3d::saveSessionStateEncoded(sourceState);
  bool ok = expect(saved.status == iggy3d::SaveLoadStatus::Ok,
                   "ai save status") &&
            expect(saved.envelope.ai.actors.size() == 1U,
                   "ai envelope count");

  const iggy3d::SaveAiActorRecord& record = saved.envelope.ai.actors.front();
  ok = ok && expect(record.actor == iggy3d::EntityId{4}, "ai actor envelope") &&
       expect(record.nextDecisionTick == 17U, "ai next decision envelope") &&
       expect(record.deterministicPolicy == 3U, "ai policy envelope") &&
       expect(!record.enabled, "ai enabled envelope") &&
       expect(record.behaviorProfileId == "passive", "ai profile envelope") &&
       expect(record.target == iggy3d::EntityId{1}, "ai target envelope") &&
       expect(record.behavior == iggy3d::AiBehaviorKind::Returning,
              "ai behavior envelope") &&
       expect(record.lastIntent == iggy3d::AiIntentKind::ReturnToAnchor,
              "ai intent envelope") &&
       expect(record.cooldownTicksRemaining == 5U, "ai cooldown envelope") &&
       expect(record.hasHomePosition, "ai has home envelope") &&
       expect(iggy3d::nearlyEqual(record.homePosition, iggy3d::Vec3{2.0F, 0.0F, 4.0F}),
              "ai home position envelope") &&
       expect(record.homeStableName == "guard_post_alpha", "ai home name envelope") &&
       expect(record.leashRadiusMeters == 6.0F, "ai leash envelope") &&
       expect(record.returnRadiusMeters == 1.0F, "ai return envelope") &&
       expect(record.homeToleranceMeters == 0.25F, "ai tolerance envelope") &&
       expect(saved.encodedSaveText.find("ai.actor.0.target=1\n") !=
                  std::string::npos,
              "ai target encoded") &&
       expect(saved.encodedSaveText.find("ai.actor.0.behavior=returning\n") !=
                  std::string::npos,
              "ai behavior encoded") &&
       expect(saved.encodedSaveText.find(
                  "ai.actor.0.lastIntent=return_to_anchor\n") !=
                  std::string::npos,
              "ai intent encoded") &&
       expect(saved.encodedSaveText.find("ai.actor.0.behavior_profile_id=passive\n") !=
                  std::string::npos,
              "ai profile encoded") &&
       expect(saved.encodedSaveText.find(
                  "ai.actor.0.cooldownTicksRemaining=5\n") !=
                  std::string::npos,
              "ai cooldown encoded") &&
       expect(saved.encodedSaveText.find("ai.actor.0.hasHomePosition=true\n") !=
                  std::string::npos,
              "ai has home encoded") &&
       expect(saved.encodedSaveText.find("ai.actor.0.homePosition=2.000,0.000,4.000\n") !=
                  std::string::npos,
              "ai home position encoded") &&
       expect(saved.encodedSaveText.find("ai.actor.0.homeStableName=guard_post_alpha\n") !=
                  std::string::npos,
              "ai home name encoded") &&
       expect(saved.encodedSaveText.find("ai.actor.0.leashRadiusMeters=6.000\n") !=
                  std::string::npos,
              "ai leash encoded") &&
       expect(saved.encodedSaveText.find("ai.actor.0.returnRadiusMeters=1.000\n") !=
                  std::string::npos,
              "ai return encoded") &&
       expect(saved.encodedSaveText.find("ai.actor.0.homeToleranceMeters=0.250\n") !=
                  std::string::npos,
              "ai tolerance encoded");

  iggy3d::Session loaded = makeSession();
  const iggy3d::LoadStateResult load =
      iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText,
                                         compatibilityFor(saved.envelope));
  const iggy3d::AiActorState* loadedActor =
      loaded.state().ai.actors.empty() ? nullptr : &loaded.state().ai.actors.front();
  ok = ok && expect(load.status == iggy3d::SaveLoadStatus::Ok,
                    "ai load status") &&
       expect(loaded.state().ai.actors.size() == 1U, "ai loaded count") &&
       expect(loadedActor != nullptr && loadedActor->actor == iggy3d::EntityId{4},
              "ai actor loaded") &&
       expect(loadedActor != nullptr && loadedActor->nextDecisionTick == 17U,
              "ai next decision loaded") &&
       expect(loadedActor != nullptr && loadedActor->deterministicPolicy == 3U,
              "ai policy loaded") &&
       expect(loadedActor != nullptr && !loadedActor->enabled, "ai enabled loaded") &&
       expect(loadedActor != nullptr && loadedActor->behaviorProfileId == "passive",
              "ai profile loaded") &&
       expect(loadedActor != nullptr && loadedActor->target == iggy3d::EntityId{1},
              "ai target loaded") &&
       expect(loadedActor != nullptr &&
                  loadedActor->behavior == iggy3d::AiBehaviorKind::Returning,
              "ai behavior loaded") &&
       expect(loadedActor != nullptr &&
                  loadedActor->lastIntent == iggy3d::AiIntentKind::ReturnToAnchor,
              "ai intent loaded") &&
       expect(loadedActor != nullptr && loadedActor->cooldownTicksRemaining == 5U,
              "ai cooldown loaded") &&
       expect(loadedActor != nullptr && loadedActor->hasHomePosition,
              "ai has home loaded") &&
       expect(loadedActor != nullptr &&
                  iggy3d::nearlyEqual(loadedActor->homePosition, iggy3d::Vec3{2.0F, 0.0F, 4.0F}),
              "ai home position loaded") &&
       expect(loadedActor != nullptr && loadedActor->homeStableName == "guard_post_alpha",
              "ai home name loaded") &&
       expect(loadedActor != nullptr && loadedActor->leashRadiusMeters == 6.0F,
              "ai leash loaded") &&
       expect(loadedActor != nullptr && loadedActor->returnRadiusMeters == 1.0F,
              "ai return loaded") &&
       expect(loadedActor != nullptr && loadedActor->homeToleranceMeters == 0.25F,
              "ai tolerance loaded") &&
       expect(loaded.stateHash() == iggy3d::computeStateHash(sourceState),
              "ai loaded hash");

  iggy3d::SessionState oldProfileState = source.state();
  iggy3d::AiActorState oldProfileActor = authoredAiActorState();
  oldProfileActor.behaviorProfileId = "default";
  oldProfileState.ai.actors.push_back(oldProfileActor);
  const iggy3d::SaveStateResult oldProfileSaved =
      iggy3d::saveSessionStateEncoded(oldProfileState);
  std::string oldProfileStyle = oldProfileSaved.encodedSaveText;
  oldProfileStyle =
      eraseLineStartingWith(oldProfileStyle, "ai.actor.0.behavior_profile_id=");
  const iggy3d::SaveDecodeResult oldProfileDecoded =
      iggy3d::decodeSaveEnvelope(oldProfileStyle);
  iggy3d::Session oldProfileLoaded = makeSession();
  const iggy3d::LoadStateResult oldProfileLoad =
      iggy3d::loadEncodedSaveIntoSession(
          oldProfileLoaded, oldProfileStyle, compatibilityFor(oldProfileDecoded.envelope));
  const iggy3d::AiActorState* oldProfileLoadedActor =
      oldProfileLoaded.state().ai.actors.empty()
          ? nullptr
          : &oldProfileLoaded.state().ai.actors.front();
  ok = ok && expect(oldProfileDecoded.status == iggy3d::SaveCodecStatus::Ok,
                    "old profile ai save decodes") &&
       expect(!oldProfileDecoded.envelope.ai.actors.empty() &&
                  oldProfileDecoded.envelope.ai.actors.front().behaviorProfileId ==
                      "default",
              "old profile decode default") &&
       expect(oldProfileLoad.status == iggy3d::SaveLoadStatus::Ok,
              "old profile ai save loads") &&
       expect(oldProfileLoadedActor != nullptr &&
                  oldProfileLoadedActor->behaviorProfileId == "default",
              "old profile loaded default");

  iggy3d::SessionState oldGuardState = source.state();
  iggy3d::AiActorState oldGuardActor = authoredAiActorState();
  oldGuardActor.hasHomePosition = false;
  oldGuardActor.homePosition = {};
  oldGuardActor.homeStableName.clear();
  oldGuardActor.leashRadiusMeters = 0.0F;
  oldGuardActor.returnRadiusMeters = 0.0F;
  oldGuardActor.homeToleranceMeters = 0.0F;
  oldGuardState.ai.actors.push_back(oldGuardActor);
  const iggy3d::SaveStateResult oldGuardSaved =
      iggy3d::saveSessionStateEncoded(oldGuardState);
  std::string oldGuardStyle = oldGuardSaved.encodedSaveText;
  oldGuardStyle = eraseLineStartingWith(oldGuardStyle, "ai.actor.0.hasHomePosition=");
  oldGuardStyle = eraseLineStartingWith(oldGuardStyle, "ai.actor.0.homePosition=");
  oldGuardStyle = eraseLineStartingWith(oldGuardStyle, "ai.actor.0.homeStableName=");
  oldGuardStyle = eraseLineStartingWith(oldGuardStyle, "ai.actor.0.leashRadiusMeters=");
  oldGuardStyle = eraseLineStartingWith(oldGuardStyle, "ai.actor.0.returnRadiusMeters=");
  oldGuardStyle = eraseLineStartingWith(oldGuardStyle, "ai.actor.0.homeToleranceMeters=");
  const iggy3d::SaveDecodeResult oldGuardDecoded =
      iggy3d::decodeSaveEnvelope(oldGuardStyle);
  iggy3d::Session oldGuardLoaded = makeSession();
  const iggy3d::LoadStateResult oldGuardLoad =
      iggy3d::loadEncodedSaveIntoSession(
          oldGuardLoaded, oldGuardStyle, compatibilityFor(oldGuardDecoded.envelope));
  const iggy3d::AiActorState* oldGuardLoadedActor =
      oldGuardLoaded.state().ai.actors.empty()
          ? nullptr
          : &oldGuardLoaded.state().ai.actors.front();
  ok = ok && expect(oldGuardDecoded.status == iggy3d::SaveCodecStatus::Ok,
                    "old guard ai save decodes") &&
       expect(!oldGuardDecoded.envelope.ai.actors.empty() &&
                  !oldGuardDecoded.envelope.ai.actors.front().hasHomePosition,
              "old guard decode has home default") &&
       expect(!oldGuardDecoded.envelope.ai.actors.empty() &&
                  oldGuardDecoded.envelope.ai.actors.front().homeStableName.empty(),
              "old guard decode home name default") &&
       expect(oldGuardLoad.status == iggy3d::SaveLoadStatus::Ok,
              "old guard ai save loads") &&
       expect(oldGuardLoadedActor != nullptr && !oldGuardLoadedActor->hasHomePosition,
              "old guard loaded has home default") &&
       expect(oldGuardLoadedActor != nullptr && oldGuardLoadedActor->homeStableName.empty(),
              "old guard loaded home name default");

  std::string oldStyle = saved.encodedSaveText;
  oldStyle = eraseLineStartingWith(oldStyle, "ai.actor.0.behavior_profile_id=");
  oldStyle = eraseLineStartingWith(oldStyle, "ai.actor.0.target=");
  oldStyle = eraseLineStartingWith(oldStyle, "ai.actor.0.behavior=");
  oldStyle = eraseLineStartingWith(oldStyle, "ai.actor.0.lastIntent=");
  oldStyle = eraseLineStartingWith(oldStyle, "ai.actor.0.cooldownTicksRemaining=");
  oldStyle = eraseLineStartingWith(oldStyle, "ai.actor.0.hasHomePosition=");
  oldStyle = eraseLineStartingWith(oldStyle, "ai.actor.0.homePosition=");
  oldStyle = eraseLineStartingWith(oldStyle, "ai.actor.0.homeStableName=");
  oldStyle = eraseLineStartingWith(oldStyle, "ai.actor.0.leashRadiusMeters=");
  oldStyle = eraseLineStartingWith(oldStyle, "ai.actor.0.returnRadiusMeters=");
  oldStyle = eraseLineStartingWith(oldStyle, "ai.actor.0.homeToleranceMeters=");
  const iggy3d::SaveDecodeResult oldDecoded = iggy3d::decodeSaveEnvelope(oldStyle);
  const iggy3d::SaveAiActorRecord* oldActor =
      oldDecoded.envelope.ai.actors.empty() ? nullptr : &oldDecoded.envelope.ai.actors.front();
  ok = ok && expect(oldDecoded.status == iggy3d::SaveCodecStatus::Ok,
                    "old ai save decodes") &&
       expect(oldActor != nullptr && oldActor->behaviorProfileId == "default",
              "old ai profile default") &&
       expect(oldActor != nullptr && !iggy3d::isValid(oldActor->target),
              "old ai target default") &&
       expect(oldActor != nullptr &&
                  oldActor->behavior == iggy3d::AiBehaviorKind::Idle,
              "old ai behavior default") &&
       expect(oldActor != nullptr &&
                  oldActor->lastIntent == iggy3d::AiIntentKind::None,
              "old ai intent default") &&
       expect(oldActor != nullptr && oldActor->cooldownTicksRemaining == 0U,
              "old ai cooldown default") &&
       expect(oldActor != nullptr && !oldActor->hasHomePosition,
              "old ai has home default") &&
       expect(oldActor != nullptr &&
                  iggy3d::nearlyEqual(oldActor->homePosition, iggy3d::Vec3{}),
              "old ai home position default") &&
       expect(oldActor != nullptr && oldActor->homeStableName.empty(),
              "old ai home name default") &&
       expect(oldActor != nullptr && oldActor->leashRadiusMeters == 0.0F,
              "old ai leash default") &&
       expect(oldActor != nullptr && oldActor->returnRadiusMeters == 0.0F,
              "old ai return default") &&
       expect(oldActor != nullptr && oldActor->homeToleranceMeters == 0.0F,
              "old ai tolerance default");

  const std::string badBehavior = replaceFirst(saved.encodedSaveText,
                                               "ai.actor.0.behavior=returning\n",
                                               "ai.actor.0.behavior=confused\n");
  const iggy3d::SaveDecodeResult badDecoded =
      iggy3d::decodeSaveEnvelope(badBehavior);
  const std::string badIntent = replaceFirst(saved.encodedSaveText,
                                             "ai.actor.0.lastIntent=return_to_anchor\n",
                                             "ai.actor.0.lastIntent=wander_home\n");
  const iggy3d::SaveDecodeResult badIntentDecoded =
      iggy3d::decodeSaveEnvelope(badIntent);
  return ok && expect(badDecoded.status == iggy3d::SaveCodecStatus::InvalidEnum,
                      "invalid ai enum rejected") &&
         expect(badDecoded.diagnosticKey == "ai.actor.0.behavior",
                "invalid ai enum key") &&
         expect(badIntentDecoded.status == iggy3d::SaveCodecStatus::InvalidEnum,
                "invalid ai intent enum rejected") &&
         expect(badIntentDecoded.diagnosticKey == "ai.actor.0.lastIntent",
                "invalid ai intent enum key");
}

bool aiStateChangesParticipateInHash() {
  iggy3d::Session source = makeSession();
  iggy3d::SessionState state = source.state();
  state.ai.actors.push_back(authoredAiActorState());
  const iggy3d::StateHashValue base = iggy3d::computeStateHash(state);

  iggy3d::SessionState changed = state;
  changed.ai.actors[0].behavior = iggy3d::AiBehaviorKind::Alert;
  bool ok = expect(iggy3d::computeStateHash(changed) != base,
                   "ai behavior changes hash");

  changed = state;
  changed.ai.actors[0].lastIntent = iggy3d::AiIntentKind::AttackTarget;
  ok = ok && expect(iggy3d::computeStateHash(changed) != base,
                    "ai intent changes hash");

  changed = state;
  changed.ai.actors[0].behaviorProfileId = "melee_training";
  ok = ok && expect(iggy3d::computeStateHash(changed) != base,
                    "ai profile changes hash");

  changed = state;
  changed.ai.actors[0].target = {2};
  ok = ok && expect(iggy3d::computeStateHash(changed) != base,
                    "ai target changes hash");

  changed = state;
  changed.ai.actors[0].cooldownTicksRemaining = 6;
  ok = ok && expect(iggy3d::computeStateHash(changed) != base,
                    "ai cooldown changes hash");

  changed = state;
  changed.ai.actors[0].hasHomePosition = false;
  ok = ok && expect(iggy3d::computeStateHash(changed) != base,
                    "ai has home changes hash");

  changed = state;
  changed.ai.actors[0].homePosition.x = 3.0F;
  ok = ok && expect(iggy3d::computeStateHash(changed) != base,
                    "ai home position changes hash");

  changed = state;
  changed.ai.actors[0].homeStableName = "guard_post_beta";
  ok = ok && expect(iggy3d::computeStateHash(changed) != base,
                    "ai home name changes hash");

  changed = state;
  changed.ai.actors[0].leashRadiusMeters = 7.0F;
  ok = ok && expect(iggy3d::computeStateHash(changed) != base,
                    "ai leash changes hash");

  changed = state;
  changed.ai.actors[0].returnRadiusMeters = 1.5F;
  ok = ok && expect(iggy3d::computeStateHash(changed) != base,
                    "ai return radius changes hash");

  changed = state;
  changed.ai.actors[0].homeToleranceMeters = 0.5F;
  return ok && expect(iggy3d::computeStateHash(changed) != base,
                      "ai home tolerance changes hash");
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

// a2 commit 1: a guard's patrol route + cursor survive save->load, a routeless guard stays
// routeless, and an old envelope missing the patrol keys loads to the empty-route defaults.
bool patrolRouteRoundTripsThroughSaveLoad() {
  iggy3d::Session source = makeSession();
  iggy3d::SessionState sourceState = source.state();
  iggy3d::AiActorState actor;
  actor.actor = {2};
  actor.behaviorProfileId = "default";
  // A non-3-decimal waypoint coord proves the lossless (to_chars) waypoint encoder.
  actor.patrolWaypoints = {{1.234567F, 0.0F, 2.5F}, {3.0F, 0.0F, 4.0F}, {0.25F, 0.0F, 7.75F}};
  actor.patrolMode = iggy3d::PatrolMode::PingPong;
  actor.patrolTargetIndex = 1;
  actor.patrolForward = false;
  sourceState.ai.actors.push_back(actor);

  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(sourceState);
  bool ok = expect(saved.status == iggy3d::SaveLoadStatus::Ok, "patrol save status");
  const iggy3d::SaveAiActorRecord& record = saved.envelope.ai.actors.front();
  ok = ok && expect(record.patrolWaypoints.size() == 3U, "patrol count envelope") &&
       expect(record.patrolMode == iggy3d::PatrolMode::PingPong, "patrol mode envelope") &&
       expect(record.patrolTargetIndex == 1U, "patrol index envelope") &&
       expect(!record.patrolForward, "patrol forward envelope") &&
       expect(saved.encodedSaveText.find("ai.actor.0.patrolWaypoint.count=3\n") !=
                  std::string::npos, "patrol count encoded") &&
       expect(saved.encodedSaveText.find("ai.actor.0.patrolMode=ping_pong\n") !=
                  std::string::npos, "patrol mode encoded");

  iggy3d::Session loaded = makeSession();
  (void)iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText,
                                           compatibilityFor(saved.envelope));
  const iggy3d::AiActorState* la =
      loaded.state().ai.actors.empty() ? nullptr : &loaded.state().ai.actors.front();
  ok = ok && expect(la != nullptr && la->patrolWaypoints.size() == 3U, "patrol loaded count") &&
       // Exact-float equality proves lossless: 1.234567 is not 3-decimal-representable.
       expect(la != nullptr && la->patrolWaypoints[0].x == 1.234567F &&
                  la->patrolWaypoints[0].z == 2.5F, "patrol waypoint 0 exact") &&
       expect(la != nullptr && la->patrolWaypoints[2].z == 7.75F, "patrol waypoint 2 exact") &&
       expect(la != nullptr && la->patrolMode == iggy3d::PatrolMode::PingPong, "patrol mode loaded") &&
       expect(la != nullptr && la->patrolTargetIndex == 1U, "patrol index loaded") &&
       expect(la != nullptr && !la->patrolForward, "patrol forward loaded");

  // A routeless guard round-trips to the empty-route defaults.
  iggy3d::SessionState freshState = makeSession().state();
  iggy3d::AiActorState fresh;
  fresh.actor = {2};
  fresh.behaviorProfileId = "default";
  freshState.ai.actors.push_back(fresh);
  const iggy3d::SaveStateResult freshSaved = iggy3d::saveSessionStateEncoded(freshState);
  iggy3d::Session freshLoaded = makeSession();
  (void)iggy3d::loadEncodedSaveIntoSession(freshLoaded, freshSaved.encodedSaveText,
                                           compatibilityFor(freshSaved.envelope));
  const iggy3d::AiActorState* fl =
      freshLoaded.state().ai.actors.empty() ? nullptr : &freshLoaded.state().ai.actors.front();
  ok = ok && expect(fl != nullptr && fl->patrolWaypoints.empty() &&
                        fl->patrolMode == iggy3d::PatrolMode::Loop && fl->patrolForward,
                    "routeless guard stays routeless");

  // Backward compat: an old envelope with NO patrol keys decodes to the defaults (no failure).
  const std::string oldStyle =
      eraseAllLinesContaining(saved.encodedSaveText, "ai.actor.0.patrol");
  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(oldStyle);
  ok = ok && expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                    "old save without patrol keys decodes") &&
       expect(decoded.envelope.ai.actors.front().patrolWaypoints.empty(),
              "old save patrol route defaults empty") &&
       expect(decoded.envelope.ai.actors.front().patrolMode == iggy3d::PatrolMode::Loop,
              "old save patrol mode defaults loop") &&
       expect(decoded.envelope.ai.actors.front().patrolForward,
              "old save patrol forward defaults true");
  return ok;
}

// a2 commit 2: a mid-engagement guard's alert FSM, last-known memory, and facing survive
// save->load with EXACT-float fidelity (the lossless encoder pin — a hash compare would pass
// trivially and is forbidden as the fidelity proof); a fresh guard stays fresh; old envelopes load.
bool alertMemoryFacingRoundTripsThroughSaveLoad() {
  iggy3d::Session source = makeSession();
  iggy3d::SessionState sourceState = source.state();
  iggy3d::AiActorState actor;
  actor.actor = {2};
  actor.behaviorProfileId = "default";
  // Non-3-decimal values: formatFloat (fixed 3-decimal) would lose these; formatFloatLossless keeps them.
  actor.alertLevel = 0.4237123F;
  actor.lastRiseTick = 4242;
  actor.maxAlertIndexThisEngagement = 3;
  actor.graceUntilTick = 4300;
  actor.graceThreshold = 0.2617319F;
  actor.graceCount = 2;
  actor.lastKnownTargetPosition = {3.1415927F, 0.0F, 2.7182817F};
  actor.lastKnownTargetTick = 4230;
  actor.hasLastKnownTarget = true;
  actor.investigateDwellTicks = 17;
  actor.facingDirection = {0.6427876F, 0.0F, 0.7660444F};  // ~40 deg, non-3-decimal
  sourceState.ai.actors.push_back(actor);

  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(sourceState);
  iggy3d::Session loaded = makeSession();
  (void)iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText,
                                           compatibilityFor(saved.envelope));
  const iggy3d::AiActorState* la =
      loaded.state().ai.actors.empty() ? nullptr : &loaded.state().ai.actors.front();

  const iggy3d::AlertProfile profile;  // default band tuning; band is derived from the exact level
  bool ok = expect(la != nullptr, "alert actor loaded");
  ok = ok &&
       // EXACT-float equality (==, NOT nearlyEqual, NOT a hash compare) proves the lossless encoder.
       expect(la != nullptr && la->alertLevel == 0.4237123F, "alertLevel exact") &&
       expect(la != nullptr && la->graceThreshold == 0.2617319F, "graceThreshold exact") &&
       expect(la != nullptr && la->lastKnownTargetPosition.x == 3.1415927F &&
                  la->lastKnownTargetPosition.z == 2.7182817F, "lastKnown position exact") &&
       expect(la != nullptr && la->facingDirection.x == 0.6427876F &&
                  la->facingDirection.z == 0.7660444F, "facingDirection exact") &&
       expect(la != nullptr && iggy3d::alertBandIndex(la->alertLevel, profile) ==
                                   iggy3d::alertBandIndex(0.4237123F, profile),
              "alert band survives (derived from the exact level)") &&
       expect(la != nullptr && la->lastRiseTick == 4242U, "lastRiseTick survives") &&
       expect(la != nullptr && la->maxAlertIndexThisEngagement == 3U, "maxAlertIndex survives") &&
       expect(la != nullptr && la->graceUntilTick == 4300U && la->graceCount == 2U,
              "grace window survives") &&
       expect(la != nullptr && la->lastKnownTargetTick == 4230U, "lastKnown tick survives") &&
       expect(la != nullptr && la->hasLastKnownTarget, "hasLastKnownTarget survives") &&
       expect(la != nullptr && la->investigateDwellTicks == 17U, "dwell survives");

  // A fresh guard round-trips to defaults (amnesiac-but-valid).
  iggy3d::SessionState freshState = makeSession().state();
  iggy3d::AiActorState fresh;
  fresh.actor = {2};
  fresh.behaviorProfileId = "default";
  freshState.ai.actors.push_back(fresh);
  const iggy3d::SaveStateResult freshSaved = iggy3d::saveSessionStateEncoded(freshState);
  iggy3d::Session freshLoaded = makeSession();
  (void)iggy3d::loadEncodedSaveIntoSession(freshLoaded, freshSaved.encodedSaveText,
                                           compatibilityFor(freshSaved.envelope));
  const iggy3d::AiActorState* fl =
      freshLoaded.state().ai.actors.empty() ? nullptr : &freshLoaded.state().ai.actors.front();
  ok = ok && expect(fl != nullptr && fl->alertLevel == 0.0F && !fl->hasLastKnownTarget &&
                        fl->investigateDwellTicks == 0U && fl->facingDirection.x == 0.0F &&
                        fl->facingDirection.z == 1.0F,
                    "fresh guard stays fresh");

  // Backward compat: an old envelope missing the commit-2 keys loads to the defaults.
  std::string oldStyle = eraseAllLinesContaining(saved.encodedSaveText, "ai.actor.0.alertLevel");
  oldStyle = eraseAllLinesContaining(oldStyle, "ai.actor.0.lastRiseTick");
  oldStyle = eraseAllLinesContaining(oldStyle, "ai.actor.0.maxAlertIndexThisEngagement");
  oldStyle = eraseAllLinesContaining(oldStyle, "ai.actor.0.graceUntilTick");
  oldStyle = eraseAllLinesContaining(oldStyle, "ai.actor.0.graceThreshold");
  oldStyle = eraseAllLinesContaining(oldStyle, "ai.actor.0.graceCount");
  oldStyle = eraseAllLinesContaining(oldStyle, "ai.actor.0.lastKnownTargetPosition");
  oldStyle = eraseAllLinesContaining(oldStyle, "ai.actor.0.lastKnownTargetTick");
  oldStyle = eraseAllLinesContaining(oldStyle, "ai.actor.0.hasLastKnownTarget");
  oldStyle = eraseAllLinesContaining(oldStyle, "ai.actor.0.investigateDwellTicks");
  oldStyle = eraseAllLinesContaining(oldStyle, "ai.actor.0.facingDirection");
  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(oldStyle);
  ok = ok && expect(decoded.status == iggy3d::SaveCodecStatus::Ok,
                    "old save without alert/memory/facing keys decodes") &&
       expect(decoded.envelope.ai.actors.front().alertLevel == 0.0F &&
                  !decoded.envelope.ai.actors.front().hasLastKnownTarget &&
                  decoded.envelope.ai.actors.front().facingDirection.z == 1.0F,
              "old save alert/memory/facing default to fresh");
  return ok;
}

}  // namespace

// MA1 s2 determinism: a Move with the sneak stance rides the (already-hashed) userData0 bit. This
// verifies the bit is deterministic, HASHED, and now SAVED -- the exact failure mode the persistence
// closes (a lost bit would make the post-load footstep louder + diverge the hash).
iggy3d::CommandRecord submittedSneakMove(iggy3d::Vec3 point) {
  iggy3d::CommandRecord command = submittedMove(point);
  command.payload.userData0 |= iggy3d::kMoveSneakBit;
  return command;
}

float lastFootstepLoudness(const iggy3d::Session& session) {
  const std::vector<iggy3d::SoundEvent>& events = session.state().transient.soundEvents;
  return events.empty() ? -1.0F : events.front().loudnessDb;
}

bool sneakBitIsHashedAndDeterministic() {
  iggy3d::Session a = makeSession();
  (void)a.submitCommand(submittedSneakMove({1.5F, 0.0F, 0.0F}));
  (void)a.tick();
  iggy3d::Session b = makeSession();
  (void)b.submitCommand(submittedSneakMove({1.5F, 0.0F, 0.0F}));
  (void)b.tick();
  iggy3d::Session walk = makeSession();
  (void)walk.submitCommand(submittedMove({1.5F, 0.0F, 0.0F}));
  (void)walk.tick();

  bool loggedBit = false;
  for (const iggy3d::CommandRecord& record : a.state().commandLog.records()) {
    if ((record.payload.userData0 & iggy3d::kMoveSneakBit) != 0) {
      loggedBit = true;
    }
  }
  return expect(loggedBit, "the sneak bit is recorded on the logged Move") &&
         expect(a.stateHash() == b.stateHash(),
                "an identical sneak sequence is deterministic (same hash)") &&
         expect(a.stateHash() != walk.stateHash(),
                "the sneak bit participates in the state hash (sneak vs walk diverge)");
}

bool sneakBitAndLoudnessSurviveSaveLoad() {
  iggy3d::Session live = makeSession();
  (void)live.submitCommand(submittedSneakMove({1.5F, 0.0F, 0.0F}));
  (void)live.tick();
  const iggy3d::SaveStateResult sneakSave = iggy3d::saveSessionStateEncoded(live.state());
  bool ok = expect(sneakSave.status == iggy3d::SaveLoadStatus::Ok, "mid-sneak save ok") &&
            expect(sneakSave.encodedSaveText.find("userData0") != std::string::npos,
                   "the sneak command persists userData0 on the wire");

  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(sneakSave.encodedSaveText);
  ok = ok && expect(decoded.status == iggy3d::SaveCodecStatus::Ok, "mid-sneak decode ok");
  bool decodedBit = false;
  for (const iggy3d::SaveCommandRecord& record : decoded.envelope.commandLog.records) {
    if ((record.userData0 & iggy3d::kMoveSneakBit) != 0) {
      decodedBit = true;
    }
  }
  ok = ok && expect(decodedBit, "the decoded command log carries the sneak bit");

  iggy3d::Session loaded = makeSession();
  const iggy3d::LoadStateResult load = iggy3d::loadEncodedSaveIntoSession(
      loaded, sneakSave.encodedSaveText, compatibilityFor(sneakSave.envelope));
  ok = ok && expect(load.status == iggy3d::SaveLoadStatus::Ok, "mid-sneak load ok") &&
       expect(loaded.stateHash() == live.stateHash(),
              "post-load hash matches -> the saved sneak bit restored byte-identically");

  // The next sneak footstep after load is as quiet as the never-saved run (profile carried + stance
  // intact). A lost bit would emit a LOUDER step here.
  (void)loaded.submitCommand(submittedSneakMove({3.0F, 0.0F, 0.0F}));
  (void)loaded.tick();
  const float loadedLoud = lastFootstepLoudness(loaded);
  (void)live.submitCommand(submittedSneakMove({3.0F, 0.0F, 0.0F}));
  (void)live.tick();
  const float liveLoud = lastFootstepLoudness(live);
  ok = ok && expect(loadedLoud > 0.0F &&
                        loadedLoud - liveLoud < 0.001F && liveLoud - loadedLoud < 0.001F,
                    "a post-load sneak footstep is identically quiet to the never-saved run");

  // A no-sneak save omits the userData keys entirely -> byte-identical to pre-MA1 saves.
  iggy3d::Session walkSession = makeSession();
  (void)walkSession.submitCommand(submittedMove({1.5F, 0.0F, 0.0F}));
  (void)walkSession.tick();
  const iggy3d::SaveStateResult walkSave =
      iggy3d::saveSessionStateEncoded(walkSession.state());
  ok = ok && expect(walkSave.encodedSaveText.find("userData0") == std::string::npos,
                    "a no-sneak save omits the userData keys (byte-identical back-compat)");
  return ok;
}

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
  ok = defaultAiActorStateHasPassiveDefaults() && ok;
  ok = aiStateRoundTripsThroughSaveLoadAndCodec() && ok;
  ok = patrolRouteRoundTripsThroughSaveLoad() && ok;
  ok = alertMemoryFacingRoundTripsThroughSaveLoad() && ok;
  ok = aiStateChangesParticipateInHash() && ok;
  ok = invalidCombatStateRejectedOnLoad() && ok;
  ok = sneakBitIsHashedAndDeterministic() && ok;
  ok = sneakBitAndLoudnessSurviveSaveLoad() && ok;
  return ok ? 0 : 1;
}
