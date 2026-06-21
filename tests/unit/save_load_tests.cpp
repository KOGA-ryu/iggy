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

iggy3d::FixtureScenarioSeed firstRoomSeed() {
  iggy3d::FixtureScenarioSeed seed;
  seed.scenarioId = "first_room.runtime_loop";
  seed.config = iggy3d::makeDefaultRuntimeConfig();
  seed.initialClockMode = iggy3d::ClockMode::Normal;
  seed.defaultRealtimeCamera = iggy3d::CameraMode::ThirdPerson;
  seed.defaultTacticalCamera = iggy3d::CameraMode::TacticalOverhead;
  seed.players.push_back({0, iggy3d::PlayerSlotKind::Local, "player"});
  seed.entities = {playerSeed(), goldKeySeed(), markerSeed()};
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

}  // namespace

int main() {
  bool ok = true;
  ok = envelopeMappingPreservesDurableState() && ok;
  ok = encodedSaveRoundtripLoadsFreshSession() && ok;
  ok = postLoadCommandIdDoesNotCollide() && ok;
  ok = invalidCursorIsRejected() && ok;
  ok = duplicateCommandIdIsRejectedByCommandLogRestore() && ok;
  ok = malformedEncodedSaveFailsDecodeAndLeavesDestinationUnchanged() && ok;
  return ok ? 0 : 1;
}
